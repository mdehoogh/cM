#include <time.h>

#include "Mtime.h"

extern unsigned long long M_MODULE_DEBUGGING;
#define DEBUGGING (M_MODULE_DEBUGGING&MM_TIME)

static Mallocationowner getOwner(uint16_t id){return (Mallocationowner){MI_TIME,id};}

Mvalue* Mnow(){
    time_t t=time(NULL);
    return _getValueOfTime(_getTime("Mnow()",t)); 
}

Mvalue* Mparsetime(Mvalue* _timetextValue){
    // let's accept any text that conforms to ISO8601 i.e. a calendar date with timezone information of the format <date><time><timezone> where <time>should start with T and <timezone> with either - or + or Z
    if(_timetextValue){
        if(_timetextValue->type==VT_INTEGER||_timetextValue->type==VT_BIGINTEGER){
            long long lltime=getValueInteger(_timetextValue);
            if(lltime>=0)return _getValueOfTime(_getTime("Mparsetime()",lltime));
        }else
        if(_timetextValue->type==VT_TEXT){
            // <date> <time> <timezone>, <date> ends with either T or -|+|Z, <time> ends with either T or -|+|Z or the end of the string (in which it assumes Z)
            // <date> consists of exactly three numeric parts, <time> exists of at least one numeric part indicating the hour of the day
            // we'll ignore any nonnumeric characters in between?
        }else
        if(_timetextValue->type==VT_MAP){
        }else
            outputError("Time initializer not an integer, text or map");
    }
    return NULL;
}