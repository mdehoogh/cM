#include <time.h>
#include <stdlib.h>

#include "Mtime.h"

extern unsigned long long M_MODULE_DEBUGGING;

static Mallocationowner getOwner(uint16_t id){return (Mallocationowner){MI_TIME,id};}

extern char const * const M_ERROR_PREFIX;
extern char const * const M_WARNING_PREFIX;
extern char const * const M_BUG_PREFIX;
extern char const * const M_INFO_PREFIX;

extern long long M_LL_INVALID;
extern char const * const M_ISO8601_FORMAT;
extern char const * const M_ISO8601_UTC_FORMAT;

// timezone global data
extern char* tzname[2];
extern long int timezone;
extern int daylight;

// helper functions
// Mtimezonenames is no longer static as we need it in Mexecution.c/h for constructing the text representation of an Mtime
/**
 * @brief stores the timezone names
 * 
 */
char** Mtimezonenames=NULL;Mallocationowner owner_timezonenames=(Mallocationowner){MI_TIME,__LINE__,1}; // the locally remembered timezones which ends with a NULL timezone!!!
/**
 * @brief returns the index of the timezone with name \p tzn
 * 
 * @param tzn 
 * @return uint16_t the (positive) index of timezone with name \p pzn, or 0 on failure
 */
static uint16_t _gettznindex(char* tzn){Mallocationowner owner=getOwner(__LINE__);
	if(NULL==tzn)return 0;
	if(NULL==Mtimezonenames){
		Mtimezonenames=MALLOC(sizeof(char*),1,-'t',owner_timezonenames);
		if(NULL==Mtimezonenames)return 0;
		*Mtimezonenames=NULL; // ??????? TODO this is the stopper?????
	}
	// ASSERT _timezonenames is not NULL
	// 1. looking for the timezone called tzn
	char** timezonenames=Mtimezonenames;
	while(*timezonenames!=NULL&&strcmp(*timezonenames,tzn)!=0)timezonenames++;
	uint16_t tznindex=(timezonenames-Mtimezonenames+1); // the (1-based) timezone index of tzn
	if(NULL==*timezonenames){ // tzn currently not registered, because we ended up on the closing NULL timezone name
		char** _newtimezonenames=REALLOC(Mtimezonenames,tznindex,tznindex+1,sizeof(char*),-'t');
		if(_newtimezonenames!=NULL){ // successfully enhanced
			char *timezonename=OWNED(_strdup(tzn),owner); // NOTE TODO should _strdup(tzn) not be included in the memory management system (as this way the timezone name is NOT registered in it)
			if(timezonename!=NULL){ // managed to create the timezone name to register
				Mtimezonenames=_newtimezonenames;
				Mtimezonenames[tznindex]=NULL; // essential!!!
				Mtimezonenames[tznindex-1]=OWNED(DISOWNED(timezonename,owner),Msubowner(owner_timezonenames,1));
			}else{
				tznindex=0;
				outputMessage("Failed to duplicate timezone name '%s'.",tzn);
			}
		}else{
			tznindex=0;
			outputMessage(M_ERROR_PREFIX,"Failed to register timezone name '%s'.",tzn);
		}
	}
	return tznindex;
}

/**
 * @brief sets the current timezone to \p tzn
 * 
 * @param tzn 
 * @return int16_t 0 on success, -1 if invalid, 1 when failing to set the TZ variable
 */
static int16_t _tznset(char* tzn){
	int16_t result=-1;
	if(tzn!=NULL){
		size_t l=strlen(tzn);
		// TODO currently only checking a couple of requisites stated in ftp://ftp.iana.org/tz/tzdb-2017c/theory.html
		//      and we are assuming here that TZ is used for storing the timezone names and not the timezone codes (as was the original purpose)
		if(l>2&&tzn[0]!='/'&&tzn[l-1]!='/'&&strrchr(tzn,'/')&&strchr(tzn,'/')!=strchr(tzn,'/')+1){
			// the question is should any registered tzn also be registered here???????
			// now the point is what the order should be??????
			if(setenv("TZ",tzn,1)==0){
				result=0;
				// how about showing some timezone information?
				tzset(); // force initializing timezone information from the TZ environment variable explicitly
				outputMessage(M_INFO_PREFIX,"System-defined timezone: name='%s' - standard time descriptor='%s' - daylight saving time descriptor='%s' - offset=%li - daylight=%d.\n",tzn,tzname[0],tzname[1],timezone,daylight);
			}else{
				result=1;
				outputMessage(M_ERROR_PREFIX,"Failed to register '%s' as system-defined timezone.",tzn);
			}
		}else
			outputMessage(M_ERROR_PREFIX,"'%s' is not a valid timezone name.",tzn);
	}
	return result;
}

// helper function that takes a valid ymd, hms and timezone indicator
// MDH@11DEC2020: tz added representing any user specified timezone (to replace any system default)
// MDH@14DEC2020: essentially either iso8601 needs to contain a timezone specification, either Z or +/-hh:mm or a timezone name needs to be available (either as tzuser or through getenv("TZ"))
//                
/**
 * @brief returns an M time from a ISO8601 representation stored in \p io8601 of a timestamp
 * 
 * @param iso8601 
 * @param tzuser 
 * @return Mtime* the M time represented by \p iso8601, NULL on failure
 */
static Mtime* _parsedTime(char const * const iso8601,char const * const tzuser){Mallocationowner owner=getOwner(__LINE__);
	if(NULL==iso8601)return NULL;
	size_t l=strlen(iso8601);
	if(l==0){outputError("Missing ISO8601 timestamp");return NULL;}
	char* Tpos=strchr(iso8601,'T'); // should always be there!!!
	if(NULL==Tpos){outputMessage(M_ERROR_PREFIX,"Missing time in ISO8601 timestamp '%s'.",iso8601);return NULL;}
	Mtime* _calendarTime=NULL; // what we will return
	char *tz=NULL,*tzenv=NULL;
	output("Parsing ISO8601 timestamp '%s'%s.",iso8601);if(tzuser)output(" in timezone '%s'",tzuser);output(".\n");
	// source: https://stackoverflow.com/questions/26895428/how-do-i-parse-an-iso-8601-date-with-optional-milliseconds-to-a-struct-tm-in-c
	// if there's NO timezone information assume that local timezone is intended!!!!
	// MDH@14DEC2020: if the following combination persists no time can be constructed!!!!
	int16_t tzsec=INT16_MIN,tznindex=0; // MDH@13DEC2020
	int year,month,monthday,hour,minute,second;
	if(iso8601[--l]=='Z'){ // not an UTC (unless of course +00:00 was specified (or -00:00 for that matter though))
		sscanf(iso8601,"%d-%d-%dT%d:%d:%dZ",&year,&month,&monthday,&hour,&minute,&second);
		tzsec=0;
	}else
	if(strrchr(iso8601,'+')||strrchr(iso8601,'-')>Tpos){ // NOTE the - of the timezone is BEHIND the position of the 'T'
		int tzh=0,tzm=0;
		// NOTE hopefully sscanf() is able to format %i
		sscanf(iso8601,"%d-%d-%dT%d:%d:%d%d:%d",&year,&month,&monthday,&hour,&minute,&second,&tzh,&tzm);
		tzsec=60*(60*tzh+tzm);
		// output("Timezone offset: %d s.\n",tzsec);
	}else{ // no explicit timezone offset defined in the timestamp
		tzsec=INT16_MIN; // just in case
		// we leave tzsec to what it is (INT16_MIN), and try to set tznindex (timezone name index)
		// here the problem is that getEnv("TZ") might not return something in which case no local timezone is available
		sscanf(iso8601,"%d-%d-%dT%d:%d:%d",&year,&month,&monthday,&hour,&minute,&second);
		// determine tznindex, and if we succeed it will be non-zero!!!
		char* tzsys=getenv("TZ"); // the systemwide timezone
		if(tzuser!=NULL&&*tzuser){ // explicit user-defined timezone (which might be incorrect!!!!)
			// if we fail to actually set the current timezone (name) to tzuser, we should not continue
			// 1. store a duplicate of the current timezone name in tzenv
			if(tzsys){
				tzenv=OWNED(_strdup(tzsys),owner);
				// if failing to do so, we will not be able to restore this timezone, therefore we cannot continue
				if(NULL==tzenv){outputMessage(M_ERROR_PREFIX,"Failed to remember the current timezone '%s'.",tzsys);return NULL;}
				outputMessage(M_INFO_PREFIX,"System-defined timezone '%s' remembered.",tzenv);
			}
			// ASSERT tzenv created, so it is essential to free it asap
			outputMessage(M_INFO_PREFIX,"Will use the user-defined timezone '%s'.",tzuser);
			int16_t tznset=_tznset(tzuser);
			if(tznset==0){ // success
				// TODO improve on the following validity check
				if(strcasecmp(tzuser,"UTC")!=0&&strcasecmp(tzuser,"GMT")!=0&&strcasecmp(tzname[0],"UTC")==0) // apparently now UTC
					outputMessage(M_ERROR_PREFIX,"'%s' not considered to be a valid timezone name.",tzuser);
				else
					tznindex=_gettznindex(tz=tzuser);
			}else // failed to register the user-defined timezone, safest to actually fail to create the associated Mtime
				outputMessage(M_ERROR_PREFIX,"Failed to register user-defined timezone '%s'!",tzuser);
			if(tznindex==0)outputMessage(M_ERROR_PREFIX,"Failed to remember user-defined timezone '%s'.",tzuser);
		}else
		if(tzsys!=NULL){
			tznindex=_gettznindex(tz=tzsys);
			if(tznindex==0)outputMessage(M_ERROR_PREFIX,"Failed to remember system-defined timezone '%s'.",tzsys);
		}else{
			outputError("No system or user defined timezone specified!");
			outputMessage(M_INFO_PREFIX,"Either add a timezone name to the call, or set the system-defined timezone with a call to settimezone().");
		}
	}
	// if neither a timezone offset or timezone name index is defined, we can't create a time
	if(tzsec!=INT16_MIN||tznindex!=0){
		// are we doing anything with the tzh and tzm??????
		// let's check on the input values
		if(month>=1&&month<=12&&monthday>=1&&monthday<=31&&hour>=0&&hour<=23&&minute>=0&&minute<=59&&second>=0&&second<=61){
			output("Calendar timestamp: year=%d - month=%d - monthday=%d - hour=%d - minute=%d - second=%d - tzsec=%d - tznindex=%d.\n",year,month,monthday,hour,minute,second,tzsec,tznindex);
			struct tm timestamp;
			timestamp.tm_year = year - 1900; // Year since 1900
			timestamp.tm_mon = month - 1;     // 0-11
			timestamp.tm_mday = monthday;        // 1-31
			timestamp.tm_hour = hour;        // 0-23
			timestamp.tm_min = minute;         // 0-59
			timestamp.tm_sec = second;    // 0-61 (0-60 in C++11)
			// timestamp.tm_gmtoff=1;
			// TODO do we need to set tm_isdst???????
			// because we're calling mktime() we do not need to set tm_zone here because mktime() ignores it and uses the local time zone: timestamp.tm_zone=timezone;
			timestamp.tm_isdst=-1; // we're expecting mktime to deal with that
			// 1. turn the input into a struct tm something
			// 2. turn struct tm into a time_t
			time_t t=(tzsec!=INT16_MIN?timegm(&timestamp):mktime(&timestamp)); // either use timegm for the UTC timestamp (when an iso timezone was defined), or determine the local time using mktime which will use the system timezone (currently tz)
			// if timezone information is specified 
			if(t>=0){
				if(tznindex>0&&timestamp.tm_isdst==1){
					tznindex=-tznindex;
					output("%s","Daylight Saving Time active!\n");
				}
				_calendarTime=owned_time(_getTime("_getCalendarTime",t,tzsec,tznindex),owner);
				// if local time was extracted, show what the DST flag is
				// how about using the DST flag 
				if(tzsec==INT16_MIN)output("DST flag: %i.\n",timestamp.tm_isdst);
			}else{
				if(tz!=NULL)
					outputMessage(M_ERROR_PREFIX,"Failed to compute the time represented by '%s' in timezone '%s'.",M_ERROR_PREFIX,iso8601,tz);
				else
					outputMessage(M_ERROR_PREFIX,"Failed to compute the time represented by '%s'",iso8601);
			}
		}else{
			if(month<1||month>12)outputMessage(M_ERROR_PREFIX,"Month (%i) out of range.",month);
			if(monthday<1||monthday>31)outputMessage(M_ERROR_PREFIX,"Monthday (%i) out of range.",monthday);
			if(hour<0||hour>23)outputMessage(M_ERROR_PREFIX,"Hour (%i) out of range.",hour);
			if(minute<0||minute>59)outputMessage(M_ERROR_PREFIX,"Minute (%i) out of range.",minute);
			if(second<0||second>61)outputMessage(M_ERROR_PREFIX,"Second (%i) out of range.",second);
		}
	}
	// careful here: if we have a tz and it equals tzuser we need to reset the system timezone to what it was before
	if(tzsec==INT16_MIN){ // no timezone offset defined
		if(tzuser!=NULL&&*tzuser){ // a user-defined timezone was specified, so tzenv should be reinstalled as timezone
			outputMessage(M_INFO_PREFIX,"Undoing the registration of user-defined timezone '%s'.",tzuser);
			if(tzenv!=NULL){
				if(setenv("TZ",tzenv,1)!=0)
					outputMessage(M_ERROR_PREFIX,"Failed to re-register '%s' as system-defined timezone.",tzenv);
				else
					outputMessage(M_INFO_PREFIX,"'%s' re-registered as system-defined timezone.\n",tzenv);
			}else{
				if(unsetenv("TZ")!=0)
					outputError("Failed to unregister the system-defined timezone");
				else
					outputMessage(M_INFO_PREFIX,"System-defined timezone unregistered!");
			}
			/* replacing:
			int reset=(tzenv?setenv("TZ",tzenv,1):unsetenv("TZ"));
			if(reset!=0)output("%sFailed to %s%s as system-defined timezone.\n",M_ERROR_PREFIX,(tzenv?"restore ":"clear"),(tzenv?tzenv:""));else output("System-defined timezone %s%s.\n",(tzenv?tzenv:""),(tzenv?" restored":"cleared"));
			*/
		}/*else
			output("No user-defined timezone to unregister.\n");*/
	}
	if(tzenv!=NULL)FREE_DISOWNED(tzenv,strlen(tzenv)+1,-'"',owner); // see _strdup for how it allocates memory!!!
	return(_calendarTime?disowned_time(_calendarTime,owner):NULL);
}
// MDH@15DEC2020: alternative to parsing a calendar time (in ISO8601 format) we allow direct manipulation
//                of an epoch time
/**
 * @brief returns an M time in timezone \p tzn parsed from epoch time \p t
 * 
 * @param t 
 * @param tzn 
 * @return Mtime* the M time equivalent of \p t in timezone \p tzn
 */
static Mtime* _parsedEpochTime(time_t t,char* tzn){
	// is relatively simple: if tzn is defined and non-empty try to register it, and if success store with _time
	// if tzn is not-defined return UTC of the given time
	Mtime* _time=NULL;
	if(tzn!=NULL&&*tzn){
		int16_t tznindex=_gettznindex(tzn);
		if(tznindex!=0)
			_time=_getTime("_parsedEpochTime",t,INT16_MIN,tznindex);
		else
			outputMessage(M_ERROR_PREFIX,"'%s' not accepted as timezone.",tzn);
	}else
		_time=_getTime("_parsedEpochTime",t,0,0);
	return _time;
}

/**
 * @brief returns now as a wrapped M time
 * 
 * @return Mvalue* the wrapped M time corresponding to the current moment in time
 */
Mvalue* Mnow(){
	time_t t=time(NULL); // what is returned is essentially UTC, therefore tzmin should be set to 0
	return _getValueOfTime(_getTime("Mnow()",t,0,0)); 
}

/**
 * @brief returns the calendar time equivalent of time wrapped in \p _timeValue in timezone wrapped in \p _tznValue
 * 
 * @param _timeValue 
 * @param _tznValue 
 * @return Mvalue* 
 */
Mvalue* Mcalendartime(Mvalue* _timeValue,Mvalue* _tznValue){Mallocationowner owner=getOwner(__LINE__);
	// let's accept any text that conforms to ISO8601 i.e. a calendar date with timezone information of the format <date><time><timezone> where <time>should start with T and <timezone> with either - or + or Z
	Mvalue* result=NULL;
	if(_timeValue!=NULL){
		// only integer or time values are usable
		Mtime* _time=NULL;
		if(_timeValue->type==VT_INTEGER||_timeValue->type==VT_BIGINTEGER)
			_time=owned_time(
						_parsedEpochTime(getValueInteger(_timeValue),
							(_tznValue&&_tznValue->type==VT_TEXT?_tznValue->value._text->_c:NULL)),owner);
		else
		if(_timeValue->type==VT_TIME)
			_time=_timeValue->value._time;
		if(_time!=NULL){
			time_t t=_time->t;
			int16_t tzsec=_time->tzsec,tznindex=0;
			char* tzenv=NULL; // the current system-defined timezone that we need to re-register afterwards (at least when tznindex is non-zero)
			struct tm* _tm=NULL;
			size_t maxsize=19; // yyyy-mm-ddThh:mm:ss
			bool isdst=false;
			if(tzsec!=INT16_MIN){ // not local time
				_tm=gmtime(&t); // the t-part represents UTC
				if(tzsec!=0)
					maxsize+=6;
				else
					maxsize+=1;
			}else{ // local time
				tznindex=_time->tznindex;
				if(tznindex!=0){
					if(tznindex<0){isdst=true;tznindex=-tznindex;}
					if(Mtimezonenames[tznindex-1]){
						char *tzsys=getenv("TZ"),*tzenv=NULL;
						if(tzsys!=NULL){
							tzenv=OWNED(_strdup(tzsys),owner);
							if(NULL==tzenv){
								output("%sFailed to remember the current system-defined timezone name",tzsys);
								return NULL;
							}
						}
						if(_tznset(Mtimezonenames[tznindex-1])!=0){
							outputMessage(M_ERROR_PREFIX,"Failed to register '%s' temporarily as system-defined timezone.",Mtimezonenames[tznindex-1]);
							tznindex=INT16_MIN;
						}else
							_tm=localtime(&t);
					}else{
						outputMessage(M_BUG_PREFIX,"Timezone name index %d invalid!",tznindex);
						tznindex=INT16_MIN; // use as invalid value
					}
					// we'll be appending either tzname[0] or tzname[1]
					if(tznindex!=INT16_MIN)
						maxsize+=strlen(tzname[isdst?1:0])+2;
				}
			}
			// if _time was created from a epoch time (integer) we should free it asap
			if(_timeValue->type!=VT_TIME)FREE_TIME(_time,owner);
			outputMessage(M_INFO_PREFIX,"Calendar time fields: year=%d - month=%d - monthday=%d - hour=%d - minute=%d - second=%d.\n"
						,_tm->tm_year+1900,_tm->tm_mon+1,_tm->tm_mday,_tm->tm_hour,_tm->tm_min,_tm->tm_sec);
			char strtm[maxsize+1];
			strtm[0]='\'';
			strftime(strtm+1,maxsize,M_ISO8601_FORMAT,_tm);
			size_t l=strlen(strtm);
			if(tzsec!=INT16_MIN){
				if(tzsec!=0){
					if(tzsec<0){
						tzsec=-tzsec;
						strtm[l]='-';
					}else
						strtm[l]='+';
					int tzh=(tzsec/3600);
					strtm[++l]=48+(tzh/10);
					strtm[++l]=48+(tzh%10);
					strtm[++l]=':';
					int tzm=(tzsec/60)%60;
					strtm[++l]=48+(tzm/10);
					strtm[++l]=48+(tzm%10);
				}else
					strtm[l]='Z';
				strtm[++l]='\0';
			}else
			if(tznindex>0){
				// we'd like to append either tzname[0] or tzname[1] depending!!!
				char* _tzname=tzname[isdst?1:0];
				_tzname+=strlen(_tzname);
				strtm[maxsize]='\0';
				strtm[l]=' ';
				while(--maxsize>l)strtm[maxsize]=*(--_tzname);
			}
			// re-register the current timezone (as stored in tzenv)
			if(tznindex!=0){
				if(tzenv!=NULL){
					if(_tznset(tzenv)!=0)
						outputMessage(M_ERROR_PREFIX,"Failed to re-register '%s' as system-defined timezone.",tzenv);
					FREE(tzenv,strlen(tzenv)+1,-'"');
				}else{
					if(unsetenv("TZ")!=0)
						outputMessage(M_ERROR_PREFIX,"Failed to unregister temporary timezone '%s'.",Mtimezonenames[abs(tznindex)-1]);
				}
			}
			return _getTextValue(strtm);
		}
	}
	return result;
}

// MDH@11DEC2020: allowing to pass in the tz to use (if _timetextValue does not contain timezone information)
// MDH@16DEC2020: debugging the parsing process of composing the iso8601 text representation from the timetext presented
static char* readint(char const * const intt,int * i){
	// skip trailing blanks
	char* t=intt;
	while((*t)==' ')t++;
	// the integer may start with signs
	bool negative=false;
	while(*t){if((*t)=='-')negative=!negative;else if((*t)!='+')break;t++;}
	// process all digits until we bump into a non-digit
	while((*t)>=48&&(*t)<=57){ // a digit character
		if(*i==INT_MIN)*i=0;else (*i)*=10;
		(*i)+=(*t)-48; // increment with digit
		output("After processing '%c': %d.\n",*t,*i);
		t++; // next character
	}
	if((*i)!=INT_MIN)if(negative)*i=-(*i); // make negative
	return t;
}
/**
 * @brief returns the M time wrapper equivalent of an ISO8601 timestamp text wrapped in \p _timetextValue in the timezone wrapped in \p _tznValue
 * 
 * @param _timetextValue 
 * @param _tznValue 
 * @return Mvalue* the M time wrapper equivalent to ISO8601 timestamp text in \p _timetextValue
 */
Mvalue* Mparsetime(Mvalue* _timetextValue,Mvalue* _tznValue){Mallocationowner owner=getOwner(__LINE__);
	// let's accept any text that conforms to ISO8601 i.e. a calendar date with timezone information of the format <date><time><timezone> where <time>should start with T and <timezone> with either - or + or Z
	Mvalue* result=NULL;
	if(_timetextValue!=NULL){
		if(_timetextValue->type==VT_INTEGER||_timetextValue->type==VT_BIGINTEGER){
			long long lltime=getValueInteger(_timetextValue);
			// OOPS can't use time() because time(&t) would simply put the current time (now) in t
			if(lltime>=0)result=_getValueOfTime(_getTime("Mparsetime()",lltime,0,0)); // assuming UTC
		}else
		if(_timetextValue->type==VT_TEXT){
			// <date> <time> <timezone>, <date> ends with either T or -|+|Z, <time> ends with either T or -|+|Z or the end of the string (in which it assumes Z)
			// <date> consists of exactly three numeric parts, <time> exists of at least one numeric part indicating the hour of the day
			// we'll ignore any nonnumeric characters in between?
			char* timetext=_timetextValue->value._text->_c;
			if(timetext!=NULL&&*timetext){
				// TODO correct timetext to be in ISO8601 format
				// I suppose we can do that now
				// expecting the format y m d h m s timezone where y m d h m s are digits so essentially we skip whatever there's in between
				// essentially y m d are to be separated by non digits, as goes for h m s but the hyphen cannot be used in h m s because that would indicate the start of the timezone
				// y m d is obligatory in that order, h m s are all optional but we allow h, h m, h m s
				Mstring *_iso8601=owned_string(__string("Mparsetime"),owner);
				if(_iso8601!=NULL){
					bool complete=false;
					// how about extracting the actual constituent parts
					// the obligatory parts are initialized to INT_MIN so we can for that afterwards
					int year=INT_MIN,month=INT_MIN,monthday=INT_MIN,hour=0,minute=0,second=0,tzh=INT_MIN,tzm=0;
					size_t yearcharacters=0;
					char c;
					char* t=timetext; // the pointer that is going to be changed
					while((*t)==' '||(*t)=='0')t++; // skip all trailing blanks and zeroes
					if(*t){
						char *yearstart=t;
						t=readint(t,&year);
						if((*t)&&(year!=INT_MIN)){
							yearcharacters=(t-yearstart); // remember how many year characters there are
							t++;
							t=readint(t,&month);
							if((*t)&&(month!=INT_MIN)){
								t++;
								t=readint(t,&monthday);
								if((c=*t)&&(monthday!=INT_MIN)){
									// the hour, minute and second are optional
									// we know that's the case if we bumped into a Z, + or - although theoretically which in a sense is problematic if someone put negative hours, minutes or seconds in there
									// we can solve that by demanding a T to start the hour, minute and second part, essentially a minus or plus sign starts the timezone part, so you need something else
									// than a Z + or - to start the hms part
									if(c!='Z'&&c!='+'&&c!='-'){ // NOT the start of the timezone part
										t++;
										t=readint(t,&hour);
										c=*t;
										if(c!=0&&c!='Z'&&c!='+'&&c!='-'){ // NOT the start of the timezone part
											t++;
											t=readint(t,&minute);
											c=*t;
											if(c!=0&&c!='Z'&&c!='+'&&c!='-'){ // NOT the start of the timezone part
												t++;
												t=readint(t,&second);
											}
										}
									}
									c=*t;
									if(c=='T'||c=='-'||c=='+'){ // there's a timezone part
										t++;
										t=readint(t,&tzh);
										if((*t)&&(tzh!=INT_MIN)){
											t++;
											t=readint(t,&tzm);
										}
									}else
									if(c=='Z')
										tzh=0;
								}
							}
						}
						if(year!=INT_MIN&&month!=INT_MIN&&monthday!=INT_MIN){
							char iso8601[yearcharacters+15+(tzh==INT_MIN?6:1)+1];
							if(tzh>=0){ // a non-negative timezone offset
								if(tzh==0&&tzm==0)
									sprintf(iso8601,"%d-%02d-%02dT%02d:%02d:%02dZ",year,month,monthday,hour,minute,second);
								else
									sprintf(iso8601,"%d-%02d-%02dT%02d:%02d:%02d+%02d:%02d",year,month,monthday,hour,minute,second,tzh,tzm);
							}else
							if(tzh!=INT_MIN) // a negative timezone offset
								sprintf(iso8601,"%d-%02d-%02dT%02d:%02d:%02d-%02d:%02d",year,month,monthday,hour,minute,second,abs(tzh),tzm);
							else // undefined timezone offset
								sprintf(iso8601,"%d-%02d-%02dT%02d:%02d:%02d",year,month,monthday,hour,minute,second);
							if(string_append(_iso8601,iso8601))complete=true;
						}
					}
					/* replacing:
					// we will consume timetext in the process (which we're allowed to do)
					// 1. skip non-digits and leading zeroes at the start
					char timetextcharacter=*timetext;
					while(timetextcharacter&&(timetextcharacter<=48||timetextcharacter>57))timetextcharacter=*(++timetext);
					if(timetextcharacter){ // the first digit character of the year ('1' through '9')
							// 2. get the year digits
							// NOTE if we fail to store the year digit, we make timetextcharacter '\0' which will be the flag to indicate premature end of year input
							do{
									output("Processing year digit '%c'.\n",timetextcharacter);
									timetextcharacter=(string_append_char(_iso8601,timetextcharacter)?*(++timetext):'\0');
							}while(timetextcharacter&&(timetextcharacter>=48&&timetextcharacter<=57));
							if(timetextcharacter&&string_append_char(_iso8601,'-')){ // year extracted and year - month separator appended
									// we may again skip all non-digits and leading zeroes
									while(timetextcharacter&&(timetextcharacter<=48||timetextcharacter>57))timetextcharacter=*(++timetext);
									if(timetextcharacter){ // first month digit
											size_t digitsleft=2;
											do{
													output("Processing month digit '%c'.\n",timetextcharacter);
													timetextcharacter=(string_append_char(_iso8601,timetextcharacter)?*(++timetext):'\0');
											}while(timetextcharacter&&(--digitsleft>0)&&(timetextcharacter>=48&&timetextcharacter<=57));
											if(timetextcharacter){ // month extracted
													if(digitsleft==0||string_insert_char(_iso8601,string_length(_iso8601)-1,'0')){ // month is now a two-digit text
															if(string_append_char(_iso8601,'-')){ // month - monthday separator appended
																	// we may again skip all non-digits and leading zeroes of the monthday indicator
																	while(timetextcharacter&&(timetextcharacter<=48||timetextcharacter>57))timetextcharacter=*(++timetext);
																	if(timetextcharacter){
																			digitsleft=2;
																			do{
																					output("Processing monthday digit '%c'.\n",timetextcharacter);
																					timetextcharacter=(string_append_char(_iso8601,timetextcharacter)?*(++timetext):'\0');
																			}while((--digitsleft>0)&&timetextcharacter>=48&&timetextcharacter<=57);
																			if(digitsleft==0||string_insert_char(_iso8601,string_length(_iso8601)-1,'0')){ // monthday is now a two-digit text
																					if(string_append_char(_iso8601,'T')){
																							// because the user could have only specified a date, timetextcharacter is now allowed to be '\0'
																							// because hms part is actually obsolete we only parse it if the current character is not a timezone character (or ends the timestamp)
																							// NOTE that h m and s are all two-digit texts so we can simply do the following three times
																							int hmsindex=3;
																							do{
																									if(timetextcharacter=='Z'||timetextcharacter=='-'||timetextcharacter=='+'||timetextcharacter=='\0')break;
																									// skip non-digits
																									while(timetextcharacter&&(timetextcharacter<=48||timetextcharacter>57))timetextcharacter=*(++timetext);
																									if(timetextcharacter){
																											digitsleft=2;
																											do{
																													timetextcharacter=(string_append_char(_iso8601,timetextcharacter)?*(++timetext):'\0');
																											}while((--digitsleft>0)&&timetextcharacter&&(timetextcharacter>=48&&timetextcharacter<=57));
																											// failure if we fail to insert the required second character
																											if(digitsleft>0&&!string_insert_char(_iso8601,string_length(_iso8601)-1,'0'))hmsindex=0;
																									}
																									hmsindex--;
																									// append the colon
																									if(hmsindex>0&&!string_append_char(_iso8601,':'))hmsindex=-1;
																							}while(hmsindex>0);
																							// complete the hms part if necessary
																							while(--hmsindex>=0){
																									if(!string_append(_iso8601,"00")||(hmsindex>0&&!string_append_char(_iso8601,':'))){
																											hmsindex=-1;
																											outputError("Failed to insert default time defaults");
																									}
																							}
																							if(hmsindex>=-1){
																									// is there a timezone specified?????
																									if(timetextcharacter=='-'||timetextcharacter=='+'){
																											// NOTE embedding an additional blank in front of the - or + sign so that _getCalendarTime can parse it more easily
																											if(string_append_char(_iso8601,timetextcharacter)){
																													// hour and minute two-digit texts to construct
																													int tzindex=2;
																													do{
																															// skip non-digits
																															while(timetextcharacter&&(timetextcharacter<=48||timetextcharacter>57))timetextcharacter=*(++timetext);
																															if(!timetextcharacter)break; // no further characters
																															digitsleft=2;
																															do{
																																	if(tzindex==2)output("Processing timezone hour digit '%c'.\n",timetextcharacter);
																																	else output("Processing timezone minute digit '%c'.\n",timetextcharacter);
																																	timetextcharacter=(string_append_char(_iso8601,timetextcharacter)?*(++timetext):'\0');
																															}while((--digitsleft>0)&&timetextcharacter&&(timetextcharacter>=48&&timetextcharacter<=57));
																															// failure if we fail to insert the required second character
																															if(digitsleft>0&&!string_insert_char(_iso8601,string_length(_iso8601)-1,'0')){
																																	tzindex=0;
																																	outputError("Failed to prepend a trailing zero to the timezone hour.\n");
																															}
																															tzindex--;
																															// append the colon
																															if(tzindex>0&&!string_append_char(_iso8601,':')){
																																	tzindex=-1;
																																	outputError("Failed to append the timezone hour - minute colon separator");
																															}
																													}while(tzindex>0);
																													// tzindex will be -1 on failure, 0 or 1 or 2 otherwise
																													while(--tzindex>=0){
																															if((tzindex>0||!string_append_char(_iso8601,':'))||!string_append(_iso8601,"00")){
																																	tzindex=-1;
																																	outputError("Failed to complete the timezone");
																															}
																													}
																													// if we end up at the end we're complete
																													if(tzindex>=-1)complete=true;
																											}else 
																													outputError("Missing hour and/or minute timezone information");
																									}else
																									if(timetextcharacter!='Z'||string_append_char(_iso8601,'Z'))complete=true;
																							}else 
																									outputError("Invalid hh(:mm(:ss)) time part");
																					}else 
																							outputError("Failed to append the date time separator character T.\n");
																			}else 
																					outputError("Invalid monthday");
																	}else 
																			outputError("Missing monthday digits");
															}else 
																	outputError("Failed to append the month - monthday dash");
													}else 
															outputError("Erroneous or incompleted monthday");
											}else
													outputError("Missing month digits");
									}else
											outputError("Month digits missing");
							}else
									outputError("Failed to append the year - month dash");
					}else
							outputError("No digits in calendar datetime text");
					*/
					if(complete)
						result=_getValueOfTime(_parsedTime(string(_iso8601),(_tznValue&&_tznValue->type==VT_TEXT?_tznValue->value._text->_c:NULL)));
					FREE_STRING(_iso8601,owner);
				}else
					outputError("Failed to initialize the ISO8601 datetime text");
			}else
				outputError("Undefined calendar datetime text");
		}else
		if(_timetextValue->type==VT_MAP){
		}else
			outputError("Time initializer not an integer, text or map");
	}
	return result;
}

/**
 * @brief sets the systemwide timezone to the timezone wrapped in \p _tznValue
 * 
 * @param _tznValue 
 * @return Mvalue* the systemwide timezone wrapper
 */
Mvalue* Msettimezone(Mvalue* _tznValue){
	if(_tznValue!=NULL&&_tznValue->type==VT_TEXT){
		char* tzn=(_tznValue->value._text?_tznValue->value._text->_c:NULL);
		if(tzn!=NULL&&*tzn){
			int16_t tznset=_tznset(tzn);
			/*
			if(tznset<0)
				outputMessage(M_ERROR_PREFIX,"'%s' not a valid timezone name.",tzn);
			else
			*/
			if(tznset>0)
				outputMessage(M_ERROR_PREFIX,"Failed to register '%s' as (system-defined) timezone.",tzn);
		}else 
			outputError("Missing timezone name");
	}else
		outputError("Timezone name specified not of type text");
	// return the current timezone (whatever it is)
	return _getValueOfText(_getSingleQuotedText(getenv("TZ")));
}
/**
 * @brief returns the systemwide timezone
 * 
 * @return Mvalue* the systemwide timezone
 */
Mvalue* Mgettimezone(){
	return _getValueOfText(_getSingleQuotedText(getenv("TZ")));
}