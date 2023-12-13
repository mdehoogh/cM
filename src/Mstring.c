#include <float.h>

#include "Mstring.h"

// MDH@21JUN2019: there's no need to set the end-of-string marker until a string is returned!!!
//				TODO if blocks is zero failed to 
extern unsigned long long M_MODULE_DEBUGGING;

static Mallocationowner getOwner(uint16_t id){return(Mallocationowner){MI_STRING,id};}

/** MDH@25DEC2018:
 *  this code is from the Internet to implement a mutable string
 *  however, letting string_get_all() return a string allocated on the heap which means it needs to be freed is 
 *  not a good idea, as we will need to release the returned pointer afterwards
 *  therefore I change the entire thing in a null terminated string to start with!!
 */

/**
 * @brief creates and returns a (mutable) Mstring*
 * 
 * @return Mstring* the created Mstring* on success, NULL on failure
 */
Mstring* __string(){Mallocationowner owner=getOwner(__LINE__);
	// MDH@09APR2020: because sizeof(Mstring) would not include what we need for the characters pointed to by chars, we need to allocated one BLOCK_SIZE of characters to start with
	Mstring* ans=(Mstring*)CALLOC_1(sizeof(Mstring),'S',owner);
	if(ans!=NULL){
		// printf("String allocated...\n");
		// NOTE calloc() will make length and blocks 0: ans->length=0;ans->blocks=0;
		// the size of each allocation is BLOCKSIZE characters
		// MDH@09APR2020: everything that is of dynamic size needs to be allocated using REALLOC even when freeing, that way we can keep track
		//				of the amount allocated in Malloc.c/h explicitly
		//				this means that we need to use REALLOC for all dynamic memory allocations of variable length
		// MDH@16APR2020: using Mchars* instance
		// MDH@03MAY2020 OOPS the size should go first!!!
		ans->_chars=owned_chars(__chars(M_BLOCK_SIZE,1,'s'),Msubowner(owner,1));
		if(NULL==ans->_chars){FREE_DISOWNED_1(ans,'S',owner);return NULL;}
		//OWNED(ans->_chars,owner);SUBOWNED(ans->_chars,1);
		ans->blocks=1;
		// printf("String contents allocated...\n");
		/* replacing:
		ans->chars=REALLOC(ans->chars,0,1,sizeof(char)*BLOCK_SIZE,'s'); // changed type 's' to '"' to prevent the check for size...
		if(!ans->chars){FREE(ans,'S');ans=NULL;}else ans->blocks=1; // if the allocation failed we release ans immediately again, so ans->blocks will always be positive!!!
		*/
		// MDH@21JUN2019 replacing: if(ans->chars){ans->blocks=1;ans->chars[0]='\0';}
	}
#ifdef __DEBUGGING__
	if(!ans)printf("\nFailed to create a string.");
#endif
	return disowned_string(ans,owner);
}

/**
 * @brief creates and returns an Mstring* initialized using C string \p s
 * 
 * @param s the C string to initialize the Mstring with
 * @return Mstring* pointer to the created and initialed Mstring
 */
Mstring* _getString(char const * const s){Mallocationowner owner=getOwner(__LINE__);
	if(NULL==s)return NULL;
	Mstring* ans=(Mstring*)CALLOC_1(sizeof(Mstring),'S',owner);
	if(ans!=NULL){
		// NOTE calloc() will make length and blocks 0: ans->length=0;ans->blocks=0;
		size_t l=strlen(s);
		// MDH@17APR2020: Mchars* replacing char*
		size_t blocks=1+(l/M_BLOCK_CHARACTERS);
		// NOTE __chars will return a disowned pointer which can then be owned by ans unless we create a subowned macro that will simply increment the level of ownership
		ans->_chars=owned_chars(__chars(M_BLOCK_SIZE,blocks,'s'),Msubowner(owner,1));
		if(ans->_chars!=NULL){
			ans->length=l;
			ans->blocks=blocks;
			memcpy(ans->_chars->chars,s,ans->length); // copy the actual characters over!!! // replacing: while(true){ans->chars[l]=s[l];if(l==0)break;l--;} // copying the characters over... TODO there's a faster way to do this of course
		}else{ // failure
			FREE_DISOWNED_1(ans,'S',owner);ans=NULL;
		}
		/* replacing:
		ans->blocks=(l/BLOCK_SIZE); // NOTE that s actually is strlen(s)+1 characters (including the '\0' at the end)
		// MDH@09APR2020: switching to using REALLOC for all dynamically allocated memory with variable length, like ans->chars!!!!
		ans->chars=REALLOC(ans->chars,0,++ans->blocks,sizeof(char)*BLOCK_SIZE,'s'); // here we increment ans->blocks (as we must)
		if(ans->chars){
				//////////////strcpy(ans->chars,s);ans->length=l; // also copies the ending '\0' over but memcpy() does not have to check for '\0' so we use memcpy()
				ans->length=l; // MDH@21JUN2019: no need to copy '\0' at the end!!! replacing: ans->length=l++; // store l, then increment it, so memcpy() will also copy '\0' over!!!
				memcpy(ans->chars,s,ans->length); // copy the actual characters over!!! // replacing: while(true){ans->chars[l]=s[l];if(l==0)break;l--;} // copying the characters over... TODO there's a faster way to do this of course
		}else{ // failure
				FREE(ans,'S');ans=NULL;
		}
		*/
	}
#ifdef __DEBUGGING__
	if(!ans)printf("\nFailed to create a string.");
#endif
	// MDH@19MAY2020: do NOT disown ans because whoever's receiving it should obtain ownership and you can only grab ownership on pointers currently being owned unless disowning
	return disowned_string(ans,owner);
}

// MDH@17APR2020: it's best for every variable size allocation unit to have a method that will return its size to be used in a call to REALLOC as from_count
/**
 * @brief returns the number of characters the Mstring pointed to by \p str can hold
 * 
 * @param str the Mstring
 * @return size_t the number of characters \p str can hold
 */
static size_t getSizeOfChars(Mstring* str){return(str!=NULL&&str->_chars!=NULL?M_BLOCK_SIZE*str->blocks:0);}
static size_t getNumberOfChars(Mstring* str){return(str!=NULL&&str->_chars!=NULL?M_BLOCK_CHARACTERS*str->blocks:0);}

// MDH@20JUN2019: instead of returning a bool (and requiring dst as second argument) we return the copy...
// WARNING: if length==0 the entire string is returned!!!!!!
/**
 * @brief returns a newly created Mstring pointer initialized from the first \p length characters in the Mstring pointed to by \p src
 * 
 * @param src the source Mstring
 * @param length the number of characters to copy
 * @return Mstring* the newly created Mstring pointer on success, or NULL on failure
 */
Mstring* _stringCopy(Mstring * const src,size_t length){Mallocationowner owner=getOwner(__LINE__);
	if(NULL==src)return NULL;
	// MDH@17APR2020: replacing src->chars by src->_chars->chars
	src->_chars->chars[src->length]='\0'; // MDH@21JUN2019: mark the end of the text in the source (OOPS we would be in trouble otherwise)
	Mstring* _result=owned_string(_getString(src->_chars->chars),owner);
	if(length>0)if(_result)string_setlength(_result,length);
	return disowned_string(_result,owner);
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
 * @brief disownes Mstring* \p _str from its current owner \p owner_str
 * 
 * @param _str the Mstring* to disown
 * @param owner_str the owner to disown
 * @return Mstring* the disowned Mstring*
 */
Mstring* disowned_string(Mstring* str,Mallocationowner owner_str){
	if(NULL==str)return NULL;
	if(str->_chars!=NULL)disowned_chars(str->_chars,owner_str);
	return DISOWNED(str,owner_str);
}

/**
 * @brief turns ownership of Mstring* \p str over to \p owner_str
 * 
 * @param str the (disowned) Mstring* to change the ownership of
 * @param owner_str the new owner
 * @return Mstring* the newly owned Mstring*
 */
Mstring* owned_string(Mstring* str,Mallocationowner owner_str){
	if(NULL==str)return NULL;
	if(str->_chars!=NULL)owned_chars(str->_chars,Msubowner(owner_str,1));
	return OWNED(str,owner_str);
}

// MDH@18MAY2020: you can see what a nuisance it is to free a string for somebody else because the caller needs to DISOWN it first, then I have to obtain ownership otherwise I can't free it
//				then there's str->_chars that we need to take ownership off as well
/**
 * @brief frees the Mstring pointed to by \p str
 * 
 * @param str the Mstring pointer to be freed
 * @return Mstring* the freed Mstring pointer on success, NULL otherwise
 */
Mstring* free_string(Mstring* str/*,Mallocationowner owner_str*/){   
	// in order to be able to free_chars but perhaps we do not need to disown str->_chars before calling free_chars????????
	if(str!=NULL){
		// MDH@17APR2020: replacing src->chars by src->_chars->chars
		// MDH@09APR2020: switching to using REALLOC instead of FREE for all variable length dynamic memory allocations
		// MDH@22MAY2020: subpointers can be disowned by passing in the superpointer owner i.e. it is NOT necessary to pass in it's own owner id (which indicates it is a subpointer)
		if(str->_chars!=NULL){
			free_chars(str->_chars/*,owner_str*/,M_BLOCK_SIZE,str->blocks,'s');
			str->_chars=NULL;
		}
		// replacing: if(str->chars)str->chars=REALLOC(str->chars,str->blocks,0,sizeof(char)*BLOCK_SIZE,'s'); // replacing: FREE(str->chars,'s');
		FREE_1(str,'S'/*,owner_str*/);
		return NULL;
	}
	return str;
}

/**
 * @brief returns the current length (number of held characters) in the Mstring pointed to by \p str
 * 
 * @param str the Mstring*
 * @return size_t the number of characters held by \p str when str is not NULL, zero otherwise
 */
size_t string_length(Mstring const * const str){return(str!=NULL?str->length:0);}

/**
 * @brief returns whether or not the Mstring pointed to by \p str is empty
 * 
 * @param str the Mstring*
 * @return true when the length of the Mstring is zero or when str is NULL
 * @return false when the length of the non-null Mstring is not zero
 */
bool string_empty(Mstring const * const str){return(str!=NULL?str->length==0:true);} // MDH@17APR2020: removing 

// MDH@26FEB2018: we might want to set the length (to a smaller one)
/**
 * @brief sets the length of the Mstring pointed to by \p str to \p length resizing Mstring is necessary padding with space characters when the length increases
 * 
 * @param str the Mstring*
 * @param length the new length of \p str
 * @return Mstring* \p str
 */
Mstring* string_setlength(Mstring* const str,size_t length){
	// MDH@22MAY2020: here we have a bit of an issue, because str->chars might change, although str won't change in which case we really need the oid from the caller
	//				unless we could extract ownership from str->chars itself????
	//				unless we decide that subpointers do not need to be owned?????
	//				it's obviously that they are NOT owned by a function (or module), ok, so that will be the decision
	//				nevertheless instead of a foid we could pass in the pointer of which it is a subclass
	// if(!OWNED(str,owner))return NULL;
	if(NULL==str)return NULL;
	if(length>str->length){ // we're supposed to increment the length
		// how many blocks do we need
		size_t blocks=1+(length/M_BLOCK_CHARACTERS);
		// if we do not have enough blocks ascertain to have enough...
		if(blocks>str->blocks){
			/////////printf("Realloc string_setlength().\n");
			// MDH@17APR2020: replacing char* by Mchars* (chars by _chars)
			Mchars* new_chars=_resized(str->_chars,M_BLOCK_SIZE,str->blocks,blocks,'s'); // MDH@22MAY2020: by using -foid we disown it immediately
			if(NULL==new_chars)return NULL; // failure
			str->blocks=blocks;
			str->_chars=new_chars; // MDH@05JUN2020 NO it is already subowned!!!! as soon as new_chars is stored in str->_chars which is a subpointer, we move the ownership to 0 i.e. it is safe if the containing pointer is
			/* replacing:
			char* new_str=REALLOC(str->chars,str->blocks,blocks,BLOCK_SIZE*sizeof(char),'s');
			if (!new_str)return NULL; // failure!!
			str->blocks=blocks;
			str->chars=new_str;
			*/
		}
		// fill with blanks??? for now that's OK
		while(str->length<length){
			str->_chars->chars[str->length]=' '; // MDH@17APR2020 replacing: str->chars[str->length]=' ';
			str->length++;
		}
		// MDH@21JUN2019 removing: str->chars[str->length]='\0'; // it's prudent to immediately set the end-of-text value (before filling)
	}else
	if(length<str->length){
		// output("Shortening the length from %zu to %zu.\n",str->length,length);
		str->length=length;
		// MDH@21JUN2019 removing: str->chars[str->length]='\0';
	}
	return str;
}

/**
 * @brief syncs the length of the Mstring pointed to by \p str to the position of the last '\0' character in the Mstring
 * 
 * @param str the Mstring*
 * @return Mstring* \p str
 */
Mstring* string_synclength(Mstring* const str){
	if(str!=NULL){
		size_t l=str->length;
		// MDH@17APR2020: str replaced by strchars
		Mchars* strchars=str->_chars;
		while(l>0)if(strchars->chars[--l]=='\0')break; 
		if(l>0||strchars->chars[0]=='\0')str->length=l; // if str->chars[l] does not equal 0 (i.e. '\0') (only possible if l equals 0) we should NOT change the length
	}
	return str;
}

/**
 * @brief decrements the length of \p str
 * @details returns NULL if \p str is empty
 * @param str 
 * @return Mstring* the input string on success, NULL on failure
 */
Mstring* string_declength(Mstring * const str){
	if(NULL==str||NULL==str->_chars||str->length==0)return NULL;
	--(str->length);
	return str;
}

/**
 * @brief 'shortens' the Mstring pointed to by \p str by \p length characters without changing any of the characters
 * 
 * @param str the Mstring*
 * @param length the new length of \p str
 * @return true when length does not exceed the current length
 * @return false when length exceeds the current length
 */
bool string_shorten(Mstring* const str,size_t length){
	if(NULL==str)return false;
	if(length>str->length)return false;
	str->length-=length;
	// MDH@21JUN2019 removing: str->chars[str->length]='\0';
	return true;
}

/**
 * @brief return character at position \p pos of the Mstring pointed to by \p str
 * 
 * @param str the Mstring*
 * @param pos the (zero-based) position of the character
 * 
 * @return char the character in the Mstring pointed to by \str at position \pos, or '\0' if that character does not exist
 */
char string_char(Mstring const * const str,size_t pos){
	// MDH@17APR2020: inserting ->_chars
	return(str!=NULL?(pos<str->length?str->_chars->chars[pos]:'\0'):'\0');
}

/**
 * @brief returns the last character stored in the Mstring pointed to by \p str
 * 
 * @param str the Mstring*
 * @return char the last character stored in the Mstring, or '\0' if \p str is NULL or of zero length
 */
char string_last_char(Mstring const * const str){
	// MDH@17APR2020: inserting ->_chars
	return(str!=NULL&&str->_chars!=NULL?(str->length>0?str->_chars->chars[str->length-1]:'\0'):'\0');
}

// MDH@13OCT2020: string_last_char_count() returns the number of times str ends with c
/**
 * @brief returns the number of characters equal to \p c in the Mstring pointed to by \p str
 * 
 * @param str the Mstring*
 * @param c the character to search for
 * @return size_t the number of occurrences at the end of the Mstring equal to \p c
 */
size_t string_last_char_count(const Mstring* const str,char c){
	size_t count=0;
	if(str!=NULL&&str->_chars!=NULL){
		size_t l=str->length;
		while(l>0){
			if(str->_chars->chars[--l]!=c)break;  
			count++;  
		}
	}
	return count;
}

/**
 * @brief returns the character removed at position \p pos of the Mstring pointed to by \p str
 * 
 * @param str the Mstring*
 * @param pos the position of the character to remove
 * @return char the removed character
 */
char string_removed_char(Mstring* const str,size_t pos){
	char rc='\0';
	if(str!=NULL&&str->_chars!=NULL){
		size_t l=str->length;
		if(pos<l){
			--(str->length); // one less long
			Mchars* strchars=str->_chars; // str replaced by strchars
			rc=strchars->chars[pos]; // remember the character that is being removed!!
			// we have to move characters pos through str->length down
			// NOTE we have \0 at position str->length, so we have to move that one as well!!!
			// MDH@0.1.7.14+28JUN2023 CORRECTION we do not need to move '\0' since we've decremented str->length and the '\0' will be placed at the right position
			//                                   so if pos=l-1 there's no need to move
			while(++pos<l)strchars->chars[pos-1]=strchars->chars[pos]; // replacing: while(pos<l){strchars->chars[pos]=strchars->chars[pos+1];pos++;} // TODO certainly this could be written more efficiently
		}
	}
	return rc;
}

/**
 * @brief returns the number of characters removed from the Mstring pointed to by \p str starting at position \p pos and ending at position \p pos + \p length - 1
 * 
 * @param str the Mstring*
 * @param pos the position of the first character to remove
 * @param length the maximum number of characters to remove
 * @return size_t the number of characters removed
 */
size_t string_removed(Mstring * const str,size_t pos,size_t length){ // MDH@03OCT2019: remove length characters from str starting at position pos
	size_t removed=0;
	if(str!=NULL&&str->_chars!=NULL){
		size_t l=str->length;
		if(pos<l){ // the position of the first character to remove is valid
			size_t remainderpos=pos+length; // the position of the first character to move
			if(remainderpos<l){ // l-remainderpos characters to move
				// we can use strcpy IFF we guarantee the end-of-string character to be the terminator
				// BUT because the arrays might overlap memmove should be used instead of strcpy as it guarantees 
				////// no need to do this when using memmove!!!!! str->chars[l]='\0';
				removed=length; // all suggested characters will be 'removed'
				Mchars* strchars=str->_chars; // MDH@17APR2020: replacing str by strchars
				memmove(strchars->chars+pos,strchars->chars+remainderpos,(sizeof(char))*(l-remainderpos)); // TODO sizeof(char) would be 1 always????
			}else // rest of string to remove i.e. l-pos characters
					removed=l-pos; // l-pos elements will be 'removed'
			str->length-=removed;
		}
	}
	return removed;
}

/**
 * @brief inserts the character \p c at position \p pos of the Mstring pointed to by \p str
 * 
 * @param str the Mstring*
 * @param pos the position to insert \p c at
 * @param c the character to insert
 * @return Mstring* \p str on success, NULL on failure
 */
Mstring* string_insert_char(Mstring* const str/*,Mallocationowner owner_str*/,size_t pos,char c){
	if(str!=NULL&&str->_chars!=NULL){
		size_t l=str->length+1; // the 'length' of the text plus 1
		// pos should never be larger than l
		if(pos<l){
			if(pos<l-1){ // a true insert, i.e. NOT replacing the last character!!
				////////printf("{%hu-%d}",l,str->blocks);
				// do we need to get another block?	
				if(l==getNumberOfChars(str)){
					/////////printf("Realloc string_insert_char().\n");
					// MDH@17APR2020: reallocating _chars (instead of str->chars)
					// size_t sizeOfChars=getSizeOfChars(str);
					Mchars* new_chars=_resized(str->_chars,M_BLOCK_SIZE,str->blocks,str->blocks+1,'s');
					if(!new_chars)return NULL;
					++(str->blocks);
					str->_chars=new_chars;
					/* replacing:
					char *new_str=REALLOC(str->chars,str->blocks,str->blocks+1,sizeof(char)*BLOCK_SIZE,'s');
					if (new_str==NULL)return NULL;
					str->chars=new_str;
					*/
					////// can't know the size of what new_str points to!!! printf("YY%lu-%dYY",sizeof(new_str),str->blocks);
				}
				if(l>=getSizeOfChars(str))return NULL;
				///printf("%s",str->chars);
				// we have to move characters at position pos onward one position up
				Mchars* strchars=str->_chars;
				while(l>pos){strchars->chars[l]=strchars->chars[l-1];l--;}
				strchars->chars[pos]=c;
				///printf("->%s",str->chars);
				++(str->length);
			}else // at end, we have to call string_append_char because str->last_char will change
			if(!string_append_char(str,c))return NULL;
		}
	}
	return str;
}

// MDH@23APR2020: could come in handy (similar to string_prepend)
/**
 * @brief inserts all characters from \p pc into the Mstring pointed to by \p str starting at position \p pos
 * 
 * @param str the Mstring*
 * @param pos the first position to replace
 * @param pc the characters to insert
 * @return Mstring* \p str
 */
Mstring* string_setchars(Mstring * const str,size_t pos,char const * const pc){
	if(str!=NULL&&pc!=NULL){
		char c;
		size_t index=0;
		while((c=pc[index])){if(!string_setchar(str,c,pos))break;index++;pos++;} // increment index at the end is better than at the beginning TODO can we do even better?
	}
	return str;
}

/**
 * @brief appends character \p c to the Mstring pointed to by \p str
 * 
 * @param str the Mstring*
 * @param c the character to append
 * @return \p str
 */
Mstring* string_append_char(Mstring* const str/*,Mallocationowner owner_str*/,char c){
	if(str!=NULL&&str->_chars!=NULL){
		if(c){ // MDH@15NOV2019: appending '\0' makes no sense does it??????
			size_t l=str->length+1;
			/////printf("{%hu-%d}",l,str->blocks);
			if(l==getNumberOfChars(str)){
				////////printf("Realloc string_append_char()...");
				// size_t sizeOfChars=getSizeOfChars(str);
				Mchars* new_chars=_resized(str->_chars,M_BLOCK_SIZE,str->blocks,str->blocks+1,'s');
				if(NULL==new_chars)return NULL;
				////////printf("resized!\n");
				++(str->blocks);
				str->_chars=new_chars;
				/* replacing:
				char *new_str=REALLOC(str->chars,str->blocks,str->blocks+1,sizeof(char)*BLOCK_SIZE,'s');
				if (!new_str)return NULL; // failure!!
				++(str->blocks);
				str->chars=new_str;
				*/
				////////printf("XX%lu-%dXX",sizeof(*new_str),str->blocks);
			}
			if(l>=getNumberOfChars(str))return NULL;
			str->_chars->chars[str->length]=c; // MDH@17APR2020 replacing: str->chars[str->length]=c;
			++(str->length);
		}
		// MDH@21JUN2019 removing: str->chars[str->length]='\0';
	}
	return str;
}

// MDH@12JUL2019: we can set a specific char which should only fail if pos is larger than the length
/**
 * @brief replaces the character at position \p pos of the Mstring pointed to by \p str by \p c
 * 
 * @param str the Mstring*
 * @param c the character to replace the current character at position \p pos
 * @param pos the position
 * @return Mstring* \p str
 */
Mstring* string_setchar(Mstring* const str,char c,size_t pos){
	if(NULL==str||NULL==str->_chars)return NULL;
	if(pos>=str->length)return NULL;
	str->_chars->chars[pos]=c; // MDH@17APR2020 inserting ->_chars
	// MDH@24SEP2019: if somebody is so smart to use '\0' for c we should adapt the length as well (which could happen in _getCompletion() in Menvironment.h/c)
	if(c=='\0')str->length=pos;
	return str;
}

/**
 * @brief returns a C string with maximum length \p length but containing all characters in the Mstring pointed to by \p str
 * 
 * @param str the Mstring*
 * @param length the maximum number of characters to return
 * @return char* the C string being returned
 */
char* _stringstart(const Mstring* const str,size_t length){
	if(NULL==str)return NULL;
	if(NULL==str->_chars)return NULL;
	// MDH@17APR2020 inserting ->_chars
	str->_chars->chars[str->length]='\0'; // mark the end of the string
	char* _result=strdup(str->_chars->chars); // create a copy of the entire string // MDH@02MAY2020 TODO should we return an Mchars* instead???????
	// MDH@14JAN2021: can't actually do this because in that case we wouldn't know how many characters to free??????? yes we can do that but it's unmanaged so you simply need to call free() on the returned pointer!!!!
	if(_result!=NULL)if(length>0&&length<str->length)_result[length]='\0'; // 'cut off' the part we don't want!!
	return _result;
}

// MDH@24SEP2019: same as string_append but stopping when count characters were appended!!!
//				changed it as little as possible by breaking out of the while as soon as the number of appended characters (index) exceeds count!!!!
/**
 * @brief appends at most \p count characters in C string \p pc to the Mstring pointed to by \p str
 * 
 * @param str the Mstring*
 * @param pc the C string containing the characters to append
 * @return \p str
 */
Mstring* string_append_chars(Mstring* const str/*,Mallocationowner owner_str*/,const char* pc,size_t count){
	if(str!=NULL&&pc!=NULL){ // something to append
		char c;
		size_t index=0;
		while((c=pc[index++])){if(index>count)break;if(string_append_char(str,c)==NULL)return NULL;}
		/////????? while(*pc!='\0'){string_append_char(str,*pc);(*pc)++;}
	}
	return str;
}

// MDH@26FEB2019: assuming cs is a zero-terminated character array
/**
 * @brief appends all characters in C string \p pc to the Mstring pointed to by \p str
 * 
 * @param str the Mstring*
 * @param pc the C string with characters to append
 * @return \p str on success, or NULL on failure
 */
Mstring* string_append(Mstring * const str/*,Mallocationowner owner_str*/,char const * const pc){
	if(str!=NULL&&pc!=NULL){ // something to append (to)
		char c;
		size_t index=0;
		while((c=pc[index++])){if(!string_append_char(str,c))return NULL;/*output("***** %c appended! ******\n",c);*/}
		/////????? while(*pc!='\0'){string_append_char(str,*pc);(*pc)++;}
	}
	return str;
}

/**
 * @brief prepend all characters in \p pc to the Mstring pointed to by \p str
 * 
 * @param str the Mstring*
 * @param pc the C string to prepend
 * @return \p str on success, or NULL on failure
 */
Mstring* string_prepend(Mstring* const str/*,Mallocationowner owner_str*/,char const * const pc){
	if(str!=NULL&&pc!=NULL){ // something to prepend (to)
		char c;
		size_t index=0;
		while((c=pc[index])){if(!string_insert_char(str,index,c))return NULL;index++;} // increment index at the end is better than at the beginning TODO can we do even better?
		/////????? while(*pc!='\0'){string_append_char(str,*pc);(*pc)++;}
	}
	return str;
}

// MDH@17APR2020: inserting ->_chars between str and ->chars
/**
 * @brief returns a pointer to all (non-duplicated) characters in the Mstring pointed to by \p str starting at position \p firstpos
 * 
 * @param str the Mstring*
 * @param firstpos the position of the first character
 * @return char* the C string on success, or NULL if firstpos exceeds the length of the Mstring
 */
char* string_remainder(Mstring* const str,size_t firstpos){
	if(NULL==str||NULL==str->_chars)return NULL;
	if(!firstpos)return string(str);
	if(firstpos>str->length)return NULL;
	str->_chars->chars[str->length]='\0'; // MDH@21JUN2019: added: mark the end of the text
	return str->_chars->chars+firstpos;
}

/**
 * @brief returns the C string representation of the Mstring pointed to by \p str
 * @details will sync the Mstring by placing a '\0' character at the length position of the C string held by the Mstring
 * @param str 
 * @return char* the C string pointer on success, or NULL on failure
 */
char* string(Mstring* const str){
	if(NULL==str||NULL==str->_chars)return NULL;
	str->_chars->chars[str->length]='\0'; // MDH@21JUN2019: added: mark the end of the text
	return str->_chars->chars;
	// MDH@21JUN2019: replacing: return (str?str->chars:NULL);
}

/**
 * @brief locates and returns the position of the first occurrence of character \p c at or after position \p pos in the Mstring pointed to by \p str
 * 
 * @param str the Mstring*
 * @param c the character to find
 * @param pos the first position to investigate
 * @return long long the position of the character in the Mstring, or -1 when not found
 */
long long string_find_char(const Mstring* const str,char c,size_t pos){
	if(str!=NULL){
		Mchars* strchars=str->_chars; // MDH@17APR2020: replacing str by strchars
		if(strchars!=NULL){
			// MDH@16DEC2018: better to increment pos inside the condition
			// MDH@25OCT2019: type of pos changed from long long to size_t and pos<l replaced by pos!=l because I'm not sure if 0<0 evaluates to false for unsigned integers like size_t
			size_t l=str->length; // first character to check
			while(pos<l){ // still within the text
				if(strchars->chars[pos]==c)return pos; // if a match return pos
				pos++; // keep looking
			}
		}
	}
	// not found!!!
	return -1;
}

/**
 * @brief reverses the characters in the Mstring pointed to by \p str
 * 
 * @param str the Mstring*
 */
void string_reverse(Mstring* const str){
	if(NULL==str)return;
	Mchars* strchars=str->_chars; // MDH@17APR2020: replacing str by strchars
	if(NULL==strchars)return;
	size_t l=str->length;
	if(l<=1)return; // when less than 2 characters nothing to reverse!!
	l--;
	long long halfway=(l>>1);
	///////////printf("\nReversing: '%s'.",string(str));
	char c;
	while(halfway>=0){
		c=strchars->chars[halfway];
		strchars->chars[halfway]=strchars->chars[l-halfway];
		strchars->chars[l-halfway]=c;
		halfway--;
	}   
	//////////////printf("\nReversed: '%s'.",string(str));
}

// MDH@24SEP2019: in order to be able to use a smaller part from the beginning of text we'd like to be able to replace a character by '\0' and later on restore it
//				we will succeed if we have a function that will return the replaced character so we can put it back in again
//				this method will NOT change str->length ever, meaning that if you forget to put the character back you're in trouble
/**
 * @brief replaces the character at position \p pos in the Mstring pointed to by \p str by character \p c
 * 
 * @param str the Mstring*
 * @param c the character to replace
 * @param pos the position of the character to replace
 * @return char the replaced character on success, or '\0' on failure
 */
char string_replacedchar(Mstring * const str,char c,size_t pos){
	if(NULL==str||NULL==str->_chars||pos>=str->length)return '\0'; // NOTE even though str->chars[str->length] might not be '\0' we're still returning '\0' in that case, as if it was there (otherwise we would have to write '\0' first as we do in string())
	char replacedchar=str->_chars->chars[pos];
	str->_chars->chars[pos]=c;
	return replacedchar;
}

// the number of matching character at the start
/**
 * @brief returns the number of characters at the start of the Mstring pointed to by \p str that match starting characters in C string \p chars
 * 
 * @param str the Mstring*
 * @param chars the C string
 * @return size_t the number of matching characters
 */
size_t string_number_of_matching_chars(Mstring const * const str,char const * chars){
	size_t numberOfMatchingCharacters=0;
	// if str and chars are used as value arguments so the pointers themselves shouldn't be constant
	if(str!=NULL&&chars!=NULL){
		char* strchars=str->_chars->chars;
		if(strchars!=NULL){
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
/**
 * @brief determines if the characters in the Mstring pointed to by \p str1 match the characters in the Mstring pointed to by \p str2
 * 
 * @param str1 the Mstring*
 * @param str2 the Mstring*
 * @return true when all characters are the same
 * @return false when not all characters are the same
 */
bool string_equal(Mstring const * const str1,Mstring const * const str2){
	if(str1==NULL&&str2==NULL)return false; // if both NULL not the same
	if(str1==str2)return true; // if pointing to the same memory, the same
	if(str1==NULL||str2==NULL)return false; // if either NULL not the same
	// ASSERT both are not NULL
	if(str1->length!=str2->length)return false; // if length not equal not the same
	if(str1->_chars==NULL||str2->_chars==NULL)return false; // we need both chars arrays (actually should never be NULL though)
	str1->_chars->chars[str1->length]='\0';str2->_chars->chars[str2->length]='\0'; // place end-of-text markers so we can use str_cmp for comparison
	return(strcmp(str1->_chars->chars,str2->_chars->chars)==0);
}

// MDH@13MAR2020: helper functions now implemented here (instead of in Mexecution.h/c)
/**
 * @brief appends the text representation of \p ull to the Mstring pointed to by \p str
 * 
 * @param str the Mstring*
 * @param ull the unsigned integer to append
 * @return Mstring* \p str with \p ull appended
 */
Mstring* string_append_ull(Mstring* const str,unsigned long long ull){
	if(str==NULL)return NULL;
	char llText[80];
	snprintf(llText,80,"%llu",ull); // TODO will this fit?
	return string_append(str,llText);
}/* VALIDATED */

// helper function
/**
 * @brief appends the text representation of \p ll to the Mstring pointed to by \p str
 * 
 * @param str the Mstring*
 * @param ll the signed integer to append
 * @return Mstring* \p str with \p ll appended
 */
Mstring* string_append_ll(Mstring* const str,long long ll){
	if(str==NULL)return NULL;
	char llText[80];
	snprintf(llText,80,"%lld",ll); // TODO will this fit?
	return string_append(str,llText);
}/* VALIDATED */

/**
 * @brief appends \p ld to the Mstring pointed to by \p str
 * 
 * @param str the Mstring*
 * @param ld a long double to append
 * @return Mstring* \p str with \p ld appended
 */
Mstring* string_append_ld(Mstring* const str,long double ld){
	if(NULL==str)return NULL;
	char ldText[80];
	// how about using scientific notation here?????
	snprintf(ldText,80,"%.*Le",LDBL_DIG,ld); //////snprintf(ldText,80,"%.*Le",LDBL_DIG,ld); // replaced f with e to get scientific notation!!
	// alternatively we could shift
	char* exp=strchr(ldText,'e');
	int l=strlen(ldText); // where we will be searching for decimal zeroes
	int exponent=0;
	if(exp){
		int e=(int)(exp-ldText);
		l=e++;
		ldText[l]='\0'; // cut off the exponent (we can still extract the exponent though)
		//////output("With exponent: '%s'",ldText);
		// extract the exponent
		bool neg=(ldText[e]=='-');if(neg||ldText[e]=='+')e++;
		while(ldText[e]!='\0'){exponent=10*exponent+(ldText[e]-'0');e++;}
		if(neg)exponent=-exponent;
		//////output("Exponent: %u.",exponent);
		/* replacing:
		while(--l>0&&ldText[l]=='0');
		if(ldText[l]=='-'||ldText[l]=='+')l--;
		if(ldText[l]=='e'){ // the e-part is zero
				///output("Zero exponent!");
				exp=NULL;
		}else{
				while(--l>0&&ldText[l]!='e'); // move to the 'e'
		}
		*/
	}///////else output("Without exponent: '%s'",ldText);

	// ASSERT l is now on the 'e' of the exponent (if any)
	
	char* period=strchr(ldText,'.');
	if(period){ // there's a decimal period
		int p=(int)(period-ldText); // p is the position of the decimal point
		// l-p-1 is the number of decimals if the exponent is smaller than that
		if(exponent>0){ // move the period up as far as necessary
			if(exponent<l-p)while(exponent>0){ldText[p]=ldText[p+1];ldText[++p]='.';exponent--;}
		}else
		if(exponent<0){ // move the period back as far as possible
			if(exponent+p>=0)while(exponent<0){ldText[p]=ldText[p-1];ldText[--p]='.';exponent++;}
		}
		// ASSERT p is the index of the period
		while(ldText[--l]=='0'); // a bit naughty to simply replacing '0' with '\0' to pretend to end the text!!!
		ldText[l+1]='\0';
	}
	if(!string_append(str,ldText))return NULL;
	if(exponent!=0)if(!string_append_char(str,'e')||!string_append_ll(str,exponent))return NULL; // append the exponent
	return str;
}/* VALIDATED */

/**
 * @brief returns an Mstring* with information on the Mstring pointed to by \p str
 * 
 * @param str the Mstring*
 * @return Mstring* the information Mstring*
 */
Mstring* _string_info(Mstring* str){Mallocationowner owner=getOwner(__LINE__);
	Mstring* str_info=owned_string(__string(),owner);
	if(str_info){
		Mstring* p=str_info;
		if(str!=NULL){
			size_t l=str->length;
			p=string_append(p,"Length: ");
			p=string_append_ull(p,l);
			p=string_append_char(p,',');
			p=string_append(p,"Blocks: ");
			p=string_append_ull(p,str->blocks);
			p=string_append_char(p,',');
			// let's append all the characters
			p=string_append(p,"Characters: ");
			Mchars* strchars=str->_chars;
			if(strchars!=NULL){ // MDH@17APR2020: replacing str->chars by strchars
				while(1){
					p=string_append_ull(p,l);
					p=string_append_char(p,'=');
					p=string_append_ull(p,strchars->chars[l]);
					if(l==0)break;
					l--;
					p=string_append_char(p,' ');
				}
			}else
				p=string_append(p,"(undefined)");
		}else
			p=string_append(p,"(undefined)");
		if(NULL==p){FREE_STRING(str_info,owner);str_info=NULL;}
	}
	return disowned_string(str_info,owner);
}

// MDH@16MAR2020: let's allow for determining the number of trailing elements
/**
 * @brief determines the number of trailing \p c characters in the Mstring pointed to by \p str
 * 
 * @param str the Mstring*
 * @param c the character to use
 * @return size_t the number of \p c characters at the end of \p str
 */
size_t string_trailing(Mstring* str,char c){
	size_t i,l=(str!=NULL?str->length:0u);
	if(l>0){
		size_t i=l;
		Mchars* strchars=str->_chars;
		if(strchars!=NULL){
			while(i>0&&strchars->chars[--i]==c);
			if(strchars->chars[i]==c)return l; // all characters equaled c
			return(l-i-1u);
		}
	}
	return 0;
}

// MDH@21OCT2020
/**
 * @brief determines if the Mstring pointed to by \p str ends with the characters in C string \p pc
 * 
 * @param str the Mstring*
 * @param pc the C string
 * @return true if the Mstring pointed to by \p str ends with all characters in C string \p pc
 * @return false if the Mstring pointed to by \p str does not end with all characters in C string \p pc
 */
bool string_endswith(Mstring const * const str,char const * const pc){
	size_t l=(str!=NULL?str->length:0),pcl=(pc!=NULL?strlen(pc):0);
	if(pcl>0&&l>=pcl){ // something to compare, and enough characters to compare
		Mchars* strchars=str->_chars;
		if(NULL==strchars)return false;
		while(pcl>0)if(pc[--pcl]!=strchars->chars[--l])return false;
		return true;
	}
	return false;
}
