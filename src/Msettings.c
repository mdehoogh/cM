#include "Msettings.h"

extern unsigned long long M_MODULE_DEBUGGING;

static Mallocationowner getOwner(uint16_t id){return(Mallocationowner){MI_SETTINGS,id};}

extern char const * const M_ERROR_PREFIX;
extern char const * const M_INFO_PREFIX;

// Edit flags
/**
 * @brief flag indicating whether to immediately accept a history command
 * 
 */
bool acceptinghistorycommand=true; // whether to immediately accept a history command
/**
 * @brief flag indicating whether to match parentheses in showing feed forward
 * 
 */
bool matchingparentheses=true;  // by default will 'match' parentheses

#ifdef __DEBUG__
/**
 * @brief the assisting flag indicating whether to assist the user
 * 
 */
bool assisting=true; // assist flag can be turned on to guide the user
/**
 * @brief the debugging flag indicating whether to show debug information to the user
 * 
 */
bool debugging=true; // program debugging flag so it will show the token information before evaluation of a command
/**
 * @brief the verbose flag indicating whether to show additional operational information to the user
 * 
 */
bool verbose=true;
#else
/**
 * @brief the assisting flag indicating whether to assist the user
 * 
 */
bool assisting=false; // assist flag can be turned on to guide the user
/**
 * @brief the debugging flag indicating whether to show debug information to the user
 * 
 */
bool debugging=false; // program debugging flag so it will show the token information before evaluation of a command
/**
 * @brief the verbose flag indicating whether to show additional operational information to the user
 * 
 */bool verbose=false;
#endif

/**
 * @brief the wrapping flag indicating whether to wrap 
 * 
 */
bool wrapping=true;

// getters
/**
 * @brief returns the assisting flag
 * 
 * @return true 
 * @return false 
 */
bool amAssisting(){return assisting;}
/**
 * @brief returns the verbose flag
 * 
 * @return true 
 * @return false 
 */
bool amVerbose(){return verbose;}
/**
 * @brief returns the wrapping flag
 * 
 * @return true 
 * @return false 
 */
bool amWrapping(){return wrapping;}
/**
 * @brief returns the debugging flag
 * 
 * @return true 
 * @return false 
 */
bool amDebugging(){return debugging;}
/**
 * @brief returns the match parentheses flag
 * 
 * @return true 
 * @return false 
 */
bool amMatchingparentheses(){return matchingparentheses;}
/**
 * @brief returns the accepting history command flag
 * 
 * @return true 
 * @return false 
 */
bool amAcceptinghistorycommand(){return acceptinghistorycommand;}
/**
 * @brief returns true if both the verbose and debugging flag are true, false otherwise
 * 
 * @return true 
 * @return false 
 */
bool amVerboseDebugging(){return verbose&&debugging;} // MDH@15MAY2020: convenience function

// flags used in (interactive) session mode
/**
 * @brief sets the assisting flag to \p newAssisting
 * 
 * @param newAssisting 
 */
void setAssisting(bool newAssisting){
	assisting=newAssisting;
	output("Will %sassist!\n",(assisting?"":"not "));
}
/**
 * @brief sets the debugging flag to \p newDebugging
 * 
 * @param newDebugging 
 */
void setDebugging(bool newDebugging){
	debugging=newDebugging;
	output("Will %sdebug!\n",(debugging?"":"not "));
}
/**
 * @brief sets the matching parentheses flag to \p newMatchingParentheses
 * 
 * @param newMatchingparentheses 
 */
void setMatchingparentheses(bool newMatchingparentheses){
	matchingparentheses=newMatchingparentheses;
	output("Will %smatch parentheses!\n",(matchingparentheses?"":"not "));
}
/**
 * @brief sets the verbose flag to \p newVerbose
 * 
 * @param newVerbose 
 */
void setVerbose(bool newVerbose){
	verbose=newVerbose;
	output("Will %sbe verbose!\n",(verbose?"":"not "));
}
/**
 * @brief sets the accept history command flag to \p newAcceptinghistorycommand
 * 
 * @param newAcceptinghistorycommand 
 */
void setAcceptinghistorycommand(bool newAcceptinghistorycommand){
	acceptinghistorycommand=newAcceptinghistorycommand;
	output("Will use history command %s!\n",(acceptinghistorycommand?"immediately":"as auto-completion"));
}

// 'Origin' mode (not 'wrap' mode) in 132 columns (if possible)
/**
 * @brief activates the current wrap mode (whenever the wrapping flag changes)
 * 
 */
void activateWrapmode(){
	int result=0;
	// result=system(wrapping?"tput smam":"tput rmam"); 
	result=outputControlText(wrapping?"?7h":"?7l"); // 7h used to be 6l doesn't seem to work though, 6h === 7l????
	if(result)
		q2outputMessage(M_ERROR_PREFIX,"Failed to %s wrapping.",(wrapping?"activate":"deactivate"));
	else 
		q2outputMessage(M_INFO_PREFIX,"Will %swrap!",(wrapping?"":"not "));
}
// M settings
/**
 * @brief sets the wrapping flag to \p newWrapping and activates it
 * 
 * @param newWrapping 
 */
void setWrapping(bool newWrapping){
	wrapping=newWrapping;
	activateWrapmode();
}



