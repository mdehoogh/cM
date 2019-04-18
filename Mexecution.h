/**
MDH@18APR2019:
- Any command executes inside an environment in which variables and functions are defined
  A variable is an identifier and an associated (literal) value!
  Because any value can also be a list, it might be a good idea to store a single value as a list of one element
  Functions have an associated identifier (different from the names of variables) that can be executed on a list of argument values
  The pointer to the name of the variable/function does not need to be 
*/
#include <math.h>

// Token and Mexpression is provided in Mexpression.h
#include "Mexpression.h"

// defining VALUE_TYPES as an enum defining all possible value types
enum Mvaluetype {VT_NUMBER,VT_STRING,VT_LIST,VT_MAP};

// we define the names of 'standard' function but it is a good idea to classify them by the number of arguments

typedef union Mnumber{
    long long l;
    double d;
}Mnumber;

struct Mvalue;
typedef struct Mvaluelistelement{
    struct Mvalue* value;
    struct Mvaluelistelement* next;
}Mvaluelistelement;

typedef struct{
    Mvaluelistelement* first;
    Mvaluelistelement* last;
    uint32_t numberOfValues; // keep track of the total number of variables
}Mvaluelist;

typedef struct Mvaluemapelement{
    char* name;
    struct Mvalue* value;
    struct Mvaluemapelement* next;
}Mvaluemapelement;

typedef struct Mvaluemap{
    Mvaluemapelement* first;
    Mvaluemapelement* last;
    uint32_t numberOfValues; // keep track of the total number of variables
}Mvaluemap;

typedef union{
    union Mnumber* n;
    mstring* s;
    Mvaluelist* vl;
    Mvaluemap* vm;
}Mvalueunion;

// a Value is either a number (numeric literal), a string literal, a list of values or a map
// you could say that a map is a list of variables, as such Menvironment holds a map of variables and a map of functions
// and we could make a separate struct to hold a map
typedef struct{
    enum Mvaluetype type;
    Mvalueunion value;
}Mvalue;

//Mvalue* getVariableValue(Mvariablelist variablelist,char* name);

typedef struct Mexpressionlistelement{
    Mexpression expression;
    struct Mexpressionlistelement* next;
}Mexpressionlistelement;

typedef struct{
    Mexpressionlistelement* first;
    Mexpressionlistelement* last;
}Mexpressionlist;

// functions
typedef struct{
    Mvaluemap* parameters; // a map of values defines the parameters and their default values (implicitly defining the expected types)
    Mexpressionlist* body;
}Mfunction;

typedef struct Mfunctionmapelement{
    mstring* name;
    Mfunction* function;
    struct Mfunctionmapelement* next;
}Mfunctionmapelement;

typedef struct{
    Mfunctionmapelement* first;
    Mfunctionmapelement* last;
}Mfunctionmap;
//Mvalue* getFunction(Mfunctionlist functionlist,char* name);

// environments

// an environment is a bag of variables and functions
typedef struct Menvironment{
    Mvaluemap* variableMap; // variables are stored by name
    Mfunctionmap* functionMap; // this would be the map of M functions defined in this environment (i.e. not the C functions/constants)
    struct Menvironment* parent;
}Menvironment;

// function prototypes
bool addVariable(Menvironment* environment,char* name,enum Mvaluetype valueType);
uint32_t getNumberOfVariables(Menvironment* environment);