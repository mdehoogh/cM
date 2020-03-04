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

#include "Msession.h"

struct termios orig_termios;

bool rawmode=false;

void disableRawmode(){
    if(!rawmode)return;
	printf("%s","Disabling character input mode.\n");
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
	printf("%s","Enabling character input mode.\n");
	rawmode=true;
	tcgetattr(STDIN_FILENO,&orig_termios);
	atexit(endOfUserInput); // or std::atexit() in C++
	struct termios raw=orig_termios;

  	// ISIG turns off Ctrl-C and Ctrl-Z
	raw.c_lflag&=~(ECHO|ICANON|ISIG); // we kill echoing so we can first look at what we received!!
	tcsetattr(STDIN_FILENO,TCSAFLUSH,&raw);
}

///////char inputChar='\0'; // the last read input character and its associated type (which we can set to o to escape to control mode!!)
// currently inputCharRead() blocks until a character can be read (and put in _c)
bool inputCharRead(char* _c){enableRawmode();return(read(STDIN_FILENO,_c,1)==1);}

int getch(){
	// ASSERT assume in one-character-at-a-time-mode!!!
    int r;unsigned char c;
    if ((r=read(STDIN_FILENO,&c,sizeof(c)))>0)return r; // MDH@11NOV2019: changed <0 into >0 which makes more sense considering how inputCharRead() is implemented!!!
    return c;
}

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

void resetOutputColor(){setColor(getInfoColor());setBackColor(getBackgroundColor());}

// MDH@27FEB2020: delegating to outputInfo after resetting the output color
void outputLine(char* s){resetOutputColor();output("%s\n",s);} // for writing a single line of output text in the info color

void activateColorscheme(){
	setBackColor(getBackgroundColor()); // MDH@17OCT2019 replacing: output(ES"%sm",getBackgroundColor()); // TODO can't use outputControlText here!!!
	clearScreenFromCursor(); // MDH@30OCT2019 replacing: clearDisplay();
	///////outputLine((colorscheme?"Will assume white background!":"Will assume black background!"));
}
void initDisplay(){
	outputControlText("=3h"); // 80x25 color mode
	outputControlText("?3l"); // switch to 132 column mode (if possible)
	outputControlText("0m");
	setColorscheme(getColorscheme());
	activateColorscheme(); // activate the current color scheme
	///////setWrapping(amWrapping()); // activate the current wrap mode!!!
}

void initSession(){ 
    // interfaces with initDisplay() TODO perhaps initialize settings here for a common interactive session???
    initDisplay();
}
