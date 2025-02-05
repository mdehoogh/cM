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
extern char* M_ERROR_PREFIX;
extern char* M_MESSAGE_PREFIX;

static struct termios orig_termios;

/**
 * @brief the global raw mode indicator 
 * 
 */
static int16_t rawmode=-1;
/**
 * @brief disables raw mode
 * 
 */
void disableRawmode(){
	if(rawmode<0)return;
	output("\nDisabling character input mode.\n");
	rawmode=-1;
	tcsetattr(STDIN_FILENO,TCSAFLUSH,&orig_termios);
}

// MDH@23OCT2021: moved over from M.c as we need it in endOfInput() below
/**
 * @brief returns a new M string wrapped current timestamp text in format \p format
 * 
 * @param format 
 * @return Mstring* a new M string wrapped current timestamp text in format \p format
 */
Mstring* _getTimestamp(char const * const format){Mallocationowner owner=getOwner(__LINE__);
	Mstring* _timestamp=owned_string(__string(),owner);
	if(_timestamp!=NULL){
		Mstring* p=string_setlength(_timestamp,50/*,owner*/);
		if(p!=NULL){
			time_t now=time(NULL);
			struct tm * nowlocal=localtime(&now);
			p=string_setlength(p,strftime(p->_chars->chars,50,(format!=NULL?format:"%Y-%m-%d %H:%M:%S"),nowlocal)/*,owner*/);
		}
		if(NULL==p){FREE_STRING(_timestamp,owner);_timestamp=NULL;}
	}
	return disowned_string(_timestamp,owner);
}
/**
 * @brief the global (timestamped) output filename
 * 
 */
Mstring* _timestampedOutputFilename=NULL;Mallocationowner owner_timestampedOutputFilename={MI_SESSION,__LINE__,1};
/**
 * @brief the global output filename prefix length
 * 
 */
size_t outputFilenamePrefixLength;
/**
 * @brief the global output filename suffix length
 * 
 */
size_t outputFilenameSuffixLength;
/**
 * @brief responds to the end of user input
 * @details renames the default output file name M.log to a timestamped output filename
 */
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
			output("\n%s%sFailed to timestamp output file '%s'.\n",M_ERROR_PREFIX,M_MESSAGE_PREFIX,outputFilename);
		else
			output("\nOutput file '%s' renamed to '%s'.\n",outputFilename,string(_timestampedOutputFilename));
		free(_outputFilenamePrefix); // ESSENTIAL
	}
	//////outputChar('\n');
	disableRawmode();
	output("\n%s\n","Thanks for using M.");
}

// MDH@30JUN2020: let's allow a timeout (number of tenths of seconds to block for input every time)
/**
 * @brief enables raw mode, if not already enabled
 * @details sets raw mode indicator to \p timeout
 * 
 * @param timeout 
 */
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
/**
 * @brief reads a single user input character (in raw mode)
 * 
 * @param _c the character read
 * @return true on success
 * @return false on failure
 */
bool inputCharRead(char* _c){
	enableRawmode(0);
	return(read(STDIN_FILENO,_c,1)==1);
}
// MDH@12JUL2020: even if updateFunction is NULL we switch to using a 1/10s timeout, only when updateFunction is not NULL do we execute the update function!!!!
//				this is so that we can use NULL to read characters received as part of an escape sequence!!
/**
 * @brief read a single input character non blocking calling update function \p updateFunction when done
 * 
 * @param _c the chararacter read
 * @param updateFunction the update function called
 * @return true on success
 * @return false on failure
 */
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
/**
 * @brief moves the cursor one line up
 * 
 */
void oneLineUp(){outputControlText("1A");} // ascertain that the previous line is visible
/**
 * @brief moves the cursor one line down
 * 
 */
void oneLineDown(){outputControlText("1B");} // one line down
/**
 * @brief moves the cursor to the start of the line
 * 
 */
void toStartOfLine(){outputChar('\r');} // replacing: '\r');}
/**
 * @brief clears the entire current cursor line
 * 
 */
void clearLine(){
	toStartOfLine();outputControlText("K");
} // MDH@30OCT2019: adjusted to always to the start of the line before clearing it, this is to ascertain that any called does not need toStartOfLine() per se
/**
 * @brief moves the cursor \p pos positions to the left
 * 
 * @param pos 
 */
void moveCursorLeft(uint16_t pos){
	if(pos)output(ES"%huD",pos);
} // TODO can't use outputControlText here!!!
/**
 * @brief moves the cursor \p pos positions to the right
 * 
 * @param pos 
 */
void moveCursorRight(uint16_t pos){
	if(pos)output(ES"%huC",pos);
} // TODO can't use outputControlText here!!!
/**
 * @brief clears the screen from the current cursor position
 * 
 */
void clearScreenFromCursor(){
	outputControlText("J");
}
/**
 * @brief clears the entire screen (starting at the cursor position)
 * 
 */
void clearDisplay(){
	clearScreenFromCursor();// replacing: outputControlText("2J");
}
/**
 * @brief beeps
 * 
 */
void beep(){outputChar('\a');} // replacing \a
/**
 * @brief removes the character behind the cursor
 * 
 */
void removeLastCharacter(){outputChar('\b');}
/**
 * @brief hides the cursor
 * 
 */
void hidecursor(){outputControlText("?25l");}
/**
 * @brief shows the cursor
 * 
 */
void showcursor(){outputControlText("?25h");}
/**
 * @brief outputs a single empty line
 * 
 */
void emptyline(){outputControlText("2K\r");}
/**
 * @brief performs a 'backspace' by moving the cursor left one position and clearing the screen from the cursor
 * 
 */
void backspace(){outputControlText("D"); /* go left one character */ outputControlText("K"); /* clear the rest of the line */}
/**
 * @brief outputs text color text \p colorText
 * 
 * @param colortext 
 */
void setColor(char const * const colortext){
	output(ES"38;5;%sm",colortext);
}
/**
 * @brief outputs background color text \p colortext
 * 
 * @param colortext 
 */
void setBackColor(char const * const colortext){
	output(ES"48;5;%sm",colortext);
}
/**
 * @brief resets the output foreground and background colors
 * 
 */
void resetOutputColor(){
	setColor(getInfoColor());
	setBackColor(getBackgroundColor());
}

// MDH@27FEB2020: delegating to outputInfo after resetting the output color
/**
 * @brief outputs text \p s and a newline character
 * 
 * @param s the text to output
 */
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
/**
 * initializes the screen at the start of the M interpreter
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
/**
 * @brief the global current window available character lines
 * 
 */
static int windowRows=0;
/**
 * @brief the global available character positions per line of the current window
 * 
 */
static int windowCols=0;
// MDH@15MAR2020: could be useful (for automatic wrapping) to know the window size
/**
 * @brief updates the global number of window lines and window line character positions
 * 
 * @return true on success
 * @return false on failure
 */
bool windowSizeDetermined() {
	struct winsize ws;
	if(ioctl(STDOUT_FILENO,TIOCGWINSZ,&ws)==-1||ws.ws_col==0)return false;
	windowCols=ws.ws_col;
	windowRows=ws.ws_row;
	return true;
}
/**
 * @brief returns the number of available window text lines
 * 
 * @return int the number of available window text lines
 */
int getNumberOfWindowTextLines(){return windowRows;}
/**
 * @brief returns the number of available character positions per text line
 * 
 * @return int 
 */
int getNumberOfWindowTextColumns(){return windowCols;}
/**
 * @brief updates and returns the number of available character positions per text line
 * 
 * @return int 
 */
int getCurrentNumberOfWindowTextColumns(){windowSizeDetermined();return windowCols;}
/**
 * @brief initializes a M interpreter session
 * 
 * @param outputfilenamePrefix the prefix of the M output file
 * @param outputfilenameSuffix the suffix of the M output file
 * @return true on success
 * @return false on failure
 */
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
/**
 * @brief returns the (wrapped) background color text associated with the integer wrapped in \p _value 
 * 
 * @param _value 
 * @return Mvalue* the (wrapped) background color text associated with the integer wrapped in \p _value 
 */
Mvalue* Mbc(Mvalue* _value){
	long long ll=(_value!=NULL?getValueInteger(_value):-1); // all negative colors default to the back color
	if(ll==M_LL_INVALID)return NULL;
	char s[16];if(ll>=0)sprintf(s,"'\\033[48;5;%lldm",ll%256);else sprintf(s,"'\\033[48;5;%sm",getBackgroundColor());
	////////////output("ANSI background color code: '%s'.\n",s);
	char* _s=strdup(s);if(NULL==_s)return NULL;
	Mvalue* _textValue=_getTextValue(_s);
	free(_s);
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
/**
 * @brief returns the (wrapped) output text color text associated with the integer wrapped in \p _value
 * 
 * @param _value 
 * @return Mvalue* the (wrapped) output text color text associated with the integer wrapped in \p _value
 */
Mvalue* Mtc(Mvalue* _value){
	long long ll=(_value!=NULL?getValueInteger(_value):-1);
	if(ll==M_LL_INVALID)return NULL;
	char s[16];if(ll>=0)sprintf(s,"'\\033[38;5;%lldm",ll%256);else sprintf(s,"'\\033[38;5;%sm",getInfoColor());
	////////output("ANSI foreground color code: '%s'.\n",s);
	char* _s=strdup(s);if(NULL==_s)return NULL;
	Mvalue* _textValue=_getTextValue(_s);
	free(_s);
	return _textValue;
	/*
	Mstring* _valueText=_getValueText(_value,true);
	size_t result=string_length(_valueText);
	if(result>0)setColor(string(_valueText));
	free_string(_valueText);
	return _getIntegerValue(result);
	*/
}