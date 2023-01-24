/**
 * MDH@02MAY2019:
 * - interactive session stuff
 */
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <termios.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdlib.h>
// for being able to determine the window size
#include <sys/ioctl.h>

#include "Msession.h"

static Mallocationowner getOwner(uint16_t id){return (Mallocationowner){MI_SESSION,id};}

extern long long M_LL_INVALID;

static struct termios orig_termios;

static int16_t rawmode=-1;

void disableRawmode(){
	if(rawmode<0)return;
	outputLine("Disabling character input mode.");
	rawmode=-1;
	tcsetattr(STDIN_FILENO,TCSAFLUSH,&orig_termios);
}

// MDH@23OCT2021: moved over from M.c as we need it in endOfInput() below
Mstring* _getTimestamp(char const * const format){Mallocationowner owner=getOwner(__LINE__);
	Mstring* _timestamp=owned_string(__string(),owner);
	if(_timestamp){
		Mstring* p=string_setlength(_timestamp,50/*,owner*/);
		if(p){
			time_t now=time(NULL);
			struct tm * nowlocal=localtime(&now);
			p=string_setlength(p,strftime(p->_chars->chars,50,(format?format:"%Y-%m-%d %H:%M:%S"),nowlocal)/*,owner*/);
		}
		if(!p){FREE_STRING(_timestamp,owner);_timestamp=NULL;}
	}
	return disowned_string(_timestamp,owner);
}

Mstring* _timestampedOutputFilename=NULL;Mallocationowner owner_timestampedOutputFilename={MI_SESSION,__LINE__,1};
size_t outputFilenamePrefixLength,outputFilenameSuffixLength;

void endOfUserInput(){Mallocationowner owner=getOwner(__LINE__);
	// return to the 'right' colors
	resetOutputColor();
	setOutputFilename(NULL); // MDH@13MAR2020: will close the current output file (if any), so it will contain all required information
	if(_timestampedOutputFilename){
		//output("\nTimestamped output filename: '%s'.",string(_timestampedOutputFilename));
		char outputFilename[outputFilenamePrefixLength+outputFilenameSuffixLength+1];
		outputFilename[0]='\0';
		char* _outputFilenamePrefix=_stringstart(_timestampedOutputFilename,outputFilenamePrefixLength);
		strcat(outputFilename,_outputFilenamePrefix);
		strcat(outputFilename,string_remainder(_timestampedOutputFilename,string_length(_timestampedOutputFilename)-outputFilenameSuffixLength));
		if(rename(outputFilename,string(_timestampedOutputFilename))==-1)
			output("\nERROR: Failed to timestamp output file '%s'.",outputFilename);
		else
			output("\nOutput file '%s' renamed to '%s'.",outputFilename,string(_timestampedOutputFilename));
		free(_outputFilenamePrefix); // ESSENTIAL
	}
	output("\n\n%s\n\n","Thanks for using M.");
	disableRawmode();
}

// MDH@30JUN2020: let's allow a timeout (number of tenths of seconds to block for input every time)
void enableRawmode(uint8_t timeout){
	if(rawmode==timeout)return;
	// output("Enabling character input mode with timeout %u.\n",timeout); // DEBUG
	if(rawmode<0){
		tcgetattr(STDIN_FILENO,&orig_termios);
		atexit(endOfUserInput); // or std::atexit() in C++
	}
	rawmode=timeout;
	struct termios raw=orig_termios; // making a copy
	raw.c_cc[VMIN]=0; // NOT doing this caused positive timeout values to fail timing out!!!!! (all of a sudden so unclear how come though)
	raw.c_cc[VTIME]=rawmode;
	// ISIG turns off Ctrl-C and Ctrl-Z
	raw.c_lflag&=~(ECHO|ICANON|ISIG); // we kill echoing so we can first look at what we received!!
	tcsetattr(STDIN_FILENO,TCSAFLUSH,&raw);
}

///////char inputChar='\0'; // the last read input character and its associated type (which we can set to o to escape to control mode!!)
// currently inputCharRead() blocks until a character can be read (and put in _c)
bool inputCharRead(char* _c){
	enableRawmode(0);
	return(read(STDIN_FILENO,_c,1)==1);
}
// MDH@12JUL2020: even if updateFunction is NULL we switch to using a 1/10s timeout, only when updateFunction is not NULL do we execute the update function!!!!
//				this is so that we can use NULL to read characters received as part of an escape sequence!!
bool inputCharReadNonBlocking(char* _c,UpdateFunction updateFunction){
	// if(!updateFunction)return inputCharRead(_c);
	enableRawmode(1);
	// outputChar('Y'); // DEBUG
	// as long as read timesout execute the updateFunction()
	ssize_t result=0;
	while(1){
		// DEBUG: outputChar(result>0?'A':'B');
		result=read(STDIN_FILENO,_c,1);
		if(result!=0){
			// output("(%u)",*_c); // DEBUG
			break;
		}
		// outputChar('X'); // DEBUG
		if(updateFunction)(*updateFunction)();
	}
	return(result>0);
}
/*
int getch(){
	// ASSERT assume in one-character-at-a-time-mode!!!
	int r;unsigned char c;
	if ((r=read(STDIN_FILENO,&c,sizeof(c)))>0)return r; // MDH@11NOV2019: changed <0 into >0 which makes more sense considering how inputCharRead() is implemented!!!
	return c;
}
*/
//////char getInputChar(){return inputChar;}

// interfacing with the console
void oneLineUp(){outputControlText("1A");} // ascertain that the previous line is visible
void oneLineDown(){outputControlText("1B");} // one line down
void toStartOfLine(){outputChar('\r');} // replacing: '\r');}
void clearLine(){
	toStartOfLine();outputControlText("K");
} // MDH@30OCT2019: adjusted to always to the start of the line before clearing it, this is to ascertain that any called does not need toStartOfLine() per se

void moveCursorLeft(uint16_t pos){
	if(pos)output(ES"%huD",pos);
} // TODO can't use outputControlText here!!!

void moveCursorRight(uint16_t pos){
	if(pos)output(ES"%huC",pos);
} // TODO can't use outputControlText here!!!

void clearScreenFromCursor(){
	outputControlText("J");
}
void clearDisplay(){
	clearScreenFromCursor();// replacing: outputControlText("2J");
}
void beep(){outputChar('\a');} // replacing \a
void removeLastCharacter(){outputChar('\b');}
void hidecursor(){outputControlText("?25l");}
void showcursor(){outputControlText("?25h");}
void emptyline(){outputControlText("2K\r");}
void backspace(){outputControlText("D"); /* go left one character */ outputControlText("K"); /* clear the rest of the line */}

void setColor(char const * const colortext){
	output(ES"38;5;%sm",colortext);
}
void setBackColor(char const * const colortext){
	output(ES"48;5;%sm",colortext);
}

void resetOutputColor(){
	setColor(getInfoColor());
	setBackColor(getBackgroundColor());
}

// MDH@27FEB2020: delegating to outputInfo after resetting the output color
void outputLine(char* s){
	resetOutputColor();
	output("%s\n",s);
} // for writing a single line of output text in the info color
/*
void activateColorscheme(){
	setBackColor(getBackgroundColor()); // MDH@17OCT2019 replacing: output(ES"%sm",getBackgroundColor()); // TODO can't use outputControlText here!!!
	clearScreenFromCursor(); // MDH@30OCT2019 replacing: clearDisplay();
	///////outputLine((colorscheme?"Will assume white background!":"Will assume black background!"));
}
*/
void initDisplay(){
	/*
	outputControlText("=3h"); // 80x25 color mode
	outputControlText("?3l"); // switch to 132 column mode (if possible)
	*/
	outputControlText("0m");
	setColorscheme(getColorscheme());
	resetOutputColor(); // MDH@16MAR2020: think we need this
	clearScreenFromCursor(); // activate the current color scheme
	activateWrapmode(); // MDH@26JUN2020: sync the wrap mode to the initial setting
	///////setWrapping(amWrapping()); // activate the current wrap mode!!!
}

static int windowRows=0,windowCols=0;
// MDH@15MAR2020: could be useful (for automatic wrapping) to know the window size
bool windowSizeDetermined() {
	struct winsize ws;
	if(ioctl(STDOUT_FILENO,TIOCGWINSZ,&ws)==-1||ws.ws_col==0)return false;
	windowCols=ws.ws_col;
	windowRows=ws.ws_row;
	return true;
}
int getNumberOfWindowTextLines(){return windowRows;}
int getNumberOfWindowTextColumns(){return windowCols;}

int getCurrentNumberOfWindowTextColumns(){windowSizeDetermined();return windowCols;}

bool sessionInitialized(char *outputfilenamePrefix,char *outputfilenameSuffix){
	_timestampedOutputFilename=owned_string(_getTimestamp(".%Y%m%d.%H%M%S"),owner_timestampedOutputFilename); // MDH@24JAN2023: replacing: getOwner(__LINE__));
  if(_timestampedOutputFilename&&string_prepend(_timestampedOutputFilename,outputfilenamePrefix))outputFilenamePrefixLength=strlen(outputfilenamePrefix);
	if(_timestampedOutputFilename&&string_append(_timestampedOutputFilename,outputfilenameSuffix))outputFilenameSuffixLength=strlen(outputfilenameSuffix);
	initDisplay();
	// interfaces with initDisplay() TODO perhaps initialize settings here for a common interactive session???
	return windowSizeDetermined();
}

// MDH@17OCT2019: instead of setting the back color we can return the text to be used in out to set the back color
// set the backcolor
Mvalue* Mbc(Mvalue* _value){
	long long ll=(_value?getValueInteger(_value):-1); // all negative colors default to the back color
	if(ll==M_LL_INVALID)return NULL;
	char s[16];if(ll>=0)sprintf(s,"'\\033[48;5;%lldm",ll%256);else sprintf(s,"'\\033[48;5;%sm",getBackgroundColor());
	////////////output("ANSI background color code: '%s'.\n",s);
	char* _s=strdup(s);if(!_s)return NULL;
	Mvalue* _textValue=_getTextValue(_s);free(_s);
	return _textValue;
	/* replacing:
	Mstring* _valueText=_getValueText(_value,true);
	size_t result=string_length(_valueText);
	if(result>0)setBackColor(string(_valueText));
	free_string(_valueText);
	return _getIntegerValue(result);
	*/
}
 // set text color
Mvalue* Mtc(Mvalue* _value){
	long long ll=(_value?getValueInteger(_value):-1);
	if(ll==M_LL_INVALID)return NULL;
	char s[16];if(ll>=0)sprintf(s,"'\\033[38;5;%lldm",ll%256);else sprintf(s,"'\\033[38;5;%sm",getInfoColor());
	////////output("ANSI foreground color code: '%s'.\n",s);
	char* _s=strdup(s);if(!_s)return NULL;
	Mvalue* _textValue=_getTextValue(_s);free(_s);
	return _textValue;
	/*
	Mstring* _valueText=_getValueText(_value,true);
	size_t result=string_length(_valueText);
	if(result>0)setColor(string(_valueText));
	free_string(_valueText);
	return _getIntegerValue(result);
	*/
}