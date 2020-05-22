#include "Msettings.h"

static uint32_t const MODULE_ID=6;
static Mallocationowner getOwner(uint16_t id){return (Mallocationowner){(MODULE_ID<<16)+id,0,0};}

// Edit flags
bool acceptinghistorycommand=true; // whether to immediately accept a history command
bool matchingparentheses=true;  // by default will 'match' parentheses

#ifdef __DEBUG__
bool assisting=true; // assist flag can be turned on to guide the user
bool debugging=true; // program debugging flag so it will show the token information before evaluation of a command
bool verbose=true;
#else
bool assisting=false; // assist flag can be turned on to guide the user
bool debugging=false; // program debugging flag so it will show the token information before evaluation of a command
bool verbose=false;
#endif

bool wrapping=true;

// getters
bool amAssisting(){return assisting;}
bool amVerbose(){return verbose;}
bool amWrapping(){return wrapping;}
bool amDebugging(){return debugging;}
bool amMatchingparentheses(){return matchingparentheses;}
bool amAcceptinghistorycommand(){return acceptinghistorycommand;}

bool amVerboseDebugging(){return verbose&&debugging;} // MDH@15MAY2020: convenience function

// flags used in (interactive) session mode
void setAssisting(bool newAssisting){
    assisting=newAssisting;
    output("Will %sassist!\n",(assisting?"":"not "));
}
void setDebugging(bool newDebugging){
    debugging=newDebugging;
    output("Will %sdebug!\n",(debugging?"":"not "));
}
void setMatchingparentheses(bool newMatchingparentheses){
    matchingparentheses=newMatchingparentheses;
    output("Will %smatch parentheses!\n",(matchingparentheses?"":"not "));
}
void setVerbose(bool newVerbose){
    verbose=newVerbose;
    output("Will %sbe verbose!\n",(verbose?"":"not "));
}
void setAcceptinghistorycommand(bool newAcceptinghistorycommand){
    acceptinghistorycommand=newAcceptinghistorycommand;
    output("Will use history command %s!\n",(acceptinghistorycommand?"immediately":"as auto-completion"));
}

// 'Origin' mode (not 'wrap' mode) in 132 columns (if possible)
void activateWrapmode(){
    outputControlText(wrapping?"?7h":"?7l"); // 7h used to be 6l doesn't seem to work though, 6h === 7l????
    output("Will %swrap!\n",(wrapping?"":"not "));
}
// M settings
void setWrapping(bool newWrapping){
	wrapping=newWrapping;
    activateWrapmode();
}



