#include "mstring.h"

// MDH@21JUN2019: there's no need to set the end-of-string marker until a string is returned!!!
//                TODO if blocks is zero failed to 

/** MDH@25DEC2018: 
 *  this code is from the Internet to implement a mutable string
 *  however, letting string_get_all() return a string allocated on the heap which means it needs to be freed is 
 *  not a good idea, as we will need to release the returned pointer afterwards
 *  therefore I change the entire thing in a null terminated string to start with!!
 */

/** Create a String */
mstring* string_create(){
    mstring* ans=calloc(1,sizeof *ans);
    if(ans){
        // NOTE calloc() will make length and blocks 0: ans->length=0;ans->blocks=0;
        ans->chars=malloc(BLOCK_SIZE*sizeof *(ans->chars));
        if(!ans->chars){free(ans);ans=NULL;}else ans->blocks=1; // if the allocation failed we release ans immediately again, so ans->blocks will always be positive!!!
        // MDH@21JUN2019 replacing: if(ans->chars){ans->blocks=1;ans->chars[0]='\0';}
    }
#ifdef __DEBUGGING__
    if(!ans)printf("\nFailed to create a string.");
#endif
    return ans;
}

mstring* new_mstring(char* s){
    if(!s)return NULL;
    mstring* ans=calloc(1,sizeof *ans);
    if(ans){
        // NOTE calloc() will make length and blocks 0: ans->length=0;ans->blocks=0;
        size_t l=strlen(s);
        ans->blocks=(l/BLOCK_SIZE); // NOTE that s actually is strlen(s)+1 characters (including the '\0' at the end)
        ans->chars=malloc((++ans->blocks)*BLOCK_SIZE); // here we increment ans->blocks (as we must)
        if(ans->chars){
            //////////////strcpy(ans->chars,s);ans->length=l; // also copies the ending '\0' over but memcpy() does not have to check for '\0' so we use memcpy()
            ans->length=l; // MDH@21JUN2019: no need to copy '\0' at the end!!! replacing: ans->length=l++; // store l, then increment it, so memcpy() will also copy '\0' over!!!
            memcpy(ans->chars,s,l); // copy the actual characters over!!! // replacing: while(true){ans->chars[l]=s[l];if(l==0)break;l--;} // copying the characters over... TODO there's a faster way to do this of course
        }else{
            free(ans);ans=NULL;
        } // failure
    }
#ifdef __DEBUGGING__
    if(!ans)printf("\nFailed to create a string.");
#endif
    return ans;
}

// MDH@20JUN2019: instead of returning a bool (and requiring dst as second argument) we return the copy...
mstring* string_copy(mstring* src){
    if(!src)return NULL;
    src->chars[src->length]='\0'; // MDH@21JUN2019: mark the end of the text in the source (OOPS we would be in trouble otherwise)
    return new_mstring(src->chars);
    /* replacing:
    mstring* dst=string_create();
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
void free_mstring(mstring* str){if(str){if(str->chars)free(str->chars);free(str);}}

/** Is the String empty? */
bool string_empty(mstring *str){
    return(!str||!str->length); // MDH@21JUN2019 replacing: chars[0]=='\0'); // MDH@25APR2019: checking the first character probably is easiest
    /* replacing:
    if(str==NULL)return true;
    if(str->length==0)return true;
    return false;
    */
}

uint32_t string_length(mstring* str){
    return(str==NULL?0:str->length);
}
 // MDH@26FEB2018: we might want to set the length (to a smaller one)
mstring* string_setlength(mstring* str,uint32_t length){
    if(!str)return NULL;
    if(length>str->length){ // we're supposed to increment the length
        // how many blocks do we need
        uint32_t blocks=(length/BLOCK_SIZE)+1;
        // if we do not have enough blocks ascertain to have enough...
        if(blocks>str->blocks){
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
void string_synclength(mstring* str){
    if(!str)return;
    uint32_t l=str->length;
    while(l>0)if(str->chars[--l]=='\0')break;
    str->length=l;
}

bool string_shorten(mstring* str,uint32_t length){
    if(!str)return false;
    if(length>str->length)return false;
    str->length-=length;
    // MDH@21JUN2019 removing: str->chars[str->length]='\0';
    return true;
}

char string_char(mstring* str,uint32_t pos){
    return(str!=NULL?(pos<str->length?str->chars[pos]:'\0'):'\0');
}

char string_last_char(mstring *str){
    return(str!=NULL?(str->length>0?str->chars[str->length-1]:'\0'):'\0');
}

char string_removed_char(mstring* str,uint32_t pos){
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
mstring* string_insert_char(mstring* str,uint32_t pos,char c){
    if(str!=NULL){
        uint32_t l=str->length+1; // the 'length' of the text plus 1
        // pos should never be larger than l
        if(pos<l){
            if(pos<l-1){ // a true insert, i.e. NOT replacing the last character!!
                ////////printf("{%hu-%d}",l,str->blocks);
                // do we need to get another block?    
                if(l==str->blocks*BLOCK_SIZE){
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
mstring* string_append_char(mstring* str,char c){
    if(str!=NULL){
        uint32_t l=str->length+1;
        /////printf("{%hu-%d}",l,str->blocks);
        if(l==str->blocks*BLOCK_SIZE){
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

// MDH@26FEB2019: assuming cs is a zero-terminated character array
mstring* string_append(mstring* str,const char* pc){
    if(str!=NULL&&pc!=NULL){ // something to append
        char c;
        uint32_t index=0;
        while((c=pc[index++]))if(string_append_char(str,c)==NULL)return NULL;
        /////????? while(*pc!='\0'){string_append_char(str,*pc);(*pc)++;}
    }
    return str;
}

char* string_remainder(mstring* str,uint32_t firstpos){
    if(!str)return NULL;
    if(!firstpos)return string(str);
    if(firstpos>str->length)return NULL;
    str->chars[str->length]='\0'; // MDH@21JUN2019: added: mark the end of the text
    return str->chars+firstpos;
}

/** 
 * Get a C-String with the proper null-terminator 
 * NOTE: returning the pointer to the characters stored in the mstring (which is str->chars)
*/
char* string(mstring* str){
    if(!str)return NULL;
    str->chars[str->length]='\0'; // MDH@21JUN2019: added: mark the end of the text
    return str->chars;
    // MDH@21JUN2019: replacing: return (str?str->chars:NULL);
}

/** Get where the first occurrence of a character in the String is */
int32_t string_find(mstring *str,char c){
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

void string_reverse(mstring* str){
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