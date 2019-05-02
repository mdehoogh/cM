/**
 * MDH@02MAY2019:
 * global M settings
 */
#include <stdbool.h>

#include "Mconstants.h"

// getters
bool amAssisting();
bool amVerbose();
bool amDebugging();
bool amMatchingparentheses();
bool amAcceptinghistorycommand();
bool amWrapping();

// setters
void setVerbose(bool newVerbose);
void setAssisting(bool newAssisting);
void setDebugging(bool newDebugging);
void setMatchingparentheses(bool newMatchingparentheses);
void setAcceptinghistorycommand(bool newAcceptinghistorycommand);
void setWrapping(bool newWrapping);