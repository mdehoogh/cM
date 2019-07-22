/**
 * MDH@02MAY2019:
 * - interactive session stuff
 */

#include <stdbool.h>
#include <unistd.h>
#include <termios.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdlib.h>

#include "Msession.h"

#include "Msettings.h"
#include "Mconstants.h"
#include "Mcolors.h"
#include "Moutput.h"

struct termios orig_termios;

bool rawmode=false;

void disableRawmode(){
    if(!rawmode)return;
	rawmode=false;
	tcsetattr(STDIN_FILENO,TCSAFLUSH,&orig_termios);
}

void endOfUserInput(){
	// return to the 'right' colors
	resetOutputColor();
	output("\n\n%s\n\n","Thanks for using M.");
	disableRawmode();
}

void enableRawmode(){
    if(rawmode)return;
	rawmode=true;
	tcgetattr(STDIN_FILENO,&orig_termios);
	atexit(endOfUserInput); // or std::atexit() in C++
	struct termios raw=orig_termios;

  	// ISIG turns off Ctrl-C and Ctrl-Z
	raw.c_lflag&=~(ECHO|ICANON|ISIG); // we kill echoing so we can first look at what we received!!
	tcsetattr(STDIN_FILENO,TCSAFLUSH,&raw);
}

///////char inputChar='\0'; // the last read input character and its associated type (which we can set to o to escape to control mode!!)
// currently inputCharRead() blocks until a character can be read (and put in inputChar)
bool inputCharRead(char* inputChar){enableRawmode();return(read(STDIN_FILENO,inputChar,1)==1);}
int kbhit(){
    struct timeval tv={0L,0L};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    return select(1,&fds,NULL,NULL,&tv);
}
int getch(){
	// ASSERT assume in one-character-at-a-time-mode!!!
    int r;unsigned char c;
    if ((r=read(STDIN_FILENO,&c,sizeof(c)))<0)return r;
    return c;
}

//////char getInputChar(){return inputChar;}

// interfacing with the console
void oneLineUp(){outputControlText("1A");} // ascertain that the previous line is visible
void oneLineDown(){outputControlText("1B");} // one line down
void toStartOfLine(){outputChar('\r');}
void clearLine(){outputControlText("K");}
void clearDisplay(){outputControlText("2J");}
void moveCursorLeft(uint16_t pos){if(pos)output(ES"%huD",pos);} // TODO can't use outputControlText here!!!
void moveCursorRight(uint16_t pos){if(pos)output(ES"%huC",pos);} // TODO can't use outputControlText here!!!
void clearScreenFromCursor(){outputControlText("J");}
void beep(){outputChar('\a');}
void removeLastCharacter(){outputChar('\b');}
void hidecursor(){outputControlText("?25l");}
void showcursor(){outputControlText("?25h");}
void emptyline(){outputControlText("2K\r");}
void backspace(){outputControlText("D"); /* go left one character */ outputControlText("K"); /* clear the rest of the line */}

void setColor(const char* colortext){output(ES"38;5;%sm",colortext);}
void setBackColor(const char* colortext){output(ES"48;5%sm",colortext);}

void resetOutputColor(){setColor(getInfoColor());setBackColor(getBackgroundColor());}

void outputLine(char* s){resetOutputColor();output("%s\n",s);} // for writing a single line of output text in the info color

void initDisplay(){
	outputControlText("=3h"); // 80x25 color mode
	outputControlText("?3l"); // switch to 132 column mode (if possible)
	outputControlText("0m");
	setColorscheme(getColorscheme()); // activate the current color scheme
	///////setWrapping(amWrapping()); // activate the current wrap mode!!!
}

void initSession(){ 
    // interfaces with initDisplay() TODO perhaps initialize settings here for a common interactive session???
    initDisplay();
}
