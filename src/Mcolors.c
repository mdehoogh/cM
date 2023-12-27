#include <stdint.h>
#include <inttypes.h>

#include "Mcolors.h"

// MDH@16APR2019: let's define the standard colors and high-itensity colors which are dark and light versions
const char BLACK[]="0";
const char DARK_RED[]="1";
const char DARK_GREEN[]="2";
const char DARK_YELLOW[]="3";
const char DARK_BLUE[]="4";
const char DARK_PURPLE[]="5";
const char DARK_CYAN[]="6";
const char DARK_GREY[]="7";
const char DARKER_GREY[]="239";
const char LIGHT_GREY[]="8";
const char LIGHTER_GREY[]="255";
const char LIGHT_RED[]="9";
const char LIGHT_GREEN[]="10";
const char LIGHT_YELLOW[]="11";
const char LIGHT_BLUE[]="45"; // "12" is really TOO dark!!
const char LIGHT_PURPLE[]="13";
const char LIGHT_CYAN[]="14";
const char CYAN[]="39";
const char GREY[]="249"; // MDH@07OCT2019: between dark and light grey
const char BLUE[]="27"; // MDH@07OCT2019: what we use for the identifier continuation text
const char WHITE[]="15";
const char ORANGE[]="202"; // instead of DARK_YELLOW use (a dark version of) ORANGE
/* the background colors (which are not used behind 38;5 or 48;5 but directly )
const char BACKGROUND_BLACK[]="40";
const char BACKGROUND_WHITE[]="47";
*/
// colors
// MDH@17OCT2019: switching to colors that do use the 256-color table i.e. BLACK and WHITE instead of BACKGROUND_BLACK and BACKGROUND_WHITE
const char* BACKGROUND_COLORS[NUMBER_OF_COLOR_SCHEMES]={BLACK,WHITE}; // assuming either a black or white background

const char* DEBUG_COLORS[NUMBER_OF_COLOR_SCHEMES]={LIGHT_GREY,DARK_GREY};
const char* INFO_COLORS[NUMBER_OF_COLOR_SCHEMES]={WHITE,BLACK};

const char* COMMENT_COLORS[NUMBER_OF_COLOR_SCHEMES]={LIGHT_GREY,DARK_GREY};
const char* ERROR_COLORS[NUMBER_OF_COLOR_SCHEMES]={LIGHT_RED,LIGHT_RED};

const char* ASSIGNMENT_COLORS[NUMBER_OF_COLOR_SCHEMES]={LIGHT_PURPLE,DARK_PURPLE};
const char* UNARY_OPERATOR_COLORS[NUMBER_OF_COLOR_SCHEMES]={LIGHT_PURPLE,DARK_PURPLE};
const char* BINARY_OPERATOR_COLORS[NUMBER_OF_COLOR_SCHEMES]={LIGHT_PURPLE,DARK_PURPLE};
const char* TERNARY_OPERATOR_COLORS[NUMBER_OF_COLOR_SCHEMES]={LIGHT_PURPLE,DARK_PURPLE};

const char* EXPRESSION_COLORS[NUMBER_OF_COLOR_SCHEMES]={WHITE,BLACK};
const char* NEW_VARIABLE_COLORS[NUMBER_OF_COLOR_SCHEMES]={LIGHT_BLUE,DARK_BLUE};
const char* VARIABLE_COLORS[NUMBER_OF_COLOR_SCHEMES]={LIGHT_CYAN,DARK_CYAN};
const char* REFERENCE_COLORS[NUMBER_OF_COLOR_SCHEMES]={LIGHT_YELLOW,ORANGE}; // yeah, you could call that a function call
const char* FUNCTION_COLORS[NUMBER_OF_COLOR_SCHEMES]={LIGHT_YELLOW,ORANGE}; /////"93"; // something more blueish
const char* LIST_COLORS[NUMBER_OF_COLOR_SCHEMES]={WHITE,BLACK};
const char* MAP_COLORS[NUMBER_OF_COLOR_SCHEMES]={WHITE,BLACK};
const char* NUMBER_COLORS[NUMBER_OF_COLOR_SCHEMES]={LIGHT_GREEN,DARK_GREEN}; //"22"; // green (to indicate a literal)
const char* STRING_COLORS[NUMBER_OF_COLOR_SCHEMES]={LIGHT_GREEN,DARK_GREEN}; //////12"; // green (to indicate a literal)

const char* RESULT_COLORS[NUMBER_OF_COLOR_SCHEMES]={WHITE,BLACK};
///////const char* OPTION_COLORS[]={BLACK,WHITE};
const char* PROMPT_COLORS[NUMBER_OF_COLOR_SCHEMES]={WHITE,BLACK}; // same as the info color

// MDH@26SEP2019: distinguishing between identifier continuation text and feed forward text (behind the cursor)
const char* FEED_FORWARD_TEXT_COLORS[NUMBER_OF_COLOR_SCHEMES]={LIGHT_GREEN,DARK_GREEN};
const char* IDENTIFIER_CONTINUATION_TEXT_COLORS[NUMBER_OF_COLOR_SCHEMES]={LIGHT_YELLOW,DARK_YELLOW}; //{LIGHTER_GREY,DARK_GREY}; // MDH@25SEP2019
const char* MANUAL_FEED_FORWARD_TEXT_COLORS[NUMBER_OF_COLOR_SCHEMES]={LIGHT_GREY,DARK_GREY}; // MDH@07OCT2019
const char* FEEDFORWARD_CLOSER_TEXT_COLORS[NUMBER_OF_COLOR_SCHEMES]={LIGHT_PURPLE,DARK_PURPLE}; // MDH@04DEC2023
const char* AUTO_COMPLETION_TEXT_COLORS[NUMBER_OF_COLOR_SCHEMES]={LIGHT_RED,DARK_RED}; // MDH@27OCT2021
// operator token colors (all the same)
const char** OPERATOR_TOKEN_COLORS[]={ASSIGNMENT_COLORS,UNARY_OPERATOR_COLORS,BINARY_OPERATOR_COLORS,TERNARY_OPERATOR_COLORS};

// value token colors
const char** VALUE_TOKEN_COLORS[]={EXPRESSION_COLORS,REFERENCE_COLORS,VARIABLE_COLORS,NEW_VARIABLE_COLORS,VARIABLE_COLORS,LIST_COLORS,NUMBER_COLORS,NUMBER_COLORS,STRING_COLORS,STRING_COLORS,STRING_COLORS,STRING_COLORS,LIST_COLORS,LIST_COLORS,MAP_COLORS,MAP_COLORS,MAP_COLORS,FUNCTION_COLORS,FUNCTION_COLORS,FUNCTION_COLORS};

/**
 * @brief the global active color scheme indicator
 * @details only possible values currently 0 (white) and 1 (black), toggle with C in control mode
 */
uint8_t colorscheme=0; // the active color scheme (either 0 for white, or 1 for black background), toggle with C in control mode

// exposing certain colors
/**
 * @brief returns the current color scheme info color
 * 
 * @return const char* the current color scheme info color
 */
const char* getInfoColor(){return INFO_COLORS[colorscheme];}
/**
 * @brief returns the current color scheme background color
 * 
 * @return const char* the current color scheme background color
 */
const char* getBackgroundColor(){return BACKGROUND_COLORS[colorscheme];}
/**
 * @brief returns the current color scheme error color
 * 
 * @return const char* the current color scheme error color
 */
const char* getErrorColor(){return ERROR_COLORS[colorscheme];}
/**
 * @brief returns the current color scheme feedforward text color
 * 
 * @return const char* the current color scheme feedforward text color
 */
const char* getFeedForwardTextColor(){return FEED_FORWARD_TEXT_COLORS[colorscheme];}
/**
 * @brief returns the current color scheme manual feed forward text color
 * 
 * @return const char* the current color scheme manual feed forward text color
 */
const char* getManualFeedforwardTextColor(){return MANUAL_FEED_FORWARD_TEXT_COLORS[colorscheme];}
/**
 * @brief returns the current color scheme identifier continuation text color
 * 
 * @return const char* the current color scheme identifier continuation text color
 */
const char* getIdentifierContinuationTextColor(){return IDENTIFIER_CONTINUATION_TEXT_COLORS[colorscheme];} // MDH@25SEP2019: special color for generated feed forward text

/**
 * @brief Get the FeedforwardCloser Text Color object
 * 
 * @return const char* the current color scheme feedforward closer text color
 */
const char* getExpectedCharacterStackTextColor(){return FEEDFORWARD_CLOSER_TEXT_COLORS[colorscheme];} // MDH@27OCT2021: special color for generated auto completion text

/**
 * @brief returns the current color scheme auto completion text color
 * 
 * @return const char* the current color scheme auto completion text color
 */
const char* getAutoCompletionTextColor(){return AUTO_COMPLETION_TEXT_COLORS[colorscheme];} // MDH@27OCT2021: special color for generated auto completion text
/**
 * @brief returns the current color scheme comment color
 * 
 * @return const char* the current color scheme comment color
 */
const char* getCommentColor(){return COMMENT_COLORS[colorscheme];}
/**
 * @brief returns the current color scheme token color of the operator with index \p opid
 * 
 * @param opid the operator index
 * @return const char* the current color scheme token color of the operator with index \p opid
 */
const char* getOperatorTokenColor(uint8_t opid){return OPERATOR_TOKEN_COLORS[opid][colorscheme];}
/**
 * @brief returns the token color of a value with token type \p tokentypeid
 * 
 * @param tokentypeid 
 * @return const char* the token color of a value with token type \p tokentypeid
 */
const char* getValueTokenColor(uint8_t tokentypeid){return VALUE_TOKEN_COLORS[tokentypeid][colorscheme];}

// display flags
/**
 * @brief returns the current color scheme
 * @details currentlu only 0 (white) and 1 (black) are supported
 * @return uint8_t the current color scheme
 */
uint8_t getColorscheme(){return colorscheme;}
uint8_t setColorscheme(uint8_t newColorscheme){
	colorscheme=(newColorscheme%NUMBER_OF_COLOR_SCHEMES);
    return colorscheme;
}
