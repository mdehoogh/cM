/**
 * MDH@02MAY2019:
 * - all colors used for color coding tokens
 */
#include <inttypes.h>

#include "Mshell.h"

#define NUMBER_OF_COLOR_SCHEMES 2

const char* getInfoColor();
const char* getBackgroundColor();
const char* getErrorColor();

const char* getManualFeedforwardTextColor();
const char* getIdentifierContinuationTextColor();
const char* getFeedForwardTextColor();
const char* getAutoCompletionTextColor();
const char* getFinisherStackTextColor();
const char* getUnreachableFinisherStackTextColor(); // MDH@10DEC2024

const char* getCommentColor();
const char* getOperatorTokenColor(uint8_t opid);
const char* getValueTokenColor(uint8_t tokentypeid);

uint8_t getColorscheme();
uint8_t setColorscheme(uint8_t newColorscheme);