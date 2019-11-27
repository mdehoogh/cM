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
    Mstring* ans=CALLOC(1,sizeof(Mstring),'S');
    if(ans){
        // NOTE calloc() will make length and blocks 0: ans->length=0;ans->blocks=0;
        // the size of each allocation is BLOCKSIZE characters
        ans->chars=MALLOC(1,sizeof(char)*BLOCK_SIZE,'s'); // changed type 's' to '"' to prevent the check for size...
        if(!ans->chars){FREE(ans,'"');ans=NULL;}else ans->blocks=1; // if the allocation failed we release ans immediately again, so ans->blocks will always be positive!!!
        // MDH@21JUN2019 replacing: if(ans->chars){ans->blocks=1;ans->chars[0]='\0';}
    }
#ifdef __DEBUGGING__
    if(!ans)printf("\nFailed to create a string.");
#endif
    return ans;
}

Mstring* _getString(const char* const s){
    if(!s)return NULL;
    Mstring* ans=CALLOC(1,sizeof(Mstring),'S');
    if(ans){
        // NOTE calloc() will make length and blocks 0: ans->length=0;ans->blocks=0;
        size_t l=strlen(s);
        ans->blocks=(l/BLOCK_SIZE); // NOTE that s actually is strlen(s)+1 characters (including the '\0' at the end)
        ans->chars=MALLOC(++ans->blocks,sizeof(char)*BLOCK_SIZE,'s'); // here we increment ans->blocks (as we must)
        if(ans->chars){
            //////////////strcpy(ans->chars,s);ans->length=l; // also copies the ending '\0' over but memcpy() does not have to check for '\0' so we use memcpy()
            ans->length=l; // MDH@21JUN2019: no need to copy '\0' at the end!!! replacing: ans->length=l++; // store l, then increment it, so memcpy() will also copy '\0' over!!!
            memcpy(ans->chars,s,ans->length); // copy the actual characters over!!! // replacing: while(true){ans->chars[l]=s[l];if(l==0)break;l--;} // copying the characters over... TODO there's a faster way to do this of course
        }else{ // failure
            FREE(ans,'S');ans=NULL;
        }
    }
#ifdef __DEBUGGING__
    if(!ans)printf("\nFailed to create a string.");
#endif
    return ans;
}

// MDH@20JUN2019: instead of returning a bool (and requiring dst as second argument) we return the copy...
Mstring* _stringCopy(Mstring* const src,size_t length){
    if(!src)return NULL;
    src->chars[src->length]='\0'; // MDH@21JUN2019: mark the end of the text in the source (OOPS we would be in trouble otherwise)
    Mstring* _result=_getString(src->chars);
    if(length>0)if(_result)string_setlength(_result,length);
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
void free_string(Mstring* str){if(str){if(str->chars)FREE(str->chars,'s');FREE(str,'S');}}

size_t string_length(Mstring const * const str){return(str?str->length:0);}

/** Is the String empty? */
bool string_empty(Mstring const * const str){return(string_length(str)==0);}

 // MDH@26FEB2018: we might want to set the length (to a smaller one)
Mstring* string_setlength(Mstring* const str,size_t length){
    if(!str)return NULL;
    if(length>str->length){ // we're supposed to increment the length
        // how many blocks do we need
        size_t blocks=(length/BLOCK_SIZE)+1;
        // if we do not have enough blocks ascertain to have enough...
        if(blocks>str->blocks){
            /////////printf("Realloc string_setlength().\n");
            char* new_str=REALLOC(str->chars,str->blocks,blocks,BLOCK_SIZE*sizeof(char),'s');
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
    size_t l=str->length;
    while(l>0)if(str->chars[--l]=='\0')break;
    str->length=l;
}

bool string_shorten(Mstring* const str,size_t length){
    if(!str)return false;
    if(length>str->length)return false;
    str->length-=length;
    // MDH@21JUN2019 removing: str->chars[str->length]='\0';
    return true;
}

char string_char(const Mstring* const str,size_t pos){
    return(str!=NULL?(pos<str->length?str->chars[pos]:'\0'):'\0');
}

char string_last_char(const Mstring* const str){
    return(str!=NULL?(str->length>0?str->chars[str->length-1]:'\0'):'\0');
}

char string_removed_char(Mstring* const str,size_t pos){
    char rc='\0';
    if(str!=NULL){
        size_t l=str->length;
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

size_t string_removed(Mstring * const str,size_t pos,size_t length){ // MDH@03OCT2019: remove length characters from str starting at position pos
    size_t removed=0;
    if(str){
        size_t l=str->length;
        if(pos<l){ // the position of the first character to remove is valid
            size_t remainderpos=pos+length; // the position of the first character to move
            if(remainderpos<l){ // l-remainderpos characters to move
                // we can use strcpy IFF we guarantee the end-of-string character to be the terminator
                // BUT because the arrays might overlap memmove should be used instead of strcpy as it guarantees 
                ////// no need to do this when using memmove!!!!! str->chars[l]='\0';
                removed=length; // all suggested characters will be 'removed'
                memmove(str->chars+pos,str->chars+remainderpos,(sizeof(char))*(l-remainderpos)); // TODO sizeof(char) would be 1 always????
            }else // rest of string to remove i.e. l-pos characters
                removed=l-pos; // l-pos elements will be 'removed'
            str->length-=removed;
        }
    }
    return removed;
}

/** 
 * insert char c at position pos in the given string 
 * NOTE: returns NULL on failure, @str otherwise 
 */
Mstring* string_insert_char(Mstring* const str,size_t pos,char c){
    if(str!=NULL){
        size_t l=str->length+1; // the 'length' of the text plus 1
        // pos should never be larger than l
        if(pos<l){
            if(pos<l-1){ // a true insert, i.e. NOT replacing the last character!!
                ////////printf("{%hu-%d}",l,str->blocks);
                // do we need to get another block?    
                if(l==str->blocks*BLOCK_SIZE){
                    /////////printf("Realloc string_insert_char().\n");
                    char *new_str=REALLOC(str->chars,str->blocks,str->blocks+1,sizeof(char)*BLOCK_SIZE,'s');
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
            if(!string_append_char(str,c))return NULL;
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
        if(c){ // MDH@15NOV2019: appending '\0' makes no sense does it??????
            size_t l=str->length+1;
            /////printf("{%hu-%d}",l,str->blocks);
            if(l==str->blocks*BLOCK_SIZE){
                //////////////printf("Realloc string_append_char().\n");
                char *new_str=REALLOC(str->chars,str->blocks,str->blocks+1,sizeof(char)*BLOCK_SIZE,'s');
                if (!new_str)return NULL; // failure!!
                ++(str->blocks);
                ////////printf("XX%lu-%dXX",sizeof(*new_str),str->blocks);
                str->chars=new_str;
            }
            if(l>=str->blocks*BLOCK_SIZE)return NULL;
            str->chars[str->length]=c;
            ++(str->length);
        }
        // MDH@21JUN2019 removing: str->chars[str->length]='\0';
    }
    return str;
}

// MDH@12JUL2019: we can set a specific char which should only fail if pos is larger than the length
Mstring* string_setchar(Mstring* const str,char c,size_t pos){
    if(!str)return NULL;
    if(pos>=str->length)return NULL;
    str->chars[pos]=c;
    // MDH@24SEP2019: if somebody is so smart to use '\0' for c we should adapt the length as well (which could happen in _getCompletion() in Menvironment.h/c)
    if(c=='\0')str->length=pos;
    return str;
}
char* _stringstart(const Mstring* const str,size_t length){
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
        size_t index=0;
        while((c=pc[index++])){if(index>count)break;if(string_append_char(str,c)==NULL)return NULL;}
        /////????? while(*pc!='\0'){string_append_char(str,*pc);(*pc)++;}
    }
    return str;
}

// MDH@26FEB2019: assuming cs is a zero-terminated character array
Mstring* string_append(Mstring * const str,char const * const pc){
    if(str&&pc){ // something to append (to)
        char c;
        size_t index=0;
        while((c=pc[index++]))if(!string_append_char(str,c))return NULL;
        /////????? while(*pc!='\0'){string_append_char(str,*pc);(*pc)++;}
    }
    return str;
}
Mstring* string_prepend(Mstring* const str,char const * const pc){
    if(str&&pc){ // something to prepend (to)
        char c;
        size_t index=0;
        while((c=pc[index])){if(!string_insert_char(str,index,c))return NULL;index++;} // increment index at the end is better than at the beginning TODO can we do even better?
        /////????? while(*pc!='\0'){string_append_char(str,*pc);(*pc)++;}
    }
    return str;
}

char* string_remainder(Mstring* const str,size_t firstpos){
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
    if(str->chars)str->chars[str->length]='\0'; // MDH@21JUN2019: added: mark the end of the text
    return str->chars;
    // MDH@21JUN2019: replacing: return (str?str->chars:NULL);
}

/** Get where the first occurrence of a character in the String is */
long long string_find(const Mstring* const str,char c){
    if(str){
        // MDH@16DEC2018: better to increment pos inside the condition
        // MDH@25OCT2019: type of pos changed from long long to size_t and pos<l replaced by pos!=l because I'm not sure if 0<0 evaluates to false for unsigned integers like size_t
        size_t pos=0,l=str->length; // first character to check
        while(pos!=l){ // still within the text
            if(str->chars[pos]==c)return pos; // if a match return pos
            pos++; // keep looking
        }
    }
    // not found!!!
    return -1;
}

void string_reverse(Mstring* const str){
    if(!str)return;
    size_t l=str->length;
    if(!l)return;
    l--;
    long long halfway=(l>>1);
    if(!halfway)return;
    ///////////printf("\nReversing: '%s'.",string(str));
    char c;
    while(halfway>=0){c=str->chars[halfway];str->chars[halfway]=str->chars[l-halfway];str->chars[l-halfway]=c;halfway--;}   
    //////////////printf("\nReversed: '%s'.",string(str));
}

// MDH@24SEP2019: in order to be able to use a smaller part from the beginning of text we'd like to be able to replace a character by '\0' and later on restore it
//                we will succeed if we have a function that will return the replaced character so we can put it back in again
//                this method will NOT change str->length ever, meaning that if you forget to put the character back you're in trouble
char string_replacedchar(Mstring * const str,char c,size_t pos){
    if(!str||pos>=str->length)return '\0'; // NOTE even though str->chars[str->length] might not be '\0' we're still returning '\0' in that case, as if it was there (otherwise we would have to write '\0' first as we do in string())
    char replacedchar=str->chars[pos];
    str->chars[pos]=c;
    return replacedchar;
}

// the number of matching character at the start
size_t string_number_of_matching_chars(Mstring const * const str,char const * chars){
    size_t numberOfMatchingCharacters=0;
    // if str and chars are used as value arguments so the pointers themselves shouldn't be constant
    if(str&&chars){
        char* strchars=str->chars;
        if(strchars){
            strchars[str->length]='\0'; // perhaps important
            // as long as the same and not end-of-line character increment
            while(*strchars==*chars&&(*chars)!='\0'){
                numberOfMatchingCharacters++; // increment number of matching characters
                ++strchars;++chars; // increment character pointers
            }
        }
    }
    return numberOfMatchingCharacters;
} 

// MDH@24OCT2019: return true if str1 and str2 are equal (qua contents)
bool string_equal(Mstring* str1,Mstring* str2){
    if(!str1&&!str2)return false; // if both NULL not the same
    if(str1==str2)return true; // if pointing to the same memory, the same
    if(!str1||!str2)return false; // if either NULL not the same
    // ASSERT both are not NULL
    if(str1->length!=str2->length)return false; // if length not equal not the same
    if(!str1->chars||!str2->chars)return false; // we need both chars arrays (actually should never be NULL though)
    str1->chars[str1->length]='\0';str2->chars[str2->length]='\0'; // place end-of-text markers so we can use str_cmp for comparison
    return(strcmp(str1->chars,str2->chars)==0);
}