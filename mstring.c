#include "mstring.h"

/** MDH@25DEC2018: 
 *  this code is from the Internet to implement a mutable string
 *  however, letting string_get_all() return a string allocated on the heap which means it needs to be freed is 
 *  not a good idea, as we will need to release the returned pointer afterwards
 *  therefore I change the entire thing in a null terminated string to start with!!
 */

/** Create a String */
mstring* string_create(){
    mstring* ans=calloc(1,sizeof *ans);
    if(ans!=NULL){
        ans->length=0;
        ans->blocks=0;
        ans->chars=malloc(BLOCK_SIZE*sizeof *(ans->chars));
        if(ans->chars!=NULL){
            ans->blocks=1;
            ans->chars[0]='\0'; // end of character string!!
        }
    }
    return ans;
}

/** Free the memory associated with a String */
void string_dispose(mstring *str){
    if(str!= NULL){
        free(str->chars);
        free(str);
    }
}

/** Is the String empty? */
bool string_empty(mstring *str){
    if(str==NULL)return true;
    if(str->length==0)return true;
    return false;
}

uint16_t string_length(mstring* str){
    return(str==NULL?0:str->length);
}

char string_char(mstring* str,uint16_t pos){
    return(str!=NULL?(pos<str->length?str->chars[pos]:'\0'):'\0');
}

char string_last_char(mstring *str){
    return(str!=NULL?(str->length>0?str->chars[str->length-1]:'\0'):'\0');
}

/** insert char c at position pos in the given string */
void string_insert_char(mstring* str,uint16_t pos,char c){
    if(str!=NULL){
        uint16_t l=str->length+1; // the 'length' of the text plus 1
       // pos should never be larger than l
        if(pos<l){
            if(pos<l-1){ // a true insert, i.e. NOT replacing the last character!!
                ////////printf("{%hu-%d}",l,str->blocks);
                // do we need to get another block?    
                if(l==str->blocks*BLOCK_SIZE){
                    char *new_str=realloc(str->chars,BLOCK_SIZE*(str->blocks+1)*sizeof *(str->chars));
                    if (new_str!=NULL){
                        ++(str->blocks);
                        ////// can't know the size of what new_str points to!!! printf("YY%lu-%dYY",sizeof(new_str),str->blocks);
                        str->chars=new_str;
                    }
                }
                if(l<str->blocks*BLOCK_SIZE){
                    // we have to move characters at position pos onward one position up
                    while(--l>=pos)str->chars[l+1]=str->chars[l];
                    str->chars[pos]=c;
                    ++(str->length);
                }
            }else // at end, we have to call string_append_char because str->last_char will change
                string_append_char(str,c);
        }
    }
}

/** Add a character to the end of the String */
void string_append_char(mstring *str,char c){
    if(str!=NULL){
        uint16_t l=str->length+1;
        /////printf("{%hu-%d}",l,str->blocks);
        if(l==str->blocks*BLOCK_SIZE){
            char *new_str=realloc(str->chars,BLOCK_SIZE*(str->blocks+1)*sizeof *(str->chars));
            if (new_str!=NULL){
                ++(str->blocks);
                ////////printf("XX%lu-%dXX",sizeof(*new_str),str->blocks);
                str->chars=new_str;
            }
        }
        if(l<str->blocks*BLOCK_SIZE){
            str->chars[str->length]=c;
            str->chars[++(str->length)]='\0';
        }
    }
}

char* string_remainder(mstring* str,uint16_t firstpos){
    if(firstpos==0)return string(str);
    if(str==NULL)return NULL;
    if(firstpos>str->length)return NULL;
    return str->chars+firstpos;
}

/** Get a C-String with the proper null-terminator */
char* string(mstring *str){
    char *res=NULL;
    if (str!=NULL){
        res=str->chars;
        /*
        res=malloc((str->length+1)*sizeof *str->chars);
        if(res!=NULL){
            memcpy(res,str->chars,str->length);
            res[str->length]='\0';
        }
        */
    }
    return res;
}

/** Get where the first occurrence of a character in the String is */
int16_t string_find(mstring *str,char c){
    if(str!=NULL){
        // MDH@16DEC2018: better to increment pos inside the condition
        int16_t pos=0; // first character to check
        while(pos<str->length){ // still within the text
            if(str->chars[pos]==c)return pos; // if a match return pos
            pos++; // keep looking
        }
    }
    // not found!!!
    return -1;
}