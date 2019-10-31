#include "Msettings.h"

#include "Moutput.h"

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

// flags used in (interactive) session mode
void setAssisting(bool newAssisting){
    assisting=newAssisting;
    output("\nWill %sassist!",(assisting?"":"not "));
}
void setDebugging(bool newDebugging){
    debugging=newDebugging;
    output("\nWill %sdebug!",(debugging?"":"not "));
}
void setMatchingparentheses(bool newMatchingparentheses){
    matchingparentheses=newMatchingparentheses;
    output("\nWill %smatch parentheses!",(matchingparentheses?"":"not"));
}
void setVerbose(bool newVerbose){
    verbose=newVerbose;
    output("\nWill %sbe verbose!",(verbose?"":"not "));
}
void setAcceptinghistorycommand(bool newAcceptinghistorycommand){
    acceptinghistorycommand=newAcceptinghistorycommand;
    output("\nWill use history command %s!",(acceptinghistorycommand?"immediately":"as auto-completion"));
}

// 'Origin' mode (not 'wrap' mode) in 132 columns (if possible)
void activateWrapmode(){
    outputControlText(wrapping?"?6l":"?7l"); // 7h used to be 6l doesn't seem to work though
    output("\nWill %s!",(wrapping?"wrap":"not wrap"));
}
// M settings
void setWrapping(bool newWrapping){
	wrapping=newWrapping;
    activateWrapmode();
}



