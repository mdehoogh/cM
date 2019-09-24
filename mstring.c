#include "Mstring.h"
#include "Malloc.h"

// MDH@21JUN2019: there's no need to set the end-of-string marker until a string is returned!!!
//                TODO if blocks is zero failed to 

/** MDH@25DEC2018: 
 *  this code is from the Internet to implement a mutable string
 *  however, letting string_get_all() return a string allocated on the heap which means it needs to be freed is 
 *  not a good idea, as we will need to release the returned pointer afterwards
 *  therefore I change the entire thing in a null terminated string to start with!!
 */

/** Create a String */
Mstring* __string(){
    Mstring* ans=CALLOC(1,sizeof *ans,'s');
    if(ans){
        // NOTE calloc() will make length and blocks 0: ans->length=0;ans->blocks=0;
        ans->chars=MALLOC(BLOCK_SIZE*sizeof *(ans->chars),'c');
        if(!ans->chars){FREE(ans,'s');ans=NULL;}else ans->blocks=1; // if the allocation failed we release ans immediately again, so ans->blocks will always be positive!!!
        // MDH@21JUN2019 replacing: if(ans->chars){ans->blocks=1;ans->chars[0]='\0';}
    }
#ifdef __DEBUGGING__
    if(!ans)printf("\nFailed to create a string.");
#endif
    return ans;
}

Mstring* _getString(const char* const s){
    if(!s)return NULL;
    Mstring* ans=CALLOC(1,sizeof *ans,'s');
    if(ans){
        // NOTE calloc() will make length and blocks 0: ans->length=0;ans->blocks=0;
        size_t l=strlen(s);
        ans->blocks=(l/BLOCK_SIZE); // NOTE that s actually is strlen(s)+1 characters (including the '\0' at the end)
        ans->chars=MALLOC((++ans->blocks)*BLOCK_SIZE,'c'); // here we increment ans->blocks (as we must)
        if(ans->chars){
            //////////////strcpy(ans->chars,s);ans->length=l; // also copies the ending '\0' over but memcpy() does not have to check for '\0' so we use memcpy()
            ans->length=l; // MDH@21JUN2019: no need to copy '\0' at the end!!! replacing: ans->length=l++; // store l, then increment it, so memcpy() will also copy '\0' over!!!
            memcpy(ans->chars,s,ans->length); // copy the actual characters over!!! // replacing: while(true){ans->chars[l]=s[l];if(l==0)break;l--;} // copying the characters over... TODO there's a faster way to do this of course
        }else{ // failure
            FREE(ans,'s');ans=NULL;
        }
    }
#ifdef __DEBUGGING__
    if(!ans)printf("\nFailed to create a string.");
#endif
    return ans;
}

// MDH@20JUN2019: instead of returning a bool (and requiring dst as second argument) we return the copy...
Mstring* _stringCopy(Mstring* const src,uint32_t length){
    if(!src)return NULL;
    src->chars[src->length]='\0'; // MDH@21JUN2019: mark the end of the text in the source (OOPS we would be in trouble otherwise)
    Mstring* _result=_getString(src->chars);
    if(length)if(_result)string_setlength(_result,length);
    return _result;
    /* replacing:
    Mstring* dst=__string();
    if(dst){
        if(src->length){ // there needs to be something to copy (NOTE we're not allocating down ever!!!!)
            // if we have more blocks for chars in src we have to realloc
            if(src->blocks>dst->blocks){
                char* new_str=realloc(dst->chars,BLOCK_SIZE*(src->blocks)*sizeof *(src->chars));
                if(!new_str)return false; // re-allocation failed!!!
                dst->blocks=src->blocks;
                dst->chars=new_str;
            }
            // ready to copy the characters over (one more than the length!)
            dst->length=src->length;
            strcpy(dst->chars,src->chars); // probably better than using memcpy as we do NOT have to tell where src->chars ends!!
        }
    }
    return dst;
    */
}

/** 
 * Free the memory associated with a String
 */
void free_string(Mstring* str){if(str){if(str->chars)FREE(str->chars,'c');FREE(str,'s');}}

/** Is the String empty? */
bool string_empty(const Mstring* const str){
    return(!str||!str->length); // MDH@21JUN2019 replacing: chars[0]=='\0'); // MDH@25APR2019: checking the first character probably is easiest
    /* replacing:
    if(str==NULL)return true;
    if(str->length==0)return true;
    return false;
    */
}

uint32_t string_length(const Mstring* const str){
    return(str==NULL?0:str->length);
}
 // MDH@26FEB2018: we might want to set the length (to a smaller one)
Mstring* string_setlength(Mstring* const str,uint32_t length){
    if(!str)return NULL;
    if(length>str->length){ // we're supposed to increment the length
        // how many blocks do we need
        uint32_t blocks=(length/BLOCK_SIZE)+1;
        // if we do not have enough blocks ascertain to have enough...
        if(blocks>str->blocks){
            /////////printf("Realloc string_setlength().\n");
            char* new_str=realloc(str->chars,BLOCK_SIZE*blocks*sizeof *(str->chars));
            if (!new_str)return NULL; // failure!!
            str->chars=new_str;
            str->blocks=blocks;
        }
        // fill with blanks??? for now that's OK
        while(str->length<length){str->chars[str->length]=' ';str->length++;}
        // MDH@21JUN2019 removing: str->chars[str->length]='\0'; // it's prudent to immediately set the end-of-text value (before filling)
    }else
    if(length<str->length){
        str->length=length;
        // MDH@21JUN2019 removing: str->chars[str->length]='\0';
    }
    return str;
}
void string_synclength(Mstring* const str){
    if(!str)return;
    uint32_t l=str->length;
    while(l>0)if(str->chars[--l]=='\0')break;
    str->length=l;
}

bool string_shorten(Mstring* const str,uint32_t length){
    if(!str)return false;
    if(length>str->length)return false;
    str->length-=length;
    // MDH@21JUN2019 removing: str->chars[str->length]='\0';
    return true;
}

char string_char(const Mstring* const str,uint32_t pos){
    return(str!=NULL?(pos<str->length?str->chars[pos]:'\0'):'\0');
}

char string_last_char(const Mstring* const str){
    return(str!=NULL?(str->length>0?str->chars[str->length-1]:'\0'):'\0');
}

char string_removed_char(Mstring* const str,uint32_t pos){
    char rc='\0';
    if(str!=NULL){
        uint32_t l=str->length;
        if(pos<l){
            --(str->length); // one less long
            rc=str->chars[pos]; // remember the character that is being removed!!
            // we have to move characters pos through str->length down
            // NOTE we have \0 at position str->length, so we have to move that one as well!!!    
            char c;     
            while(pos<l){str->chars[pos]=str->chars[pos+1];pos++;}
        }
    }
    return rc;
}

/** 
 * insert char c at position pos in the given string 
 * NOTE: returns NULL on failure, @str otherwise 
 */
Mstring* string_insert_char(Mstring* const str,uint32_t pos,char c){
    if(str!=NULL){
        uint32_t l=str->length+1; // the 'length' of the text plus 1
        // pos should never be larger than l
        if(pos<l){
            if(pos<l-1){ // a true insert, i.e. NOT replacing the last character!!
                ////////printf("{%hu-%d}",l,str->blocks);
                // do we need to get another block?    
                if(l==str->blocks*BLOCK_SIZE){
                    /////////printf("Realloc string_insert_char().\n");
                    char *new_str=realloc(str->chars,BLOCK_SIZE*(str->blocks+1)*sizeof *(str->chars));
                    if (new_str==NULL)return NULL;
                    ++(str->blocks);
                    ////// can't know the size of what new_str points to!!! printf("YY%lu-%dYY",sizeof(new_str),str->blocks);
                    str->chars=new_str;
                }
                if(l>=str->blocks*BLOCK_SIZE)return NULL;
                ///printf("%s",str->chars);
                // we have to move characters at position pos onward one position up
                while(l>pos){str->chars[l]=str->chars[l-1];l--;}
                str->chars[pos]=c;
                ///printf("->%s",str->chars);
                ++(str->length);
            }else // at end, we have to call string_append_char because str->last_char will change
            if(string_append_char(str,c)==NULL)return NULL;
        }
    }
    return str;
}

/** 
 * Add a character to the end of the String 
 * NOTE: returns NULL on failure
 */
Mstring* string_append_char(Mstring* const str,char c){
    if(str!=NULL){
        uint32_t l=str->length+1;
        /////printf("{%hu-%d}",l,str->blocks);
        if(l==str->blocks*BLOCK_SIZE){
            //////////////printf("Realloc string_append_char().\n");
            char *new_str=realloc(str->chars,BLOCK_SIZE*(str->blocks+1)*sizeof *(str->chars));
            if (new_str==NULL)return NULL; // failure!!
            ++(str->blocks);
            ////////printf("XX%lu-%dXX",sizeof(*new_str),str->blocks);
            str->chars=new_str;
        }
        if(l>=str->blocks*BLOCK_SIZE)return NULL;
        str->chars[str->length]=c;
        ++(str->length);
        // MDH@21JUN2019 removing: str->chars[str->length]='\0';
    }
    return str;
}

// MDH@12JUL2019: we can set a specific char which should only fail if pos is larger than the length
Mstring* string_setchar(Mstring* const str,char c,uint32_t pos){
    if(!str)return NULL;
    if(pos>=str->length)return NULL;
    str->chars[pos]=c;
    // MDH@24SEP2019: if somebody is so smart to use '\0' for c we should adapt the length as well (which could happen in _getCompletion() in Menvironment.h/c)
    if(c=='\0')str->length=pos;
    return str;
}
char* _stringstart(const Mstring* const str,uint32_t length){
    if(!str)return NULL;
    str->chars[str->length]='\0'; // mark the end of the string
    char* _result=strdup(str->chars); // create a copy of the entire string
    if(_result)if(length>0&&length<str->length)_result[length]='\0'; // 'cut off' the part we don't want!!
    return _result;
}

// MDH@24SEP2019: same as string_append but stopping when count characters were appended!!!
//                changed it as little as possible by breaking out of the while as soon as the number of appended characters (index) exceeds count!!!!
Mstring* string_append_chars(Mstring* const str,const char* pc,size_t count){
    if(str!=NULL&&pc!=NULL){ // something to append
        char c;
        uint32_t index=0;
        while((c=pc[index++])){if(index>count)break;if(string_append_char(str,c)==NULL)return NULL;}
        /////????? while(*pc!='\0'){string_append_char(str,*pc);(*pc)++;}
    }
    return str;
}

// MDH@26FEB2019: assuming cs is a zero-terminated character array
Mstring* string_append(Mstring* const str,const char* pc){
    if(str!=NULL&&pc!=NULL){ // something to append
        char c;
        uint32_t index=0;
        while((c=pc[index++]))if(string_append_char(str,c)==NULL)return NULL;
        /////????? while(*pc!='\0'){string_append_char(str,*pc);(*pc)++;}
    }
    return str;
}
Mstring* string_prepend(Mstring* const str,const char* pc){
    if(str!=NULL&&pc!=NULL){ // something to append
        char c;
        uint32_t index=0;
        while((c=pc[index++]))if(string_insert_char(str,index-1,c)==NULL)return NULL; // TODO a better way must exist
        /////????? while(*pc!='\0'){string_append_char(str,*pc);(*pc)++;}
    }
    return str;
}

char* string_remainder(Mstring* const str,uint32_t firstpos){
    if(!str)return NULL;
    if(!firstpos)return string(str);
    if(firstpos>str->length)return NULL;
    str->chars[str->length]='\0'; // MDH@21JUN2019: added: mark the end of the text
    return str->chars+firstpos;
}

/** 
 * Get a C-String with the proper null-terminator 
 * NOTE: returning the pointer to the characters stored in the Mstring (which is str->chars)
*/
char* string(Mstring* const str){
    if(!str)return NULL;
    str->chars[str->length]='\0'; // MDH@21JUN2019: added: mark the end of the text
    return str->chars;
    // MDH@21JUN2019: replacing: return (str?str->chars:NULL);
}

/** Get where the first occurrence of a character in the String is */
int32_t string_find(const Mstring* const str,char c){
    if(str){
        // MDH@16DEC2018: better to increment pos inside the condition
        int32_t pos=0;
        uint32_t l=str->length; // first character to check
        while(pos<l){ // still within the text
            if(str->chars[pos]==c)return pos; // if a match return pos
            pos++; // keep looking
        }
    }
    // not found!!!
    return -1;
}

void string_reverse(Mstring* const str){
    if(!str)return;
    uint32_t l=str->length;
    if(!l)return;
    l--;
    int32_t halfway=(l>>1);
    if(!halfway)return;
    ///////////printf("\nReversing: '%s'.",string(str));
    char c;
    while(halfway>=0){c=str->chars[halfway];str->chars[halfway]=str->chars[l-halfway];str->chars[l-halfway]=c;halfway--;}   
    //////////////printf("\nReversed: '%s'.",string(str));
}

// MDH@24SEP2019: in order to be able to use a smaller part from the beginning of text we'd like to be able to replace a character by '\0' and later on restore it
//                we will succeed if we have a function that will return the replaced character so we can put it back in again
//                this method will NOT change str->length ever, meaning that if you forget to put the character back you're in trouble
char string_replacedchar(Mstring* const str,char c,uint32_t pos){
    if(!str||pos>=str->length)return '\0';
    char replacedchar=str->chars[pos];
    str->chars[pos]=c;
    return replacedchar;
}
