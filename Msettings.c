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
    if(verbose)output("\n%s",(assisting?"Will assist!":"Will not assist!"));
}
void setDebugging(bool newDebugging){
    debugging=newDebugging;
    if(verbose)output("\n%s",(debugging?"Will debug!":"Will not debug!"));
}
void setMatchingparentheses(bool newMatchingparentheses){
    matchingparentheses=newMatchingparentheses;
    if(verbose)output("\n%s",(matchingparentheses?"Will match parentheses!":"Will not match parentheses!"));
}
void setVerbose(bool newVerbose){
    if(verbose)output("\n%s",(newVerbose?"Will not be silent!":"Will be silent!"));
    verbose=newVerbose;
}
void setAcceptinghistorycommand(bool newAcceptinghistorycommand){
    acceptinghistorycommand=newAcceptinghistorycommand;
    if(verbose)output("\n%s",(acceptinghistorycommand?"Will use history command immediately!":"Will use history command as auto-completion!"));
}

// 'Origin' mode (not 'wrap' mode) in 132 columns (if possible)
void activateWrapmode(){outputControlText(wrapping?"?6l":"?7l");if(verbose)output(wrapping?"\nWill wrap!\n":"\nWill not wrap!\n");}
// M settings
void setWrapping(bool newWrapping){
	wrapping=newWrapping;
    activateWrapmode();
}



