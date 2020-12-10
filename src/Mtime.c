#include <time.h>
#include <stdlib.h>

#include "Mtime.h"

extern unsigned long long M_MODULE_DEBUGGING;
#define DEBUGGING (M_MODULE_DEBUGGING&MM_TIME)

static Mallocationowner getOwner(uint16_t id){return (Mallocationowner){MI_TIME,id};}

extern char const * const M_ERROR_PREFIX;
extern char const * const M_WARNING_PREFIX;
extern long long M_LL_INVALID;
extern char const * const M_ISO8601_FORMAT;
extern char const * const M_ISO8601_UTC_FORMAT;

Mvalue* Mnow(){
    time_t t=time(NULL); // what is returned is essentially UTC, therefore tzsec should be set to NULL
    return _getValueOfTime(_getTime("Mnow()",t,0)); 
}

// helper function that takes a valid ymd, hms and timezone indicator
static Mtime* _getCalendarTime(char* iso8601){
    if(!iso8601)return NULL;
    size_t l=strlen(iso8601);
    if(l==0){outputError("Missing ISO8601 timestamp.");return NULL;}
    char* Tpos=strchr(iso8601,'T'); // should always be there!!!
    if(!Tpos){output("%sMissing time in ISO8601 timestamp '%s'.\n",M_ERROR_PREFIX,iso8601);return NULL;}
    output("Parsing ISO8601 '%s'.\n",iso8601);
    // source: https://stackoverflow.com/questions/26895428/how-do-i-parse-an-iso-8601-date-with-optional-milliseconds-to-a-struct-tm-in-c
    // if there's NO timezone information assume that local timezone is intended!!!!
    bool timezonespecified=true;
    int year,month,monthday,hour,minute,second,tzsec,tzh=0,tzm=0;
    if(iso8601[--l]=='Z'){ // not an UTC (unless of course +00:00 was specified (or -00:00 for that matter though))
        sscanf(iso8601,"%d-%d-%dT%d:%d:%dZ",&year,&month,&monthday,&hour,&minute,&second);
    }else
    if(strrchr(iso8601,'+')||strrchr(iso8601,'-')>Tpos){ // NOTE the - of the timezone is BEHIND the position of the 'T'
        // NOTE hopefully sscanf() is able to format %i
        sscanf(iso8601,"%d-%d-%dT%d:%d:%d%d:%d",&year,&month,&monthday,&hour,&minute,&second,&tzh,&tzm);
        // output("Timezone offset: %d s.\n",tzsec);
    }else{
        // here the problem is that getEnv("TZ") might not return something in which case no local timezone is available
        sscanf(iso8601,"%d-%d-%dT%d:%d:%d",&year,&month,&monthday,&hour,&minute,&second);
        char* tzenv=getenv("TZ");
        if(tzenv){
            timezonespecified=false;
            output("%sNo timezone specified; the local timezone '%s' will be used.\n",M_WARNING_PREFIX,tzenv);
        }else
            outputWarning("No timezone specified, and no local timezone set: will assume UTC");
    }
    tzsec=(tzh!=0||tzm!=0?60*(60*tzh+tzm):0);
    output("Timezone offset (%0*d:%0*d): %d s.\n",2,tzh,2,tzm,tzsec);
    // are we doing anything with the tzh and tzm??????
    // let's check on the input values
    if(month>=1&&month<=12&&monthday>=1&&monthday<=31&&hour>=0&&hour<=23&&minute>=0&&minute<=59&&second>=0&&second<=61){
        output("Calendar timestamp numbers: year=%d - month=%d - monthday=%d - hour=%d - minute=%d - second=%d - tzsec=%d.\n",year,month,monthday,hour,minute,second,tzsec);
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
        time_t t=(timezonespecified?timegm(&timestamp):mktime(&timestamp)); // if timezone information is specified get the UTC time
        output("DST flag with timezone %s: %i.\n",(timezonespecified?"specified":"not specified"),timestamp.tm_isdst);
        // if timezone information is specified 
        if(t>=0)
            return _getTime("_getCalendarTime",t,(timezonespecified?tzsec:M_LL_INVALID));
        output("%sFailed to compute the time represented by '%s'!\n",M_ERROR_PREFIX,iso8601);
    }else{
        if(month<1||month>12)output("%sMonth (%i) out of range.\n",M_ERROR_PREFIX,month);
        if(monthday<1||monthday>31)output("%sMonthday (%i) out of range.\n",M_ERROR_PREFIX,monthday);
        if(hour<0||hour>23)output("%sHour (%i) out of range.\n",M_ERROR_PREFIX,hour);
        if(minute<0||minute>59)output("%sMinute (%i) out of range.\n",M_ERROR_PREFIX,minute);
        if(second<0||second>61)output("%sSecond (%i) out of range.\n",M_ERROR_PREFIX,second);
    }
    return NULL;
}
Mvalue* Mcalendartime(Mvalue* _timeValue){Mallocationowner owner=getOwner(__LINE__);
    // let's accept any text that conforms to ISO8601 i.e. a calendar date with timezone information of the format <date><time><timezone> where <time>should start with T and <timezone> with either - or + or Z
    Mvalue* result=NULL;
    if(_timeValue&&_timeValue->type==VT_TIME){
        Mtime* _time=_timeValue->value._time;
        if(_time){
            time_t t=_time->t;
            long long tzsec=_time->tzsec;
            struct tm* _tm;
            size_t maxsize=strlen(M_ISO8601_FORMAT)+1;
            if(tzsec!=M_LL_INVALID){ // not local time
                _tm=gmtime(&t); // the t-part represents UTC
                if(tzsec!=0)
                    maxsize+=6;
                else
                    maxsize+=1;
            }else // local time
                _tm=localtime(&t);
            output("Calendar time fields: year=%d - month=%d - monthday=%d - hour=%d - minute=%d - second=%d.\n"
                    ,_tm->tm_year,_tm->tm_mon,_tm->tm_mday,_tm->tm_hour,_tm->tm_min,_tm->tm_sec);
            char strtm[maxsize];
            strftime(strtm,maxsize,M_ISO8601_FORMAT,_tm);
            size_t l=strlen(strtm);
            if(tzsec!=M_LL_INVALID){
                strtm[maxsize-1]='\0';
                if(tzsec>0){
                    strtm[l]='+';
                }else
                if(tzsec<0){
                    strtm[l]='-';
                }else{
                    strtm[l]='Z';
                }
            }
            return _getTextValue(strtm);
        }
    }
    return result;
}
Mvalue* Mparsetime(Mvalue* _timetextValue){Mallocationowner owner=getOwner(__LINE__);
    // let's accept any text that conforms to ISO8601 i.e. a calendar date with timezone information of the format <date><time><timezone> where <time>should start with T and <timezone> with either - or + or Z
    Mvalue* result=NULL;
    if(_timetextValue){
        if(_timetextValue->type==VT_INTEGER||_timetextValue->type==VT_BIGINTEGER){
            long long lltime=getValueInteger(_timetextValue);
            // OOPS can't use time() because time(&t) would simply put the current time (now) in t
            if(lltime>=0)result=_getValueOfTime(_getTime("Mparsetime()",lltime,0)); // assuming UTC
        }else
        if(_timetextValue->type==VT_TEXT){
            // <date> <time> <timezone>, <date> ends with either T or -|+|Z, <time> ends with either T or -|+|Z or the end of the string (in which it assumes Z)
            // <date> consists of exactly three numeric parts, <time> exists of at least one numeric part indicating the hour of the day
            // we'll ignore any nonnumeric characters in between?
            char* timetext=_timetextValue->value._text->_c;
            if(timetext&&strlen(timetext)){
                // TODO correct timetext to be in ISO8601 format
                // I suppose we can do that now
                // expecting the format y m d h m s timezone where y m d h m s are digits so essentially we skip whatever there's in between
                // essentially y m d are to be separated by non digits, as goes for h m s but the hyphen cannot be used in h m s because that would indicate the start of the timezone
                // y m d is obligatory in that order, h m s are all optional but we allow h, h m, h m s
                Mstring *_iso8601=owned_string(__string("Mparsetime"),owner);
                if(_iso8601){
                    // we will consume timetext in the process (which we're allowed to do)
                    // 1. skip non-digits and leading zeroes at the start
                    bool complete=false;
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
                                                                        if((tzindex>0&&!string_append_char(_iso8601,':'))||!string_append(_iso8601,"00")){
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
                    if(complete)
                        result=_getValueOfTime(_getCalendarTime(string(_iso8601)));
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