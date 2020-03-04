/**
 * MDH@02MAY2019:
 * - takes care of all interactive session stuff
 */

// terminal input stuff
#include <stdbool.h>
#include <inttypes.h>

// MDH@04MAR2020: Mcolors.h is the first interactive session file
#include "Mcolors.h"

// input functions
void enableRawmode();
bool inputCharRead(char* inputChar); // passing in a pointer to where the input character is to be stored...
// MDH@27FEB2020 moved over to Mmessage.h: int kbhit(); // check whether keyboard hit
//int getch();

/////////char getInputChar();
void endOfUserInput();

void oneLineUp(); // ascertain that the previous line is visible
void oneLineDown(); // one line down
void toStartOfLine();
void clearLine();
void clearDisplay();
void moveCursorLeft(uint16_t pos); // TODO can't use outputControlText here!!!
void moveCursorRight(uint16_t pos); // TODO can't use outputControlText here!!!
void clearScreenFromCursor();
void beep();
void removeLastCharacter();
void hidecursor();
void showcursor();
void emptyline();
void backspace();

void setColor(const char* colortext);
void setBackColor(const char* colortext);

void resetOutputColor();

void outputLine(char* s); // for writing a single line of output text in the info color

void initSession();

// moved over from Mfunctions.h/c because only available in interactive sessions
Mvalue* Mbc(Mvalue* _value);
Mvalue* Mtc(Mvalue* _value);