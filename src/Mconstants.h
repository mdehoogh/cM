/**
 * MDH@02MAY2019:
 * constants
 */

// fixed constants

// global ANSI escape sequence prefix...
// as used by Moutput (could be defined there though)
#define ES "\033["

#define ESCAPE_CHARACTER 27

#define UNDEFINED_VALUETEXT "?"

// MDH@09DEC2020
#define M_CLOCKS_PER_MS (CLOCKS_PER_SEC*1000)

// MDH@29APR2024: OS dependent macros
#if defined _WIN32 || defined _WIN64 || defined __WIN32 || defined _WCE || defined MSDOS || defined __MSDOS || defined OS2 || defined _OS2 || defined __OS2___
#    define FILE_EOLN "\r\n"
#    define PATH_SEP "\\"
#    define WRITE_FILE_CONTENTS "type"
#    define WRITE_DIR_FILES "dir"
#    define ECHO_TEXT_PREFIX "echo -n /p=\""
#    define ECHO_TEXT_SUFFIX "\""
#    define WHERE_COMMAND "where"
#    define ECHO_DOUBLE_QUOTE "\""
#else
#    define FILE_EOLN "\n"
#    define PATH_SEP "/"
#    define WRITE_FILE_CONTENTS "cat"
#    define WRITE_DIR_FILES "ls"
#    define ECHO_TEXT_PREFIX "echo $'"
#    define ECHO_TEXT_SUFFIX "'"
#    define WHERE_COMMAND "whereis"
#    define ECHO_DOUBLE_QUOTE "\\"
#endif
