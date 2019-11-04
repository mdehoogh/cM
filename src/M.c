// remove the line below when not in debug mode
/////#define __DEBUG__

#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <stdbool.h>
#include <inttypes.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>

#include "Malloc.h"
#include "Msettings.h"
#include "Moutput.h"
#include "Msession.h"

// Menvironment includes Mvalue includes Mexecution includes ...
#include "Menvironment.h"

char const * const M_VERSION="0.1.0";

//char const * const M_BUILD="1";char const * const M_DATE="21 October 2019, 17:00";
//char const * const M_BUILD="2";char const * const M_DATE="22 October 2019, 12:00";
//char const * const M_BUILD="3";char const * const M_DATE="23 October 2019, 18:00";
//char const * const M_BUILD="4";char const * const M_DATE="24 October 2019, 11:00";
//char const * const M_BUILD="5";char const * const M_DATE="25 October 2019, 16:00"; // managed to get rid of (mostly) all the warnings!!!
//char const * const M_BUILD="6";char const * const M_DATE="26 October 2019, 20:20";
//char const * const M_BUILD="7";char const * const M_DATE="27 October 2019, 12:00";
//char const * const M_BUILD="8";char const * const M_DATE="28 October 2019, 18:00";
//char const * const M_BUILD="9";char const * const M_DATE="30 October 2019, 18:00";
char const * const M_BUILD="10";char const * const M_DATE="31 October 2019, 12:00";

// used externally
//Mvaluetype={VT_UNDEFINED,VT_TOKEN,VT_INTEGER,VT_BIGINTEGER,VT_DECIMAL,VT_RATIONAL,VT_FLOAT,VT_TEXT,VT_LIST,VT_MAP}
const char* VALUETYPENAMES[]={"unknown","token","integer","big integer","decimal","rational","float","text","list","map","reference"};
const char* const IFFUNCTION_NAME="if";
const char* const WHILEFUNCTION_NAME="while";
const char* const FORFUNCTION_NAME="for";
const char* const DOFUNCTION_NAME="do"; // MDH@05AUG2019: the do function allowing the creation of variables local to the do execution
const char* const EVALFUNCTION_NAME="eval"; // MDH@28OCT2019: evaluating a text is nice
const char* const DEFINEUSERFUNCTION_NAME="function";
const char* const MUTABLEVALUETYPECHARS="uoibdqftlmr"; // the characters associated with each of the value types
const char* const IMMUTABLEVALUETYPECHARS="UOIBDQFTLMR"; // the characters associated with each of the value types
const char* const ERROR_PREFIX="ERROR: "; // used in Mexecution.c as well (defined there as extern!!!)

// MDH@31OCT2019: if the value of something equals the NULL value, this is the text to use to represent it, this is also the name of the NULL variable!!!
//                alternatively we could use capital letters to denote the variable, and lowercase to denote the value (which makes sense I suppose)
//                to prevent confusion it's best to use the same text for the value, otherwise they see 'null' as value and think they can use that to embed a NULL value!!!
//                OK the NULL value is displayed in the normal foreground color whereas the variable is displayed in another color (see showValueColored() for the coloring)
const char* const M_NULL_VALUE_TEXT_REPRESENTATION="NULL"; // the text to represent values that are undefined...
const char* const M_NULL_VARIABLE_NAME="NULL";

const long long M_LL_INVALID=LLONG_MIN; // the invalid long long defaults to LLONG_MIN
// it's preferable if the allowed range of integer (long long) values, does not include LLONG_MIN
const long long M_LL_MIN=LLONG_MIN+1;
const long long M_LL_MAX=LLONG_MAX;
const long long M_FALSE=0;
const long long M_TRUE=1;
const long long M_ZERO=0;
const long long M_POSITIVE=1;
const long long M_NEGATIVE=-1;
//const enum BOOLEAN_ENUM {M_FALSE,M_TRUE};
//const enum SIGN_ENUM {M_NEGATIVE,M_ZERO,M_POSITIVE};
const long double M_LD_NAN=0.0/0.0; // or strtold("nan",NULL) would work as well
const long double M_LD_Q_EPS=1e-18; // this is the exact boundary to use for approximating 13/11 (which seems to be an notorious long double to approximate with rational (13/11)!!!)
const long double M_LD_PI=3.1415926535897932384626433832795L; // 31 non-zero decimal digits of PI (before the first 0)
const long double M_LD_E=2.718281828459045235360287471353L; // 30 decimal digits of E

long long M_DP=20; // the default decimal precision (initially 20) TODO should this be a constant after all?????????

const unsigned long long M_BITS_PER_ENV_LEVEL=8; // the minimum is 4 (to allow for a depth of 15 environments at the same time), the maximum is 60 of course in which case the maximum depth is 1, 8 gives a maximum depth of 7 and 256 at each level

const char M_WHITESPACE_CHARACTER=' '; // MDH@31OCT2019: let's use another character for storing whitespace in tokens (would normally be a blank)
const char M_NEWLINE_CHARACTER='\\'; // MDH@31OCT2019: the character to request a newline with!!!

// I guess we could allow the user to specify another eps value through the QEPS command line argument!!!

void writeTimestamp(FILE* _file){
	if(_file){
    	time_t now=time(NULL);
		struct tm * nowlocal=localtime(&now);
    	char buffer[50];
		strftime(buffer,sizeof(buffer),"%Y-%m-%d %H:%M:%S",nowlocal);
    	fprintf(_file,"%s\t",buffer);
	}
}

FILE* debugfile=NULL;
#include <stdarg.h>
#ifdef __GNUC__
    __attribute__((format(printf, 1, 2)))
#endif
void debugWrite(const char* fmt,...){
	if(!debugfile){
		debugfile=fopen("./Mdebug.txt","a+t"); // append (or create) in text mode
		fputc('\n',debugfile); // start with a single empty line (separating the sessions)
	}
	if(debugfile){
    va_list args;
    va_start(args,fmt);
		writeTimestamp(debugfile);		
    vfprintf(debugfile,fmt,args);
    fputc('\n',debugfile);
    fflush(debugfile);
    va_end(args);
	}
}

/*
Mvalueunion* getMNumberValueunion(Mnumber* pMnumber){
	if(!pMnumber)return NULL;
	Mvalueunion* pMnumberValueunion=(Mvalueunion*)malloc(sizeof(Mvalueunion));
	if(pMnumberValueunion){pMnumberValueunion->n=pMnumber;} // store the Mnumber instance
	return pMnumberValueunion; // redirection operator
}
bool initVariable(Menvironment* _Menvironment,char* name,double d){
	Mvalueunion* pMValueunion=getMNumberValueUnion(getDoubleMnumber(d)); // dynamically allocated value union (i.e. on the heap)
	return setVariableValue(addVariable(_Menvironment,name,VT_NUMBER),*pMValueunion); // pass the union itself (can you actually assign a union???)
}
*/
// there will be a root (M) environment

// PI all little more accurate (we could make a PI100 from these numbers)
// source: https://blog.wolfram.com/2011/06/30/all-rational-approximations-of-pi-are-useless/
// wolfram has a Rationalize function to compute rational approximations to a certain accuracy (see https://reference.wolfram.com/language/ref/Rationalize.html)
/////const char* M_QNUM_PI100="394372834342725903069943709807632345074473102456264";
/////const char* M_QDEN_PI100="125532772013612015195543173729505082616186012726141";


Mbiginteger* _Iadd(Mbiginteger* a,Mbiginteger* b,bool freeonfailure){
	// ASSERT do NOT call with either a or b NULL
	Mbiginteger* sum=NULL;
	if(a&&b){
		if(!isBigintegerZero(a)&&!isBigintegerZero(b)){
			sum=__biginteger();
			if(mp_add(a,b,sum)!=MP_OKAY){free_biginteger(sum);sum=NULL;} // if the addition fails return 0
		}else
			sum=_getBigintegerCopy(isBigintegerZero(a)?b:a);
	}
	///////outputBiginteger("\nBig integer sum of ",a,NULL);outputBiginteger(" and ",b,NULL);outputBiginteger(" equals ",sum,".");
	if(!sum)if(freeonfailure){free_biginteger(a);free_biginteger(b);}
	return sum;
} // adding two big integers, if either is NULL return NULL
Mbiginteger* _Imultiply(Mbiginteger* a,Mbiginteger* b,bool freeonfailure){
	Mbiginteger* product=NULL;
	if(a&&b){
		if(!isBigintegerOne(a)&&!isBigintegerOne(b)){
			product=__biginteger(); // defaults to zero, which would be the result as well if either big integer is zero!!!
			if(mp_mul(a,b,product)!=MP_OKAY){free_biginteger(product);product=NULL;}
		}else
			product=_getBigintegerCopy(isBigintegerOne(a)?b:a);
	}
	//////////outputBiginteger("\nProduct of big integers ",a,NULL);outputBiginteger(" and ",b,NULL);outputBiginteger(" equals ",product,".");
	if(!product)if(freeonfailure){free_biginteger(a);free_biginteger(b);}
	return product;
} // multiplying two big integers, if either is NULL return NULL

// _Imul is special big integer multiplier that assumes a NULL big integer equals 1
Mbiginteger* _Imul(Mbiginteger* a,Mbiginteger* b){
	if(!a&&!b)return NULL;
	if(!a)return _getBigintegerCopy(b);
	if(!b)return _getBigintegerCopy(a);
	Mbiginteger* product=__biginteger();
	if(mp_mul(a,b,product)!=MP_OKAY){free_biginteger(product);product=NULL;}
	return product;
}

// RATIONAL STUFF
// long double helper functions for use with the delta of rationals
long double realneg(Mfloat* _real){return (floatIsUndefined(_real)?M_LD_NAN:-_real->ld);}

/* MDH@19SEP2019: replaced by appropriate versions in Mrational.h/c
// operators applied to rationals
Mrational* _qmultiply(Mrational* _rational1,Mrational* _rational2){
	if(!_rational1||!_rational2)return NULL;
	bool delta1undefined=floatIsUndefined(_rational1->delta),delta2undefined=floatIsUndefined(_rational2->delta);
	Mbiginteger *_num=_Imul(_rational1->num,_rational2->num),*_den=_Imul(_rational1->den,_rational2->den);
	// pure rationals are easy
	if(delta1undefined&&delta2undefined)return _getRational(_num,_den,M_LD_NAN,true,true);
	// TODO take the delta's into account!!!
	return NULL;
}
Mrational* _qdivide(Mrational* _rational1,Mrational* _rational2){
	if(!_rational1||!_rational2)return NULL;
	// even if we have delta's we will always need these products
	Mbiginteger *_num=_Imul(_rational1->num,_rational2->den),*_den=_Imul(_rational2->num,_rational1->den);
	bool delta1undefined=floatIsUndefined(_rational1->delta),delta2undefined=floatIsUndefined(_rational2->delta);
	// pure rationals are easy
	if(delta1undefined&&delta2undefined){
	    return _getRational(_num,_den,M_LD_NAN,true,true);
	}
	// if we assume the delta's to be very small (as they will be), the parts containing squares to be too small to care about 
	if(delta1undefined){
		// second denominator term is -(b*d*delta2)**2 which for reasonably small b and d (both denominators) will be negligable

	}
	if(delta2undefined){

	}
	// neither undefined
	return NULL;
}
*/
/* replacing:
Mrational* _qdivide(Mrational* rational1,Mrational* rational2){
	// if either of them has a delta (i.e. is not pure), convert to double reals first
	Mrational* _rational=NULL;
	if(rational1&&rational2){
		long double delta1=getRealLongDouble(rational1->delta),delta2=getRealLongDouble(rational2->delta);
		if((ldIsNaN(delta1)||ldIsZero(delta1))&&(ldIsNaN(delta2)||ldIsZero(delta2))){ // both are 'pure' rationals
			if(amVerbose())output("Pure rational division.");
			// compute the nsew numerator and denominator, both should not be NULL as the numerators are not NULL, so should be freed if we can't drop them
			Mbiginteger* _numerator=(rational2->den?_Imultiply(rational1->num,rational2->den,false):_getBigintegerCopy(rational1->num));
			Mbiginteger* _denominator=(rational1->den?_Imultiply(rational2->num,rational1->den,false):_getBigintegerCopy(rational2->num));
			if(!_numerator||!_denominator){ // failed to compute either, so somewhere it went wrong
				free_biginteger(_numerator);free_biginteger(_denominator);
			}else{
				if(isBigintegerOne(_denominator)){free_biginteger(_denominator);_denominator=NULL;} // prevent storing 1 explicitly...
				_rational=_getRational(_numerator,_denominator,M_LD_NAN,true,true); // free num/den when failing to bind them
			}
		}else{ // either one or both are unpure i.e. 'reals' approximated by rationals 
			if(amVerbose())output("Approximate rational division.");
		}
	}
	return _rational;
}

// MDH@17SEP2019: _qsum used to be _qadd but we now have a _qadd in Mrational.c which will check for errors in performing the big integer multiplications, so is better
Mrational* _qsum(Mrational* _rational1,Mrational* _rational2){
	Mrational* _rational=__rational(); // we need a rational to hold the sum
	if(_rational){
		outputLine("Adding two rationals.");
		mp_err status=_qadd(_rational,_rational1,_rational2);
		output("Two rationals added (status=%i).\n",status);
		if(status!=MP_OKAY){free_rational(_rational);_rational=NULL;outputError("Failed to add two rationals");}else outputLine("Two rationals added successfully."); // addition failed somehow...
	}
	return _rational;
}
*/
/* replacing:
Mrational* _qadd(Mrational* _rational1,Mrational* _rational2){
	Mrational* _rational=NULL;
	if(_rational1&&_rational2){
		// we can speed things up by using a special multiplication method
		Mbiginteger *_num1=_Imul(_rational1->num,_rational2->den),*_num2=_Imul(_rational2->num,_rational1->den);
		if(_num1&&_num2) // we got (and need) both
			_rational=_getRational(_Iadd(_num1,_num2,false),_Imul(_rational1->den,_rational2->den),realsum(_rational1->delta,_rational2->delta),true,true); // free the numerator and denominator
		if(_num1)free_biginteger(_num1);if(_num2)free_biginteger(_num2);
	}
	return _rational;
}
*/

/* MDH@19SEP2019: replaced by _getRationalDifference in Mrational.h/c
Mrational* _qneg(Mrational* _rational){
	// normalizes the negated rational if not currently normalized, otherwise it will not normalize it
	Mrational* _rationalNeg=(_rational?_getRational(_getNegatedBiginteger(_rational->num),_getBigintegerCopy(_rational->den),realneg(_rational->delta),!_rational->normalized,true):NULL);
	if(_rationalNeg)if(_rational->normalized)_rationalNeg->normalized=true; // if original assumed normalized, so is the negated value
	return _rationalNeg;
}
Mrational* _qsubtract(Mrational* _rational1,Mrational* _rational2){
	if(!_rational1||!_rational2)return NULL;
	Mrational* _rational2Neg=_qneg(_rational2); // get the negated rational2
	Mrational* _rational=_qsum(_rational1,_rational2Neg);
	free_rational(_rational2Neg); // free the negated rational2
	return _rational;
}
*/
// end rational stuff

// decimal stuff
/* MDH@20JUN2019: by not using DP_value anymore, we solved the problem of DP_value holding a reference to the decimal precision value which apparently was released at some point
                  so as soon as the value pointer is released by the value list, a reference is still kept by DP_value but the memory will be reused and _integer might point into uncharted territory at some point in the future
				  if we were to keep using DP_value we should have called assignValue() to assign the value and not DP_value=_getIntegerValue() (see initEnvironment())
Mvalue* DP_value=NULL; 
*/
Mdecimalcontext* M_DECIMALCONTEXT=NULL; // the application-wide decimal context
long long getDP(){
	if(!M_DECIMALCONTEXT)M_DECIMALCONTEXT=_getDecimalcontext(M_DP); // _decimalContext won't be created until it's actually needed (so other decimal contexts might be created before!!!!!)
	// better to get it directly out of the _decimalContext (as that holds the actual decimal context being used)
	long long dp=(M_DECIMALCONTEXT?M_DECIMALCONTEXT->mpd_context->prec:M_LL_INVALID); // replacing: long long dp=(DP_value?DP_value->value._integer->ll:M_LL_INVALID);
	if(dp==M_LL_INVALID)outputLine("BUG: No default decimal context active!");
	return dp;
}
// MDH@18OCT2019: if someone wants to know about the decimal context
Mvalue* getdc(Mvalue* value){
	if(value&&value->type==VT_DECIMAL){
		Mdecimalcontext* decimalcontext=_getDecimalcontext(value->value._decimal->prec);
		mpd_context_t* mpd_context=(decimalcontext?decimalcontext->mpd_context:NULL);
		if(mpd_context){
			Mmap* _contextMap=_getMapOfType(VT_INTEGER);
			if(_contextMap){
				appendedToMap(_contextMap,"status",_getIntegerValue(mpd_context->status));
				appendedToMap(_contextMap,"precision",_getIntegerValue(mpd_context->prec));
				appendedToMap(_contextMap,"round",_getIntegerValue(mpd_context->round));
				appendedToMap(_contextMap,"exponentminimum",_getIntegerValue(mpd_context->emin));
				appendedToMap(_contextMap,"exponentmaximum",_getIntegerValue(mpd_context->emax));
				appendedToMap(_contextMap,"allcr",_getIntegerValue(mpd_context->allcr));
				appendedToMap(_contextMap,"clamp",_getIntegerValue(mpd_context->clamp));
				appendedToMap(_contextMap,"newtrap",_getIntegerValue(mpd_context->newtrap));
				appendedToMap(_contextMap,"traps",_getIntegerValue(mpd_context->traps));
				return _getValueOfMap(_contextMap,true);
			}
		}
	}
	return NULL;
}
Mvalue* getdp(Mvalue* value){
	return(value&&value->type==VT_DECIMAL?_getIntegerValue(value->value._decimal->prec):NULL);
}
Mvalue* setdp(Mvalue* value){
	// how about returning the current value, no matter what the argument is????
	long long olddecimalprecision=getDP();
	// ignore if NO value specified...
	if(value&&value->type==VT_INTEGER){
		long long decimalprecision=value->value._integer->ll;
		if(decimalprecision!=M_LL_INVALID){ // if not the default!!!
			if(decimalprecision>=6){
				// if I fail to create the associated decimal context, no go
				Mdecimalcontext* _newDecimalContext=_getDecimalcontext(decimalprecision);
				if(_newDecimalContext){
					M_DECIMALCONTEXT=_newDecimalContext;
					M_DP=decimalprecision; // OOPS forgot this earlier TODO should we do this or not????
					////DP_value->value._integer->ll=_decimalContext->prec;
				}else
					output("%sActive decimal context not replaced: failed to create a decimal context with precision %llu.\n",ERROR_PREFIX,decimalprecision);
			}else
				output("%sRequested decimal precision (%llu) not activated: it should at least be 6.\n",ERROR_PREFIX,decimalprecision);
		}
	}
	return _getIntegerValue(olddecimalprecision);
}

// convenience method to obtain the wrapped mpd_context pointer
mpd_context_t* get_default_mpd_context(){return(M_DECIMALCONTEXT?M_DECIMALCONTEXT->mpd_context:NULL);}

/* replaced by methods in Mdecimal.h/c
Mdecimal* _dadd(Mdecimal* _decimal1,Mdecimal* _decimal2){
	if(!_decimal1||!_decimal2)return NULL;
	Mdecimalcontext* decimalcontext=_getDecimalcontext(MAX(_decimal1->prec,_decimal2->prec));
	mpd_context_t* mpd_context=(decimalcontext?decimalcontext->mpd_context:M_DECIMALCONTEXT->mpd_context);
	if(!mpd_context){outputError("No decimal context!");return NULL;}
	Mdecimal* _result=__decimal(mpd_context,0,0);
	///////outputLine("Adding two decimals.");
	if(_result){
		uint32_t status=0;
		mpd_qadd(_result->mpd,_decimal1->mpd,_decimal2->mpd,mpd_context,&status);
		if(status&0xEFBF){free_decimal(_result);_result=NULL;outputError("Failed to compute the sum of two decimals.");}
	}else
		outputError("Failed to create the sum decimal");
	return _result;
}
Mdecimal* _ddiv(Mdecimal* _decimal1,Mdecimal* _decimal2){
	if(!_decimal1||!_decimal2)return NULL;
	Mdecimalcontext* decimalcontext=_getDecimalcontext(MAX(_decimal1->prec,_decimal2->prec));
	mpd_context_t* mpd_context=(decimalcontext?decimalcontext->mpd_context:M_DECIMALCONTEXT->mpd_context);
	if(!mpd_context){outputError("No decimal context!");return NULL;}
	Mdecimal* _result=__decimal(mpd_context,0,0);
	///////outputLine("Dividing two decimals.");
	if(_result){
		uint32_t status=0;
		mpd_qdiv(_result->mpd,_decimal1->mpd,_decimal2->mpd,mpd_context,&status);
		if(status&0xEFBF){
			free_decimal(_result);_result=NULL;
			outputError("Failed to compute the quotient of two decimals");
		}
	}else 
		outputError("Failed to create the quotient decimal");
	return _result;
}
Mdecimal* _dmul(Mdecimal* _decimal1,Mdecimal* _decimal2){
	if(!_decimal1||!_decimal2)return NULL;
	Mdecimalcontext* decimalcontext=_getDecimalcontext(MAX(_decimal1->prec,_decimal2->prec));
	mpd_context_t* mpd_context=(decimalcontext?decimalcontext->mpd_context:M_DECIMALCONTEXT->mpd_context);
	if(!mpd_context){outputError("No decimal context!");return NULL;}
	Mdecimal* _result=__decimal(mpd_context,0,0);
	///////outputLine("Multiplying two decimals.");
	if(_result){
		uint32_t status=0;
		mpd_qmul(_result->mpd,_decimal1->mpd,_decimal2->mpd,mpd_context,&status);
		if(status&0xEFBF){
			free_decimal(_result);_result=NULL;
			outputError("Failed to compute the product of two decimals");
		}
	}else
		outputError("Failed to create the product decimal");
	return _result;
}
Mdecimal* _dsub(Mdecimal* _decimal1,Mdecimal* _decimal2){
	if(!_decimal1||!_decimal2)return NULL;
	Mdecimalcontext* decimalcontext=_getDecimalcontext(MAX(_decimal1->prec,_decimal2->prec));
	mpd_context_t* mpd_context=(decimalcontext?decimalcontext->mpd_context:M_DECIMALCONTEXT->mpd_context);
	if(!mpd_context){outputError("No decimal context!");return NULL;}
	Mdecimal* _result=__decimal(mpd_context,0,0);
	///////outputLine("Multiplying two decimals.");
	if(_result){
		uint32_t status=0;
		mpd_qsub(_result->mpd,_decimal1->mpd,_decimal2->mpd,mpd_context,&status);
		if(status&0xEFBF){
			free_decimal(_result);_result=NULL;
			outputError("Failed to compute the difference of two decimals");
		}
	}else
		outputError("Failed to create the difference decimal");
	return _result;
}
*/

// wolfram reports 13 different approximations to pi at http://functions.wolfram.com/Constants/Pi/10/

/*
the following very fast approximation (which computes decimal digits), wich I guess we should use to compute pi with a sufficient number of decimal digits
source: https://en.wikipedia.org/wiki/Chudnovsky_algorithm
from decimal import Decimal as Dec, getcontext as gc

def PI(maxK=70, prec=1008, disp=1007): # parameter defaults chosen to gain 1000+ digits within a few seconds
    gc().prec = prec
    K, M, L, X, S = 6, 1, 13591409, 1, 13591409
    for k in range(1, maxK+1):
        M = (K**3 - 16*K) * M // k**3 
        L += 545140134
        X *= -262537412640768000
        S += Dec(M * L) / X
        K += 12
    pi = 426880 * Dec(10005).sqrt() / S
    pi = Dec(str(pi)[:disp]) # drop few digits of precision for accuracy
    print("PI(maxK={} iterations, gc().prec={}, disp={} digits) =\n{}".format(maxK, prec, disp, pi))
    return pi

Pi = PI()
print("\nFor greater precision and more digits (takes a few extra seconds) - Try")
print("Pi = PI(317,4501,4500)") 
print("Pi = PI(353,5022,5020)")
 */
// but the primary formula is pretty simple: 4*sum((-1)k/(2k+1)): this is the very slow Gregory-Leibniz series approximation
// this is a very slow algorithm
Mvalue* pi_ql(Mvalue* value){
	if(value&&value->type==VT_INTEGER){
		long long maxiter=value->value._integer->ll;
		if(maxiter>=0){
			if(amVerbose())output("Approximating pi/4 by a sum of %llu rational fractions.\n",maxiter);
			// the first approximation (when iter=0) equals 4
			Mrational* _rational=_getRational(_getBiginteger(1),NULL,M_LD_NAN,false,true);
			if(_rational){
				// obviously we can add 2 to the big integer storing the numerator
				if(maxiter>0){
					Mbiginteger* _addendumDenominator=_getBiginteger(3);
					Mbiginteger* _denominatorIncrement=_getBiginteger(2);
					if(_addendumDenominator&&_denominatorIncrement){
						for(int iter=1;iter<=maxiter;iter++){
							// compute the numerator and (new) denominator of the addendum rational
							Mbiginteger* _addendumNumerator=_getBiginteger(iter%2?-1:1); // the numerator is either 1 or -1
							if(!_addendumNumerator){
								free_biginteger(_addendumDenominator);
								output("%sFailed to set the addendum numerator at iteration %u.\n",ERROR_PREFIX,iter);
								break;
							}
							// both _addendumNumerator and _addendumDenominator are now available to be bound in the rational
							Mrational* _addendumRational=_getRational(_addendumNumerator,_addendumDenominator,M_LD_NAN,false,true);
							if(!_addendumRational){ // failed to bind in the rational
								output("%sFailed to compute the rational to add to the approximation of pi in step %u.",ERROR_PREFIX,iter);
								break;
							}
							// add the addendum to the current rational
							if(amVerbose()){
								outputRational("Sum so far: ",_rational,NULL);
								outputRational(", addendum: ",_addendumRational,".\n");
							}
							Mrational* _newRational=_getRationalSum(_rational,_addendumRational); // _qsum replaced by _getRationalSum in Mrational.h/c
							if(!_newRational){
								// we have to free the addendum numerator and denominator
								output("%sFailed to add this addendum at step %u in approximating pi.\n",ERROR_PREFIX,iter);
								free_rational(_addendumRational); // to free the addendum numerator and denominator bound to _addendumRational
								break;
							}
							// increment the denominator BEFORE we loose the addendum denominator we have now (as part of _rational)
							if(amVerbose())outputBiginteger("Incrementing the addendum denominator by ",_denominatorIncrement,".\n");		
							Mbiginteger* _newAddendumDenominator=_Iadd(_addendumDenominator,_denominatorIncrement,false);
							if(!_newAddendumDenominator){
								free_biginteger(_addendumDenominator); // won't be using this in the addendum rational
								outputError("Failed to increment the addendum denominator");
								break;
							}
							if(amVerbose())outputBiginteger("New addendum denominator: ",_newAddendumDenominator,".\n");
							if(amVerbose())outputRational("New approximation to pi/4: ",_newRational,".\n");
							free_rational(_addendumRational); // to free the addendum numerator and denominator bound to _addendumRational
							// replace _rational by _newRational
							free_rational(_rational);
							_rational=_newRational;
							//if(amVerbose())
							if(amVerbose())outputRational("Sum approximation of pi/4 so far: ",_rational,".\n");
							// no need to normalize as the addendum is always normalized by itself
							// replace the addendum denominator with the new one)
							_addendumDenominator=_newAddendumDenominator;
							if(amVerbose())outputBiginteger("New addendum denominator: ",_addendumDenominator,".\n");
						}
					}else{
						outputError("Failed to initialize the addendum numerator and its increment value (2)");
						free_biginteger(_addendumDenominator);
					}
				}
				if(mp_mul_2d(_rational->num,2,_rational->num)!=MP_OKAY){
					if(amVerbose()){output("%s",ERROR_PREFIX);outputRational("Failed to multiply the approximation of pi/4 (",_rational," by 4.\n");}
					free_rational(_rational);
					return NULL;
				} // multiply the numerator by 4 i.e. 2**2
				normalizeRational(_rational);
				if(amVerbose())outputRational("Normalized approximation of pi: ",_rational,".\n");
				return _getRationalValue(_rational,true);
			}
		}
	}
	return NULL;
}

// and the following is an implementation that can approximate pi using this formula
Mvalue* pi_q(Mvalue* value){
	if(value&&value->type==VT_INTEGER){
		long long iter=value->value._integer->ll;
		if(iter>=0){
			if(amVerbose())output("Computing %llu continued fractions of pi.\n",iter);
			Mrational* _rational=_getRational(_getBiginteger(3),NULL,M_LD_NAN,false,true);
			if(_rational){
				if(iter>0){
					// working backwards starting with the last denominator quotient seems to be best
					// in every step you have to compute i**2/6 the second term of the denominator
					Mrational* _denominatorRational=_getRational(_getBiginteger(6),NULL,M_LD_NAN,false,true); // the final denominator equals 6
					if(_denominatorRational){
						if(amVerbose())output("First denominator rational computed.\n");
						long long square;
						Mbiginteger* _bi6=_getBiginteger(6); // the one we want to reuse in the computation (that we need to free when done)
						if(!_bi6){free_rational(_denominatorRational);return NULL;} // what a nuisance
						while(--iter>0){
							square=4*(iter+1)*iter+1;
							////////////if(amVerbose())output("Square numerator: %llu.",square);
							Mbiginteger* _bigintegerSquare=_getBiginteger(square);
							if(!_bigintegerSquare){output("%sFailed to compute the big integer of square %llu.\n",ERROR_PREFIX,square);break;}
							if(amVerbose())output("%llu fractions yet to compute using numerator square '%llu'.\n",iter,square);
							// the new denominator becomes 6+square/prev denominator=
							Mbiginteger* _mult=_Imultiply(_denominatorRational->num,_bi6,false);
							Mbiginteger* _add=(_denominatorRational->den?_Imultiply(_denominatorRational->den,_bigintegerSquare,false):_bigintegerSquare);
							Mbiginteger* _denominatorNumerator=(_mult?_Iadd(_mult,_add,false):NULL);
							// free all intermediate big integers
							free_biginteger(_mult);free_biginteger(_bigintegerSquare);if(_denominatorRational->den)free_biginteger(_add);
							// update the denominator rational, free the numerator if we fail to bind it to _denominatorRational
							// what's dangerous in the following is that _denominatorRational->num is not freed!!!!
							Mbiginteger* _previousDenominatorNumerator=_getBigintegerCopy(_denominatorRational->num);
							free_rational(_denominatorRational); // get the 'previous' numerator and denominator released!!!!!!
							_denominatorRational=_getRational(_denominatorNumerator,_previousDenominatorNumerator,M_LD_NAN,false,true);
							if(!_denominatorRational)break; // let's keep it normalized???? TODO is that necessary
							if(amVerbose())outputRational("Denominator (unnormalized): ",_rational,".\n");
						}
						free_biginteger(_bi6);
					}
					if(!_denominatorRational){outputError("Final denominator could not be computed");return NULL;}
					// NOTE: do NOT use the originals in inverting the denominator because those will be freed below so we need to pass in copies
					Mrational* _inverseDenominatorRational=_getInverseRational(_denominatorRational);
					Mrational* _result=NULL;
					if(!_inverseDenominatorRational){
						output("%s",ERROR_PREFIX);outputRational("Failed to compute the fractional part of pi (by inverting denominator rational ",_denominatorRational,").\n");
						free_rational(_rational);_rational=NULL;
					}else
						_result=_getRationalSum(_rational,_inverseDenominatorRational); // _qsum() replaced by _getRationalSum in Mrational.h/c
					free_rational(_denominatorRational);
					if(_result){
						_rational=_result;
						if(amVerbose())outputRational("Approximation of pi: ",_rational,".\n");
					}else{
						free_rational(_rational);
						_rational=NULL;
					}
				}
				return _getRationalValue(_rational,true);
			}
		}
	}
	return NULL;
}

// we can also use decimals to approximate pi to a certain precision (=decimal digits)
/* Python test program for approximating pi!!!!
import cdecimal
import decimal

def pi(module, prec):
    """From the decimal.py documentation"""
    module.getcontext().prec = prec + 2
    D = module.Decimal
    lasts, t, s, n, na, d, da = D(0), D(3), D(3), D(1), D(0), D(0), D(24)
    while s != lasts:
        lasts = s
        n, na = n+na, na+8
        d, da = d+da, da+32
        t = (t * n) / d
        s += t
    module.getcontext().prec -= 2
    return +s

for i in range(10000):
    x = pi(cdecimal, 28)

for i in range(10000):
    y = pi(decimal, 28)
 */
Mvalue* pi_d(Mvalue* value){
	// _value should be a positive integer defining the required precision
	if(amVerbose())output("Computing pi using decimals.\n");
	// MDH@17AUG2019: delegate to pi_decimal defined in Mdecimal.h/c
	return _getDecimalValue(pi_decimal(value&&value->type==VT_INTEGER?_getDecimalcontext(value->value._integer->ll):NULL),true);
}

// how about storing all results here?????? instead of in the root environment????
Mvalue* _resultListValue=NULL; // were the results are being kept
// the function that is used to return a specific result value
Mvalue* getResult(Mvalue* indexValue){
	if(amVerbose())outputLine("Result requested!");
	if(!indexValue)return _resultListValue;
	long long indexValueInteger=getValueInteger(indexValue); // NOTE all index values should be positive!!!
	return (indexValueInteger>0?getValueAtIndex(_resultListValue->value._list,indexValueInteger):NULL); // TODO are we calling getResult anywhere????
}

// LIST CONVERSIONS
////////Mvalue* ml(Menvironment* _executionEnvironment){return _getListValue(VT_LIST);} // a list that may only contain list elements is acceptable as map list!!

// list to map
Mvalue* l2m(Mvalue* value){
	Mvalue* _mapValue=NULL;
	if(value&&value->type==VT_LIST){
		_mapValue=_getMapValue(value->type,false); // create a map that is of the same type as the list is (typically VT_UNDEFINED)
		if(!listAppendedToMap(_mapValue->value._map,value->value._list))return NULL; // TODO should we 'release' the map that was created somehow???? I guess the map not getting assigned will be released somehow automatically...
	}
	return _mapValue;
}
// list to map list
Mvalue* l2ml(Mvalue* value){
	Mvalue* _maplistValue=NULL;
	if(value&&value->type==VT_LIST){
		_maplistValue=_getListValue(VT_LIST,false); // a map list ALWAYS requires element of type VT_LIST
		if(!listAppendedToMaplist(_maplistValue->value._list,value->value._list))return NULL; // TODO should we 'release' the map that was created somehow???? I guess the map not getting assigned will be released somehow automatically...
	}
	return _maplistValue;
}
// map list to list conversion
Mvalue* ml2l(Mvalue* value){
	Mvalue* _maplistValue=NULL;
	if(value&&value->type==VT_LIST){
		_maplistValue=_getListValue(value->type,false); // create a map that is of the same type as the list is (typically VT_UNDEFINED)
		if(!maplistAppendedToList(_maplistValue->value._list,value->value._list))return NULL; // TODO should we 'release' the map that was created somehow???? I guess the map not getting assigned will be released somehow automatically...
	}
	return _maplistValue;
}
Mvalue* ml2m(Mvalue* value){
	Mvalue* _mapValue=NULL;
	if(value&&value->type==VT_LIST){
		_mapValue=_getMapValue(value->type,false); // create a map that is of the same type as the list is (typically VT_UNDEFINED)
		if(!maplistAppendedToMap(_mapValue->value._map,value->value._list))return NULL; // TODO should we 'release' the map that was created somehow???? I guess the map not getting assigned will be released somehow automatically...
	}
	return _mapValue;
}

// map to map list conversion i.e. each list element is a attribute name - value pair
Mvalue* m2ml(Mvalue* value){
	Mvalue* _maplistValue=NULL;
	if(value&&value->type==VT_MAP){
		_maplistValue=_getListValue(VT_LIST,false); // a map list should always have element of type VT_LIST (this is the only additional requirement for a list to be accepted as map lists)
		if(!mapAppendedToMaplist(_maplistValue->value._list,value->value._map))return NULL; // TODO should we release the list that was created somehow????
	}
	return _maplistValue;
}
Mvalue* m2l(Mvalue* value){
	Mvalue* _listValue=NULL;
	if(value&&value->type==VT_MAP){
		_listValue=_getListValue(value->type,false);
		if(!mapAppendedToList(_listValue->value._list,value->value._map))return NULL; // TODO should we release the list that was created somehow????
	}
	return _listValue;
}
// conversion functions
 // the value wrapper for not a real and not an integer...
Mvalue* NAF_value=NULL;
Mvalue* NAI_value=NULL;
Mvalue* NULL_value=NULL; // the value containing the text to show when a value equals NULL

long double getNAR(){return NAF_value->value._float->ld;}
long long getNAI(){return NAI_value->value._integer->ll;}

// conversion to decimal,  hex and binary
// the 'real' number of octets used by a long double
#define M_LONG_DOUBLE_OCTETS 10
typedef union {
	long long ll;
	uint8_t octets[sizeof(long long)];
} longlongunion;
// a long double itself is 10 octets but sizeof(long double) might be 12 or 16
typedef union {
	long double ld;
	uint8_t octets[sizeof(long double)];
} longdoubleunion;
// return the value decimals in little endian order
Mvalue* getIntegerDecimalListValue(long long ll,bool littleEndianOrder){
	Mlist* _dlist=_getListOfType(VT_INTEGER);
	if(!_dlist)return NULL;
	longlongunion llu;
	llu.ll=ll;
	int l=sizeof(long long);
	while(--l>=0&&appendedToList(_dlist,_getIntegerValue(llu.octets[l]),(isLittleEndian()&&littleEndianOrder?l+1:M_LL_INVALID))>0);
	return _getValueOfList(_dlist,true);
}
const char* const REAL_OCTET_INDEX_IDS[]={"1","2","3","4","5","6","7","8","9","10"};
Mvalue* getRealDecimalMapValue(long double ld,bool littleEndianOrder){
	Mmap* _dmap=_getMapOfType(VT_INTEGER);
	if(!_dmap)return NULL;
	longdoubleunion lld;
	lld.ld=ld;
	int l=sizeof(long double);if(l>10)l=10; // assume 10-byte extended precision if sizeof(long double) exceeds 10 (like 12 or 16)
	// if we make a map with m0 through m7 for the mantisse, and e0 and e1 for the exponent
	if(littleEndianOrder^isLittleEndian()){ // user wants to see them in little endian order i.e. m0 first
		while(--l>=0)if(!appendedToMap(_dmap,REAL_OCTET_INDEX_IDS[l],_getIntegerValue(lld.octets[l])))break;
	}else{
		for(int i=0;i<l;i++)if(!appendedToMap(_dmap,REAL_OCTET_INDEX_IDS[i],_getIntegerValue(lld.octets[i])))break;
	}
	// how about extracting the mantisse and the exponent as well
	uint64_t mantisse;uint16_t exponent;extractMantisseAndExponent(ld,&mantisse,&exponent);
	// let's return the binary representation of exponent and mantisse with single quotes around it!!
	Mstring* _mantisseText=_getUint64BinaryText(mantisse,'\'');if(_mantisseText){appendedToMap(_dmap,"m",_getTextValue(string(_mantisseText),false));free_string(_mantisseText);}
	Mstring* _exponentText=_getUint16BinaryText(exponent,'\'');if(_exponentText){appendedToMap(_dmap,"e",_getTextValue(string(_exponentText),false));free_string(_exponentText);}
	/* replacing:
	Mbiginteger* _mantisse=new_Mbiginteger();mp_set_u64(_mantisse,mantisse); // we need a big integer here because uint64_t might not fit into a long long!!
	appendedToMap(_dmap,"m",_getBigintegerValue(_mantisse));appendedToMap(_dmap,"e",_getIntegerValue(exponent));
	*/
	return _getValueOfMap(_dmap,true);
}
char* _getIntegerCharacters(long long ll){
	char str[20];sprintf(str,"%lld",ll);return _strdup(str);
}
Mvalue* getTextDecimalMapValue(Mtext* text,bool ascendingindex){
	if(!text)return NULL;
	Mmap* _dmap=_getMapOfType(VT_INTEGER);
	if(!_dmap)return NULL;
	char* characters=text->_c;
	long long index=0;
	char* _indexCharacters;
	if(ascendingindex){
		appendedToMap(_dmap,"0",_getIntegerValue(text->presuffix)); // the quote character
		while(*characters){
			_indexCharacters=_getIntegerCharacters(++index);
			appendedToMap(_dmap,_indexCharacters,_getIntegerValue(*characters));
			free(_indexCharacters);
			characters++; // OOPS pretty essential
		}
	}else{
		// go to the end
		while(*characters){index++;characters++;}
		while(index){
			_indexCharacters=_getIntegerCharacters(index--);
			characters--;
			appendedToMap(_dmap,_indexCharacters,_getIntegerValue(*characters));
			free(_indexCharacters);
		}
		appendedToMap(_dmap,"0",_getIntegerValue(text->presuffix)); // the quote character
	}
	return _getValueOfMap(_dmap,true);
}
Mvalue* getRealDecimalListValue(long double ld,bool littleEndianOrder){
	Mlist* _dlist=_getListOfType(VT_INTEGER);
	if(!_dlist)return NULL;
	longdoubleunion lld;
	lld.ld=ld;
	int l=sizeof(long double);if(l>10)l=10; // assume 10-byte extended precision if sizeof(long double) exceeds 10 (like 12 or 16)
	// how about adding a two-element list with the first equal to the field name?????
	while(--l>=0&&appendedToList(_dlist,_getIntegerValue(lld.octets[l]),(isLittleEndian()&&littleEndianOrder?l+1:M_LL_INVALID))>0);
	return _getValueOfList(_dlist,true);
}

// we need d to compute the decimal from a given value instead of digitizing, so I suppose we'll rename d to b (for getting the bytes)
// TODO we should delegate to (_)getValueDecimal
Mvalue* d(Mvalue* value){
	if(value){
		switch(value->type){
			// TODO all other types_
			case VT_INTEGER:return _getDecimalValue(__decimal(NULL,value->value._integer->ll,0),true); // TODO assuming long long and int64_t are the same type!!!!!
			case VT_BIGINTEGER:return _getDecimalValue(_getBigintegerDecimal(value->value._biginteger),true);
			case VT_RATIONAL:return _getDecimalValue(_getRationalDecimal(value->value._rational),true);
			case VT_DECIMAL:return value;
			case VT_FLOAT: // TODO check whether somewhere I am converting a long double without using text
			default:return _getDecimalValue(_getValueTextDecimal(value),true);
		}
	}
	return NULL;
}
Mvalue* b(Mvalue* value){ // little-endian representation list to return
	if(value){
		switch(value->type){
			case VT_INTEGER:return getIntegerDecimalListValue(value->value._integer->ll,true);
			case VT_FLOAT:return getRealDecimalMapValue(value->value._float->ld,true);
			case VT_TEXT:return getTextDecimalMapValue(value->value._text,true);
			default:break;
		}
	}
	return NULL;
} 

Mvalue* B(Mvalue* value){ // big endian decimal representation list to return
	if(value){
		switch(value->type){
			case VT_INTEGER:return getIntegerDecimalListValue(value->value._integer->ll,false);
			case VT_FLOAT:return getRealDecimalMapValue(value->value._float->ld,false);
			case VT_TEXT:return getTextDecimalMapValue(value->value._text,false);
			default:break;
		}
	}
	return NULL;
}
// TODO to add h/H and b/B functions

Mvalue* i(Mvalue* value){
	if(amVerbose())outputValue("Converting '",value,"' to an integer.\n");
	long long ll=getValueInteger(value);
	return(ll!=M_LL_INVALID?_getIntegerValue(ll):NULL);
}

// convert to a big integer
Mvalue* I(Mvalue* value){
	if(value){
		if(value->type==VT_BIGINTEGER)return value; // already a big integer
		Mbiginteger* _bigInteger=_getValueBiginteger(value);
		if(_bigInteger)return _getBigintegerValue(_bigInteger,true);
	}
	return NULL;
}


// TODO complete the q function

// double to rational conversion (called rat_approx which computes int64_t* num and denom parameters)
// now returning an Mrational*, the larger md is choosen so we might stick to using LLONG_MAX as largest possible denominator
// source: https://rosettacode.org/wiki/Convert_decimal_number_to_rational#C
/* f : number to convert.
 * num, denom: returned parts of the rational.
 * md: max denominator value.  Note that machine floating point number
 *     has a finite resolution (10e-16 ish for 64 bit double), so specifying
 *     a "best match with minimal error" is often wrong, because one can
 *     always just retrieve the significand and return that divided by 
 *     2**52, which is in a sense accurate, but generally not very useful:
 *     1.0/7.0 would be "2573485501354569/18014398509481984", for example.
 */
/*
Mrational* _getLongDoubleRational(long double f){ // taking out: int64_t md, int64_t *num, int64_t *denom){
	//  a: continued fraction coefficients.
	long long a, h[3] = { 0, 1, 0 }, k[3] = { 1, 0, 0 };
	long long x, d, n = 1;
	int i, neg = 0;
 
	long long md=1000000; //////LLONG_MAX; // the largest possible long long

	// MDH@03JUN2019: with md equal to LLONG_MAX no need for: if (md <= 1) { *denom = 1; *num = (int64_t) f; return; }
 
	if (f < 0) { neg = 1; f = -f; }
 
	while (f != floor(f)) { n <<= 1; f *= 2; }
	d = f;
  
	output("%llu.",d);

	// continued fraction and check denominator each step
	for (i = 0; i < 64; i++) {
		a = n ? d / n : 0;
		if (i && !a) break;
 
		x = d; d = n; n = x % n;
 
		x = a;
		if (k[1] * a + k[0] >= md) {
			x = (md - k[0]) / k[1];
			if (x * 2 >= a || k[1] >= md)
				i = 65;
			else
				break;
		}
 
		h[2] = x * h[1] + h[0]; h[0] = h[1]; h[1] = h[2];
		k[2] = x * k[1] + k[0]; k[0] = k[1]; k[1] = k[2];
	}
	return _getRational(_getBiginteger(neg?-h[1]:h[1]),_getBiginteger(k[1]),true);
	// replacing:*denom = k[1];*num = neg ? -h[1] : h[1];
}
*/
/* a Java version
public Rational limitDenominator(long maximumDenominator) {
    if (maximumDenominator < 1) {
        throw new IllegalArgumentException("Denominator cannot be less than 1.");
    }
    if(this.den <= maximumDenominator)
        // we can't get closer than the current value
        return this;
    long p0 = 0;
    long q0 = 1;
    long p1 = 1;
    long q1 = 0;
    long n = this.num;
    long d = this.den;
    while(true) {
        long a = n / d;
        long q2 = q0 + a * q1;
        if(q2 > maximumDenominator)
            break;
        long oldP0 = p0;
        p0 = p1;
        q0 = q1;
        p1 = oldP0 + a * p1;
        q1 = q2;
        long oldN = n;
        n = d;
        d = oldN - a * d;
    }
    long k = (maximumDenominator - q0) / q1;
    Rational bound1 = new Rational(p0 + k * p1, q0 + k * q1);
    Rational bound2 = new Rational(p1, q1);
    if(bound2.minus(this).abs().compareTo(bound1.minus(this).abs()) <= 0){
        return bound2;
    } else {
        return bound1;
    }
}
*/
Mrational* _getRationalCopy(Mrational* _rational){
	if(!_rational)return NULL;
	Mbiginteger *_numeratorBiginteger=_getBigintegerCopy(_rational->num),*_denominatorBiginteger=(_rational->den?_getBigintegerCopy(_rational->den):NULL);
	if(!_numeratorBiginteger||(!_denominatorBiginteger&&_rational->den)){free_biginteger(_numeratorBiginteger);free_biginteger(_denominatorBiginteger);return NULL;} // some error
	// MDH@13JUN2019: if we can't get a rational, free the numerator and denominator
	Mrational* _copyRational=_getRational(_numeratorBiginteger,_denominatorBiginteger,(_rational->delta?_rational->delta->ld:M_LD_NAN),false,true);
	if(_copyRational)_copyRational->normalized=_rational->normalized; // copy the rational flag
	return _copyRational;
}
// _getValueRational() returns a (new) rational from the value stored in _value
Mrational* _getValueRational(Mvalue* _value){
	Mrational* _rational=NULL;
	if(_value){
		if(amVerbose())outputValue("Extracting the rational from '",_value,"'.\n");
		switch(_value->type){
			case VT_INTEGER:
			case VT_BIGINTEGER:
				_rational=_getRational(_getValueBiginteger(_value),NULL,M_LD_NAN,false,false); // not to free what's wrapped in _value
				break;
			case VT_DECIMAL:
				_rational=_getDecimalRational(_value->value._decimal);
				/* replacing (and augmenting in case of a repeating fractional part):
				{ // until we find a way to get the associated rational using the internal representation we stick to extracting the rational from the text representation of the decimal (which should be exact)
					char* _decimalText=mpd_to_sci(_value->value._decimal->mpd,0);
					if(_decimalText){_rational=_getDecimalTextRational(_decimalText,false);free(_decimalText);}else output("ERROR: Failed to obtain the text representation of a decimal.");
				}
				*/
				break;
			case VT_TEXT:
				_rational=_getDecimalTextRational(_value->value._text->_c);
				break;
			case VT_RATIONAL:
				_rational=_getRationalCopy(_value->value._rational); // NOTE return a copy NOT the original rational, only Mvalue things are immutable and the reference count is kept (and you should not use its contents elsewhere!!!)
				break;
			case VT_FLOAT:
				_rational=_getLongDoubleRational(_value->value._float->ld,250); // TODO how many iterations at most???
				break;
			case VT_LIST:
				if(_value->value._list->numberOfElements>1)
					_rational=_getRational(_getValueBiginteger(_value->value._list->_first->_value),_getValueBiginteger(_value->value._list->_first->_next->_value),
											(_value->value._list->numberOfElements>2?getValueLongDouble(_value->value._list->_first->_next->_next->_value):M_LD_NAN),true,false);
				break;
			default:break;
		}
	}
	return _rational;
}
// MDH@11AUG2019: why wasn't this here before???
Mrational* getValueRational(Mvalue* _value){
	if(_value&&_value->type==VT_RATIONAL)return _value->value._rational;
	return _getValueRational(_value);
}

// MDH@09OCT2019: unpure rationals can be purified using _getPurifiedRational
long double getReal(Mfloat* _real){return(_real?_real->ld:M_LD_NAN);}
Mrational* _getPurifiedRational(Mrational* pureRational,long double delta){
	Mrational* _purifiedRational=NULL;
	if(pureRational){
		// convert delta into a rational
		Mrational* _deltaRational=_getLongDoubleRational(delta,0);
		if(_deltaRational){
			_purifiedRational=_getPureRationalSum(pureRational,_deltaRational);
			free_rational(_deltaRational);
			if(!_purifiedRational)outputError("Failed to sum two pure rationals");
		}else
			outputError("Failed to rationalize a real");
	}else
		outputError("No base pure rational to use in purification");
	return _purifiedRational;
}

// TODO how many iterations would we accept at most?????
Mvalue* Q(Mvalue* _value){
	if(!_value)return NULL;
	if(_value->type==VT_RATIONAL)return _value; // if the value holds a rational itself, return just that
	if(_value->type==VT_FLOAT)return _getValueOfList(_getLongDoubleRationalList(_value->value._float->ld,250),true); // the intermediate results are stored in a list, and the last element will be the final result!!!
	Mvalue* _rationalValue=_getRationalValue(_getValueRational(_value),true); // make a rational from it and wrap it again
	if(amVerbose())outputValue("Converted to rational '",_rationalValue,"'.");
	return _rationalValue;
}
// MDH@09OCT2019: TODO=DONE how about turning a unpure rational into a pure rational???? yes, that's a good idea
Mvalue* q(Mvalue* _value){
	if(!_value)return NULL;
	if(_value->type==VT_RATIONAL){
		Mrational* rational=_value->value._rational;
		if(floatIsUndefinedOrZero(rational->delta))return _value;
		long double rationaldelta=getReal(rational->delta);
		Mrational* _purifiedRational=NULL;
		Mrational* _pureRational=_getRational(_getBigintegerCopy(rational->num),_getBigintegerCopy(rational->den),M_LD_NAN,true,true);
		if(_pureRational){
			_purifiedRational=_getPurifiedRational(_pureRational,rationaldelta);
			free_rational(_pureRational);
			if(!_purifiedRational)outputError("Failed to purify a rational");
		}else
			outputError("Failed to create a pure rational");
		return _getRationalValue(_purifiedRational,true);
	}
	if(_value->type==VT_FLOAT)return _getRationalValue(_getLongDoubleRational(_value->value._float->ld,250),true); // forcefully free the _getLongDoubleRational if we failed to wrap it
	Mvalue* _rationalValue=_getRationalValue(_getValueRational(_value),true); // make a rational from it and wrap it again
	if(amVerbose())outputValue("Converted to rational '",_rationalValue,"'.");
	return _rationalValue;
}

// convert to a real

// TODO complete with conversion from big integer and rational
Mvalue* f(Mvalue* _value){
	Mvalue* _floatValue=NAF_value;
	if(_value){
		if(amVerbose()){outputValue("Converting '",_value,"'");output(" of type %s to a floating point value.\n",VALUETYPENAMES[_value->type]);}
		switch(_value->type){
			case VT_INTEGER:_floatValue=_getFloatValue((long double)_value->value._integer->ll);break;
			case VT_BIGINTEGER:_floatValue=_getFloatValue(mp_get_long_double(_value->value._biginteger));break;
			case VT_DECIMAL:_floatValue=_getFloatValue(getDecimalLongDouble(_value->value._decimal));break;
			case VT_RATIONAL:_floatValue=_getFloatValue(getRationalLongDouble(_value->value._rational));break;
			case VT_FLOAT:_floatValue=_value;break; // TODO should we make a copy here? NO, Mvalue* instances don't need to be duplicated because they are immutable
			case VT_TEXT:_floatValue=_getFloatValue(_strtold(_value->value._text->_c,getNAR()));break;
			default:break;
		}
	}
	if(amVerbose())outputValue("Converted to '",_floatValue,"'.\n");
	return _floatValue;
}
// MDH@build 2: text representation of a value with a given format (either an integer denoting the number of positions to place the text in)
Mvalue* t(Mvalue* value,Mvalue* format){
	Mvalue* result=NULL;
	Mstring* _valueText=_getValueText(value,true); // typically dequoted
	if(_valueText){
		if(amVerbose()){outputValue("Text representation of '",value,"' before formatting: ");output("'%s'.\n",string(_valueText));}
		if(format){
			if(format->type==VT_INTEGER){
				long long ll=format->value._integer->ll;
				if(ll>0){ // left-aligned in ll positions
					ll-=string_length(_valueText); // number of blanks to append
					while(--ll>=0)if(!string_append_char(_valueText,' '))break;
				}else
				if(ll<0){ // right-aligned in -ll positions
					ll+=string_length(_valueText); // - number of blanks to prepend
					while(++ll<=0)if(!string_insert_char(_valueText,0,' '))break;
				}
			}
		}
		if(string_insert_char(_valueText,0,(value->type==VT_TEXT?value->value._text->presuffix:'\''))){ // prepend a quote character otherwise we're in trouble in _getTextValue
			if(amVerbose()){outputValue("Text representation of '",value,"': ");output("'%s'.\n",string(_valueText));}
			result=_getTextValue(string(_valueText),false);
		}else
			outputError("Failed to prepend a quote character to a text representation");
		free_string(_valueText);
	}
	return result;
}

// the type of a value
Mvalue* type(Mvalue* _value){
	// every value should have a type text, even if NULL
	// MDH@03NOV2019: actually _value should be the name of a variable because it not we cannot determine whether or not
	//                the variable is mutable, that's why settype() requires the name of the variable (as text)
	char result[3]="' ";
	if(_value)
	result[1]=MUTABLEVALUETYPECHARS[_value->type];
	/* ewplacing:
	switch(_value->type){
		case VT_UNDEFINED:result[1]='-';break;
		case VT_TOKEN:return _getTextValue("'T",false); // can we find another character for that, so we can use t for text????
		case VT_INTEGER:return _getTextValue("'i",false);
		case VT_BIGINTEGER:return _getTextValue("'b",false);
		case VT_DECIMAL:return _getTextValue("'d",false);
		case VT_RATIONAL:return _getTextValue("'q",false);
		case VT_FLOAT:return _getTextValue("'f",false);
		case VT_TEXT:return _getTextValue("'t",false); // t for text
		case VT_LIST:return _getTextValue("'l",false);
		case VT_MAP:return _getTextValue("'m",false);
		case VT_REFERENCE:return _getTextValue("'r",false); // r now short for reference, as changing real into float
		/////case VT_USERFUNCTION:return _getTextValue("'f",false);
	}
	*/
	return _getTextValue(result,false);
}

Mvalue* add(Mvalue* _value1,Mvalue* _value2);
Mvalue* Msum(Mvalue* _value){
    if(_value){
		if(amVerbose())outputValue("Computing the sum of '",_value,"'.\n");
        if(_value->type==VT_LIST){
			Mvalue* _sumValue=NULL;
			// all the values in the list could be integer
			Mlist* _list=_value->value._list;
			if(_list){
				Mlistelement* _listelement=_list->_first;
				if(_listelement){
					// how about adding as decimals????
					assignValue(&_sumValue,_listelement->_value); // TODO I suppose we can do this????
					while(_listelement->_next){_listelement=_listelement->_next;_sumValue=add(_sumValue,_listelement->_value);}
				}
			}
			return _sumValue;
		}
    }
    return _value; // the default
}

// MDH@10OCT2019: applying unary operator (=function) to all elements in a list
Mvalue* _functionAppliedToList(Mlist* _list,OneArgumentFunction function){
	// scalars are to be added to each element of the original list
	// lists are to be added to the elements at the same position, so listwise
	Mlist* _result=NULL;
	if(function&&_list){ // we need both a function and a list
		_result=_getListOfType(_list->valuetype); // this could pose a problem as the function may not return the same value type as the elements in the list (i.e. if it doesn't we're in trouble!!!!)
		Mlistelement* _listelement=_list->_first;
		while(_listelement&&appendedToList(_result,function(_listelement->_value),_listelement->index))_listelement=_listelement->_next;
	}
	return(_result?_getValueOfList(_result,true):NULL);
}
// MDH@10OCT2019: a special function to compute a reciprocal value
Mvalue* Mreciprocal(Mvalue* value){
	if(value)
	switch(value->type){
		case VT_LIST:return _functionAppliedToList(value->value._list,Mreciprocal);
		case VT_FLOAT:return _getFloatValue(1/value->value._float->ld); // TODO check what happens when the real equals 0
		case VT_RATIONAL:return _getRationalValue(_getInverseRational(value->value._rational),true);
		case VT_INTEGER:return _getRationalValue(_getRational(NULL,_getBiginteger(value->value._integer->ll),M_LD_NAN,true,true),true);
		case VT_BIGINTEGER:return _getRationalValue(_getRational(NULL,value->value._biginteger,M_LD_NAN,true,false),true); // same as with VT_INTEGER but without freeing the to remain bound big integer
		case VT_DECIMAL:return _getDecimalValue(_getInverseDecimal(value->value._decimal),true);
		default:break; // TODO maps!!!!
	}
	return NULL;
}
// MDH@29OCT2019: concatenate textual, typically used for lists
Mstring* _getConcatenated(Mlist* list,char* separator){
	Mstring* _concatenated=__string();
	if(_concatenated){
		Mlistelement* listelement=list->_first;
		Mvalue* listelementValue=NULL;
		while(listelement){
			listelementValue=listelement->_value;
			Mstring* _listelementText=NULL;
			if(listelementValue){
				if(listelementValue->type==VT_LIST)
					_listelementText=_getConcatenated(listelementValue->value._list,separator);
				else
					_listelementText=_getValueText(listelementValue,true);
			}
			if(_listelementText){
				if(separator)if(string_length(_concatenated)>0)string_append(_concatenated,separator);
				string_append(_concatenated,string(_listelementText));
				free_string(_listelementText);
			}
			listelement=listelement->_next;
		}
	}else 
		outputError("Failed to initialize the concatenation result text");
	return _concatenated;
}
Mvalue* Mconcat(Mvalue* value1,Mvalue* value2){
	// the first value would be the list of things to concatenate, the second value the separator text (if any)
	if(value1){
		Mstring* _separator=(value2?_getValueText(value2,true):NULL); // _getValueText() would return ? when receiving NULL, so for now we have to prevent that!!
		Mstring* _result;
		if(value1->type==VT_LIST)
			_result=_getConcatenated(value1->value._list,(_separator?string(_separator):NULL));
		else
			_result=_getValueText(value1,true);
		Mvalue* _resultValue=NULL;
		if(_result){
			// _result itself won't contain quotes, so in order to make it usable we need to prepend either a single quote or a double quote
			if(string_insert_char(_result,0,'\''))_resultValue=_getTextValue(string(_result),false);else outputError("Failed to construct the concatenation text");
			free_string(_result);
		}
		if(_separator)free_string(_separator);
		return _resultValue;
	}
	return NULL;
}
Mvalue* Mfibonacci(Mvalue* value){
	// are we allowing big integers?
	if(!value||value->type==VT_MAP)return NULL;
	if(value->type==VT_LIST)return _functionAppliedToList(value->value._list,Mfibonacci);
	// ASSERT assuming scalars
	Mbiginteger* _biginteger=_getValueBiginteger(value);
	if(_biginteger){
		mp_err status=MP_OKAY; // keep track of the result status
		Mbiginteger* _fibonacciBiginteger=NULL;
		if(isBigintegerUndefined(_biginteger)==M_FALSE){ // not an undefined big integer
			if(isBigintegerNegative(_biginteger)!=M_TRUE){ // not a negative big integer
				Mbiginteger *_counterBiginteger=_getBigintegerCopy(_biginteger); // the number of times we will have to do an addition
				_fibonacciBiginteger=__biginteger(); // where the result should be stored
				if(_fibonacciBiginteger&&_counterBiginteger)status=mp_decr(_counterBiginteger);else status=MP_ERR;
				if(status==MP_OKAY){
					if(isBigintegerPositive(_counterBiginteger)==M_TRUE){ // at least one addition to do
						Mbiginteger *_firstBiginteger=_getBiginteger(0),*_secondBiginteger=_getBiginteger(1);
						if(_firstBiginteger&&_secondBiginteger){
							// NOTE _counterBiginteger defines the number of times we need to add the first and second big integer
							while(isBigintegerZero(_counterBiginteger)!=M_TRUE){ // the counter is not zero yet
								if((status=mp_decr(_counterBiginteger))!=MP_OKAY)break;
								if((status=mp_add(_firstBiginteger,_secondBiginteger,_fibonacciBiginteger))!=MP_OKAY)break;
								// if we're smart we only need to exchange one big integer
								if((status=mp_copy(_secondBiginteger,_firstBiginteger))!=MP_OKAY)break;
								if((status=mp_copy(_fibonacciBiginteger,_secondBiginteger))!=MP_OKAY)break;
							}
						}else 
							outputError("Failed to initialize the Fibonacci sequence");
						free_biginteger(_firstBiginteger);free_biginteger(_secondBiginteger);
					}else // no additions
						status=mp_copy(_biginteger,_fibonacciBiginteger);
				}else
					outputError("Failed to initialize the Fibonacci sum");
				free_biginteger(_counterBiginteger);
			}
		}
		if(value->type!=VT_BIGINTEGER)free_biginteger(_biginteger);
		if(status==MP_OKAY)return _getBigintegerValue(_fibonacciBiginteger,true);
		// ASSERT something went wrong in the computations
		if(_fibonacciBiginteger)free_biginteger(_fibonacciBiginteger); // not bound, so free here
	}
	return NULL;
}

Menvironment* _Menvironment; // this is the root (M) environment
///// NOT HERE see Mexecution.c!!!! Menvironment* _executionEnvironment=NULL; // the current execution environment (in which functions are called!!!)

// some prototypes we need in initEnvironment()
Mtoken* _getToken(Mtoken* prevToken,TokenType newTokenType);
Mvalue* Miffunction(Mvalue* _conditionTokenValue,Mvalue* _thenTokenValue,Mvalue* _elseTokenValue);
Mvalue* Mwhilefunction(Mvalue* _conditionTokenValue,Mvalue* _whilebodyTokenValue);
Mvalue* Mdofunction(Mvalue* _doTokenValue);
Mvalue* Mforfunction(Mvalue* _initializationTokenValue,Mvalue* _conditionTokenValue,Mvalue* _incrementTokenValue,Mvalue* _forbodyTokenValue);
Mvalue* Mevalfunction(Mvalue* value);

bool initEnvironment(){

	long long decimalprecision=getDP();
	if(decimalprecision==M_LL_INVALID)return false; // let's force starting with a default decimal context
	output("Default decimal precision: %llu. Call setdp() to change it.\n",decimalprecision);

	NAF_value=_getFloatValue(M_LD_NAN); // NaN is defined in Mexecution.h as 0.0/0.0 (as a constant)
	NAI_value=_getIntegerValue(M_LL_INVALID);

	NULL_value=__value(); 
	if(!NULL_value){outputError("Failed to initialize NULL.");return false;}

	// MDH@23OCT2019: we really want NULL to be a variable with NO value, so we can actually use it to NULL a value!!
	//                therefore it shouldn't be a token value 
	/*
	NULL_value=_getValueOfToken(_getToken(NULL,TT_SQSTRING),true);
	if(NULL_value)NULL_value->value._token->text=__string("NULL");else outputError("BUG: Failed to create NULL value!");
	*/

	// either set the DP_value to 0 (failed to get a decimal context somehow)
	/* MDH@20JUN2019: no need for DP_value anymore (as setdp() return _decimalContext->prec now): 
	_decimalContext=get_mpd_context(M_DP); // initialize the application-wide decimal context with precision M_DP
	assignValue(DP_value,_getIntegerValue(_decimalContext?_decimalContext->prec:0L));
	if(!DP_value)output("WARNING: Failed to initialize the decimal precision.");
	*/

	_resultListValue=_getListValue(VT_UNDEFINED,true); // ascertain to have a list value in which the results can be stored
	// ESSENTIAL not to loose this list immediately!!!
	if(_resultListValue)incrementReferenceCount(_resultListValue);else outputLine("WARNING: Failing to create the results list. The results will not be available through the M function!");
	_Menvironment=__environment(); // MDH@17JUL2019: calling the generic 'constructor' that will create a variable map for us automatically
	if(_Menvironment){
		_Menvironment->_name=_strdup("M"); // TODO why make a dynamic copy???
		Mmap* environmentVariableMap=_Menvironment->_variableMap; // which must exist!!!
		Mfunctionmap* environmentFunctionMap=CALLOC(1,sizeof(Mfunctionmap),'M');
		if(environmentFunctionMap){
			// TODO should we allow assigning to NULL by defining NULL as a variable??????
			// MDH@29MAY2019: we've got (symbol) NULL
			if(!addVariable(_Menvironment,M_NULL_VARIABLE_NAME,VT_UNDEFINED,true)||!setValue(_Menvironment,M_NULL_VARIABLE_NAME,NULL_value)){
				outputLine("WARNING: Failed to create, add or initialize constant NULL.");
			}
			if(!NAF_value||!addVariable(_Menvironment,"NAF",VT_FLOAT,true)||!setValue(_Menvironment,"NAF",NAF_value)){
				outputLine("WARNING: Failed to create, add or initialize Not-a-float constant NAF.");
				////////return false;
			}
			if(!NAI_value||!addVariable(_Menvironment,"NAI",VT_INTEGER,true)||!setValue(_Menvironment,"NAI",NAI_value)){
				outputLine("WARNING: Failed to create, add or initialize Not-an-integer default NAI.");
				////////return false;
			}
			/* MDH@13JUN2019: allow user to change the decimal precision
			if(!DP_value||!addVariable(_Menvironment,"$decimalprecision",VT_INTEGER,false)||!setValue(_Menvironment,"$decimalprecision",DP_value)){
				outputLine("WARNING: Failed to create, add or initialize Not-an-integer default NAI.");
				////////return false;
			}*/
			// create and add PI and E constants!!!
			Mvalue* PI_value=_getFloatValue(M_LD_PI);
			if(!PI_value){
				outputLine("ERROR: Failed to create PI.");
				return false;
			}
			if(!addVariable(_Menvironment,"PI",VT_FLOAT,true)){
				outputLine("ERROR: Failed to add PI.");
				///////free_value(PI_value);
				return false;
			}
			if(!setValue(_Menvironment,"PI",PI_value)){
				////////free_value(PI_value);
				outputLine("ERROR: Failed to initialize PI.");
				return false;
			}
			Mvalue* E_value=_getFloatValue(M_LD_E);
			if(!E_value){
				///////free_value(E_value);
				outputLine("ERROR: Failed to create E.");
				return false;
			}
			if(!addVariable(_Menvironment,"E",VT_FLOAT,true)){
				outputLine("ERROR: Failed to add E.");
				return false;
			}
			if(!setValue(_Menvironment,"E",E_value)){
				//////free_value(E_value);
				outputLine("ERROR: Failed to initialize E.");
				return false;
			}
			/*
			// we're going to store all commands in a list called M
			Mvalue* Mvalue=_getListValue(VT_UNDEFINED);
			if(!M_value){
				outputLine("ERROR: Failed to create the M result list.");
				return false;
			}
			if(!addVariable(_Menvironment,"M",VT_LIST,true)){
				outputLine("ERROR: Failed to add the M result list.");
				return false;
			}
			if(!setValue(_Menvironment,"M",M_value)){
				outputLine("ERROR: Failed to initialize the M result list.");
				return false;
			}
			*/
			_Menvironment->_functionMap=environmentFunctionMap;
			// register if, while and for special functions
		    if(!completedValueTokenTokenFunction(_getFunction(_Menvironment,IFFUNCTION_NAME),IFFUNCTION_NAME,Miffunction))return false;
		    if(!completedTokenTokenFunction(_getFunction(_Menvironment,WHILEFUNCTION_NAME),WHILEFUNCTION_NAME,Mwhilefunction))return false;
		    if(!completedTokenTokenTokenTokenFunction(_getFunction(_Menvironment,FORFUNCTION_NAME),FORFUNCTION_NAME,Mforfunction))return false;
			// MDH@05AUG2019: the do function has a single token to process
		    if(!completedTokenListFunction(_getFunction(_Menvironment,DOFUNCTION_NAME),DOFUNCTION_NAME,Mdofunction))return false;
		    if(!completedValueFunction(_getFunction(_Menvironment,EVALFUNCTION_NAME),EVALFUNCTION_NAME,Mevalfunction))return false;

			if(!registerInternalFunctions(_Menvironment)){
				outputError("Failed to register all internal functions");
				return false;
			}
			// additional functions some of which need to know the root environment, I suppose a function should have access to its environment?????
			if(_resultListValue&&!completedIntegerFunction(_getFunction(_Menvironment,"M"),"M",getResult)){
				outputError("Failed to register function M (for requesting previous results)");
				return false;
			}
			/*
			if(!completedFunction(_getFunction(_Menvironment,"ml"),ml)){
				outputLine("ERROR: Failed to register map list (constructor) function.");
				return false;
			}
			*/
			if(!completedIntegerFunction(_getFunction(_Menvironment,"setdp"),"setdp",setdp)||!completedIntegerFunction(_getFunction(_Menvironment,"getdc"),"getdc",getdc)||!completedIntegerFunction(_getFunction(_Menvironment,"getdp"),"getdp",getdp)){
				outputError("Failed to register the setdp, getdc and getdp functions");
				return false;
			}
			// pi() functions (decimal and rational)
			if(!completedIntegerFunction(_getFunction(_Menvironment,"pi$q"),"pi$q",pi_q)||!completedIntegerFunction(_getFunction(_Menvironment,"pi$ql"),"pi$ql",pi_ql)||!completedIntegerFunction(_getFunction(_Menvironment,"pi"),"pi",pi_d)){
				outputError("Failed to register the pi, pi$q and pi$ql functions");
				return false;
			}
			// conversions (MDH@30OCT2019: real renamed to float because we actually have multiple representations of a real (like decimals and rationals))
			if(!completedValueFunction(_getFunction(_Menvironment,"i"),"i",i)||!completedValueFunction(_getFunction(_Menvironment,"I"),"I",I)
					||!completedValueValueFunction(_getFunction(_Menvironment,"t"),"t",t)
					||!completedValueFunction(_getFunction(_Menvironment,"f"),"f",f)
					||!completedValueFunction(_getFunction(_Menvironment,"q"),"q",q)||!completedValueFunction(_getFunction(_Menvironment,"Q"),"Q",Q)
					||!completedValueFunction(_getFunction(_Menvironment,"d"),"d",d)
					||!completedValueFunction(_getFunction(_Menvironment,"b"),"b",b)||!completedValueFunction(_getFunction(_Menvironment,"B"),"B",B)){
				outputError("Failed to register value type conversion functions");
				return false;
			}
			if(!completedValueFunction(_getFunction(_Menvironment,"type"),"type",type)){
				outputError("Failed to register the type function");
				return false;
			}
			if(!completedValueFunction(_getFunction(_Menvironment,"keys"),"keys",Mkeys)){
				outputError("Failed to register the keys function");
				return false;
			}
			if(!completedValueFunction(_getFunction(_Menvironment,"neg"),"neg",Mneg)||!completedValueFunction(_getFunction(_Menvironment,"bnot"),"bnot",Mbnot)||!completedValueFunction(_getFunction(_Menvironment,"not"),"not",Mnot)){
				outputError("Failed to register all unary (neg, bnot, and not) functions");
				return false;
			}
			if(!completedValueFunction(_getFunction(_Menvironment,"exists"),"exists",Mexists)||!completedValueFunction(_getFunction(_Menvironment,"scalar"),"scalar",Mscalar)||!completedValueFunction(_getFunction(_Menvironment,"null"),"null",Mnull)||!completedValueFunction(_getFunction(_Menvironment,"undefined"),"undefined",Mundefined)){
				outputError("Failed to register the exists, scalar, null and undefined functions");
				return false;
			}
			if(!completedValueFunction(_getFunction(_Menvironment,"sign"),"sign",Msign)){
				outputError("Failed to register the sign function");
				return false;
			}
			if(!completedValueFunction(_getFunction(_Menvironment,"zero"),"zero",Mzero)||!completedValueFunction(_getFunction(_Menvironment,"positive"),"positive",Mpositive)||!completedValueFunction(_getFunction(_Menvironment,"negative"),"negative",Mnegative)){
				outputError("Failed to register the zero, positive and negative functions");
				return false;
			}
			if(!completedValueFunction(_getFunction(_Menvironment,"sum"),"sum",Msum)||!completedValueFunction(_getFunction(_Menvironment,"len"),"len",Mlen)){
				outputError("Failed to register the sum and len list functions");
				return false;
			}
			// MDH@01NOV2019: I have some generic list functions implemented
			if(!completedValueFunction(_getFunction(_Menvironment,"empty"),"empty",Mempty)){
				outputError("Failed to register the empty function");
				return false;
			}
			if(!completedListFunction(_getFunction(_Menvironment,"pull"),"pull",Mpull)||!completedListFunction(_getFunction(_Menvironment,"pop"),"pop",Mpop)||!completedListFunction(_getFunction(_Menvironment,"first"),"first",Mfirst)||!completedListFunction(_getFunction(_Menvironment,"last"),"last",Mlast)){
				outputError("Failed to register the pull, pop, first and last function");
				return false;
			}
			if(!completedListValueFunction(_getFunction(_Menvironment,"push"),"push",Mpush)||!completedListValueFunction(_getFunction(_Menvironment,"drop"),"drop",Mpush)||!completedListValueFunction(_getFunction(_Menvironment,"shove"),"shove",Mshove)||!completedListFunction(_getFunction(_Menvironment,"pop"),"pop",Mpop)){
				outputError("Failed to register the push(=drop), shove and pop functions");
				return false;
			}

			if(!completedValueFunction(_getFunction(_Menvironment,"tl"),"tl",Mtl)){
				outputError("Failed to register the tl text function");
				return false;
			}
			if(!completedValueFunction(_getFunction(_Menvironment,"fac"),"fac",Mfac)||!completedValueFunction(_getFunction(_Menvironment,"facd"),"facd",Mfacd)){
				outputError("Failed to register the fac and facd function");
				return false;
			}
			if(!completedValueFunction(_getFunction(_Menvironment,"reciprocal"),"reciprocal",Mreciprocal)||!completedValueFunction(_getFunction(_Menvironment,"fibonacci"),"fibonacci",Mfibonacci)){ // MDH@10OCT2019
				outputError("Failed to register the reciprocal and fibonacci function");
				return false;
			}
			if(!completedValueValueFunction(_getFunction(_Menvironment,"concat"),"concat",Mconcat)){
				outputError("Failed to register the concat function");
				return false;
			}
			// register list conversions
			if(!completedListFunction(_getFunction(_Menvironment,"l2m"),"l2m",l2m)||!completedListFunction(_getFunction(_Menvironment,"l2ml"),"l2ml",l2ml)||!completedListFunction(_getFunction(_Menvironment,"ml2l"),"ml2l",ml2l)||!completedListFunction(_getFunction(_Menvironment,"ml2m"),"ml2m",ml2m)){
				outputError("Failed to register list conversion functions");
				return false;
			}
			// register map conversions
			if(!completedListFunction(_getFunction(_Menvironment,"m2ml"),"m2ml",m2ml)||!completedListFunction(_getFunction(_Menvironment,"m2l"),"m2l",m2l)){
				outputError("Failed to register map conversion functions");
				return false;
			}
		}
	}
	if(pushExecutionEnvironment(_Menvironment))return true;
	outputError("Failed to activate the M environment");
	return false;
}

// user interaction stuff
#include "Msession.h"
/*
char* _getFormattedText(uint8_t maxlength,char* fmt,...){
	char* str=malloc(maxlength+1); // get enough room on the heap
	if(str){va_list args;va_start(args,fmt);sprintf(str,fmt,args);va_end(args);}
	return str;
}
*/
/* source: https://stackoverflow.com/questions/16839658/printf-width-specifier-to-maintain-precision-of-floating-point-value
#ifdef DBL_DECIMAL_DIG
  #define OP_DBL_Digs (LDBL_DECIMAL_DIG)
#else  
  #ifdef DECIMAL_DIG
    #define OP_DBL_Digs (LDECIMAL_DIG)
  #else  
    #define OP_DBL_Digs (LDBL_DIG + 3)
  #endif
#endif
*/

Mstring* _getFunctionMapText(Mfunctionmap* _functionmap){
	Mstring* s=__string();
	if(s){
		Mstring* p=string_append_char(s,'[');
		if(_functionmap){
			///printf("\n%s(%d)",string(p),_functionmap->numberOfFunctions);
			Mfunctionmapelement* _functionmapelement=_functionmap->_first;
			while(_functionmapelement){
				///printf("\n%s","start");
				Mfunction* _function=_functionmapelement->_function;
				if(!_function)continue;
				///printf("\n%s","func");
				p=string_append(p,string(_functionmapelement->_name)); // _name moved from _function to _functionmapelement
				if(!p)break;
				///printf("\n%s","name");
				// I guess we might show the parameter map (if any)
				string_append_char(p,'(');
				if(_function->_parameterMap){
					Mstring* parameterMapText=_getMapText(_function->_parameterMap,false,false,false); // do NOT show curly braces, quotes or missing defaults
					if(parameterMapText){
						string_append(p,string(parameterMapText));
						free_string(parameterMapText);
					}
				}
				///printf("\n%s","params");
				string_append_char(p,')');
				_functionmapelement=_functionmapelement->_next;
				if(_functionmapelement)p=string_append(p,", "); // only when there's a next map element to process
				///printf("\n%s","next");
			}
		}
		//printf("\n%s(%d)",string(p),string_length(p));
		p=string_append_char(p,']');
		///printf("\n%s",string(p));
		// if we failed, we have to free s here!!!
		if(!p){free_string(s);s=NULL;}
	}
	return s;
}
void outputFunctions(){
	Mstring* _functionsText=_getFunctionMapText(getEnvironment()->_functionMap);
	output("\nFunctions: %s.\n",string(_functionsText));
	free_string(_functionsText);
}/* VALIDATED */
void outputVariables(){
	// much easier now that we get the text of any Mvalue (like the variable map of an environment!)
	// MDH@24OCT2019: now using _getVariableMapText() instead of _getMapText() because the former is environment aware and can show the symbols with the same value (if any)
	Mstring* _variablesText=_getVariableMapText(getEnvironment(),false,false,true);
	output("\nVariables: %s.\n",string(_variablesText));
	free_string(_variablesText);
}/* VALIDATED */

enum INPUTMODE_ENUM {IM_COMMAND,IM_CONTROL,IM_SHELL}; // the possible input modes: command, control, and shell

enum INPUTMODE_ENUM inputMode=IM_COMMAND; // whether or not in command mode

char* promptinfo[]={"Command mode: clear the command with Ctrl-C.","Control mode: Flags: Assist|color scheme (0 or 1)|Debug|Match parentheses|Use history command|Verbose|Wrap - Options: Beep|Reset|eXit|Functions|History|Shell.","Shell mode: enter a system command to execute."};
/**
call prompt() when ready to receive a new command
 */
const char OPTION_CHAR='`'; // TODO should this character become part of options????

// USER INPUT STUFF

/* TODO are we using the storeCursor() and restoreCursor() sometime?
// VT100 codes...
void storeCursor(){printf("\0337");}
void restoreCursor(){printf("\0338");}
*/

void displayFlags(){
	output("Edit flags: %c%c%c%c%c - Display flags: %c%c.\n",amAssisting()?'A':'a',amDebugging()?'D':'d',amMatchingparentheses()?'M':'m',amVerbose()?'V':'v',amAcceptinghistorycommand()?'U':'u',amWrapping()?'W':'w',48+getColorscheme());
}

void outputFlags(){
	output("%c%c%c%c%c%c%c",amAssisting()?'A':'a',(48+getColorscheme()),amDebugging()?'D':'d',amMatchingparentheses()?'M':'m',amVerbose()?'V':'v',amWrapping()?'W':'w',amAcceptinghistorycommand()?'U':'u');
}

// MDH@19JUL2019: in order to be able to obtain the body code of functions we're keeping a stack of function names of which the body is requested
typedef struct FunctionBodyRequest{
	char* functionName;
	struct FunctionBodyRequest *_next;
}FunctionBodyRequest;
// requests can come out of a single command containing multiple function definitions
// _firstFunctionBodyRequest represents the first one to execute
FunctionBodyRequest *_firstFunctionBodyRequest=NULL,*_lastFunctionBodyRequest=NULL;
FunctionBodyRequest* requestBodyOfFunction(char* functionName){
	if(functionName&&strlen(functionName)){ // a 'valid' function name
		// technically it should not have been requested already (or exist)
		FunctionBodyRequest* _functionBodyRequest=_firstFunctionBodyRequest;
		while(_functionBodyRequest&&strcmp(functionName,_functionBodyRequest->functionName))_functionBodyRequest=_functionBodyRequest->_next;
		if(_functionBodyRequest){
			output("%sBody of function '%s' already requested.\n",ERROR_PREFIX,functionName);
			return NULL;
		}
		if(amVerbose())output("The body of function '%s' being requested.\n",functionName);
		_functionBodyRequest=CALLOC(1,sizeof(FunctionBodyRequest),'B');
		if(_functionBodyRequest){
			_functionBodyRequest->functionName=functionName;
			if(_lastFunctionBodyRequest)_lastFunctionBodyRequest->_next=_functionBodyRequest;
			_lastFunctionBodyRequest=_functionBodyRequest;
			if(!_firstFunctionBodyRequest)_firstFunctionBodyRequest=_lastFunctionBodyRequest;
			return _lastFunctionBodyRequest;
		}
		output("%sFailed to register the request for the body of function '%s'.\n",ERROR_PREFIX,functionName);
	}
	return NULL;
}

bool isExecutionEnvironmentInitialized(Menvironment* _executionEnvironment,Mmap* _variableMap){
	bool executionEnvironmentInitialized=true;
	Mmapelement* variableMapelement=_variableMap->_first;
	Mvariable* variableMapelementVariable;
	while(executionEnvironmentInitialized&&variableMapelement){
		variableMapelementVariable=variableMapelement->_variable;
		if(!strlen(variableMapelementVariable->_name))continue; // no use to create a variable with no name
		// NOTE the map element variable name seems to be enclosed in quotes, and should be dequoted unless we do that when the argument map is created
		if(!addVariable(_executionEnvironment,variableMapelementVariable->_name,variableMapelementVariable->valuetype,false)){
			output("%sFailed to add variable '%s' as local variable.\n",ERROR_PREFIX,variableMapelementVariable->_name);
			executionEnvironmentInitialized=false;
		}else
		if(!setValue(_executionEnvironment,variableMapelementVariable->_name,variableMapelementVariable->_value)){
			output("%sFailed to initialize local variable '%s'.\n",ERROR_PREFIX,variableMapelementVariable->_name);
			executionEnvironmentInitialized=false;
		}else
			variableMapelement=variableMapelement->_next;
	}
	return executionEnvironmentInitialized;
}
/*
\brief returns the environment for executing the the function called \p functionName
\p functionName the name of the function to execute
obviously when defining the function body there will be no commands to execute
 */
Menvironment* _getFunctionExecutionEnvironment(Mfunction* _function,char* functionName,Mmap* _argumentMap){
	// 1. create an environment in which to execute the expression list of the given function initialized with the argument map provided with the current argument variable values
	Menvironment* _functionExecutionEnvironment=__environment(); // free asap
	if(_functionExecutionEnvironment){
		_functionExecutionEnvironment->_name=_strdup(functionName); // store the name of the function as environment name!!!
		/* NO, instead, just before popping the function body execution environment, we copy the function map reference
		// MDH@20JUL2019: this is fun, we're referencing the internal functions defined in the user function, and as we never free the functions
		//                we do not need to distinguish between the originals and the references (so we never loose the referenced functions
		//                when an execution environment is freed)
		_functionExecutionEnvironment->_functionMap=_function->functionunion._userfunction->_functionMap;
		*/
		// 2. make the definition environment the parent of the function execution environment
		_functionExecutionEnvironment->_parent=_function->_definitionEnvironment;
		// 3. create the argument map fields as variables in the function execution environment
		bool functionExecutionEnvironmentInitialized=isExecutionEnvironmentInitialized(_functionExecutionEnvironment,_argumentMap);
		if(functionExecutionEnvironmentInitialized){
		// add the result variable ($ or perhaps later a variable with empty name????) TODO make a predefined constant char* out of it
			if(!addVariable(_functionExecutionEnvironment,"$",VT_UNDEFINED,false)){
				outputError("Failed to add the result variable to the function execution environment");
				functionExecutionEnvironmentInitialized=false;
			}else // also add the function exit flag variable (with name ! which cannot be set in the code because it is an invalid name)
			if(!addVariable(_functionExecutionEnvironment,"!",VT_UNDEFINED,false)){
				outputError("Failed to add the exit flag variable to the function execution environment");
				functionExecutionEnvironmentInitialized=false;
			}
		}
		if(!functionExecutionEnvironmentInitialized){free_environment(_functionExecutionEnvironment);_functionExecutionEnvironment=NULL;}
	}
	return _functionExecutionEnvironment;
}

typedef struct FunctionBodyInput{
	////////char* functionName;
	Muserfunction* _function;
	struct FunctionBodyInput* _prev;
	struct FunctionBodyRequest* _request;
}FunctionBodyInput;
FunctionBodyInput *_functionBodyInputStack=NULL,*_currentFunctionBodyInput=NULL; // the stack of function bodies being constructed
bool createFunctionBodyInput(const FunctionBodyRequest* const _firstFunctionBodyRequest){
	// ASSERT don't call with _firstFunctionBodyRequest equal to NULL
	///////////if(!_firstFunctionBodyRequest)return false;
	_currentFunctionBodyInput=CALLOC(1,sizeof(FunctionBodyInput),'I'); // free if not bound
	if(!_currentFunctionBodyInput){outputError("Failed to create function body input");return false;} // TODO improve feedback
	Mfunction* function=getFunction(getEnvironment(),_firstFunctionBodyRequest->functionName);
	if(function&&function->type==FT_USER){
		// it's better to put the next request in, so after finishing with this request we can do the following if any
		_currentFunctionBodyInput->_request=_firstFunctionBodyRequest->_next; // remember the request that initiated this body input
		_currentFunctionBodyInput->_function=function->functionunion._userfunction;
		if(!_functionBodyInputStack)_functionBodyInputStack=_currentFunctionBodyInput;
		// if we succeed in activating the execution environment of the new function we're good to go
		// we can use the functions parameterMap as argumentMap (providing the defaults to use for executing the newly entered body commands)
		Menvironment* _functionExecutionEnvironment=_getFunctionExecutionEnvironment(function,_firstFunctionBodyRequest->functionName,function->_parameterMap);
		if(_functionExecutionEnvironment){
			if(pushExecutionEnvironment(_functionExecutionEnvironment))return true;
			free_environment(_functionExecutionEnvironment);
		}
		outputError("Failed to create function execution environment for accepting its body commands"); // TODO improve feedback
	}else
		output("%sCan't find function '%s' for accepting its body commands.\n",ERROR_PREFIX,_firstFunctionBodyRequest->functionName);
	free(_currentFunctionBodyInput);
	return false;
}
bool startFunctionBodyInput(){
	// ASSERT only call with _firstFunctionBodyRequest not NULL
	// move out of the queue into the stack
	// push on top of the functionBodyInputStack
	/////////if(!_firstFunctionBodyRequest)return true; // NO function body request to 'execute'
	FunctionBodyRequest* nextFunctionBodyRequest=_firstFunctionBodyRequest->_next; // remember the function body request to do next
	char* functionName=_firstFunctionBodyRequest->functionName;
	bool functionBodyInputCreated=createFunctionBodyInput(_firstFunctionBodyRequest);
	free(_firstFunctionBodyRequest);_firstFunctionBodyRequest=NULL; // always free the function body request
	if(functionBodyInputCreated)return true; // succeeded, so done
	// failed, so do the next one
	_firstFunctionBodyRequest=nextFunctionBodyRequest; // simply skip this request!!
	output("%sFailed to honour the request to input the body of function '%s'.\n",ERROR_PREFIX,functionName);
	// if we still have a first function body request start that one, otherwise 
	return(_firstFunctionBodyRequest?startFunctionBodyInput():true);
}
/*
 \brief will only fail when we fail to start the next one
 */
bool endFunctionBodyInput(){
	// ASSERT do NOT call with _currentFunctionBodyInput equal to NULL
	// pop the function body request execution environment we just ended
	// MDH@20JUL2019: I need to get a reference to the execution environments function map (before the execution environment get's freed and we loose the reference!!)
	_currentFunctionBodyInput->_function->_functionMap=getEnvironment()->_functionMap;
	popExecutionEnvironment();
	// the new first function body request is the successor of the previous one
	// TODO shouldn't we free it?
	_firstFunctionBodyRequest=_currentFunctionBodyInput->_request; // the next function body request as stored in the _request field
	free(_currentFunctionBodyInput);_currentFunctionBodyInput=NULL; // I suppose I should get rid of the current function body input in case we're done anyway
	if(!_firstFunctionBodyRequest){_lastFunctionBodyRequest=NULL;return true;} // done with all the requests
	return startFunctionBodyInput(); // will NULL _firstFunctionBodyInput to ascertain not to get called in the main user input loop
}
// MDH@19JUL2019 END

// FEED FORWARD STUFF
void inputInfo(const char* const fmt,...); // prototype
void inputError(const char* const fmt,...); // prototype

// manual feed forward characters stuff
// what the user consumed manually, and is supposed to remain continguous i.e. uninterrupted by other feed forward texts
// it's possible that manual feed forward text is empty so it will block the identifier continuation text when that is the case!!
Mstring* _manualFeedforwardText=NULL;
size_t numberOfIdentifierContinuationManualFeedforwardCharacters=0; // MDH@06OCT2019: determine the number of manual feed forward characterr matching the identifier continuation
bool manualFeedforwardCharacterPrepended(char c){
	if(!c)return false;
	if(!_manualFeedforwardText)_manualFeedforwardText=__string();
	return(_manualFeedforwardText&&string_insert_char(_manualFeedforwardText,0,c));
}
bool prependedToManualFeedforwardText(char const * const characters){
	if(!characters)return false;
	if(!_manualFeedforwardText)_manualFeedforwardText=__string();
	return string_prepend(_manualFeedforwardText,characters);
}
char getFirstManualFeedforwardCharacterRemoved(){
	return(_manualFeedforwardText?string_removed_char(_manualFeedforwardText,0):'\0');
}

// identifier continuation stuff
// MDH@28OCT2019: because now often we need both the first and last token in a command it's probably best to combine them in a single command
typedef struct{
	Mtoken *_firstToken,*_lastToken;
	/////////////////bool identifierContinuationIsDirty; // convenient to keep it with the command itself
}Mcommand;
void free_command(Mcommand* _command){
	if(!_command)return;
	if(_command->_firstToken)free_token(_command->_firstToken); // will free ALL connected tokens!!!
	free(_command);
}
Mtoken* _getNewCommandToken(Mtoken* lastCommandToken,TokenType tokenType/*,bool endOfInput*/); // prototype
Mcommand* _getNewCommand(bool withFirstToken){
	Mcommand* _command=CALLOC(1,sizeof(Mcommand),'C');
	if(_command){
		if(amDebugging())inputInfo("New command created.");
		if(withFirstToken){
			_command->_firstToken=_getNewCommandToken(NULL,TT_EXPRESSION/*,endInput*/);
			if(_command->_firstToken){ // we've got a first token allocated
				if(amDebugging())inputInfo("New command token created.");
				_command->_lastToken=_command->_firstToken;
				_command->_firstToken->expr=NULL;
			}else{ // too bad, out of memory!
				FREE(_command,'C');_command=NULL;
				if(amDebugging())inputError("Failed to create the first command token.");
			}
		}
	}else
	if(amDebugging())inputError("Failed to create the command.");
	return _command;
}
// MDH@28OCT2019: not needed here anymore... Mtoken* _userInputCommand->_lastToken=NULL; // the last token in the sequence of tokens starting with _userInputCommand->_firstToken
// MDH@02OCT2019: might need this in multiple places!!
// MDH@04NOV2019: added TT_REFERENCE tokens as well
bool inIdentifierToken(Mtoken* lastCommandToken){
	return(lastCommandToken?lastCommandToken->type==TT_VARIABLE||lastCommandToken->type==TT_FUNCTION||lastCommandToken->type==TT_NEW_VARIABLE||lastCommandToken->type==TT_REFERENCE:false);
}

Mcommand* _userInputCommand=NULL; // the current input command
/* MDH@28OCT2019 replacing: 
Mtoken* _userInputCommand->_lastToken=NULL; // the last token in the command input by the user
*/
bool userInputCommandIdentifierContinuationNeedsUpdating=false; // whether or not the identifier has changed
char* _identifierContinuationCharacters=NULL; // the single text that we can continue the current identifier token with
char getFirstIdentifierContinuationCharacter(){
	inputInfo("%s","Determining the first identifier continuation character!");
	return(_identifierContinuationCharacters?_identifierContinuationCharacters[0]:'\0');
}
bool deleteFirstIdentifierContinuationCharacter(){
	if(!_identifierContinuationCharacters)return false;
	// not undefined or empty...
	char* newIdentifierContinuationText=(strlen(_identifierContinuationCharacters)>1?strdup(_identifierContinuationCharacters+1):NULL); // get dynamic copy of remainder, ignore if we fail!!!
	free(_identifierContinuationCharacters);
	_identifierContinuationCharacters=newIdentifierContinuationText; // NOTE if we failed to copy the remainder, we get NULL now
	return true;
}
char* _identifierContinuationOptionalCharacters=NULL; // the set of characters expected next of existing identifiers
// MDH@26SEP2019: we create a separate method that will determine the last token identifier continuation text and continuation characters
void deleteIdentifierContinuation(){
	if(_identifierContinuationOptionalCharacters){free(_identifierContinuationOptionalCharacters);_identifierContinuationOptionalCharacters=NULL;}
	if(_identifierContinuationCharacters){free(_identifierContinuationCharacters);_identifierContinuationCharacters=NULL;}
}
// user input command specific

// MDH@20SEP2019: we used to keep track of the behind cursor text, in a single Mstring instance, but because we also want to be able to add variable completion we keep a sequence of char* 
//                each feed forward char* is associated with a single token, and if that token is removed so should the associated feed forward
// call deleteTokenautocompletiontexts whenever the behind cursor text changes
typedef struct Mtokenautocompletiontext{
	Mtoken* token;
	char* _text;
	struct Mtokenautocompletiontext* _next;
	///////bool inactive; // keep track of whether or not active... (a feed forward text can become inactive when the associated token itself is still around but the text was moved to the command with a left or right arrow key)
}Mtokenautocompletiontext;
// MDH@30SEP2019: all token feed forward texts accepted (i.e. consumed) are pointed to by _lastConsumedAutocompletiontext
//                consumption of feed forward texts is done by right arrow (one character at a time) or tab (all feed forward characters)
//                right arrow reads the first feed forward character, gets it accepted and then moves the feed forward character to the consumed feed forward texts
Mtokenautocompletiontext *_firstTokenautocompletiontext=NULL,*_lastConsumedAutocompletiontext=NULL;
// MDH@26SEP2019: it's prudent to store the identifier continuation text separated from the rest of the (autogenerated) feed forward text
Mstring* _getAutoCompletionText(char sep){
	// constructs the total feed forward text
	Mstring* _autocompletionText=__string();
	if(_autocompletionText){
		Mtokenautocompletiontext* tokenautocompletiontext=_firstTokenautocompletiontext;
		while(tokenautocompletiontext){
			if(tokenautocompletiontext->_text&&!string_append(_autocompletionText,tokenautocompletiontext->_text))break;
			///////if(autocompletiontext->token)if(!string_append_char(_suggestedText,'#'))break;
			if(sep&&!string_append_char(_autocompletionText,sep))break;
			tokenautocompletiontext=tokenautocompletiontext->_next;
		}
		/* MDH@04OCT2019: moved over to updateAutoCompletionText()
		// MDH@03OCT2019: remove the identifier continuation text from the start of the feed forward text (as we should)
		if(!sep){
			if(_identifierContinuationCharacters){
				size_t numberOfIdentifierContinuationCharacters=strlen(_identifierContinuationCharacters);
				if(numberOfIdentifierContinuationCharacters>0){
					size_t numberOfFeedforwardCharacters=string_length(_suggestedText);
					if(numberOfIdentifierContinuationCharacters<numberOfFeedforwardCharacters){ // there might be feed forward characters left
						char c=string_replacedchar(_suggestedText,'\0',numberOfIdentifierContinuationCharacters); // temporarily pretend the feed forward text to have the same length as the identifier continuation text
						bool matching=(strcmp(string(_suggestedText),_identifierContinuationCharacters)==0); // get the comparison result
						string_setchar(_suggestedText,c,numberOfIdentifierContinuationCharacters); // // put the removed character back BEFORE removing the identifier continuation when matching
						if(matching) // the same!!!
							if(string_removed(_suggestedText,0,numberOfIdentifierContinuationCharacters)!=numberOfIdentifierContinuationCharacters) // remove the identifier continuation from the feed forward text
								inputError("Failed to remove the identifier continuation (completely) from the start of the rest of the suggested text");
					}else
					if(numberOfIdentifierContinuationCharacters==numberOfFeedforwardCharacters) // 'remove' all feed forward characters
						string_setlength(_suggestedText,0);
				}
			}
		}
		*/
	}
	return _autocompletionText;
}

size_t numberOfBehindPromptCharactersWritten=0; // MDH@25SEP2019: the total number of text characters written behind the prompt

// the globally constructed behind cursor text (without separator!!!)
Mstring* _autoCompletionText=NULL; // MDH@27FEB2019: we keep track of the auto completion text
void deleteAutocompletionText(){
	if(_autoCompletionText){
		if(amDebugging())inputInfo("Deleting autocompletion text.");
		free_string(_autoCompletionText);
		_autoCompletionText=NULL;
	}else
	if(amDebugging())inputInfo("No auto completion text to delete.");
}
void updateAutoCompletionText(){
	if(_autoCompletionText)free_string(_autoCompletionText); // free what we might currently have
	_autoCompletionText=_getAutoCompletionText('\0'); // get the new characters
	// merge with identifier continuation text
	if(_autoCompletionText&&_identifierContinuationCharacters){
		size_t numberOfIdentifierContinuationCharacters=strlen(_identifierContinuationCharacters);
		if(numberOfIdentifierContinuationCharacters>0){
			size_t numberOfFeedforwardCharacters=string_length(_autoCompletionText);
			if(numberOfIdentifierContinuationCharacters<numberOfFeedforwardCharacters){ // there might be feed forward characters left
				char c=string_replacedchar(_autoCompletionText,'\0',numberOfIdentifierContinuationCharacters); // temporarily pretend the feed forward text to have the same length as the identifier continuation text
				bool matching=(strcmp(string(_autoCompletionText),_identifierContinuationCharacters)==0); // get the comparison result
				string_setchar(_autoCompletionText,c,numberOfIdentifierContinuationCharacters); // // put the removed character back BEFORE removing the identifier continuation when matching
				if(matching) // the same!!!
					if(string_removed(_autoCompletionText,0,numberOfIdentifierContinuationCharacters)!=numberOfIdentifierContinuationCharacters) // remove the identifier continuation from the feed forward text
						inputError("Failed to remove the identifier continuation (completely) from the start of the rest of the suggested text");
			}else
			if(numberOfIdentifierContinuationCharacters==numberOfFeedforwardCharacters) // 'remove' all feed forward characters
				string_setlength(_autoCompletionText,0);
		}
	}
}

char getTokenTypeFeedforwardCharacter(TokenType tokenType){
	// some token types have an associated feed forward character!!!
	switch(tokenType){
		case TT_BINARY_aErU:return '=';
		case TT_DQSTRING:return '"'; 
		case TT_FUNCTION:return '(';
		case TT_FUNCTION_CALL:return ')';
		case TT_LIST:return ']';
		case TT_MAP:return '}';
		case TT_NEW_VARIABLE:return '=';
		case TT_SQSTRING:return '\'';
		default:break;
	}
	return '\0';
}

// MDH@24SEP2019: we do not always want to set the identifier continuation characters
// MDH@03OCT2019: now excluding the last token immediate feed forward text because that is being taken care of whenever the last token (type) change
//                because of this only the matching parentheses feed forward characters remain so TODO simplify this
char* _getLastTokenAutoCompletionText(){
	char* tokenAutoCompletionText=""; // on the stack
	if(amMatchingparentheses())
	if(_userInputCommand&&_userInputCommand->_lastToken)
	switch(_userInputCommand->_lastToken->type){
		case TT_ASSIGNMENT:break;
		case TT_BINARY_AeRu:case TT_BINARY_Aeru:case TT_BINARY_aERu:break;
		// MDH@03OCT2019: case TT_BINARY_aErU:tokenFeedforwardText="=";break;
		case TT_BINARY_aeru:break;
		case TT_COMMENT:break;
		// MDH@03OCT2019: case TT_DQSTRING:tokenFeedforwardText="\"";break; // TODO using " for the double quoted string might change in the future and we'd be in trouble then
		case TT_END_OF_DQSTRING:case TT_END_OF_FUNCTION_CALL:case TT_END_OF_LIST:case TT_END_OF_MAP:case TT_END_OF_SQSTRING:break;
		case TT_ERROR:break;
		case TT_EXPRESSION:if(string_last_char(_userInputCommand->_lastToken->text)=='(')tokenAutoCompletionText=")";break; // TODO use other type e.g. TT_FUNCTION_CALL instead of TT_EXPRESSION on (
		// MDH@03OCT2019: case TT_FUNCTION:tokenFeedforwardText="(";break;
		case TT_FUNCTION_CALL:tokenAutoCompletionText=")";break;
		case TT_INTEGER:break;
		case TT_LIST:tokenAutoCompletionText="]";break;
		case TT_LISTELEMENT:break;
		case TT_MAP:tokenAutoCompletionText="}";break;
		case TT_MAP_VALUE:break;
		// MDH@27SEP2019: if there's an identifier continuation behind the name of a new variable, the user can't reach it, so appending = is not going to be a good idea
		//                however, this would require the identifier continuation text to be updated BEFORE calling this method
		// MDH@01OCT2019: it's probably better to leave the assignment operator character showing to indicate that the current identifier is new, the user can always delete the identifier continuation using Delete key
		// MDH@03OCT2019: case TT_NEW_VARIABLE:/*if(!_identifierContinuationCharacters)*/tokenFeedforwardText="=";break;
		case TT_REAL:break;
		// MDH@03OCT2019: case TT_SQSTRING:tokenFeedforwardText="'";break; // TODO using ' for the double quoted string might change in the future and we'd be in trouble then
		case TT_TERNARY_aeru:break;
		case TT_UNARY:break;
		case TT_VARIABLE:case TT_REFERENCE:
		default:break;
	}
	return strdup(tokenAutoCompletionText); // TODO I suppose we might decide to no longer create a copy on the heap here (due to separating identifier continuation text from other feed forward text)
}

void free_tokenautocompletiontext(Mtokenautocompletiontext* _autocompletiontext){
	if(!_autocompletiontext)return;
	if(_autocompletiontext->_next)free_tokenautocompletiontext(_autocompletiontext->_next);
	if(_autocompletiontext->_text)free(_autocompletiontext->_text);
	FREE(_autocompletiontext,'F');
}

// MDH@04OCT2019: if we remember the immediate feed forward token we can determine whether or not we need to remove the associated feed forward text
Mtoken* immediateFeedforwardToken=NULL;

void deleteTokenautocompletiontexts(){
	deleteAutocompletionText();
	////////if(amDebugging())inputInfo("Autocompletion text deleted.");
	/////////////////numberOfBehindPromptCharactersWritten=getCommandLength(); // MDH@25SEP2019: TODO if you know a better place to do this then here let me know
	free_tokenautocompletiontext(_firstTokenautocompletiontext);
	_firstTokenautocompletiontext=NULL; // OOPS pretty essential!!!!
	immediateFeedforwardToken=NULL; // MDH@04OCT2019: also pretty essential as we won't have a feed forward text with this token anymore
	if(amDebugging())inputInfo("Token autocompletion texts deleted.");
}
/*
void deleteTokenautocompletionCharacters(size_t numberOfTokenAutocompletionCharacters){

}
*/
// every time feed forward text is to be added, it is prepended to the list of feed forward texts setting the token pointer to _userInputCommand->_lastToken
Mtokenautocompletiontext* getTokenAutocompletionText(Mtoken* token){
	Mtokenautocompletiontext* tokenautocompletiontext=_firstTokenautocompletiontext;
	while(tokenautocompletiontext&&tokenautocompletiontext->token!=token)tokenautocompletiontext=tokenautocompletiontext->_next;
	return tokenautocompletiontext;
}
// _text is dynamically allocated and should be freed if not bound to some!!
// MDH@24SEP2019: whenever the user uses the left arrow to move characters out of the token into the feed forward text
//                it should be remembered that these characters were associated with this token
//                so that when the feed forward text for a given token is set
//                the removed token characters and the actual feed forward characters of the token can be combined
void setLastTokenAutocompletionText(char* _text){
	// do not prepend empty feed forward texts!!!
	if(!_text)return;
	// MDH@01OCT2019: BUG FIX it would be wrong to NOT update the last token feed forward text when no text is specified as we were doing
	// MDH@02OCT2019: BUG FIX if _text is empty and there is no token yet do NOT add the token
	/////////// removing: if(strlen(_text)>0){ // there's text to store
	// if we already have an autogenerated feed forward text associated with the last command token
	Mtokenautocompletiontext* lastTokenAutocompletionText=getTokenAutocompletionText(_userInputCommand->_lastToken);
	if(lastTokenAutocompletionText){ // yes, so replace the text contents
		if(lastTokenAutocompletionText->_text)free(lastTokenAutocompletionText->_text);
		deleteAutocompletionText();
		lastTokenAutocompletionText->_text=_text; // _text now bound!!
	}else{ // not present yet, so add (i.e. prepend!!)
		if(strlen(_text)>0){
			if(amDebugging())inputInfo("Prepending auto completion text '%s' of token '%s' with offset %zu.",_text,string(_userInputCommand->_lastToken->text),_userInputCommand->_lastToken->offset);
			Mtokenautocompletiontext* _tokenautocompletiontext=CALLOC(1,sizeof(Mtokenautocompletiontext),'F');
			if(_tokenautocompletiontext){
				deleteAutocompletionText(); // I guess this is a bit confusing
				_tokenautocompletiontext->_text=_text; // and bound
				_tokenautocompletiontext->token=_userInputCommand->_lastToken;
				// how about skipping all anonymous feed forward texts????
				// if the first one is anonymous mark that one as last anonymous feed forward text
				Mtokenautocompletiontext *lastAnonymousTokenautocompletiontext=(_firstTokenautocompletiontext&&!_firstTokenautocompletiontext->token?_firstTokenautocompletiontext:NULL);
				// if the successor is also anonymous increment
				while(lastAnonymousTokenautocompletiontext&&lastAnonymousTokenautocompletiontext->_next&&!lastAnonymousTokenautocompletiontext->_next->token)
					lastAnonymousTokenautocompletiontext=lastAnonymousTokenautocompletiontext->_next;
				if(lastAnonymousTokenautocompletiontext){
					_tokenautocompletiontext->_next=lastAnonymousTokenautocompletiontext->_next;
					lastAnonymousTokenautocompletiontext->_next=_tokenautocompletiontext;
				}else{
					_tokenautocompletiontext->_next=_firstTokenautocompletiontext;
					_firstTokenautocompletiontext=_tokenautocompletiontext;
				}
				if(amDebugging())inputInfo("Feed forward text '%s' of token '%s' prepended!",_text,string(_userInputCommand->_lastToken->text));
			}else
				inputInfo("Failed to prepend feed forward text '%s' of token '%s'.",_text,string(_userInputCommand->_lastToken->text));
			return;
		}
		free(_text); // not bound, so to be freed!!
	}
}

// MDH@01OCT2019: in general when a token is removed its associated (autogenerated) suggested (identifier continuation and feed forward text) should be removed as well
//                NOTE deleteAutocompletionTextOfToken delegates to deleteAutocompletionTextOfToken
bool deleteAutocompletionTextOfToken(Mtoken* token){
	// locate the (autogenerated) feed forward text associated with this token (most likely the first one)
	Mtokenautocompletiontext *prevtokenautocompletiontext=NULL,*tokenautocompletiontext=_firstTokenautocompletiontext;
	while(tokenautocompletiontext&&tokenautocompletiontext->token!=token){prevtokenautocompletiontext=tokenautocompletiontext;tokenautocompletiontext=prevtokenautocompletiontext->_next;}
	if(tokenautocompletiontext){
		if(prevtokenautocompletiontext)prevtokenautocompletiontext->_next=tokenautocompletiontext->_next;else _firstTokenautocompletiontext=tokenautocompletiontext->_next;
		deleteAutocompletionText(); // the feed forward text changed so needs to be reconstructed whenever it is to be shown
		tokenautocompletiontext->_next=NULL;free_tokenautocompletiontext(tokenautocompletiontext); // only free the token feed forward text we are to remove!!
	} // MDH@04OCT2019: it's NOT there, so we may consider it deleted
	// relink the rest of the feed forward text chain to skip this feed forward text
	// if we have a previous feed forward text make if point to the successor of the feed forward text we are now removing, otherwise we get a new first feed forward text
	return true;
}

char getImmediateFeedforwardCharacterOfUserInputCommand(){
	// returns the character that might directly follow the current token (matching parentheses feed forward characters excluded)
	Mtoken* token=(_userInputCommand?_userInputCommand->_lastToken:NULL);
	if(token)
	switch(token->type){
		case TT_NEW_VARIABLE:case TT_BINARY_aErU:return '=';
		case TT_DQSTRING:return '"';
		case TT_FUNCTION:return '(';
		case TT_SQSTRING:return '\'';
		default:break;
	}
	return '\0';
}
/*
void removeFirstFeedforwardCharacterFromLastTokenWhenMatching(char inputChar){
	// find the last token's feed forward text (if any)
	Mtokenautocompletiontext* _autocompletiontext=_firstTokenautocompletiontext;while(_autocompletiontext&&_autocompletiontext->token!=_userInputCommand->_lastToken)_autocompletiontext=_autocompletiontext->_next;
	if(!_autocompletiontext)return;
	char* p=_autocompletiontext->_text;
	if(!p)return; // if the pointer to the text is undefined, we don't have it
	// NOTE no need to test the length because inputChar typically will not equal '\0'
	if(*p!=inputChar)return; // if the character pointed to by the pointer does not match the input character nothing to do
	// we'll be cutting off the first character!!
	deleteTokenautocompletiontexts();
	char* _leftover=strdup(p+1); // make a dynamic copy starting at the second character!
	free(p);
	_autocompletiontext->_text=_leftover;
}
*/
// MDH@30SEP2019: here the problem is that if we remove the first feed forward character we loose the token reference that generated it
//                so we might split up this functionality in two parts: 1. get the first feed forward character 2. remove it either by consuming it or delete if it disappears completely
//                alternatively we can simply remove the character but not remove the feed forward text when it becomes empty, that way we can still consume the feed forward text
//                however, suppose accepting this character as command characters fails, it would make sense to NOT actually remove the feed forward character until after accepting it?????????
//                I suppose the initial solution should be to extract the character and once used successfully remove it
//                so, replacing getFirstAutocompletionCharacterRemoved() by getFirstAutocompletionCharacter() and a function to actually delete that first feed forward character (and either consume or delete it)
char getFirstAutocompletionCharacter(bool autogenerated){
	// if autogenerated is set any first character should be returned, if not, the first feed forward character should be in a feed forward text associated with a token (i.e. autogenerated)
	// most of time _firstTokenautocompletiontext will contain that first feed forward character
	Mtokenautocompletiontext *tokenautocompletiontext=_firstTokenautocompletiontext;
	while(tokenautocompletiontext){
		if(tokenautocompletiontext->_text&&tokenautocompletiontext->_text[0]!='\0')return(!autogenerated||tokenautocompletiontext->token?tokenautocompletiontext->_text[0]:'\0');
		tokenautocompletiontext=tokenautocompletiontext->_next; // get the next to check
	}
	return '\0';
}
bool deleteFirstAutocompletionCharacter(char firstAutocompletionCharacter,bool consumed){
	if(firstAutocompletionCharacter!='\0'){
		// find it
		Mtokenautocompletiontext *prevtokenautocompletiontext=NULL,*tokenautocompletiontext=_firstTokenautocompletiontext;
		while(tokenautocompletiontext){
			if(tokenautocompletiontext->_text&&tokenautocompletiontext->_text[0]==firstAutocompletionCharacter)break;
			prevtokenautocompletiontext=tokenautocompletiontext; // remember the current feed forward text as the previous feed forward text
			tokenautocompletiontext=prevtokenautocompletiontext->_next; // get the next to check
		}
		if(tokenautocompletiontext){ // got it
			deleteAutocompletionText(); // ESSENTIAL otherwise it wouldn't update the feed forward text when it needs to (when writing the behind cursor text!!)
			// NOTE currently the length of the feed forward text will ALWAYS be 1 but if that would not always be the case we would need to consider addressing that happening as well
			if(strlen(tokenautocompletiontext->_text)==1){
				if(consumed){ // i.e. we should remember the token text, so we can unconsume it
					Mtokenautocompletiontext* nextFirstTokenautocompletiontext=NULL;
					while(_firstTokenautocompletiontext){
						nextFirstTokenautocompletiontext=_firstTokenautocompletiontext->_next; // remember what the next first token feed forward text will become
						// make the current first token feed forward text point to what the current last consumed token feed forward text is
						_firstTokenautocompletiontext->_next=_lastConsumedAutocompletiontext;
						// set the last consumed token feed forward text to the current first token feed forward text
						_lastConsumedAutocompletiontext=_firstTokenautocompletiontext;
						// update the first token feed forward text to what it was originally pointing to
						_firstTokenautocompletiontext=nextFirstTokenautocompletiontext;
						// if the token feed forward text with the first feed forward character was consumed, we're done
						if(_lastConsumedAutocompletiontext==tokenautocompletiontext)break;
					}
				}else{ // a true delete
					// replace the current text by what's behind the first character (if any)
					char* p=tokenautocompletiontext->_text;
					tokenautocompletiontext->_text=(strlen(p)>1?strdup(p+1):NULL);
					free(p); // free the currently used dynamic memory still pointed to by p
					if(!tokenautocompletiontext->_text){ // nothing left (or failing to copy the remainder over)
						Mtokenautocompletiontext* nexttokenautocompletiontext=tokenautocompletiontext->_next; // remember where to link the previous to
						tokenautocompletiontext->_next=NULL;free_tokenautocompletiontext(tokenautocompletiontext); // get rid of the feed forward text
						// link the predecessor to the successor
						if(prevtokenautocompletiontext)prevtokenautocompletiontext->_next=nexttokenautocompletiontext;else _firstTokenautocompletiontext=nexttokenautocompletiontext;
					}
				}
			}else{
				// TODO implement this when this might become the case in the future
			}
			return true;
		}
		inputError("First suggested character vanished.");
	}
	return false;
}
// MDH@30SEP2019: TODO still using the following for true feed forward deletes but should in due course be replaced by using the above two methods
char getFirstAutocompletionCharacterRemoved(){
	if(amVerbose())inputInfo("%s","Determining the first feed forward character!");
	char firstAutocompletionCharacterRemoved='\0';
	// find first feed forward text with text (so skipping all without text)
	Mtokenautocompletiontext *prevtokenautocompletiontext=NULL,*tokenautocompletiontext=_firstTokenautocompletiontext;
	while(tokenautocompletiontext&&(!tokenautocompletiontext->_text||tokenautocompletiontext->_text[0]=='\0')){prevtokenautocompletiontext=tokenautocompletiontext;tokenautocompletiontext=prevtokenautocompletiontext->_next;}
	if(tokenautocompletiontext){
		char* p=tokenautocompletiontext->_text;
		if(p){
			if(p[0]){
				firstAutocompletionCharacterRemoved=p[0];
				deleteAutocompletionText(); // ESSENTIAL otherwise it wouldn't update the feed forward text when it needs to (when writing the behind cursor text!!)
				if(amVerbose())inputInfo("First feed forward character '%c'.",firstAutocompletionCharacterRemoved);
				// replace the current text by what's behind the first character (if any)
				tokenautocompletiontext->_text=(strlen(p)>1?strdup(p+1):NULL);
				free(p); // free the currently used dynamic memory still pointed to by p
				if(!tokenautocompletiontext->_text){ // nothing left (or failing to copy the remainder over)
					Mtokenautocompletiontext* nexttokenautocompletiontext=tokenautocompletiontext->_next; // remember where to link the previous to
					tokenautocompletiontext->_next=NULL;free_tokenautocompletiontext(tokenautocompletiontext); // get rid of the feed forward text
					// link the predecessor to the successor
					if(prevtokenautocompletiontext)prevtokenautocompletiontext->_next=nexttokenautocompletiontext;else _firstTokenautocompletiontext=nexttokenautocompletiontext;
				}
			}else
				inputError("%s","No feed forward characters!");
		}else
			inputError("%s","No feed forward text!");
	}else
		inputError("%s","No feed forward found!");	
	return firstAutocompletionCharacterRemoved;
}

// a character can be 'anonymously' prepended to the feed forward text
// BUT if it matches the feed forward text of the current token and the current token does not have a feed forward text yet, it shouldn't be anonymous!!!
// MDH@24SEP2019: NO not anonymous because the character was removed from the current token and should be remembered as such
//                however this cause a problem if the originating token actually is the feed forward character of another token
// MDH@30SEP2019: now we should unconsume this character if it was last consumed
// MDH@04SEP2019: immediate feed forward characters should ALWAYS be prepended explicitly and so true is to be passed in for \p new
//                NOTE currently there are only two calls, one to prepend an immediate feed forward character (which should always be flagged as new), and left arrow prepending which is marked as false
Mtokenautocompletiontext*  getAutocompletionTextOfCharacterPrepended(char c,bool new){
	Mtokenautocompletiontext* result=NULL;
	if(c){
		// possible recently consumed...
		// TODO technically we should check the last character in last consumed feed forward text (instead of assuming every feed forward text consists of a single character!!)
		if(!new&&_lastConsumedAutocompletiontext&&_lastConsumedAutocompletiontext->_text&&_lastConsumedAutocompletiontext->_text[0]==c){
			Mtokenautocompletiontext* nextLastConsumedTokenautocompletiontext=_lastConsumedAutocompletiontext->_next; // remember what the last consumed token feed forward text is pointing to
			_lastConsumedAutocompletiontext->_next=_firstTokenautocompletiontext; // make the last consumed token feed forward text to the current first token feed forward text
			_firstTokenautocompletiontext=_lastConsumedAutocompletiontext; // replace the first token feed forward text by the last consumed token feed forward text
			_lastConsumedAutocompletiontext=nextLastConsumedTokenautocompletiontext; // replace the last consumed token feed forward text by what it was previously pointing to
			result=_lastConsumedAutocompletiontext;
		}else{
			// TODO for now always use an anonymous (nontoken) prepend
			Mtokenautocompletiontext* autocompletiontext=CALLOC(1,sizeof(Mtokenautocompletiontext),'F');
			if(autocompletiontext){
				Mstring* _string=__string(); // free asap
				if(_string){
					string_append_char(_string,c);
					/*
					if(autocompletiontext->_text){
						string_append(_string,autocompletiontext->_text);
						free(autocompletiontext->_text); // get rid of the memory with the original text
						autocompletiontext->_text=NULL; // just in case (of what?)
					}
					*/
					autocompletiontext->_text=strdup(string(_string));
					// free the memory occupied by the vessel
					free_string(_string);
					// if our feed forward text is not the current first fft make it so
					if(autocompletiontext!=_firstTokenautocompletiontext){autocompletiontext->_next=_firstTokenautocompletiontext;_firstTokenautocompletiontext=autocompletiontext;}
					result=autocompletiontext; // remember the 
					deleteAutocompletionText();
				}
				// if failed, ascertain to free what is not bound in the feedforward text list
				if(!result){autocompletiontext->_next=NULL;free_tokenautocompletiontext(autocompletiontext);autocompletiontext=NULL;}
			}
		}
	}
	return result;
}

// MDH@03OCT2019: it's essential to differentiate between current token dependent feed forward and other feed forward
//                deleteLastTokenImmediateFeedforwardText() is to be called when _userInputCommand->_lastToken stops being the current token or when the type of the current token changes
//                updateImmediateFeedforwardTextOfUserInputCommand() is to be called when _userInputCommand->_lastToken just became the current token (or when its type changes)
// MDH@04OCT2019: deciding to keep the immediate feed forward text separate from the other feed forward texts, that way it is easier to merge the identifier continuation and feed forward texts
Mstring* _immediateFeedforwardText=NULL;
bool immediateFeedforwardToBeUpdated=false;
bool updateImmediateFeedforwardTextOfUserInputCommand(){
	if(!_immediateFeedforwardText)return false; // should have one
	char lastTokenImmediateFeedforwardCharacter=getImmediateFeedforwardCharacterOfUserInputCommand();
	return(!lastTokenImmediateFeedforwardCharacter||!string_append_char(_immediateFeedforwardText,lastTokenImmediateFeedforwardCharacter));
}
/* replacing:
// \brief prepends the immediate feed forward character of the current token (if any), returns true on success, false otherwise
bool updateImmediateFeedforwardTextOfUserInputCommand(){
	// MDH@03OCT2019: getAutocompletionTextOfCharacterPrepended was adjusted to return true when the character passed to it equals '\0'!!
	//                however TODO currently the prepending is anonymous, whereas this prepending should NOT be done anonymous, otherwise we can't delete it later on
	// get rid of any current immediate feed forward text
	if(immediateFeedforwardToken&&!deleteAutocompletionTextOfToken(immediateFeedforwardToken))return false;
	immediateFeedforwardToken=NULL;
	char lastTokenImmediateFeedforwardCharacter=getImmediateFeedforwardCharacterOfUserInputCommand(_userInputCommand->_lastToken);
	if(!lastTokenImmediateFeedforwardCharacter)return true;
	// try to prepend the character
	Mtokenautocompletiontext* lastTokenImmediateFeedforwardtext=getAutocompletionTextOfCharacterPrepended(lastTokenImmediateFeedforwardCharacter,true); // second argument forces always prepending this character!!
	if(!lastTokenImmediateFeedforwardtext){inputError("Failed to prepend the immediate feed forward character!");return false;} // failed to prepend the immediate feed forward text anonymously
	immediateFeedforwardToken=_userInputCommand->_lastToken;
	lastTokenImmediateFeedforwardtext->token=immediateFeedforwardToken; // remember the immediate feed forward token!!!
	return true;
}
*/
// the following functions are user input command specific
Mtoken* setLastUserInputCommandToken(Mtoken* lastUserInputCommandToken){
	if(!_userInputCommand){inputError("BUG: No user input command");return NULL;}
	_userInputCommand->_lastToken=lastUserInputCommandToken;
	// MDH@30OCT2019: userInputCommandIdentifierContinuationNeedsUpdating=inIdentifierToken(lastUserInputCommandToken); // MDH@02OCT2019: as we're setting the type of the token AFTER creating it, we wait until after doing so to update identifierContinuationIsDirty!!	
	return _userInputCommand->_lastToken;
}
/*
Mtoken* setLastUserInputCommandToken(Mtoken* newLastCommandToken){
	// MDH@03OCT2019: every time the current command token changes (preferably done by calling this function), we should first remove the immediate feed forward of the current token, update the current token, and add the immediate feed forward text on the newly accepted current token
	//                however this should also happen when the type of the current token changes
	// MDH@04OCT2019: a little less efficient to move it to the input loop but more reliable!!!
	//// removing: if(!deleteLastTokenImmediateFeedforwardText())inputError("Failed to remove the last token immediate feed forward text.");
	if(_userInputCommand)_userInputCommand->_lastToken=newLastCommandToken;else inputError("BUG: No user input command");
	//// removing: if(!updateImmediateFeedforwardTextOfUserInputCommand())inputError("Failed to add the last token immediate feed forward text.");
	userInputCommandIdentifierContinuationNeedsUpdating=inIdentifierToken(_userInputCommand);
}
*/
/*
void deleteAutocompletionTextOfToken(Mtoken* token,bool deleteIdentifierContinuationText){
	if(!token)return;
	if(deleteIdentifierContinuationText)deleteIdentifierContinuation(); // this is the easiest way but makes sense
	deleteAutocompletionTextOfToken(token);
}
*/
Mstring* _suggestedText=NULL; // MDH@04SEP2019: where we'll be storing the entire feed forward text (i.e. identifier continuation, immediate feed forward and auto completion text)

// keeping track of the command count, the cursor position and the prompt length (so we can write information messages on the line above where the prompt is)
long long commandCount=0; // the total number of command input
long long commandIndex=0;

Mstring* shellCommand=NULL;
/// MDH@28OCT2019: replaced by _userInputCommand: Mtoken* _userInputCommand->_firstToken=NULL;

// keeping track of both the cursor position and the total command length
size_t getUserInputLength(){
	return(inputMode==IM_COMMAND?(_userInputCommand&&_userInputCommand->_lastToken?_userInputCommand->_lastToken->offset+string_length(_userInputCommand->_lastToken->text):0):(inputMode==IM_SHELL?string_length(shellCommand):0));
}

// MDH@30OCT2019: for each user input line, keep track of the total number of characters written by the user
typedef struct Muserinputline{
	size_t offset,index; // the number of characters on previous lines and the line index
	struct Muserinputline *_prev; // for accessing previous lines
}Muserinputline;
Muserinputline* _userinputline=NULL; // reference to the current user input line
// call _userinputline() when starting a new line
Muserinputline* __userinputline(){
	Muserinputline* _newUserinputline=CALLOC(1,sizeof(Muserinputline),'L');
	if(_newUserinputline){
		_newUserinputline->_prev=_userinputline;
		_newUserinputline->offset=getUserInputLength(); // number of characters in front of it
		_newUserinputline->index=(_userinputline?_userinputline->index:0)+1; // count the lines
		_userinputline=_newUserinputline;
	}
	return _newUserinputline;
}
// call free_userinputline() when starting a new user input command
size_t free_userinputline(){
	size_t numberOfUserInputLines=0;
	Muserinputline* prevUserinputline;
	while(_userinputline){
		numberOfUserInputLines++;
		prevUserinputline=_userinputline->_prev;
		FREE(_userinputline,'L');
		_userinputline=prevUserinputline;
	}
	return numberOfUserInputLines;
}
void removeUserinputline(){
	Muserinputline* prevUserinputline=_userinputline->_prev;
	FREE(_userinputline,'L');
	_userinputline=prevUserinputline;
}
// MDH@30OCT2019 END
size_t getNumberOfSuggestedCharacters(){return(_suggestedText?string_length(_suggestedText):0);}
size_t getCommandLength(){return getUserInputLength()+getNumberOfSuggestedCharacters();} // TODO not correct this way!!!!

uint8_t promptLength=0;
void showPrompt(){
	resetOutputColor();
	numberOfBehindPromptCharactersWritten=0; // MDH@27SEP2019: so far no characters were written behind the prompt
	///////////printf("%d-",commandIndex);
	char str[11]; // with a maximum of 2,xxx,xxx,xxx 11 positions would suffice
	promptLength=0;
	switch(inputMode){
		case IM_COMMAND:
			{
				Mstring* _environmentName=_getEnvironmentName(); // free asap
				if(_environmentName){
					output(string(_environmentName));
					promptLength=string_length(_environmentName);
					free_string(_environmentName);
				}
				/* replacing:
				output("M");
				promptLength=1;
				*/
				// MDH@19JUL2019: when dealing with a function body being entered, we show a different prompt
				if(_currentFunctionBodyInput)
					sprintf(str,"%lld",1+getNumberOfFunctionCommands(getEnvironment()->_name));	// replacing: printf("%lu",(commandCount+1));
				else
					sprintf(str,"%lld",(commandCount+1));	// replacing: printf("%lu",(commandCount+1));
				output("[%s] = ",str);
				promptLength+=strlen(str)+5;
				clearScreenFromCursor();
			}
			break;
		case IM_CONTROL:
			// how about showing the flags?????
			outputFlags();
			output(" > ");
			promptLength=5+3; // the flags and the control mode prompt
			break;
		case IM_SHELL:
			output("$ ");
			promptLength=2;
			break;
	}
	/////////storeCursor();
	///////////inputMode=true; // expecting a command (until the option character is received)
	/* MDH@26FEB2019: we do not need the following because that's taken care of in writeTokens(_userInputCommand->_firstToken) right after promptForUserInput()
	getUserInputLength()=0; // starting at position 0
	*/
}

// MDH@30OCT2019: we'd like to be able to continue a command on the next line
bool showContinuedPrompt(){
	// ASSERT only to be called in command mode with _userInputCommand not NULL
	if(!__userinputline())return false; // if we fail to create a new user input line (to keep track of the number of characters on previous user input lines)
	clearScreenFromCursor(); // to get rid of any suggested text behind the cursor
	outputChar('\n'); // move over to the next line
	uint8_t blanks=promptLength;while(blanks>3){outputChar(' ');blanks--;}
	resetOutputColor();
	output(" = ");
	return true;
	/// can't call this here!!!! outputTokenColor(_userInputCommand->_lastToken);
}
// MDH@30OCT2019 END
void promptForUserInput(){
	free_userinputline();if(_userinputline)outputError("BUG: Failed to release user input line info"); // MDH@30OCT2019: get rid of all previously stored user input line info
	enableRawmode();
	resetOutputColor();
	output("\n%s\n",promptinfo[inputMode]); // show the appropriate input mode prompt info
	showPrompt();
	//////if(inputMode==IM_COMMAND)
}
/* The following ANSI escape sequences are currently supported.
 * If n and/or m are omitted, they default to 1.
 *   ESC [nA moves up n lines
 *   ESC [nB moves down n lines
 *   ESC [nC moves right n spaces
 *   ESC [nD moves left n spaces
 *   ESC [m;nH" moves cursor to (m,n)
 *   ESC [J clears screen from cursor
 *   ESC [K clears line from cursor
 *   ESC [nL inserts n lines ar cursor
 *   ESC [nM deletes n lines at cursor
 *   ESC [nP deletes n chars at cursor
 *   ESC [n@ inserts n chars at cursor
 *   ESC [nm enables rendition n (0=normal, 4=bold, 5=blinking, 7=reverse)
 *   ESC M scrolls the screen backwards if the cursor is on the top line
 */
void outputText(char* fmt,char* text){resetOutputColor();output(fmt,text);}

// Token is now defined in Mexpression.h which is included by Mexecution.h so struct Token is indirectly supplied by Mexpression.h!!!

// the list of token type ids in the corresponding order!!!
const uint8_t TOKENTYPE_IDS[NUMBER_OF_TOKEN_TYPES]={0,0b01010000,0b01000000,0b01100000,0b01100101,0b01101010,0b01100110,0b01101000,0b01110000,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,0b1000000,0b11111111};

const char* getTokenColor(enum TOKENTYPE_ENUM tokenType){
	uint8_t tokentype_id=TOKENTYPE_IDS[tokenType];
	////////output("(%d)",tokentype_id);
	switch(tokentype_id>>6){
		case 0: // value token
			return getValueTokenColor(tokentype_id);
		case 1: // operator: unary, binary, ternary, assignment the operator category will be: (tokentype_id&0x30)>>4
			return getOperatorTokenColor((tokentype_id&0x30)>>4);
		case 2: // comment or end of comment
			return getCommentColor();
		case 3: // error token
			/////////outputChar('E');
			return getErrorColor();
	}
	return "";
}
void outputTokenTypeColor(TokenType tokenType){
	setBackColor(getBackgroundColor());
	setColor(getTokenColor(tokenType));
}
void outputTokenColor(Mtoken* _token){
	if(_token)outputTokenTypeColor(_token->type);
	///////printf("[%d]",_userInputCommand->_lastToken->type);
	// ah, the token colors will be a problem with the new type definitions, I suppose we need to distinguish between the operator and non-operator tokens	
}
size_t outputToken(Mtoken* _token){
	size_t numberOfCharactersToOutput=(_token&&_token->text?string_length(_token->text):0);
	if(numberOfCharactersToOutput>0){
		// MDH@31OCT2019: by introducing ` as new line request character (whitespace) we'll be having visible whitespace characters at the end of the token which we do not want to show in the same color
		// ascertain that the token text ends at the first whitespace character (if there is any whitespace) NOTE there's no need to put '\0' back, therefore we use '\0' if we didn't replace the character to start with
		char firstWhitespaceCharacter=(_token->significantCharacterCount>0?string_replacedchar(_token->text,'\0',_token->significantCharacterCount):'\0');
		// if we allow comments in tokens we're in trouble!!!
		outputTokenColor(_token);output("%s",string(_token->text)); // although string() will write the '\0' at the end we've already written one in front of that position
		// if there's whitespace text to start with write it in the default output color
		if(firstWhitespaceCharacter){ // some whitespace left to write
			string_setchar(_token->text,firstWhitespaceCharacter,_token->significantCharacterCount);
			resetOutputColor();
			output("%s",string_remainder(_token->text,_token->significantCharacterCount));
		}
	}
	return numberOfCharactersToOutput;
	/////////if(amAssisting()){resetOutputColor();outputChar('|');}
}
void outputLastTokenChar(Mtoken* _token){
	///////outputTokenColor(_userInputCommand->_lastToken);
	outputChar(string_last_char(_token->text));
	//////////resetOutputColor();
}
// MDH@30APR2019: when a function returns to a variable and the other way round
void reoutputToken(Mtoken* _token){
	if(!_token)return;
	size_t tokenLength=(_token->text?string_length(_token->text):0);
	if(tokenLength==0)return; // shouldn't happen though
	// MDH@31OCT2019: this is particularly hard if the token is written over several lines
	//                I solved this by making the newline character always end a token (by treating it as whitespace that effectively ends any current token)
	//                but now we have the situation that the given token might be at the previous line, this is the case when the token was ended with a newline and we're now at the start of the next line
	//                the situation is: we're at the end of the token and its type changed and we have to write it again and return to the current position
	//                we might have consumed an assignment operator behind it returning us to this token which might be variable that could also be a function (?????)
	//                the problem with writing the token is that it does not recognize the newline character at the end
	//                for safety reasons we look at _userinputline because if _userinputline is NULL there's no previous command input line
	bool tokenOnPreviousInputLine=(_userinputline?(string_last_char(_token->text)==M_NEWLINE_CHARACTER):false);
	if(tokenOnPreviousInputLine){oneLineUp();toStartOfLine();moveCursorRight(promptLength+(_userinputline->offset-(_userinputline->_prev?_userinputline->_prev->offset:0)));}
	moveCursorLeft(tokenLength);
	outputToken(_token); // back where we started (hopefully)
	if(tokenOnPreviousInputLine){oneLineDown();toStartOfLine();moveCursorRight(promptLength+getUserInputLength()-_userinputline->offset);}
}
/**
 * freeToken() frees the memory @_userInputCommand->_lastToken points to and returns true on successfully removing the entire chain of tokens it points to
 * @returns the previous token (as we need that )  
 */
Mtoken* freeToken(Mtoken* _token){
	// MDH@30APR2019: let's delegate to free_token()
	Mtoken* _prevToken=NULL;if(_token){_prevToken=_token->prev;free_token(_token);}return _prevToken;
}
// output functions that require access to the current token
void outputUserInputCommandTokenColor(){
	if(_userInputCommand&&_userInputCommand->_lastToken)outputTokenColor(_userInputCommand->_lastToken); // return to the current token color
}
void returnToUserInputCommandCursorPosition(){
	// ASSERT we're on the last user input line i.e. the input line the user is currently entering command characters
	toStartOfLine();
	moveCursorRight(promptLength+getUserInputLength()-(_userinputline?_userinputline->offset:0)); // the offset of the current user input line (if any) determines how many characters the user typed on this input line
	outputUserInputCommandTokenColor();
}

// MDH@16MAY2019: not showing the error on the line above the user input line, but now below (in info color)
// MDH@22MAY2019 NOTE: const Mvalue* const is protested against in the call to _getValueText
// MDH@30OCT2019: if we let toInfoInputLine() return the number of lines it moved back we can pass that into toUserInputCursorPosition() to go down that number of lines
size_t toInfoInputLine(){
	size_t linesUp=0,lines=(_userinputline?_userinputline->index:0)+1;
	while(linesUp<lines){oneLineUp();linesUp++;}clearLine();return linesUp;} // MDH@30OCT2019: only after moving all the input lines up do we need to go to the start, also clearLine() will ascertain to end up at the start of the line
void toUserInputCursorPosition(size_t linesDown){while(linesDown>0){oneLineDown();linesDown--;}returnToUserInputCommandCursorPosition();}
void inputInfo(const char* const fmt,...){
	if(fmt&&strlen(fmt)){ // we have a format
		size_t linesMovedUp=toInfoInputLine();
		resetOutputColor(); // get the default output color!!
		// NOTE we have to call vprintf here NOT printf!!!
		// MDH@22JUL2019: as we're not calling output() here, we can make output() read a character to allow interuption????
		va_list args;va_start(args,fmt);vprintf(fmt,args);va_end(args); // NOTE would be a mistake to call output() here, resulting
		toUserInputCursorPosition(linesMovedUp);
	}
}
void inputError(const char* const fmt,...){
	if(fmt&&strlen(fmt)){ // we have a format
		size_t linesMovedUp=toInfoInputLine();
		setColor(getErrorColor());setBackColor(getBackgroundColor());
		va_list args;va_start(args,fmt);vprintf(fmt,args);va_end(args); // NOTE would be a mistake to call output() here, resulting
		toUserInputCursorPosition(linesMovedUp);
	}
}
void clearInfo(){toUserInputCursorPosition(toInfoInputLine());} // MDH@30OCT2019: toInfoInputLine() automatically clears the info input line!!!
void inputInfoCommand(Mcommand* command){
	size_t linesMovedUp=toInfoInputLine();
	resetOutputColor();
	if(command){Mtoken* token=command->_firstToken;while(token){output("%s|",string(token->text));token=token->next;}}
	toUserInputCursorPosition(linesMovedUp);
}

void outputStatus(char inputChar,char inputCharType){
	////////printf("[%u,%u]",getUserInputLength(),getCommandLength());
	Mstring* _separatedBehindCursorText=_getAutoCompletionText('|');
	/////////debugWrite("Status: Cursor position=%u - command length=%u - behind cursor text='%s'.",getUserInputLength(),getCommandLength(),string(feedforwardText));
	inputInfo("Input character: %c(=0x%x) | Input character type: %c | Token type: %s | Cursor position: %zu | Command length: %zu | Manual feed forward: '%s' | Identifier continuation: '%s' | Feed forward: '%s'.",inputChar,inputChar,inputCharType,(_userInputCommand->_lastToken!=NULL?TOKENTYPE_STRING[_userInputCommand->_lastToken->type]:""),getUserInputLength(),getCommandLength(),(_manualFeedforwardText?string(_manualFeedforwardText):""),(_identifierContinuationCharacters?_identifierContinuationCharacters:""),string(_separatedBehindCursorText));
	free_string(_separatedBehindCursorText);
	//////outputInfo("Status: Cursor position=%u - command length=%u - behind cursor text='%s'.",getUserInputLength(),getCommandLength(),string(feedforwardText));
}
void outputDebugInfo(){
	////////printf("[%u,%u]",getUserInputLength(),getCommandLength());
	Mstring* _separatedBehindCursorText=_getAutoCompletionText('|');
	/////////debugWrite("Status: Cursor position=%u - command length=%u - behind cursor text='%s'.",getUserInputLength(),getCommandLength(),string(feedforwardText));
	inputInfo("Cursor position: %zu | Command length: %zu | Token type: % s | Manual feed forward: '%s' | Identifier continuation: '%s' | Auto completion: '%s'.",getUserInputLength(),getCommandLength(),(_userInputCommand&&_userInputCommand->_lastToken?TOKENTYPE_STRING[_userInputCommand->_lastToken->type]:""),(_manualFeedforwardText?string(_manualFeedforwardText):""),(_identifierContinuationCharacters?_identifierContinuationCharacters:""),string(_separatedBehindCursorText));
	free_string(_separatedBehindCursorText);
	//////outputInfo("Status: Cursor position=%u - command length=%u - behind cursor text='%s'.",getUserInputLength(),getCommandLength(),string(feedforwardText));
}

// MDH@23SEP2019: setting the type of the new token is moved outside because setLastTokenType() replaces setting the type of a token directly
//                this means that _getToken can use newTokenType but should NOT set ->type of the given token unless we decide to remove newTokenType from _getToken of cours in the future...
Mtoken* _getToken(Mtoken* prevToken,TokenType newTokenType){
	Mtoken* pNewToken=__token();
	if(pNewToken){
		/////if(amDebugging())inputInfo("E1");
		// MDH@03MAY2019: if the previous token starts an expression itself, use prevToken itself and not its expr field!!!!
		if(prevToken){
			// finish the previous token
			prevToken->next=pNewToken; // how could I forget about doing this (and checking whether prevToken is not NULL!)!!
			if(!prevToken->significantCharacterCount)prevToken->significantCharacterCount=string_length(prevToken->text); // MDH@22MAR2019: if the token character length is NOT set, set it now...
			// initialize the new token
			pNewToken->prev=prevToken; // set the predecessor
			/////if(amDebugging())inputInfo("E2");
			// MDH@27MAY2019: let's by default copy prevToken-expr over

			// MDH@18MAY2019: if a , starts an expression we won't be pointing to the opening parenthesis!!!
			//                which would mean that on verification we'd have to jump back until we found a non-comma!!!
			//                so we can fix this by NOT including TT_EXPRESSION prev tokens to point to!!!
			//                BUT the first (dummy) expression token should be included though!!!
			// TODO having to test an expression for starting with ( is a bit of a nuisance (so we won't accidently do that on the initial expression token and any comma token!!!)
			// MDH@27MAY2019: set expr NOTE the first token behind the (start of) expression token, should keep pointing to NULL
			// MDH@23JUL2019: we're going to change this a little bit because we want } ) ] to point to what the expr of prevToken points to
			//                and NOT wait for the next token
			//                typically a new token points to the same expr that the predecessor points to
			//                but we want 
			// take special care when the new token ends a list, map or function call
			// MDH@29OCT2019: no need for \p first anymore (that we used previously) because testing for the first TT_EXPRESSION can also be done by looking at the text in the expression
			//                TODO in time we should change the first token into a WHITESPACE token
			if(prevToken->type==TT_LIST||prevToken->type==TT_FUNCTION_CALL||prevToken->type==TT_MAP||(prevToken->type==TT_EXPRESSION&&string_length(prevToken->text)>0&&string_char(prevToken->text,0)!=' '))
				pNewToken->expr=prevToken;
			else
				pNewToken->expr=prevToken->expr; // DEFAULT: take over the expr of the previous token
			
			// MDH@09AUG2019: before we actually kill the expr in the end of function call we update the envid
			// if ending a special function call, we should zero the last set octet, but determining whether that is the case is not as easy as it seems
			// I suppose the argument of the expr field of the new token will tell us if it is a special function call (because the argument field would then be positive)
			if(newTokenType==TT_END_OF_FUNCTION_CALL&&pNewToken->expr&&pNewToken->expr->type==TT_FUNCTION_CALL&&pNewToken->expr->argument>0){
				//////////inputInfo("*** End of special function call! ***");
				// we have to decrement the octet that should be incremented
				// it would be nicer to make the octet we loose 0 in the process because in that case we do not need to do that when we nest again
				// the number of bits per level determines value to increment ander with and shift (at this moment the maximum depth is at most 15 i.e. 4 bits are always used to keep track of the current level)
				uint64_t ander=0,incrementoctet=0;while(incrementoctet!=(prevToken->envid&15)){ander=(ander<<M_BITS_PER_ENV_LEVEL)+((1<<M_BITS_PER_ENV_LEVEL)-1);incrementoctet++;}
				pNewToken->envid=(((prevToken->envid>>4)<<4)+incrementoctet-1)&((ander<<4)+15); // shifting ander by 4 additional bits and adding 15 to maintain the level value (increment octet)
			}else
				pNewToken->envid=prevToken->envid; // MDH@09AUG2019: take over the environment id!!

			// MDH@16OCT2019: if the previous token was an end of list/function call/map it was accepted and itself would be pointing to the start of the list/function call/map
			//                therefore we do not need to set 
			if(prevToken->type==TT_END_OF_LIST||prevToken->type==TT_END_OF_FUNCTION_CALL||prevToken->type==TT_END_OF_MAP){
				// MDH@23JUL2019: this new token is actually only allowed when there's a matching token, but if there isn't pNewToken->expr will most likely be NULL
				//                TODO this is checked afterwards, so perhaps we should do that here?????
				if(pNewToken->expr)pNewToken->expr=pNewToken->expr->expr;else newTokenType=TT_ERROR;
			}
			// we still have to recognize an error
			if(newTokenType==TT_END_OF_LIST||newTokenType==TT_END_OF_FUNCTION_CALL||newTokenType==TT_END_OF_MAP)if(!pNewToken->expr)newTokenType=TT_ERROR;

			/* replacing:
			if(newTokenType==TT_END_OF_LIST||newTokenType==TT_END_OF_FUNCTION_CALL||newTokenType==TT_END_OF_MAP){
				// MDH@23JUL2019: this new token is actually only allowed when there's a matching token, but if there isn't pNewToken->expr will most likely be NULL
				//                TODO this is checked afterwards, so perhaps we should do that here?????
				if(pNewToken->expr)pNewToken->expr=pNewToken->expr->expr;else newTokenType=TT_ERROR;
			}
			*/
			/*
			if(amVerbose()){
				if(pNewToken->expr)inputInfo("Matching: %s",string(pNewToken->expr->text));else inputInfo("%s","-");
			}
			*/
			///////if(amVerbose()){if(pNewToken->expr)inputInfo("Pointing to %s of type %s.",string(pNewToken->expr->text),TOKENTYPE_STRING[pNewToken->expr->type]);else inputInfo("Nothing to point to.");}
			//////// ending with NULL means all is Ok!! if(!pNewToken->expr)pNewToken->expr=_userInputCommand->_firstToken; // TODO will this help???
			pNewToken->offset=prevToken->offset+string_length(prevToken->text); // set the offset
			// MDH@07AUG2019: a token 'inherits' the prevIdentifier and argument of its previous token, to be adapted if necessary depending on what it is
			//                of course if prevToken is an identifier itself, the new token should point to that token and not to the identifier prevToken is pointing to
			//                how about function identifiers? they are special in that they change the argument value
			/////if(amDebugging())inputInfo("E3");
			if(prevToken->type==TT_FUNCTION){ // a function identifier that we can point to (although perhaps we should not do that?) TODO shouldn't we test whether the new token type is TT_FUNCTION_CALL instead??????
				pNewToken->prevIdentifier=prevToken;
				// what should now be the argument value? this depends on the name of the function
				char* _functionName=_stringstart(prevToken->text,prevToken->significantCharacterCount); // free asap
				// all new tokens have argument equal to zero (and counting down on each comma encountered, so all variables created are considered global, because only the tokens with argument equal to 1 should be considered local)
				// MDH@11AUG2019: the default now no longer should be zero, because 1 will be toggled to -1 and back, therefore we should not encounter -1s in an ordinary function call
				if(!strcmp(_functionName,DOFUNCTION_NAME)||!strcmp(_functionName,FORFUNCTION_NAME))pNewToken->argument=1;else if(!strcmp(_functionName,DEFINEUSERFUNCTION_NAME))pNewToken->argument=2;else pNewToken->argument=-2;
				// MDH@09AUG2019: special function calls have arguments that declare local variables explicitly, execution of these function calls will run in their own execution environment in which these local variables are created, 
				if(pNewToken->argument){ // a special function call
					uint64_t incrementoctet=(prevToken->envid&15),environmentid=prevToken->envid,addendum=16; // addendum: what we need to add to the envid to get a new unique environment id, ander: what we need to and the envid with to make the octet to the left 0 again (ready for having nested special function calls)
					// the maximum value of incrementoctet (the environment depth) is 60/M_BITS_PER_ENV_LEVEL
					if((incrementoctet*M_BITS_PER_ENV_LEVEL)<60&&(prevToken->envid)>>((incrementoctet+1)*M_BITS_PER_ENV_LEVEL)<(2<<M_BITS_PER_ENV_LEVEL)-1){ // checking the octet to increment as well because it should not be 15 (or we would get overflow!!)
						while(incrementoctet>0){addendum<<=M_BITS_PER_ENV_LEVEL;incrementoctet--;}
						// we have to increment the addendum by 1 because we also need to increment the octet that should be incremented when a nested special function call is encountered!!
						pNewToken->envid=(prevToken->envid+addendum+1); // ander will take care of removing what's too the left
					}else{ // can't increment
						pNewToken->type=TT_ERROR;
						inputError("Cannot exceed the maximum number of 15 (nested) special function calls");
					}
				}
				free(_functionName);
				// every , that ends a function call argument should decrement the argument value
			}else{ // not a function identifier	
				/////if(amDebugging())inputInfo("E4");		
				if(prevToken->type!=TT_NEW_VARIABLE&&prevToken->type!=TT_VARIABLE&&prevToken->type!=TT_END_OF_FUNCTION_CALL) // not behind a variable identifier or end of function call
					/////inputInfo("Checking new token of type %s behind token of type %s!",TOKENTYPE_STRING[newTokenType],TOKENTYPE_STRING[prevToken->type]);	
					pNewToken->prevIdentifier=prevToken->prevIdentifier;
				else // behind a variable identifier or end of function call
					pNewToken->prevIdentifier=prevToken;
				/////if(amDebugging())inputInfo("E5");
				// what to do with the argument if a function call ends???????
				// the function name of the function call should contain the right argument value TODO check this!!!!!!!!
				// BUG FIX aha end of function call does not always end a function call, but an expression (a single opening parenthesis without a function name in front of it), so explicitly checking for that!!!
				if(prevToken->type==TT_END_OF_FUNCTION_CALL&&prevToken->expr&&prevToken->expr->type==TT_FUNCTION_CALL)
					pNewToken->argument=prevToken->expr->prev->argument;
				else
					pNewToken->argument=prevToken->argument;
				/////if(amDebugging())inputInfo("E6");
				// should we change the argument??????
				if(newTokenType==TT_LISTELEMENT){ // ha ha, can't use pNewToken->type here as not assigned yet!!!
					///////inputInfo("List element!");	
					// careful now, is this a comma that ends a function call argument??????
					// let's inspect the expr field which should point to start parenthesis
					// BUT we should only subtract from argument when this is a `do`, `for` or `function` call
					if(pNewToken->expr){
						if(pNewToken->expr->type==TT_FUNCTION_CALL){
							if(pNewToken->expr->argument>0){
								pNewToken->argument=pNewToken->argument-1;
								if(amDebugging())inputInfo("New function call argument!");
							}else
							if(amDebugging())
								inputInfo("Non-local variable function call argument");
						}else
						if(pNewToken->expr->type!=TT_LIST&&pNewToken->expr->type!=TT_MAP){
							newTokenType=TT_ERROR;
							inputError("Comma not allowed in expression of type %s.",TOKENTYPE_STRING[pNewToken->expr->type]);
						}
					}else{ // a comma should always match either a map or list or expression start
						newTokenType=TT_ERROR;
						inputError("Comma not allowed outside map, list or function call!");
					}
				}
				/////if(amDebugging())inputInfo("E7");
			}
		}
		/////if(amDebugging())inputInfo("E8");
		// MDH@03MAY2019: TT_EXPRESSION is the default (0) now (always ending at the next non-space character): pNewToken->type=TT_EXPRESSION; // makes more sense to start as expression (same as what we get after a ( or [
		pNewToken->text=__string();
		// MDH@23JUL2019: we can do this for now TODO this is a serious memory error which a better way to deal with that is crucial
		if(!pNewToken->text){
			inputError("Failed to initialize the new token.");
			pNewToken->type=TT_ERROR; 
		}
		if(amDebugging())inputInfo("New token text initialized."); // TODOhow about 
		/////if(amDebugging())inputInfo("E9");
		/* not needed with calloc() allocation
		pNewToken->significantCharacterCount=0; // MDH@22MAR2019: remembers the amount of significant characters (to be set when the token ends)
		pNewToken->next=NULL;
		*/
	}
	if(!pNewToken)inputError("Failed to create a new token.");else pNewToken->type=newTokenType;
	return pNewToken;
}

// keep track of all commands so far
#define COMMAND_BLOCKSIZE 8
Mcommand** commands=NULL; // array for storing the pointers to the first token of all commands entered
uint32_t commandBlocks=0;
bool registerCommand(Mcommand* command){
	if(!command)return false;
	if(!_currentFunctionBodyInput){ // a top-level (non function body) command
		if(commandCount==commandBlocks*COMMAND_BLOCKSIZE){
			// I have to copy all first token pointers to a new array large enough
			commandBlocks++;
			Mcommand** createUserInputCommands=realloc(commands,COMMAND_BLOCKSIZE*commandBlocks*sizeof(Mtoken*));
			if(createUserInputCommands==NULL)return false;
			commands=createUserInputCommands;
		}
		commands[commandCount++]=command;
		return true;
	}else{ // should be added to the function body
		// NOTE we can create the value and when it is not appended to the list it will not be bound, and be released by the 'garbage collector'
		Mvalue* _commandToEvaluateTokenValue=_getValueOfToken(command->_firstToken,false);
		if(_commandToEvaluateTokenValue){
			if(!_currentFunctionBodyInput->_function->_bodyCommandList)_currentFunctionBodyInput->_function->_bodyCommandList=CALLOC(1,sizeof(Mlist),'L');
			if(appendedToList(_currentFunctionBodyInput->_function->_bodyCommandList,_commandToEvaluateTokenValue,M_LL_INVALID)>0)return true;
			outputError("Failed to add the command to the body of the function");
		}
	}
	return false;
}
// MDH@21JUN2019: reset() takes care of removing all stored commands
void reset(){
	while(commandCount>0){
		free_command(commands[--commandCount]);
	}
#ifdef __ADEBUG__
	outputChar('\n');
	syncallocations();
#endif
}

// associated every possible input characters (0 through 127) with a character type where a period denotes a non-command input character
// t=tab(feedforward variable),n=newline(end of command),U=unary operator,D=double quoted string literal,C=comment,L=letter (in identifiers),l=letter (not at start of identifier)
// D=digit,d=digit (not at start of numeric value),e=the letter e which may be part of an 'extended' number (or represent the constant e)
// B=binary operator,b=binary operator that cannot be used as first binary operator character,A=assignment operator,
// E=starts an expression(a comma),e=ends and expression ( ) and ]), (NOTE: some characters are best represented by themselves
// all lowercase characters represent control characters, like t=tab, n=newline, x=escape control character,o=switch to control mode,d=delete,b=backspace
// O=operator that can be either unary or binary depending on its position (+ and - characters)
// use x for eXit (e.g. with Ctrl-C and Ctrl-Z), c for cancel command, and m for going into M (control) mode
// as for operators: there are 8 different groups of operators
// !     not unary operator or first character of binary operator !=
// ~     pure unary operator
// -+    sign unary operator or binary minus/plus operator
// %^    pure binary operator
// */    binary operator extensible to make ** power operator or // integer division operator
// <>    binary operator extensible to make << or >> operator but can also be followed by an = sign (is this not the same as */?)
// =     assignment operator that can follow most of the binary operators (except < and >)
// |&    binary or and operator extensible to make || logical or or && logical and operator but the latter cannot be followed by =
// MDH@16APR2019: removing the o input character type (for switching explicitly to or from control mode), replacing it by n, so we can use the backtick for certain purposes...
//                in certain languages it means evaluate this (or the result of a system command??????)
//                furthermore we're combining operators to a single input character type: \^~% become %, /* become * and |& become &
// MDH@31OCT2019: let's use the backtick (`) as special whitespace character to use when one wants to insert a line break (i.e. continue the command on the next line)
//                although this would mean that it would show up when writing the tokens
//                we tried inserting a TT_WHITESPACE token with a backtick character (i.e. using ` as associated input character type) but ran into all kinds of problems so now we treat ` as W input character type
//                so it is appended to the current token, we only need to get it displayed in another color
//                ok, we're going to use \ for newline request character, so \ used to be % now becomes for type \ indicating a newline request (or escape character in a string!!!!)
//                switched to using the blank to indicate a newline request (using \ is a bit clumsy, backtick goes back to being the backtick, although no idea what we can use it for)
//                no we let \ be whitespace but we can turn it into a blank when it's a functional newline request
// MDH@04NOV2019: in order to be able to pass value references (i.e. variables) to a function we define @ as the redirection operator so that not the value but the value reference is returned (unresolved)
//                by defining @ as of type R we indicate that it refers to an identifier that has to be an existing variable!!!
//                                -------------------------------- !"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~-
const char INPUTCHARACTERTYPES[]="iiiciiiihtniiniiiiiiiiiiiixmiiiiW!DCL%&S()*+,-.*NNNNNNNNNN:;>=>?RLLLLLLLLLLLLLLLLLLLLLLLLLL[W]%L`LLLLELLLLLLLLLLLLLLLLLLLLL{&}~b";
// replacing: const char INPUTCHARACTERTYPES[]="iiiciiiibtniiniiiiiiiiiiiixmiiiiW!DCL%&S()*+,-./NNNNNNNNNN:;<=>?@LLLLELLLLLLLLLLLLLLLLLLLLL[%]%L`LLLLELLLLLLLLLLLLLLLLLLLLL{|}~d";

// now we define all the state transitions i.e. what input character types result in which new token type
// NOTE this can be organized in many ways perhaps it's easiest to tell per input character what the transformation is
//      only changes to the token type need to be registered, so if the change is NOT present, no need to put it in the transition table
//      EWW means that when starting an expression any whitespace starts a whitespace token, we use * to indicate ALL possible input character types
//      *WW means that any W character received in any state will result in a W state 
// we can make an array of transitions with each element corresponding to the character in TOKENTYPES, so the first entry contains all responses to E, the second entry the responses to W etc.
// it's easier to tell for any possible resulting token type which input character types will result in that type
// it's a hell of a job to create the token type transitions matrix
/* LEGEND:
   - signs are allowed in an EREAL but only directly behind the E, which means we have to somehow have an EREALEXPONENT element unless you treat this E as a binary operator which I think is a very good idea!!!
   - E stands for *10** so is this an assignable operator I suppose you could make it assignable as in 4e=3 to muliply by 1000, yes this look strange, as such . could also be considered an operator but Ok
     E is Assignable e r u, so we can get rid of the EREAL token type!!!
*/
char* const NO_TRANSITIONS[NUMBER_OF_FINISHABLE_TOKEN_TYPES]={"","","","","","","","","","","","","","q","q","`D","`S","","","","","","","","LEN","",""}; // MDH@30APR2019: oops one extra needed...

/* MDH@18MAR2019: I have to add all token containing operator characters which is any of 8 different types of operators
   NOTE some operators are temporary in that they can be completed to become another (final) operator like ! or = when an = could be added, so it's actually a transition from an existing token to the same token
   Operator token types:
   UNARY 						! - + 				which consist of ! (not) and - and + first characters at a place where a unary operator is acceptable
   ASSIGNMENT 					=					any token that ends with = with an identifier in front of (possibly of a list element which will make it more complex)
   BIN_UNEXT_ASSIGNABLE			+ - ~ ^ \ %			a non-extendable binary operator but that is assignable behind a variable identifier
   BIN_EXT_ASSIGNABLE 			* /					a binary operator that is extendable (with the same character) but (both) with an assignment operator (behind an identifier token)
   BIN_EXT_OR_ASSIGNABLE		& |					a binary operator that can either be extended (with the same character) or assigned (because it's a binary operator by itself)
   BIN_EQ_OR_NEQ				! =					binary equal or unequal operator (to be postfixed with =) where a binary operator is expected (behind an identifier or some other value argument)
   BINARY  						? :					things that are immediately binary (and that do not allow additional characters in the token)
   COMPARISON					< >					comparison operator that is extendable with the same sign and it assignable after adding this second sign, but still = can be added to it to become binary
   You may notice that the interpretation of the first character may differ for ! - + (unary or binary) = (binary assignment behind identifier or equality operator elsewhere)
   Some of these token types are intermediate that is INCOMPLETE and I think these are the first token that is not inherently complete immediately as with identifiers and literals (wel double quoted string are also inccomplete)
   Technically we could finish up with UNARY and BINARY or even OPERATOR as the position determine if it's a unary or binary operator BUT there's nothing wrong with keeping ASSIGNMENT, COMPARISON, EQUAL_OR_UNEQUAL, COMPARISON
   We can code these characters with digits 1, 2, 3, 4, 5, 6, 7, 8 unary could be encoded with 1 
   Well characters with multiple meanings like ! - + and = could be represented by themselves but the first letter of the token type that would be U A B C which leaves us with four additional for which we can use % / & 
*/
/* MDH@23MAR2019: syntacticly we have less operators
	TOKENTYPE(TT_ONE_CHAR_UNARY=0b10000001) 						!(un) -(un) +(un)
	TOKENTYPE(TT_ONE_CHAR_BINARY_=0b10100001)      					?
	TOKENTYPE(TT_ONE_CHAR_ASSIGNABLE_BIANRY=0b10101010)  			= ~ ^ % \ -(bin) +(bin)
	TOKENTYPE(TT_TWO_CHAR_BINARY=0b10101110)      					! (followed by =)
	TOKENTYPE(TT_TWO_CHAR_ONCE_ASSIGNABLE_BINARY=0b10101011)		& | (interesting =+= and &+= and |+= and itself)
	TOKENTYPE(TT_TWO_CHAR_ASSIGNABLE_BINARY=0b10111011)				< > * /
	printf("\nError                                        : %d.",TT_ERROR);
	printf("\nOne character unary operator                 : %d.",TT_ONE_CHAR_UNARY);
	printf("\nAssignment operator                          : %d.",TT_ASSIGNMENT);
	printf("\nOne character binary operator                : %d.",TT_ONE_CHAR_BINARY);
	printf("\nOne character assignable binary operator     : %d.",TT_ONE_CHAR_ASSIGNABLE_BINARY);
	printf("\nTwo character binary operator                : %d.",TT_TWO_CHAR_BINARY);
	printf("\nTwo character once assignable binary operator: %d.",TT_TWO_CHAR_ONCE_ASSIGNABLE_BINARY);
	printf("\nTwo character assignable binary operator     : %d.",TT_TWO_CHAR_ASSIGNABLE_BINARY);
	printf("\nComparison or shift operator                 : %d.",TT_COMPARISON_OR_SHIFT_BINARY);
*/
/* MDH@10APR2019: 
- some transitions only change the type but do not start a new token, but this is true for all binary operators, so I guess we can force that programmatically
- if we put ERROR at the end we do not need to add an array for dealing with error transitions (as we cannot leave an error!!)
*/
// operator input type characters: ! ~ + - % * < = | (8 different operator groups)
// ! ~ and + start a unary operator when a value is expected
// MDH@15APR2019: still to determine what to do with @ and ` (the latter for system commands????)
//                inserting macro's should also be possible somehow...
// MDH@05AUG2019: it's a pity that I need to allow a , behind a new variable in order to allow that when a do function call executes code after initializing these variables that are not yet recognized as created
//                we can solve this by remembering ALL variables when they are created in every expression that is tokenized, this would be possible by creating a tokenizing environment where we remember all created variables in in the tokenizing process
// MDH@04NOV2019: the reference token type added, so we can pass references to functions wrapped inside a value
/*
 "EXPR","UNA" ,"A","Baeru","BaErU","BAeRu","BaERu","BAeru" ,"Taeru","REF" ,"VAR"  ,"NEWVAR","L_EL","INT","REAL","DQSTRING","SQSTRING","END_DQS","END_SQS","LIST","END_L","MAP","M_V","END_M","FUNCTION","F_CALL","END_FC","CM","ERROR"},*/
const char * const TRANSITIONS[NUMBER_OF_FINISHABLE_TOKEN_TYPES][NUMBER_OF_TOKEN_TYPES]={ \
{"("   ,"!-+~","" ,""     ,""     ,""     ,""      ,""     ,""     ,"R"   ,"LE"   ,""      ,""    ,"N"  ,"."   ,"D"       ,"S"       ,""       ,""       ,"["   ,""     ,"{"  ,""   ,""     ,""        ,""      ,""      ,""  ,"` ; C  % )&*  , >?:    ] }="}, /* EXPRESSION */ \
{"("   ,"!-+~","" ,""     ,""     ,""     ,""      ,""     ,""     ,""    ,"LE"   ,""      ,""    ,"N"  ,"."   ,""        ,""        ,""       ,""       ,"["   ,""     ,""   ,""   ,""     ,""        ,""      ,""      ,""  ,"`R; CDS% )&*  , >?:    ]{}="}, /* ONE CHARACTER UNARY !-+~ */ \
{"("   ,"!-+~","" ,"="    ,""     ,""     ,""      ,""     ,""     ,"R"   ,"LE"   ,""      ,""    ,"N"  ,"."   ,"D"       ,"S"       ,""       ,""       ,"["   ,""     ,"{"  ,""   ,""     ,""        ,""      ,""      ,""  ,"` ; C  % )&*  , >?:    ] }" }, /* ASSIGNMENT = */ \
{"("   ,"!-+~","" ,""     ,""     ,""     ,""      ,""     ,""     ,""    ,"LE"   ,""      ,""    ,"N"  ,"."   ,"D"       ,"S"       ,""       ,""       ,"["   ,""     ,"{"  ,""   ,""     ,""        ,""      ,""      ,""  ,"`R; C  % )&*  , >?:    ] }="}, /* Baeru finished bin.op. */ \
{""    ,""    ,"" ,"="    ,""     ,""     ,""      ,""     ,""     ,""    ,""     ,""      ,""    ,""   ,""    ,""        ,""        ,""       ,""       ,""    ,""     ,""   ,""   ,""     ,""        ,""      ,""      ,""  ,"`R;!CDS%()&*+-,.>?:LEN[]{}" }, /* BaErU unfinished bin.op. */ \
{"("   ,"!-+~","=",""     ,""     ,""     ,""      ,"R"    ,""     ,""    ,"LE"   ,""      ,""    ,"N"  ,"."   ,""        ,""        ,""       ,""       ,"["   ,""     ,"{"  ,""   ,""     ,""        ,""      ,""      ,""  ,"`R; CDS% )&*  , >?:    ]"   }, /* BAeRu assignable repeatable */ \
{"("   ,"!-+~","" ,"="    ,""     ,""     ,""      ,"R"    ,""     ,""    ,"LE"   ,""      ,""    ,"N"  ,"."   ,"D"       ,"S"       ,""       ,""       ,"["   ,""     ,"{"  ,""   ,""     ,""        ,""      ,""      ,""  ,"`R; C  % )&*  ,  ?:    ]"   }, /* BaERu comp. (<>) bin.op. */ \
{"("   ,"!-+~","=",""     ,""     ,""     ,""      ,""     ,""     ,""    ,"LE"   ,""      ,""    ,"N"  ,"."   ,"D"       ,"S"       ,""       ,""       ,"["   ,""     ,"{"  ,""   ,""     ,""        ,""      ,""      ,""  ,"`R; C  % )&*  , >?:    ]"   }, /* BAeru assignable bin.op. */ \
{"("   ,"!-+~","=",""     ,""     ,""     ,""      ,""     ,""     ,""    ,"LE"   ,""      ,""    ,"N"  ,"."   ,"D"       ,"S"       ,""       ,""       ,"["   ,""     ,"{"  ,""   ,""     ,""        ,""      ,""      ,""  ,"`R; C  % )&*  , >?:    ]{}" }, /* Taeru ternary op. (? only now) */ \
{""    ,""    ,"" ,""     ,""     ,""     ,""      ,""     ,""     ,"LEN.",""     ,""      ,","   ,""   ,""    ,""        ,""        ,""       ,""       ,""    ,"]"    ,""   ,""   ,"}"    ,""        ,""      ,")"     ,"C" ,"`R;! DS%( &*+-  >?:   [ { ="}, /* REFERENCE to an existing variable */ \
{""    ,""    ,"=",""     ,"!"    ,"&*"   ,">"     ,"-+%"  ,"?"    ,""    ,"LEN." ,""      ,","   ,""   ,""    ,""        ,""        ,""       ,""       ,"["   ,"]"    ,""   ,":"  ,"}"    ,""        ,""      ,")"     ,"C" ,"`R;  DS (               {"  }, /* VARIABLE (identifier that is NOT a function) FUNCTION: some identifier not yet recognized as function name */ \
{""    ,""    ,"=",""     ,""     ,""     ,""      ,""     ,""     ,""    ,""     ,"LEN."  ,","   ,""   ,""    ,""        ,""        ,""       ,""       ,""    ,"]"    ,""   ,""   ,"}"    ,""        ,""      ,""      ,"C" ,"`R;! DS%()&*+- .>?:   [ {"  }, /* NEW_VARIABLE (variable that does not exist yet) */ \
{"("   ,"!-+~","" ,""     ,""     ,""     ,""      ,""     ,""     ,"R"   ,"LE"   ,""      ,","   ,"N"  ,"."   ,"D"       ,"S"       ,""       ,""       ,"["   ,"]"    ,"{"  ,""   ,""     ,""        ,""      ,""      ,""  ,"` ; C  % )&*    >?:      }="}, /* LIST ELEMENT (similar to expression) */ \
{";"   ,""    ,"" ,"?:"   ,"!="   ,"&*"   ,">"     ,"-+%E" ,"?"    ,""    ,""     ,""      ,","   ,"N"  ,"."   ,""        ,""        ,""       ,""       ,""    ,"]"    ,""   ,":"  ,"}"    ,""        ,""      ,")"     ,"C" ,"`R   DS (          L  [ {"  }, /* INTEGER: (signless) list of digits */ \
{";"   ,""    ,"" ,"?:"   ,"!="   ,"&*"   ,">"     ,"-+%E" ,"?"    ,""    ,""     ,""      ,","   ,""   ,"N"   ,""        ,""        ,""       ,""       ,""    ,"]"    ,""   ,":"  ,"}"    ,""        ,""      ,")"     ,"C" ,"`R   DS (      .   L  [ {"  }, /* REAL: part behind a decimal period */ \
{""    ,""    ,"" ,""     ,""     ,""     ,""      ,""     ,""     ,""    ,""     ,""      ,""    ,""   ,""    ,""        ,""        ,"D"      ,""       ,""    ,""     ,""   ,""   ,""     ,""        ,""      ,""      ,""  ,""                           }, /* DQSTRING: double quoted string */ \
{""    ,""    ,"" ,""     ,""     ,""     ,""      ,""     ,""     ,""    ,""     ,""      ,""    ,""   ,""    ,""        ,""        ,""       ,"S"      ,""    ,""     ,""   ,""   ,""     ,""        ,""      ,""      ,""  ,""                           }, /* SQSTRING: single quoted string */ \
{";"   ,""    ,"" ,"+"    ,"!="   ,"&"    ,">"     ,""     ,"?"    ,""    ,""     ,""      ,","   ,""   ,""    ,"D"       ,"S"       ,""       ,""       ,""    ,"]"    ,""   ,":"  ,"}"    ,""        ,""      ,")"     ,"C" ,"`R   DS%&( * - .   LEN[ {"  }, /* END_DQSTRING: double quoted string at end of double quoted string */ \
{";"   ,""    ,"" ,"+"    ,"!="   ,"&"    ,">"     ,""     ,"?"    ,""    ,""     ,""      ,","   ,""   ,""    ,""        ,""        ,""       ,""       ,""    ,"]"    ,""   ,":"  ,"}"    ,""        ,""      ,")"     ,"C" ,"`R   DS%&( * - .   LEN[ {"  }, /* END_SQSTRING single quoted string at end of single quoted string */ \
{"("   ,"!-+~","" ,""     ,""     ,""     ,""      ,""     ,""     ,"R"   ,"LE"   ,""      ,","   ,"N"  ,""    ,"D"       ,"S"       ,""       ,""       ,"["   ,"]"    ,"{"  ,""   ,""     ,""        ,""      ,")"     ,""  ,"` ; C  %& )*   .>?:      }="}, /* LIST: [ starts a list */ \
{";"   ,""    ,"=","?"    ,"!"    ,"&*"   ,">"     ,"-+%"  ,"?"    ,""    ,""     ,""      ,","   ,""   ,""    ,""        ,""        ,""       ,""       ,""    ,"]"    ,""   ,":"  ,"}"    ,""        ,""      ,")"     ,"C" ,"`R   DS  (     .  :LEN  {"  }, /* END_OF_LIST: behind ] that ends a list */ \
{"("   ,"!-+~","" ,""     ,""     ,""     ,""      ,""     ,""     ,"R"   ,"LE"   ,""      ,""    ,"N"  ,""    ,"D"       ,"S"       ,""       ,""       ,"["   ,""     ,""   ,""   ,"}"    ,""        ,""      ,")"     ,""  ,"` ; C  %& )*  ,.>?:    ]{ ="}, /* MAP: { starts a map */ \
{"("   ,"!-+~","" ,""     ,""     ,""     ,""      ,""     ,""     ,"R"   ,"LE"   ,""      ,""    ,"N"  ,"."   ,"D"       ,"S"       ,""       ,""       ,"["   ,""     ,"{"  ,""   ,""     ,""        ,""      ,")"     ,""  ,"` ; C  %& )*  , >?:    ] }="}, /* MAP_VALUE: : starts a map value */ \
{";"   ,""    ,"" ,"?"    ,"!="   ,"&*"   ,">"     ,"+"    ,"?"    ,""    ,""     ,""      ,","   ,""   ,""    ,""        ,""        ,""       ,""       ,""    ,"]"    ,""   ,""   ,"}"    ,""        ,""      ,")"     ,"C" ,"`R   DS% (   - .  :LEN  {"  }, /* END_OF_MAP: behind } that ends a map */ \
{""    ,""    ,"" ,""     ,""     ,""     ,""      ,""     ,""     ,""    ,""     ,""      ,""    ,""   ,""    ,""        ,""        ,""       ,""       ,""    ,""     ,""   ,""   ,""     ,""        ,"("     ,""      ,""  ,"`R;!CDS%& )*+-,.>?:   []{}="}, /* FUNCTION: some identifier recognized as function name */ \
{"("   ,"!-+~","" ,""     ,""     ,""     ,""      ,""     ,""     ,"R"   ,"LE"   ,""      ,","   ,"N"  ,"."   ,"D"       ,"S"       ,""       ,""       ,"["   ,""     ,"{"  ,""   ,""     ,""        ,""      ,")"     ,""  ,"` ; C  %&  *    >?:    ] }="}, /* FUNCTION_CALL ( following the name of a function */ \
{";"   ,""    ,"" ,"?:"   ,"!="   ,"&*"   ,">"     ,"-+%E" ,"?"    ,""    ,""     ,""      ,","   ,""   ,""    ,""        ,""        ,""       ,""       ,""    ,"]"    ,""   ,":"  ,"}"    ,""        ,""      ,")"     ,"C" ,"`R   DS  (     .   L N  {"  }, /* END_OF_FUNCTION_CALL ) at end of last function call argument, ending a function call */ \
};

/* MDH@11AUG2019: NOT doing the following anymore, instead we store the identifier information in the tokens themselves
// MDH@06AUG2019: I have to keep track of variable initializations in the current command so that I know when a variable is new or not
//                analysis: variables used in a command typically refer to variables created in previous commands
//                i.e. to variables kept in the current execution environment
//                but we want to allow for local variables like the counter in a for loop or local to a group of commands as now possible in a do() function call
//                and probably also in a user function call
//                so technically calls to the predefined functions called for, do and function can have arguments in which local variables are defined
//                these local variables should be considered to exist for the duration of the call i.e. in all following arguments of the call when being entered, so these variables do not need to be 'global'
//                in case of for the first argument contains the local variable initializations, we can do the same in the do function call, in function its the second argument (where the first argument represents the name of the function)
//                this means that we need to know in which argument of any for, do or function call when it is being entered, of course with function we could create a function by assigning it to a variable name instead of defining the function name as first argument
//                in which case the first argument would be come the argument with the local variables, which would be more convenient!!!!
typedef struct Minitialization{
	char* _variableName;
	int64_t argument; // keep track of the argument this value is initialized at the beginning and decremented/incremented whenever necessary this argument needs to be zero
	struct Minitialization *_prev;
}Minitialization;
Minitialization *_lastInitialization=NULL; // stack of initializations
void free_initialization(Minitialization* _initialization){if(_initialization){if(_initialization->_variableName)free(_initialization->_variableName);FREE(_initialization,'I');}}
bool pushInitialization(char* variableName){
	Minitialization* _initialization=(variableName&&strlen(variableName)?CALLOC(1,sizeof(Minitialization),'I'):NULL);
	if(_initialization){
		_initialization->_variableName=_strdup(variableName);
		if(_initialization->_variableName){
			_initialization->_prev=_lastInitialization;
			// typically we copy the argument count over from the previous initialization but if this initialization is an end of a function call, we have to copy the argument 
			Minitialization* _argumentCountInitialization=_lastInitialization;
			if(*variableName==')'){ // this 'initialization' ends the current function call
				bool startOfFunctionCallInitialization;
				while(_argumentCountInitialization){
					startOfFunctionCallInitialization=(*(_argumentCountInitialization->_variableName)=='(');
					_argumentCountInitialization=_argumentCountInitialization->_prev;
					if(startOfFunctionCallInitialization)break;
				}
			}
			_initialization->argument=(_argumentCountInitialization?_argumentCountInitialization->argument:-1);
			_lastInitialization=_initialization;
		}else{
			FREE(_initialization,'I'); // no need to call free_initialization as no variable to free
			_initialization=NULL;
		}
		/// do this in the caller!!!! inputError("Failed to remember initialization '%s'.",_variableName);
	}
	return(_initialization!=NULL);
}
bool popInitialization(){
	if(_lastInitialization){
		Minitialization* _initialization=_lastInitialization->_prev;
		free_initialization(_lastInitialization);
		_lastInitialization=_initialization;
		return true;
	}
	return false;
}
bool initializable(){return(!_lastInitialization||_lastInitialization->argument==0);} // we need to be in the right argument to be initializable, MUST be called BEFORE calling pushInitialization() when a variable name is pushed!!!
void showInitializations(){
	// we have to compose a text first, then call inputInfo()
	Mstring* _initializationsText=__string();
	if(_initializationsText){
		Mstring* p=_initializationsText;
		Minitialization* _initialization=_lastInitialization;
		while(p&&_initialization){
			char* _initializationText=_getFormattedText(80," %s:%lld",_initialization->_variableName,_initialization->argument);
			if(_initializationText){
				p=string_prepend(p,_initializationText);
				free(_initializationText);
			}
			_initialization=_initialization->_prev;
		}
		if(p)inputInfo("Initializations:%s.",string(_initializationsText));else inputError("Failed to show the initializations.");
		free_string(_initializationsText);
	}
}
void removeInitializations(){while(popInitialization());} // keep popping until failure
void determineCommandInitializations(){
	// TODO this is going to be quite hard...

}
bool initialized(char* variableName){
	// it's a little harder than just looking for a matching _variableName in the initializations as we can have subinitializations in do() and for() function calls which we will need to skip
	// as such we will need to count the level of initializations
	Minitialization* _initialization=_lastInitialization;
	unsigned long long level=0;
	char firstVariableNameCharacter;
	while(_initialization){
		firstVariableNameCharacter=*(_initialization->_variableName);
		switch(firstVariableNameCharacter){
			case '(':level--;break; // end of sublevel
			case ')':level++;break; // start of sublevel
			case ',':break; // a previous argument
			default:if(level==0&&!strcmp(_initialization->_variableName,variableName))return true;
		}
		_initialization=_initialization->_prev;
	}
	return(_initialization!=NULL);
}
// MDH@06AUG2019 END
*/
// suggesting NOT to be able to get out of an error condition but to allow viewing information on the error somehow!!! (how about tab as this will do feed forward!!!!!)
// if we put the error info in the error token

// MDH@05JUN2019: it's prudent to return the negative value of the input token type if the given input character type ends the token 
//                i.e. when NO_TRANSITIONS is a match, so that the caller can set the significantCharacterCount
int8_t nextTokenType(uint8_t inputTokenType,char inputCharacterType){
	if(inputTokenType<NUMBER_OF_FINISHABLE_TOKEN_TYPES){ // can only move to another token type if currently inside a valid token (i.e. you cannot get out of a TT_ERROR token type!!!)
		// finding the type will be more difficult actually if we end up with the token type character instead of the token type index!!!
		char* noTransition=NO_TRANSITIONS[inputTokenType];
#ifdef __DEBUG__
		printf("'%s'",noTransition);
#endif
		// TODO we can improve on the following
		///////////if(noTransition[0]!='`'&&!strchr(noTransition,inputCharacterType))return -inputTokenType;
		if(strlen(noTransition)==0||(noTransition[0]=='`'?strchr(noTransition,inputCharacterType)!=NULL:strchr(noTransition,inputCharacterType)==NULL)){
			int8_t tokenType=NUMBER_OF_TOKEN_TYPES; // MDH@10APR2019: BUG FIX uint8_t changed to int8_t otherwise would circle around
			while(--tokenType>=0)if(strchr(TRANSITIONS[inputTokenType][tokenType],inputCharacterType)!=NULL)return tokenType;
		}
#ifdef __DEBUG__
		else{
			outputChar('=');
		}
#endif
	}
	return inputTokenType; // if no match was found assume no change to the token type!!
}

//// first operator characters (0=assignment character, 1-6: binary 1 and 2-character operators, 7-8: 1-character binary, 9-10: unary/binary, 11-12: 1-character unary)
//const char ASSIGNMENT_CHARACTER='=';
//const char FIRST_OPERATOR_CHARACTERS[]={ASSIGNMENT_CHARACTER,'<','>','|','&','*','/','^','%','+','-','~','!','\0'}; // i.e. "=<>|&*/^%+-~!";
// continuation of 2-character operators
////////const char* SECOND_OPERATOR_CHARACTERS[]={"=","=<>","=<>","|","&","*","/"};
/* if we have a token representing an operator we can check whether a new character is acceptable as continuation
bool continuesOperator(Mtoken* _userInputCommand->_lastToken,char inputChar){
	unsigned int l=string_get_length(_userInputCommand->_lastToken->text);
	if(inputChar==ASSIGNMENT_CHARACTER){ // appending the 'assignment' operator
		return(l==1||string_get_last_char(_userInputCommand->_lastToken->text)!=ASSIGNMENT_CHARACTER);
	}else{
		if(l>1)return false; // cannot continue a two-character operator
		// if subtype is assumed to represent the index into the first operator character set
		unsigned int operatorType=_userInputCommand->_lastToken->type.subtype;
		return(operatorType<=6&&strchr(SECOND_OPERATOR_CHARACTERS[operatorType],inputChar)!=NULL);
	}
}
*/
bool isBinaryOperatorTokenType(uint8_t tokenType){return(TOKENTYPE_IDS[tokenType]>>4)==0b0110;}
bool isOneCharacterTokenType(uint8_t tokenType){
	// TODO how about TT_EXPRESSION -> NO because a TT_EXPRESSION token is always considered ended, i.e. significantCharacterCount is not an issue in determining whether a new token starts there
	return(tokenType==TT_ASSIGNMENT||tokenType==TT_UNARY||tokenType==TT_TERNARY_aeru||tokenType==TT_LIST||tokenType==TT_LISTELEMENT||tokenType==TT_END_OF_LIST||tokenType==TT_MAP||tokenType==TT_END_OF_MAP||tokenType==TT_FUNCTION_CALL||tokenType==TT_END_OF_FUNCTION_CALL||tokenType==TT_END_OF_DQSTRING||tokenType==TT_END_OF_SQSTRING);
}

// keep track of the state of entering a command
// MDH@01OCT2019: result booled, but TODO can removeToken() fail??????
bool removeToken(){	
	// MDH@20SEP2019: if a token is removed, we also need to remove any associated feed forward text associated with the token
	deleteAutocompletionTextOfToken(_userInputCommand->_lastToken);
	// ASSERT _userInputCommand->_lastToken should NOT be NULL and empty (i.e. empty tokens should be removed!!!)
	// NOTE if we call freeToken() to free this token all forwardly connected tokens are also freed, so pPrevToken->next should become NULL
	// MDH@02OCT2019: every time _userInputCommand->_lastToken changes, call setLastUserInputCommandToken which will set identifierContinuationIsDirty if it's an identifier token
	setLastUserInputCommandToken(freeToken(_userInputCommand->_lastToken));
	/* replacing:
	_userInputCommand->_lastToken=freeToken(_userInputCommand->_lastToken); // _userInputCommand->_lastToken now equals its own previous token!!
	if(inIdentifierToken())identifierContinuationIsDirty=true; // every time the last command to evaluate token changes, we need this
	*/
	if(!_userInputCommand->_lastToken){_userInputCommand=NULL;return false;} // TODO is it safe to NULL _userInputCommand without freeing?????
	_userInputCommand->_lastToken->next=NULL;
	return true;
}

// MDH@04NOV2019: moved from line 1800 or so over here as it calls removeToken() and we do not like to have to use prototypes TODO remove all prototype() definitions
void updateUserInputCommandIdentifierContinuation(){
	// MDH@30OCT2019: simplified updating the identifier continuation a bit so wee do not need to be afraid that it won't work AND we no longer need the userInputCommandIdentifierContinuationNeedsUpdating flag!!!!
	//                BUT right after a delete we should be allowed to set the flag so the continuation will be deleted and nothing more
	//                OK, to make delete work, I have to check the NeedsUpdating flag which should be set to true when the token is set
	deleteIdentifierContinuation();
	// if currently in an identifier, we technically need updating the identifier continuation unless the NeedsUpdating flag has been turned off
	bool canHaveAnIdentifierContinuation=inIdentifierToken(_userInputCommand?_userInputCommand->_lastToken:NULL);
	if(canHaveAnIdentifierContinuation){ // theoretically we could have identifier continuation
		if(userInputCommandIdentifierContinuationNeedsUpdating){ // not blocked
			// MDH@04NOV2019: if inside a reference, skip the reference 'operator' at the start of the reference when requesting completion text
			Mstring* _completionText=(_userInputCommand->_lastToken->type!=TT_REFERENCE?_getCompletion(string(_userInputCommand->_lastToken->text),true):_getCompletion(string_remainder(_userInputCommand->_lastToken->text,1),false));
			// need at least two characters (the type and something text behind it)
			// OOPS if there's only one character in the text (just the type) which is quite possible we will need to free _completionText even then!!!
			if(_completionText){
				if(string_length(_completionText)>1){
					switch(string_char(_completionText,0)){
						case 1:
							_identifierContinuationCharacters=strdup(string(_completionText)+1);
							if(!_identifierContinuationCharacters)inputError("Failed to create the identifier continuation");else if(amVerbose())inputInfo("Identifier continuation: '%s'.",_identifierContinuationCharacters);
							break;
						case 2:
							_identifierContinuationOptionalCharacters=strdup(string(_completionText)+1);
							if(_identifierContinuationOptionalCharacters)inputInfo("Identifier continuation characters: %s.",_identifierContinuationOptionalCharacters);else inputError("No identifier continuation characters!");
							break;
					}
				}else{
					if(_userInputCommand->_lastToken->type==TT_REFERENCE){
						// MDH@04NOV2019: if we mark the entire reference token as error, we're going to have problems recovering it, so it would make sense to mark the character that caused not having a completion text anymore as erroneous
						//                so we should cut off that last character and put it in an error token, ready to be removed again
						size_t lastTokenLength=string_length(_userInputCommand->_lastToken->text);
						if(lastTokenLength>1){ // at least one character of the existing variable present in the reference
							char c=string_last_char(_userInputCommand->_lastToken->text); // get the last character (to mark as erroneous)
							if(c){ // we've got the character that is responsible for not getting a completion text anymore
								if(!containsVariable(NULL,string_remainder(_userInputCommand->_lastToken->text,1))){ // not already complete TODO perhaps there's a better way to compose the completion text in this case in _getCompletion()
									// let's do something like a backspace but without moving the cursor on the screen
									// replacing the last character with a blank is another option????
									if(string_setlength(_userInputCommand->_lastToken->text,lastTokenLength-1)){ // managed to 'cut off' c (although it's still there, because when you set the length only ->length is adjusted nothing yet to the text itself)
										Mtoken* _errorToken=_getNewCommandToken(_userInputCommand->_lastToken,TT_ERROR); // by passing in NULL all the complicated stuff is not happening!!!
										if(_errorToken){
											_userInputCommand->_lastToken=_errorToken; // update the last token assuming we will succeed in doing what needs doing
											if(!string_append_char(_errorToken->text,c)){
												if(removeToken())
													inputError("Failed to mark the last invalid reference character as erroneous because it cannot result in a reference to an existing variable.");
												else
													inputError("BUG: Failed to undo failing to mark the last character as erroneous.");
											}else
												reoutputToken(_errorToken);
										}else
											inputError("The supposed reference can never become an existing variable reference.");
										// if we failed to create the error token, we have to append the removed character again (should be no problem because Mstring does not reduce the memory when deleting characters from the end)
										if(_errorToken!=_userInputCommand->_lastToken){
											if(!string_setlength(_userInputCommand->_lastToken->text,lastTokenLength)){inputError("BUG: Couldn't undo the adjustments made to an erroneous reference.");}
										}
									}else
										inputError("Failed to retrieve the last (erroneous) character in a variable reference.");
								}else
								// if(amVerbose())
								inputInfo("Reference complete!");
							}else
								inputError("BUG: Last character in reference vanished.");
						}else
							inputError("There is no existing variable that can be referenced anymore.");
					}
				}
				free_string(_completionText);
			}else 
			if(amVerbose())inputInfo("No identifier continuation.");
		}
	}
	// by forcing the flag to be true AFTER each update, we ascertain that you can only block it once, AND there's no need actually to set it on every new token!!!!
	userInputCommandIdentifierContinuationNeedsUpdating=true; // as long as we're in an identifier token, keep the 'dirty' flag true
	/* replacing:
	bool couldHaveAnIdentifierContinuation=inIdentifierToken(_userInputCommand?_userInputCommand->_lastToken:NULL);
	// get rid of the identifier continuation if we're can't have one or the identifier has supposedly changed
	if(!couldHaveAnIdentifierContinuation)userInputCommandIdentifierContinuationNeedsUpdating=false; // if we're not in an identifier token always consider the identifier to be unchanged
	if(!couldHaveAnIdentifierContinuation||userInputCommandIdentifierContinuationNeedsUpdating)deleteIdentifierContinuation();
	if(userInputCommandIdentifierContinuationNeedsUpdating){ // the identifier has supposedly changed, and we could have an identifier continuation update it
		Mstring* _completionText=_getCompletion(string(_userInputCommand->_lastToken->text));
		// need at least two characters (the type and something text behind it)
		// OOPS if there's only one character in the text (just the type) which is quite possible we will need to free _completionText even then!!!
		if(_completionText){
			if(string_length(_completionText)>1)
			switch(string_char(_completionText,0)){
				case 1:
					{
						_identifierContinuationCharacters=strdup(string(_completionText)+1);
						if(!_identifierContinuationCharacters)inputError("Failed to create the identifier continuation");else if(amVerbose())inputInfo("Identifier continuation: '%s'.",_identifierContinuationCharacters);
					}
					break;
				case 2:
					{
						_identifierContinuationOptionalCharacters=strdup(string(_completionText)+1);
						if(_identifierContinuationOptionalCharacters)inputInfo("Identifier continuation characters: %s.",_identifierContinuationOptionalCharacters);else inputError("No identifier continuation characters!");
					}
					break;
			}
			free_string(_completionText);
		}else 
		if(amVerbose())inputInfo("No identifier continuation.");
	}
	*/
	if(amDebugging())inputInfo("User input command identifier continuation updated.");
}/* VALIDATED */

// HERE THE EVALUATION OF EXPRESSIONS TAKE PLACE
/* MDH@21MAY2019: every Mvalue* should be created on the value stack and never elsewhere, every assignment to an Mvalue should be done using assignValue() and never using =
Mvalue* getListValue(Mlist* _list){
	Mvalue* _listValue=(Mvalue*)calloc(1,sizeof(Mvalue)); // OOPS, not the sizeof the pointer but Mvalue itself!!!!
	if(_listValue)_listValue->value._list=_list;
	return _listValue;
}
*/
// the following is not required if we only allow the x[i/a,i/a,i/a] syntax or alternatively x[i/a][i/a] etc. and we only need to keep the last value and the 'index' or 'attribute' value reference
/* MDH@06MAY2019: expressions contain references to places where values are stored which is not a variable
typedef struct Mvaluepointeritem{
	Mvalue* _value; // index (list) or attribute 
	struct Mvaluepointeritem* _item; // the next item to access within this composite value
}Mvaluepointeritem;
// at the top we have a pointer that references a variable (in some environment)
typedef struct Mvaluepointer{
	Mvariable* _variable;
	Mvaluepointeritem* _item;
}Mvaluepointer;
// and at some point we'd need to get the value of where the value pointer points to
Mvaluepointeritem* getLastValuepointeritem(Mvaluepointer* _valuepointer){
	// the problem here is that to make assignments possible we have to remember the last value pointer item
	// this is because Mvalue instances themselves are immutable!!!
	Mvalue* _value=NULL;
	if(_valuepointer){
		_value=_valuepointer->_variable->_value;
		Mvaluepointeritem* _item=_valuepointer->_item;
		// if we have an item and a value
		while(_item&&_value){
			Mvalue* _itemvalue=_item->_value;
			// the value of the item could be of the wrong type i.e. 
			if(_itemvalue->type==VT_INTEGER&&_value->type==VT_LIST){
				_value=getListElement(_value->value._list,_itemvalue->value._integer);
			}else
			if(_itemvalue->type==VT_TEXT&&_value->type==VT_MAP){
				_value=getMapElement(_value->value._list,_itemvalue->value._text);
			}else // invalid reference
				_value=NULL;
		}
	}
	return _value;
}
*/

/* MDH@26OCT2019: moved over to Mvalue.h/c so we can use it in Mvalue's as well
// MDH@06MAY2019: Mvaluereference stands for a variable in combination with an item id, this will allow assignments as we know the variable involved!!!!
typedef struct Mvaluereference{
	char* _name; // the name of the host variable or NULL if we're in a substructure
	Mvalue* _value; // either the host value (if no variable name is defined), or the value of the host variable
	Mvalue* _itemid; // the item referenced!!!
}Mvaluereference;
*/
/*
typedef struct Mexpressionvalue{
	Mvaluereference* _valuereference; // thre result of evaluating an expression is always a single value!!!
	Mtoken* token; // supposed to be the token the evaluation ended with (so the token in front of the first in the next evaluation)
}Mexpressionvalue;
// free_expressionvalue does NOT free the token as it will probably be passed on...
void free_expressionvalue(Mexpressionvalue* _expressionvalue){
	if(_expressionvalue){
		if(amVerbose())outputLine("Releasing the result!");
		// MDH@03MAY2019: append the result value at the proper index (as indicated by the command index)
		if(_resultListValue){if(appendedToList(_resultListValue->value._list,_expressionvalue->_valuereference->_variable->_value,commandCount+1))outputLine("ERROR: Failed to save the result.");else if(amVerbose())outputLine("Result saved.");}
		// replacing: if(!appendToListVariable(_Menvironment,"M",_expressionvalue->_value))outputLine("ERROR: Failed to append the result to the M list.");else if(amVerbose())outputLine("Result appended to the M list.");
		//// NEVER free what does not have an underscore at the start!!!! free_token(_expressionvalue->token); // probably NULLed already as this will not be new token, so I guess we could remove the _ to prevent freeing!!!
		////////output("Freeing expression!");
		decrementReferenceCount(_expressionvalue->_valuereference->_variable->_value); // as where freeing _expressionvalue!!!!
		free(_expressionvalue);
	}else
	if(amVerbose())outputLine("No result to free!");
	
}
*/
/* MDH@21MAY2019: replaced by placing the list and map (which is what it was used for) on the main value list, so it can be removed when no longer referenced!!!!
// helper function to get an expression value of hold a value of a specific type
// OOPS THIS value is NOT stored on the central value list!!!
Mvalue* getValueOfExpressionOfType(enum Mvaluetype valuetype){
	Mvalue* _value=(Mvalue*)calloc(1,sizeof(Mvalue));
	// allocate the value to hold, if a composite type (map or list), initialize the map and list to an empty map or list (integer, real and string are not set in advance)
	if(valuetype!=VT_UNDEFINED){
		_value->type=valuetype;
		switch(_value->type){
			case VT_INTEGER:_value->value._integer=(Minteger*)calloc(1,sizeof(Minteger));break; // initialized to 0 I presume
			case VT_FLOAT:_value->value._float=(Mfloat*)calloc(1,sizeof(Mfloat));break; // initialized to 0.0 I presume
			case VT_TEXT:_value->value._text=(Mtext*)calloc(1,sizeof(Mtext));break;
			case VT_LIST:_value->value._list=(Mlist*)calloc(1,sizeof(Mlist));break;
			case VT_MAP:_value->value._map=(Mmap*)calloc(1,sizeof(Mmap));break;
			default:break;
		}
	}
	return _value;
}
*/
/*
an expression represents a value, and therefore:
<expression>::=<value>{<binary operator><value>}
NOTE that binary operator is atomic
this is not a recursive definition but it could be <expression>::=<value>[<binary operator><expression>] but that would result in right-to-left evaluation
But I forgot to include assignment 


NOTE that a formula differs from an expression in that it does not contain assignments!!!!

This poses the question what x=3+4 evaluates to; we do not want to write x=(3+4) to get it properly evaluated, therefore x=3+4 means x= 3+4 i.e. everything behind x= is evaluated before being assigned
i.e. assignment is NOT a binary operator
<expression>::=[<variable>[<shortcut binary operator>]<assignment operator>]<formula>
<formula>::=<value>{<binary operator><value>} this way a binary operator never ends a expression and is not recursively processed
<value>::={<unary operator>} [function]<(>{<expression><,>}<expression><)> | <value literal> | <variable>)
<value literal>::= <integer> | <real> | <string> | <[><expression>{,<expression>}<]> | <{><string literal>:<expression>{,<string value>:<expression><}>

<variable> ::= <variable identifier> [<[>{<integer expression><,>}<integer expresssion><]>]

Note that certain elements have repeating elements (optional) like argument list, binary operator lists, and map element lists, which have different separators
I guess we can use that in the evaluation because these define the separators!!!! so with any list we can define the token types that separate the successive list elements!!!
but <value><operator><value> here operator is a set of token types that separate the values but the operators should end up in the produced list as they are significant/meaningful
*/

/* MDH@07JUL2019: we need to put the current token of the expression being evaluated in the execution environment, so we can execute a user function call
                  without loosing the original expression we're evaluating
Mtoken* expressionToken=NULL; // the current evaluation token
*/
/**
 * getValueOfExpression() evaluates an expression, obviously this means that we need to have some sort of understanding of where expression occur in the syntax of the M language
 * @info: some information text on the expression type (used in messages)
 * @resulttype: one character to indicate the type of expression result value (e.g. 'i' stands for index, i.e. an index into a list variable)
 * @firstToken: the first token in the expression to process
 * @endTokenTypes[]: the tokens that end the expression
 * @endTokenTypeCount: the number of end tokens
 * returns: the last token processed (which should be one of the end tokens) or NULL if all tokens were processed, and the Mvalue the expression evaluates to
 */
Mvalue* getValueOfExpression(const char* info,char resulttype,TokenType endTokenTypes[],uint8_t endTokenTypeCount); // prototype definition of getValueOfExpression() so we can call it from getValueOfList() and getValueOfMap()

/*
 * \brief makes a copy useful for evaluation (not for editing)
 */
Mtoken* _getEvaluatableTokenCopy(Mtoken* _token){
	Mtoken* _tokenCopy=(_token?__token():NULL);
	if(_tokenCopy){
		if(amVerbose()){
			output("Copying token '%s' of type '%s'.\n",string(_token->text),TOKENTYPE_STRING[_token->type]);
			if(_token->expr)output("\tpointing to token '%s' of type '%s'.\n",string(_token->expr->text),TOKENTYPE_STRING[_token->expr->type]);
		}
		_tokenCopy->type=_token->type;
		_tokenCopy->significantCharacterCount=_token->significantCharacterCount;
		if(_token->text)_tokenCopy->text=_stringCopy(_token->text,0);
		_tokenCopy->expr=_token->expr; // TODO do I need to do this??? this is also an issue because if we start comparing expr (on evaluation)
		_tokenCopy->argument=_token->argument; // MDH@11AUG2019: we need the argument as well bro' TODO how about the envid?????
		// we're NOT copying _next, _prev, _offset
		//////_tokenCopy->prev=NULL;_tokenCopy->next=NULL;_tokenCopy->offset=0;
	}
	return _tokenCopy;
}

Mvalue* Miffunction(Mvalue* _conditionTokenValue,Mvalue* _thenTokenValue,Mvalue* _elseTokenValue){
	Mvalue* _result=NULL;
	if(isValueZero(_conditionTokenValue)==M_TRUE){
		if(_elseTokenValue&&_elseTokenValue->type==VT_TOKEN){
			getEnvironment()->expressionToken=_elseTokenValue->value._token;
			_result=getValueOfExpression("else clause",'e',NULL,0);
		}
	}else{
		if(_thenTokenValue&&_thenTokenValue->type==VT_TOKEN){
			getEnvironment()->expressionToken=_thenTokenValue->value._token;
			_result=getValueOfExpression("then clause",'t',NULL,0);
		}
	}
    return _result;
}
Mvalue* Mwhilefunction(Mvalue* _conditionTokenValue,Mvalue* _whilebodyTokenValue){
	Mvalue* _result=NULL;
	if(_conditionTokenValue&&_conditionTokenValue->type==VT_TOKEN&&_whilebodyTokenValue&&_whilebodyTokenValue->type==VT_TOKEN){
		while(true){
			// evaluate the condition
			getEnvironment()->expressionToken=_conditionTokenValue->value._token;
			Mvalue* _conditionValue=getValueOfExpression("while condition",'w',NULL,0);
			if(isValueZero(_conditionValue)==M_TRUE)break; // condition evaluates to zero
			// evaluate the body
			getEnvironment()->expressionToken=_whilebodyTokenValue->value._token;
			_result=getValueOfExpression("while loop",'l',NULL,0);
		}
	}
	return _result;
}
// MDH@05AUG2019: the do function allows for executing a single command in its own environment, so all variables created are local
//                the problem is that we want to allow the user to enter a list of token things i.e. an infinite list of arguments instead of having to wrap the single argument in a list itself
//                this is solvable if we convert the list of arguments to a single Mvalue wrapping the entire list of arguments before calling Mdofunction
Mvalue* Mdofunction(Mvalue* _doTokenValue){
	Mvalue* _result=NULL;
	if(_doTokenValue&&_doTokenValue->type==VT_LIST){
		Mlist* doList=_doTokenValue->value._list;
		if(doList&&doList->_first){ // something to do
			Menvironment* _doEnvironment=__environment();
			if(_doEnvironment){
				_doEnvironment->_name=_strdup("do");
				// let's add variable $ as result variable and ! as exit flag variable
				bool doEnvironmentInitialized=addVariable(_doEnvironment,"$",VT_UNDEFINED,false)&&addVariable(_doEnvironment,"!",VT_INTEGER,false)&&setValue(_doEnvironment,"!",_getIntegerValue(0));
				if(doEnvironmentInitialized){
					if(pushExecutionEnvironment(_doEnvironment)){
						Mlistelement* tokenValueListelement=doList->_first;
						Mvalue *tokenExpressionValue,*expressionValue=NULL;
						while(tokenValueListelement){
							tokenExpressionValue=tokenValueListelement->_value;
							if(tokenExpressionValue&&tokenExpressionValue->type==VT_TOKEN){ // some token to interpret
								_doEnvironment->expressionToken=tokenExpressionValue->value._token;
								if(_doEnvironment->expressionToken){
									expressionValue=getValueOfExpression("do",'d',(TokenType[]){},0); // evaluate the expression
									if(isValueZero(getValue(_doEnvironment,"!"))!=M_TRUE)break; // if the exit flag was set, exit
								}
							}
							// move over to the next expression to evaluate...
							tokenValueListelement=tokenValueListelement->_next;
						}
						Mvalue* doResultValue=getValue(_doEnvironment,"$");
						_result=(doResultValue?doResultValue:expressionValue);
						popExecutionEnvironment(); // pop the do environment we successfully pushed
					}
				}else{
					outputError("Failed to create the do environment");
					free_environment(_doEnvironment);
				}
			}
		}
	}
	return _result;
}
Mvalue* Mforfunction(Mvalue* _initializationTokenValue,Mvalue* _conditionTokenValue,Mvalue* _incrementTokenValue,Mvalue* _forbodyTokenValue){
	Mvalue* _result=NULL;
	if( (!_initializationTokenValue||_initializationTokenValue->type==VT_TOKEN)&&
		(_conditionTokenValue&&_conditionTokenValue->type==VT_TOKEN)&&
		(!_incrementTokenValue||_incrementTokenValue->type==VT_TOKEN)&&
		(_forbodyTokenValue&&_forbodyTokenValue->type==VT_TOKEN)){
		Menvironment* _forEnvironment=__environment();
		if(_forEnvironment){
			_forEnvironment->_name=_strdup("for loop");
			// better wait with pushing until _forEnvironment is initialized appropriately
			bool forEnvironmentInitialized=addVariable(_forEnvironment,"$",VT_UNDEFINED,false)&&addVariable(_forEnvironment,"_",VT_INTEGER,false)&&setValue(_forEnvironment,"_",_getIntegerValue(0));
			if(forEnvironmentInitialized){
				if(pushExecutionEnvironment(_forEnvironment)){
					// evaluate the initialization inside the for environment once
					if(_initializationTokenValue){
						_forEnvironment->expressionToken=_initializationTokenValue->value._token;
						Mvalue* initializationValue=getValueOfExpression("for initialization",'i',(TokenType[]){},0); // return value NOT imported
						// any map is used to initialize as local variables (just like we did in defining functions)
						// interestingly any text can be used to variables (outside the identifiers allowed by the interpreter)
						// although perhaps we should exclude using $ and _ well especially _
						if(initializationValue&&initializationValue->type==VT_MAP&&!isExecutionEnvironmentInitialized(_forEnvironment,initializationValue->value._map)){
							outputError("Failed to initialize the for loop local variables");
							forEnvironmentInitialized=false;
						}
					}
					if(forEnvironmentInitialized){
						Mvalue *_forBodyValue=NULL,*_forIncrementValue=NULL;
						while(true){
							// evaluate the condition
							_forEnvironment->expressionToken=_conditionTokenValue->value._token;
							Mvalue* _conditionValue=getValueOfExpression("for condition",'f',(TokenType[]){},0);
							if(isValueZero(_conditionValue)==M_TRUE)break; // condition evaluates to zero
							// increment the implicit loop counter variable BEFORE executing the loop AFTER evaluating the condition
							setValue(_forEnvironment,"_",_getIntegerValue(getValue(_forEnvironment,"_")->value._integer->ll+1));
							if(amVerbose()){
								outputValue("For loop condition in iteration #",getValue(_forEnvironment,"_"),NULL);
								outputValue(" evaluates to '",_conditionValue,"'.\n");
							}
							if(_forbodyTokenValue){
								// evaluate the for body
								_forEnvironment->expressionToken=_forbodyTokenValue->value._token;
								_forBodyValue=getValueOfExpression("for loop",'l',(TokenType[]){},0);
								if(amVerbose()){
									outputValue("For loop body in iteration #",getValue(_forEnvironment,"_"),NULL);
									outputValue(" evaluates to '",_forBodyValue,"'.\n");
								}
							}
							if(_incrementTokenValue){
								// evaluate the increment
								_forEnvironment->expressionToken=_incrementTokenValue->value._token;
								_forIncrementValue=getValueOfExpression("for increment",'i',(TokenType[]){},0);
								if(amVerbose()){
									outputValue("For loop increment in iteration #",getValue(_forEnvironment,"_"),NULL);
									outputValue(" evaluates to '",_forBodyValue,"'.\n");
								}
							}
						}
						_result=getValue(_forEnvironment,"$"); // get the result
						if(!_result)_result=getValue(_forEnvironment,"_"); // just return the value of the counter if $ was not set!!
					}
					popExecutionEnvironment(); // pop the for execution environment (freeing it in the process)
				}else{
					outputError("Failed to activate the for loop execution environment");
					forEnvironmentInitialized=false;
				}
			}else
				output("Failed to add or initialize the for loop result and counter local variables $ and _.\n",ERROR_PREFIX);
			if(!forEnvironmentInitialized)free_environment(_forEnvironment); // have to free the environment myself
		}else
			outputError("Failed to create the for execution environment");
	}
	return _result;
}

// NOTE by adding endTokenType and maximumNumberOfElements to getListExpressionValue we can use it as well for getting an arguments list...
Mvalue* getValueOfList(TokenType endTokenType,uint32_t maximumNumberOfElements,uint32_t numberOfElementsToNotEvaluate,bool weak){
	Mtoken* expressionToken=getEnvironmentExpressionToken(); // does NOT need to be freed, so no _ in front of it!
	if(amVerbose())output("Composing a list of %u elements with %u unevaluatable elements starting with '%s'.\n",maximumNumberOfElements,numberOfElementsToNotEvaluate,string(expressionToken->text));
	// MDH@21MAY2019: _getListValue() as opposed to getValueOfExpressionOfType() creates a Mvalue on the value list which will be removed when the reference count of the Mvalue list ends up being 0
	//                then, the list element values will be dereferenced and if their reference count becomes zero freed as well successfully!!!!
	Mvalue* _listValue=_getListValue(VT_UNDEFINED,weak); // replacing: getValueOfExpressionOfType(VT_LIST);
	Mlist* _list=_listValue->value._list; // grab the (empty) list to fill
	if(!_list){output("Failed to create a list to return.\n");return NULL;}
	if(_list->_first||_list->_last){output("Supposedly empty list not initialized correctly.\n");return NULL;}
	if(amVerbose())output("Composing a list starting with token '%s' of type '%s'.\n",string(expressionToken->text),TOKENTYPE_STRING[expressionToken->type]);
	///////enum TOKENTYPE_ENUM listElementEndTokenTypes[]={TT_END_OF_LIST,TT_LISTELEMENT};
	// we iterate over the list elements, so at the start we assume expressionToken represents the start token of the list (literal)
	unsigned long long listElementIndex=0;
	uint32_t firstElementToNotEvaluate=(maximumNumberOfElements==0||numberOfElementsToNotEvaluate>maximumNumberOfElements?0:maximumNumberOfElements-numberOfElementsToNotEvaluate+1);
	if(amVerbose())output("First element not to evaluate: %u.\n",firstElementToNotEvaluate);
	Mtoken* expr=expressionToken; // we need this when we are not to evaluate a list element, this will match the expr of all comma's and the list end token
	// keep advancing the expression token until we're out of them (MDH@17JUL2019: now getting them from the current execution environment)
	while((expressionToken=nextEnvironmentExpressionToken())){
		if(expressionToken->type==endTokenType)break; // missing elements should be skipped but counted
		listElementIndex++;
		if(amVerbose())output("Processing list element #%llu starting with token '%s' of type '%s'.\n",listElementIndex,string(expressionToken->text),TOKENTYPE_STRING[expressionToken->type]);
		Mvalue* _listElementValue=NULL;
		if(firstElementToNotEvaluate>0&&listElementIndex>=firstElementToNotEvaluate){ // copy the tokens in the argument
			// it's easier to tell getValueOfExpression not to evaluate the tokens and make it copy them by passing in a boolean flag
			// however this would require passing the bool argument along to every function getValueOfExpression calls
			// so it's easier to find where this list element ends by checking expr on a list element or end of list we encounter in forward direction
			Mtoken* _firstUnevaluatedToken=_getEvaluatableTokenCopy(expressionToken);
			if(_firstUnevaluatedToken){
				if(amVerbose())
				output("Evaluating special function call argument tokens:");
				Mtoken* unevaluatedToken=_firstUnevaluatedToken;
				while(unevaluatedToken){
					if(amVerbose())
					output(" %s(%" PRId32 ")",string(unevaluatedToken->text),unevaluatedToken->argument);
					expressionToken=nextEnvironmentExpressionToken();
					if(!expressionToken)break; // NOTE shouldn't happen though
					if(!expressionToken->expr||expressionToken->expr==expr)if(expressionToken->type==endTokenType||expressionToken->type==TT_LISTELEMENT)break;
					unevaluatedToken->next=_getEvaluatableTokenCopy(expressionToken); // set next to the copy of the expression token
					unevaluatedToken=unevaluatedToken->next;
				}
				if(amVerbose())
				outputChar('\n');
				_listElementValue=_getValueOfToken(_firstUnevaluatedToken,true);
			}
		}else{ // evaluate
			// theoretically it is possible that this list element is empty in which case we should append NULL to the list
			_listElementValue=(expressionToken->type!=TT_LISTELEMENT?getValueOfExpression("list element",'l',(TokenType[]){endTokenType,TT_LISTELEMENT},2):NULL);
			expressionToken=getEnvironmentExpressionToken(); // essential after calling any function that might advance the current token pointer
			if(amVerbose())outputValue("List element value: '",_listElementValue,"'.\n");
		}
		if(!_listElementValue){if(amVerbose())output("List element missing!\n");continue;} // undefined list elements should NEVER be added to the list
		if(expressionToken)if(amVerbose())output("List element ending token: %s.\n",TOKENTYPE_STRING[expressionToken->type]);
		// get the next list element value, here's a problem as we're supposed to return the offset not the first token
		// if we already have the maximum number of elements, we do not append this list element!!!
		// we're NOT using the number of elements in the list to check agains anymore but the list element index
		if(maximumNumberOfElements==0||listElementIndex<=maximumNumberOfElements){
			if(amVerbose())output("Appending list element #%llu.\n",listElementIndex);
			unsigned long long newListElementIndex=appendedToList(_list,_listElementValue,listElementIndex);
			// MDH@21MAY2019 IMPORTANT: because NULL list elements are NOT stored explicitly in the list (because a list is stored sparse), the list index should be passed in
			if(newListElementIndex==0){
				output("%s",ERROR_PREFIX);
				outputValue("Failed to append list element '",_listElementValue,"'.\n");
				break;
			}
			if(amVerbose())output("List element #%lld appended to list with index %lld!\n",listElementIndex,newListElementIndex);
		}else
		if(amVerbose())output("Maximum number of elements reached.\n");
		if(!expressionToken)break; // MDH@15OCT2019: might be useful!! TODO how can we prevent this from happening????????
		if(expressionToken->type==endTokenType)break; // the list element could have ended with the end token type, in which case we're done!!!
	}
	if(amVerbose())outputValue("List '",_listValue,"' extracted!\n");
	return _listValue;
}

Mvalue* getValueOfMap(){
	Mtoken* expressionToken=getEnvironmentExpressionToken(); // MDH@17JUL2019: one of five functions that use and advance the current expression token
	Mvalue* _mapValue=_getMapValue(VT_UNDEFINED,false); // MDH@21MAY2019 for the same reason as above: replacing: getValueOfExpressionOfType(VT_MAP);
	Mmap* _map=_mapValue->value._map; // grab the map to fill
	//enum TOKENTYPE_ENUM mapAttributeNameEndTokenTypes[]={TT_MAP_VALUE,TT_END_OF_MAP,TT_LISTELEMENT};
	//enum TOKENTYPE_ENUM mapAttributeValueEndTokenTypes[]={TT_END_OF_MAP,TT_LISTELEMENT};
	// NOTE a map can be empty in which case _firstToken will immediately be of type TT_END_OF_MAP
	while((expressionToken=nextEnvironmentExpressionToken())){
		if(expressionToken->type==TT_END_OF_MAP)break;
		if(expressionToken->type==TT_LISTELEMENT)continue; // missing attribute name-value pair
		// get the next attribute name, value pair
		// obviously the name should be something that evaluates to a string
		Mvalue* _attributeNameValue=getValueOfExpression("map attribute name",'s',(TokenType[]){TT_MAP_VALUE,TT_END_OF_MAP,TT_LISTELEMENT},3);
		expressionToken=getEnvironmentExpressionToken(); // essential after calling a function that might advance the current expression token
		// MDH@22JUL2019: it's better to dequote the name here because otherwise the name of the attribute would be in quotes (and it is clear to be text)
		Mstring* _attributeName=_getValueText(_attributeNameValue,true); // parse the attribute name value (could be undefined though)
		// NOTE _attributeNameValue will be released after evaluation because it is not assigned to something else...
		/////////////////////if(expressionToken->type==TT_END_OF_MAP)break;
		// for now let's decide to simply not store the attribute if the name is not of type string
		Mvalue* _attributeValueValue=NULL;
		if(expressionToken->type==TT_MAP_VALUE){
			expressionToken=nextEnvironmentExpressionToken(); // move to first element after the colon
			_attributeValueValue=getValueOfExpression("map attribute value",'v',(TokenType[]){TT_END_OF_MAP,TT_LISTELEMENT},2);
			expressionToken=getEnvironmentExpressionToken(); // essential after calling a function that might advance the current expression token
		}
		if(!_attributeName)continue; // unable to parse the attribute name expression value into a string
		// MDH@22JUL2019: let's allow empty attribute name as well (why not!)
		////////if(string_length(_attributeName)>0)
		if(!appendedToMap(_map,string(_attributeName),_attributeValueValue)){
			output("%s",ERROR_PREFIX);outputValue("Failed to append the value of attribute '",_attributeNameValue,"'.\n");
		} // NOTE can't break until we actually bump into the TT_END_OF_MAP!!!
		free_string(_attributeName); // ALWAYS free the name text
		if(!expressionToken)break;
		if(expressionToken->type==TT_END_OF_MAP)break;
		if(amVerbose())output("Continued map parsing with token of type '%s'.\n",TOKENTYPE_STRING[expressionToken->type]);
	}
	return _mapValue;
}

// a function call needs a function and a map of arguments (defining the values to use for the formal parameters of the function)
Mvalue* getValueOfFunctionCall(Mfunction* _function,char* functionName,Mmap* _argumentMap){
	////////Mvalue* _resultValue=NULL;
	switch(_function->type){
		case FT_USER:
			{
				// TODO replace following by calling getFunctionExecutionEnvironment
				// 1. create an environment in which to execute the expression list of the given function initialized with the argument map provided with the current argument variable values
				Menvironment* _functionExecutionEnvironment=_getFunctionExecutionEnvironment(_function,functionName,_argumentMap);
				if(_functionExecutionEnvironment){
					if(pushExecutionEnvironment(_functionExecutionEnvironment)){
						// execute ALL the commands in _bodyCommandList
						Mlist* functionBodyCommandList=_function->functionunion._userfunction->_bodyCommandList;
						Mvalue *functionEvaluationValue=NULL,*functionBodyCommandValue=NULL;
						if(functionBodyCommandList){
							Mlistelement* functionBodyCommandListelement=functionBodyCommandList->_first;
							while(functionBodyCommandListelement){
								// MDH@22JUL2019: ALWAYS skip the initial dummy TT_EXPRESSION token of any command!!
								_functionExecutionEnvironment->expressionToken=functionBodyCommandListelement->_value->value._token->next;
								// evaluate the body command and remember the result
								functionBodyCommandValue=getValueOfExpression("function body command evaluation",'f',(TokenType[]){},0);
								// MDH@24JUL2019: check the function exit flag variable if it is set we're done
								if(getValue(_functionExecutionEnvironment,"!"))break; // the exit variable is set (by the return statement!!!!)
								functionEvaluationValue=functionBodyCommandValue; // store command evaluation result as function result
								if(amVerbose())outputValue("Function evaluation value so far: '",functionEvaluationValue,"'.\n");
								functionBodyCommandListelement=functionBodyCommandListelement->_next;
							}
						}else
							output("No commands in body of user function '%s' to execute!\n",functionName);
						// before popping the function execution environment, see if the result was set
						if(amVerbose())output("Extracting the result of the execution of function '%s'.\n",functionName);
						Mvalue* functionResultValue=getValue(_functionExecutionEnvironment,"$");
						if(amVerbose())output("Exiting the environment of executing function '%s'.\n",functionName);
						popExecutionEnvironment();
						// the function result value (if set) takes precedence over the function evaluation value
						return (functionResultValue?functionResultValue:functionEvaluationValue);
					}else{
						output("%sFailed to create the function execution environment of function '%s'.\n",ERROR_PREFIX,functionName);
						free_environment(_functionExecutionEnvironment);
					}
				}else
					output("%sFailed to create the environment to execute function '%s'.\n",ERROR_PREFIX,functionName);
				return NULL;
			}
			break;
		case FT_INTERNAL_NO_ARGUMENTS:
			if(amVerbose())output("Calling no-argument function '%s'.\n",functionName);
			return (*_function->functionunion.noArgumentFunction)();
		case FT_INTERNAL_ONE_ARGUMENT:
			if(amVerbose()){output("Applying one-argument function '%s'",functionName);outputValue(" to '",_argumentMap->_first->_variable->_value,"'.\n");}
			return (*_function->functionunion.oneArgumentFunction)(_argumentMap->_first->_variable->_value);
		case FT_INTERNAL_TWO_ARGUMENTS:
			{
				Mmapelement* _firstArgumentmapelement=_argumentMap->_first;
				Mmapelement* _secondArgumentmapelement=(_firstArgumentmapelement?_firstArgumentmapelement->_next:NULL);
				if(amVerbose()){
					output("Applying two-argument function '%s'",functionName);
					if(_firstArgumentmapelement)outputValue(" to '",_firstArgumentmapelement->_variable->_value,"'");
					if(_secondArgumentmapelement)outputValue(" and '",_secondArgumentmapelement->_variable->_value,"'");
					outputChar('.');outputChar('\n');
				}
				return (*_function->functionunion.twoArgumentFunction)((_firstArgumentmapelement?_firstArgumentmapelement->_variable->_value:NULL)
																	  ,(_secondArgumentmapelement?_secondArgumentmapelement->_variable->_value:NULL));
			}
		case FT_INTERNAL_THREE_ARGUMENTS:
			{
				Mmapelement* _firstArgumentmapelement=_argumentMap->_first;
				Mmapelement* _secondArgumentmapelement=(_firstArgumentmapelement?_firstArgumentmapelement->_next:NULL);
				Mmapelement* _thirdArgumentmapelement=(_secondArgumentmapelement?_secondArgumentmapelement->_next:NULL);
				if(amVerbose()){
					output("Applying three-argument function '%s'",functionName);
					if(_firstArgumentmapelement)outputValue(" to '",_firstArgumentmapelement->_variable->_value,"'");
					if(_secondArgumentmapelement)outputValue(" and '",_secondArgumentmapelement->_variable->_value,"'");
					if(_thirdArgumentmapelement)outputValue(" and '",_thirdArgumentmapelement->_variable->_value,"'");
					outputChar('.');outputChar('\n');
				}
				return (*_function->functionunion.threeArgumentFunction)((_firstArgumentmapelement?_firstArgumentmapelement->_variable->_value:NULL)
																		,(_secondArgumentmapelement?_secondArgumentmapelement->_variable->_value:NULL)
																		,(_thirdArgumentmapelement?_thirdArgumentmapelement->_variable->_value:NULL));
			}
		case FT_INTERNAL_FOUR_ARGUMENTS:
			{
				Mmapelement* _firstArgumentmapelement=_argumentMap->_first;
				Mmapelement* _secondArgumentmapelement=(_firstArgumentmapelement?_firstArgumentmapelement->_next:NULL);
				Mmapelement* _thirdArgumentmapelement=(_secondArgumentmapelement?_secondArgumentmapelement->_next:NULL);
				Mmapelement* _fourthArgumentmapelement=(_thirdArgumentmapelement?_thirdArgumentmapelement->_next:NULL);
				if(amVerbose()){
					output("Applying four-argument function '%s'",functionName);
					if(_firstArgumentmapelement)outputValue(" to '",_firstArgumentmapelement->_variable->_value,"'");
					if(_secondArgumentmapelement)outputValue(" and '",_secondArgumentmapelement->_variable->_value,"'");
					if(_thirdArgumentmapelement)outputValue(" and '",_thirdArgumentmapelement->_variable->_value,"'");
					if(_fourthArgumentmapelement)outputValue(" and '",_fourthArgumentmapelement->_variable->_value,"'");
					outputChar('.');outputChar('\n');
				}
				return (*_function->functionunion.fourArgumentFunction)((_firstArgumentmapelement?_firstArgumentmapelement->_variable->_value:NULL)
																		,(_secondArgumentmapelement?_secondArgumentmapelement->_variable->_value:NULL)
																		,(_thirdArgumentmapelement?_thirdArgumentmapelement->_variable->_value:NULL)
																		,(_fourthArgumentmapelement?_firstArgumentmapelement->_variable->_value:NULL));
			}

	}
	return NULL;
}

/**
 * MDH@Jacky=65yrs:
 * getValueOfExpression() returns the value of the tokens behind _offsetToken together with the token that ends the expression in an Mexpressionvalue*
 * the general idea is to make it recursive so it delegates getting specific subvalues from getValueOfExpression()
 * let's analyze evaluating an expression:
 * an expression 'evaluates' to a value means that we have to apply functions (or unary operators) to arguments, and binary operators to arguments as well
 * this value can also be a composite value like a list or a map, nevertheless a list or a map is a single value
 * it makes sense to delegate getting a list or a map literal to another function
 * NOTE getValueOfExpression() knows nothing about the type of expression it is processing, so it has to check whether to delegate or not
 *      however the general structure would be: <value><binary operator><value> or perhaps <value><ternary operator><value> but the idea is the same
 *      we could store these parts in elements of a list, where operator is stored as string and value as Mvalue*, so technically simply a list of Mvalue's so an Mlist*
 *      we can call these operands and operators or perhaps expressionelements??????
 * 			at the end the operators would need to be removed from the expressionelements array and we'd end up with a single value as result...
 * 		  if the resulting value is an variable, we should return the value of the variable, technically this means that an expressionelement cannot be a variable that makes sense
 *      so I guess we should only accept assignments at the start of an expression (which makes perfect sense)
 */

/**
 * \brief wraps \p _value
 * \param _value the Mvalue to wrap
 */
Mvaluereference* _getValuereference(Mvalue* _value){
	if(amVerbose())outputValue("Wrapping value '",_value,"'.\n");
	Mvaluereference* _valuereference=(Mvaluereference*)calloc(1,sizeof(Mvaluereference));
	_valuereference->_value=_value; // MDH@02NOV2019 replacing: assignValue(&_valuereference->_value,_value);
	if(amVerbose())outputValue("Value '",_value,"' wrapped in value reference.\n");
	return _valuereference;
}
/* MDH@26OCT2019: moved over to Mvalue.h/c as we need it there so we can have value references as well!!!!!
void free_valuereference(Mvaluereference* _valuereference){
	if(_valuereference){
		if(_valuereference->_name)free(_valuereference->_name);
		// values themselves are never freed!!!
		if(_valuereference->_value)decrementReferenceCount(_valuereference->_value);
		if(_valuereference->_itemid)decrementReferenceCount(_valuereference->_itemid);
		free(_valuereference);
	}
}
*/
void outputValuereference(char* prefix,Mvaluereference* _valuereference,char* suffix){
	if(prefix)output("%s",prefix);
	if(_valuereference){
		if(_valuereference->_name)output("%s",_valuereference->_name);
		if(_valuereference->_itemid)outputValue(NULL,_valuereference->_itemid,NULL);
		if(_valuereference->_value)outputValue("='",_valuereference->_value,"'");
	}
	if(suffix)output("%s",suffix);
}
// two essential methods for getting and setting referenced values
Mvalue* getReferencedValue(Mvaluereference* _valuereference){
	// _itemid now represents the entire list of index/attribute name combinations
	if(!_valuereference)return NULL;
	Mvalue* referencedValue=NULL;
	///////if(amVerbose())outputValuereference("ZZZZZZZZZZ Requesting the value of value reference '",_valuereference,"'.\n");
	if(!_valuereference->_value){ // not an actual (preset) value
		// if we do NOT have a name it's a literal
		if(_valuereference->_name){
			// MDH@04NOV2019: now that we've added the TT_REFERENCE token, the name may start with @ to indicate a variable reference
			if(_valuereference->_name[0]=='@'){ // a reference to a variable which we need to leave as is i.e. wrap it inside a value
				// I suppose we need to wrap a copy unless we make a separate reference thing where we store the name of the variable which could just be an Mstring?????
				Mvariable* variable=getVariable(NULL,&_valuereference->_name[1],false);
				if(variable)referencedValue=_getReferenceValue(_getReference(variable),true);else output("%sReferenced variable '%s' vanished.\n",ERROR_PREFIX,_valuereference->_name[1]);
			}else{ // a non-referenced variable which means we are supposed to return the value of the variable
				// if there is no itemid we simply return the 'entire' value of the given variable
				Mvalue* _value=getValue(getEnvironment(),_valuereference->_name); // the value at the top level
				if(amVerbose()){output("Current value of referenced variable '%s': ",_valuereference->_name);outputValue("'",_value,"'.\n");}
				// if we have index/attribute names we have to get the final subvalue
				if(_valuereference->_itemid){
					if(amVerbose()){output("Item id of indexed variable '%s'",_valuereference->_name);outputValue(": '",_valuereference->_itemid,"'.\n");}
					Mlist* _itemidlist=_valuereference->_itemid->value._list; // let's assume that is it always a list
					// empty lists should also return the full element, so only something to do when we actually have list elements!!!
					if(_itemidlist->_first){
						// let's get the first index/attribute name
						Mlistelement* indexorattributenameListelement=_itemidlist->_first;
						Mvalue* indexorattributenameListelementValue;
						while(indexorattributenameListelement){
							indexorattributenameListelementValue=indexorattributenameListelement->_value;
							// after extracting the value increment indexorattributenameListelement, so we can use continue
							indexorattributenameListelement=indexorattributenameListelement->_next;
							// if no value is defined, it is ignored TODO should we????
							if(indexorattributenameListelementValue){
								if(amVerbose())outputValue("Index or attribute list element value: '",indexorattributenameListelementValue,"'.\n");
								// if we are accessing a map we have to ascertain that the attribute name in a string
								if(_value->type==VT_MAP){
									Mstring* attributenameText=_getValueText(indexorattributenameListelementValue,true); // TODO should we dequote??
									if(attributenameText){
										referencedValue=getValueOfAttribute(_value->value._map,string(attributenameText));		
										free_string(attributenameText);
										continue;	
									}
									output("%s",ERROR_PREFIX);
									outputValue("Failed to convert assumed attribute name '",indexorattributenameListelementValue,"' to text.\n");		
								}
								if(_value->type==VT_LIST){
									// MDH@17OCT2019: how about allowing an index to be a list of indices????
									long long index;
									if(indexorattributenameListelementValue->type==VT_LIST){
										// we'll be returning a list value
										Mlist* _referencedValueList=_getListOfType(_value->value._list->valuetype);
										Mlist* indexelementList=indexorattributenameListelementValue->value._list;
										Mlistelement* indexelementListelement=indexelementList->_first;
										Mvalue* valueAtIndex;
										while(indexelementListelement){
											index=getValueInteger(indexelementListelement->_value);
											valueAtIndex=(index!=0&&index!=M_LL_INVALID?getValueAtIndex(_value->value._list,index):NULL);
											// OOPS can't append with 0 anymore, because 0 will do prepending
											if(appendedToList(_referencedValueList,valueAtIndex,M_LL_INVALID)==0)break;
											indexelementListelement=indexelementListelement->_next;
										}
										referencedValue=_getValueOfList(_referencedValueList,true);
									}else{
										// try to convert the index value into a positive integer
										long long index=getValueInteger(indexorattributenameListelementValue);
										if(index!=0&&index!=M_LL_INVALID){
											referencedValue=getValueAtIndex(_value->value._list,index);
											continue;
										}
										if(index){
											output("%s",ERROR_PREFIX);
											outputValue("Assumed index '",indexorattributenameListelementValue,"' does not represent an integer.\n");
										}else
											outputError("A zero index is not allowed");
									}
								}
								// neither a list nor a map, so nothing to return!!!
								////////////////////////return NULL;
								/* replacing:
								// check the validity of the index or attribute name against the current value
								if(indexorattributenameListelementValue->type!=VT_INTEGER&&indexorattributenameListelementValue->type!=VT_TEXT){outputValue("\nAssumed index/attribute name '",indexorattributenameListelementValue,"' not an integer/string.");return NULL;}
								if(indexorattributenameListelementValue->type==VT_INTEGER){
									if(_value->type!=VT_LIST){outputValue("ERROR: Value '",_value,"' not a list.");return NULL;}
									_value=getValueAtIndex(_value->value._list,indexorattributenameListelementValue);
								}else{
									if(_value->type!=VT_MAP){outputValue("ERROR: Value '",_value,"' not a map.");return NULL;}
									_value=getValueOfAttribute(_value->value._map,indexorattributenameListelementValue);
								}
								*/
							}
						}
						if(amVerbose())outputValue("Value of indexed variable: '",_value,"'.\n");
					}
				}
			}
		}
	}else{
		if(amVerbose())outputValue("Returning referenced value: '",_valuereference->_value,"'.\n");
		referencedValue=_valuereference->_value; // MDH@02NOV2019 replacing: assignValue(&referencedValue,_valuereference->_value); // TODO must we use assignValue here??????????
	}
	///////if(amVerbose()){outputValuereference("ZZZZZZZ Value of value reference '",_valuereference,"'");outputValue(": '",referencedValue,"'.\n");}
	return referencedValue;
}
// when assigning, we're supposed to assign to something with a variable name (and optional index/attribute name list) associated with it
bool setReferencedValue(Mvaluereference* _valuereference,Mvalue* _newValue){
	bool result=false;
	if(_valuereference&&_valuereference->_name){
		if(amVerbose()){output("Setting the value reference of '%s'",_valuereference->_name);outputValue(" to '",_newValue,"'.\n");}
		// MDH@18OCT2019: without an _itemid the variable is allowed to NOT yet exist
		if(_valuereference->_itemid){ // the hard part: index/attribute name list assignment!!
			Mvalue* _value=getValue(getEnvironment(),_valuereference->_name); // we'll be needing the value at the top level to start with!!!!
			if(_value&&(_value->type==VT_LIST||_value->type==VT_MAP)){
				result=true;
				if(amVerbose())outputLine("Element to set.");
				Mlist* _itemidlist=_valuereference->_itemid->value._list; // let's assume that is it always a list
				// let's get the first index/attribute name
				Mlistelement* indexorattributenameListelement=_itemidlist->_first;
				// MDH@18OCT2019: we now allow a list that is empty (indicative of appending to the list), in that case indexorattributenameListelement would be NULL
				//                this works for lists not for maps
				if(indexorattributenameListelement||_value->type==VT_LIST){ // we've got one, so not an empty index/attribute name list!!
					// we need to find the last index or attribute name
					Mvalue* indexorattributenameListelementValue;
					if(indexorattributenameListelement) // MDH@18OCT2019: might NOT happen now (on lists that is), so we need to test for that!!!
					while(indexorattributenameListelement->_next){
						indexorattributenameListelementValue=indexorattributenameListelement->_value;
						indexorattributenameListelement=indexorattributenameListelement->_next; // immediately increment
						// if no value is defined, it is ignored TODO should we????
						if(indexorattributenameListelementValue){
							// if we are accessing a map we have to ascertain that the attribute name in a string
							if(_value->type==VT_MAP){
								Mstring* attributenameText=_getValueText(indexorattributenameListelementValue,true);
								if(attributenameText){
									_value=getValueOfAttribute(_value->value._map,string(attributenameText));		
									free_string(attributenameText);
									continue;	
								}
								outputValue("\nERROR: Failed to convert assumed attribute name '",indexorattributenameListelementValue,"' to text.");		
							}
							if(_value->type==VT_LIST){
								if(indexorattributenameListelementValue->type==VT_INTEGER){
									_value=getValueAtIndex(_value->value._list,indexorattributenameListelementValue->value._integer->ll);
									continue;
								}
								outputValue("\nERROR: Assumed index '",indexorattributenameListelementValue,"' not an integer.");
							}
							// neither a list nor a map, so nothing to return!!!
							break;
						}
					}
					// now indexorattributenameListelement should point to the last index/attribute name and _value at the list/map to change
					if(_value->type==VT_MAP){
						Mstring* _attributeName=_getValueText(indexorattributenameListelement->_value,true);
						if(!appendedToMap(_value->value._map,string(_attributeName),_newValue)){
							result=false;
						}
						free_string(_attributeName);
						if(!result)return false;
					}else
					if(_value->type==VT_LIST){
						// NOTE allow appending using 0 or inserting with negative values
						// MDH@18OCT2019: we now have four situations: 0=prepend, NULL=append, negative integers=set from the back (-1=last element)
						//                so if no list element is defined, we just append to the list!!!!
						//                the only invalid situations is when the _value is NULL although it still could NOT denote an integer
						long long index=(indexorattributenameListelement?getValueInteger(indexorattributenameListelement->_value):M_LL_INVALID);
						// replace the index to the actual index with the index of the element in the list (so getReferencedValue() will not complain!!!)
						if(!indexorattributenameListelement||index!=LLONG_MIN){
							index=appendedToList(_value->value._list,_newValue,index);
							// MDH@18OCT2019: why are we doing this????? i.e. is the value in the list still pointing somewhere??????
							if(index>0){
								if(indexorattributenameListelement)
								assignValue(&indexorattributenameListelement->_value,_getIntegerValue(index));
							}else
								result=false;	
						}else
							result=false;
					}
				}else
				if(_value->type!=VT_LIST)
					outputError("No index/attribute name specified");
			}else
				output("%sReferenced variable '%s' cannot be indexed: it's value is not a list or a map.\n",ERROR_PREFIX,_valuereference->_name);
		}else
		if(setValue(getEnvironment(),_valuereference->_name,_newValue)){
			// NOTE even if the value itself is NULL, its address is never NULL
			_valuereference->_value=_newValue; // MDH@02NOV2019 replacing: assignValue(&_valuereference->_value,_newValue);
			result=true;
			if(amVerbose())outputLine("Value set!");
		}
		// MDH@20JUL2019: here when we succeed in performing the assigment, we should update the value reference as well!!!!
	}
	return result;
}

// MDH@24OCT2019: we need a method that can convert a big integer to an integer
long long getBigintegerInteger(Mbiginteger* biginteger){
	long long result=M_LL_INVALID;
	if(biginteger)if(mp_cmp(biginteger,getBigintegerLLMin())!=MP_LT&&mp_cmp(biginteger,getBigintegerLLMax())!=MP_GT)result=mp_get_i64(biginteger);
	return result;
}

Mvalue* applyUnaryOperator(char operator,Mvalue* _value){
	if(amVerbose()){
		output("Applying unary operator '%c'",operator);
		if(_value){outputValue(" to value '",_value,"'");output(" of type %u.\n",_value->type);}else output(".\n");
	}
	// delegating to the one argument functions that we have is best!!!
	switch(operator){
		case '~':return Mbnot(_value);
		case '!':return Mnot(_value);
		case '-':return Mneg(_value);
		case '+':return _value;
	}
	return NULL;
}

/**
 * getValueReference() retrieves a single value reference that either ends when a binary operator token is encountered or one of the end token types
 * a value reference syntax: optionally a number of unary operators, optionally followed by function call with arguments, and variable or value literal
 * we need to store the value in a value reference just in case the value is the destination of an assignment, so yes, reference is an apt name
*/
Mvaluereference* getValueReference(char* info,TokenType endTokenTypes[],uint8_t endTokenTypeCount){
	Mtoken* expressionToken=getEnvironmentExpressionToken();

	Mvaluereference* _valueReference=NULL;

	if(amVerbose())output("getValueReference() extracting a(n) '%s' value that starts with token '%s' of type '%s'.\n",info,string(expressionToken->text),TOKENTYPE_STRING[expressionToken->type]);

	Mstring* unaryOperators=NULL; // a value starts with a number (zero or more) of unary operators
		
	while(expressionToken&&expressionToken->type==TT_UNARY){
		char unaryOperatorChar=string_char(expressionToken->text,0);
		if(unaryOperatorChar!='+'){
			if(!unaryOperators)unaryOperators=__string();
			string_append_char(unaryOperators,unaryOperatorChar);
		}
		expressionToken=nextEnvironmentExpressionToken();
	}
	if(amVerbose()){if(unaryOperators)output("Unary operators: '%s'.\n",string(unaryOperators));else output("No unary operators!\n");}
	// ASSERT unary operators extracted

	if(expressionToken){
		if(amVerbose())output("getValueReference() interpreting first value token '%s' of type %s.\n",string(expressionToken->text),TOKENTYPE_STRING[expressionToken->type]);
		_valueReference=(Mvaluereference*)CALLOC(1,sizeof(Mvaluereference),'R');
		// expecting either a function (call), (new) variable or (integer, real, string, list or map) literal
		/* NO we can NOT change the tokens themselves (to keep them editable!!!)
		if(expressionToken->type==VT_INTEGER){
			if(expressionToken->next&&expressionToken->next->type==VT_FLOAT){
				expressionToken=expressionToken->next;
				// let's prepend the integer token text to the real (fraction) token text
				string_prepend(string(expressionToken->prev->text),expressionToken->text);
			}
		}
		*/
		char* _significantTokenText=_stringstart(expressionToken->text,expressionToken->significantCharacterCount);
		switch(expressionToken->type){
			case TT_FUNCTION:
				{
					if(amVerbose())output("Call of function '%s'.\n",_significantTokenText);
					Mfunction* function=getFunction(getEnvironment(),_significantTokenText); // get the function associated with the name of the function
					if(function){
						// MDH@17JUL2019: we know the function and when the name is one of the special functions
						//                like 'function' to define a function we know not to evaluate the third argument!!
						//                it's easiest to define first element not to evaluate (i.e. to store the tokens in the list)
						// MDH@25JUL2019: adding if, while and for functions
						unsigned long long numberOfFunctionParameters=function->_parameterMap->numberOfElements,numberOfElementsToNotEvaluate=0;
						if(!strcmp(_significantTokenText,DEFINEUSERFUNCTION_NAME)){
							if(amVerbose())outputLine("Definition of a user function encountered!");
							numberOfElementsToNotEvaluate=1;		
						}else
						if(!strcmp(_significantTokenText,IFFUNCTION_NAME)||!strcmp(_significantTokenText,WHILEFUNCTION_NAME)){
							numberOfElementsToNotEvaluate=2;
						}else
						if(!strcmp(_significantTokenText,FORFUNCTION_NAME)){ // the initialization argument should always be evaluated (once)
							numberOfElementsToNotEvaluate=4;
						}else
						if(!strcmp(_significantTokenText,DOFUNCTION_NAME)){ // all arguments to the do function should not be evaluated beforehand
							numberOfElementsToNotEvaluate=LLONG_MAX; // all elements should NOT be evaluated
							// the do function is special in that it allows an infinite number of arguments although the function itself expects them wrapped in a single Mvalue
							numberOfFunctionParameters=LLONG_MAX; // replacing 1 with the actual number of parameters we allow for the function
						}
						// 1. get the list of function arguments, which depends on the function!!
						expressionToken=nextEnvironmentExpressionToken();
						// MDH@02NOB2019: force the arguments value list to be weak
						Mvalue* _functionArgumentsValue=getValueOfList(TT_END_OF_FUNCTION_CALL,numberOfFunctionParameters,numberOfElementsToNotEvaluate,true);
						expressionToken=getEnvironmentExpressionToken(); // OOPS always update expressionToken after calling a function that might advance it
						if(_functionArgumentsValue){
							if(amVerbose())outputValue("Function argument list: '",_functionArgumentsValue,"'.\n");
							// MDH@05AUG2019: if we're dealing with the do function I have to map all the arguments to a single list value
							Mlist* functionCallArgumentList=NULL;
							if(!strcmp(_significantTokenText,DOFUNCTION_NAME)){
								// MDH@02NOV2019: making the list weak
								functionCallArgumentList=listMadeWeak(_getListOfType(VT_UNDEFINED)); // creating a list
								if(functionCallArgumentList&&!appendedToList(functionCallArgumentList,_functionArgumentsValue,M_LL_INVALID)){
									outputError("Failed to create the to do expression list");
									free_list(functionCallArgumentList);
									functionCallArgumentList=NULL; // so nothing will get done!!
								}
								// TODO what should we do with functionCallArgumentList (which we created) once we're done with it??????
								// DONE see below where it's freed as soon as we created the function call argument map, in the process decrement the reference count of all the argument list elements (Mvalues)
							}else
								functionCallArgumentList=_functionArgumentsValue->value._list; // use the wrapped list
							// 2. get the arguments map
							// MDH@02NOV2019 NOTE: this map will be weak as returned by _getFunctionArgumentMap!!
							Mmap* _functionCallArgumentMap=_getFunctionArgumentMap(function,functionCallArgumentList); // assuming to have a list returned by getListExpressionValue()
							// if this is a do() function call, we need to get rid of the single element list we created to wrap all arguments
							if(!strcmp(_significantTokenText,DOFUNCTION_NAME))free_list(functionCallArgumentList);
							/// we do not need to release the function arguments list value because it it never assigned by itself, it is simply a container for the argument list elements (which do have a reference count incremented when added to the list)
							/*
							if(amVerbose())output("Decrementing the reference count of the function arguments value!");
							decrementReferenceCount(_functionArgumentsValue); // TODO is this correct?????
							if(amVerbose())output("Reference count of the function arguments value decremented!");
							*/
							// 3. the result of applying the function to the arguments is the end result
							// MDH@19JUL2019: we need to know when a function is being created, so we can ask for the body commands in command mode
							Mvalue* functionCallValue=getValueOfFunctionCall(function,_significantTokenText,_functionCallArgumentMap);
							// if this was a call to the 'define user function' function
							if(!strcmp(_significantTokenText,DEFINEUSERFUNCTION_NAME)){ // a function being defined
								// is the result 1???
								if(functionCallValue&&functionCallValue->type==VT_INTEGER&&functionCallValue->value._integer->ll){ // function successfully created
									// let's push the function name on the stack of functions to create
									// we know the first argument contains the function name
									char* definedFunctionName=_functionCallArgumentMap->_first->_variable->_value->value._text->_c;
									Mfunction* definedFunction=getFunction(getEnvironment(),definedFunctionName);
									// if the function now exists but does not yet have a body, queue the function name on the list of bodies to be set
									if(definedFunction&&definedFunction->type==FT_USER&&!definedFunction->functionunion._userfunction->_bodyCommandList)
										requestBodyOfFunction(definedFunctionName);
									else
									if(amVerbose())output("Function '%s' completely specified with single body command!\n",definedFunctionName);
								}
							}
							//////outputValue("Function call value '",functionCallValue,"'.\n");
							_valueReference->_value=functionCallValue; // MDH@02NOV2019 replacing: assignValue(&_valueReference->_value,functionCallValue);
							// except getValueOfFunctionCall() doesn't CORRECTION can't harm can it????
							expressionToken=getEnvironmentExpressionToken(); // essential to update after calling a function that updates the expression token
							// we have to free the map ourselves (this is what the _ in front of getFunctionArgumentMap means)
							if(amVerbose()){outputValue("Function call result value: '",_valueReference->_value,"'.\n");outputLine("Freeing the function argument map!");}
							// MDH@02NOV2019: release the function call argument map to be treated as weak map (i.e. the values do not need to be dereferenced)
							free_map(_functionCallArgumentMap); // MDH@21MAY2019: no need for the function argument map anymore!!!
							if(amVerbose())outputLine("Function argument map freed!");
						}else
							outputError("No function arguments");
					}else
						output("%sFunction '%s' unknown!\n",ERROR_PREFIX,_significantTokenText);
				}
				break;
			case TT_NEW_VARIABLE: // a non-existing value reference
				// we have to create the variable first (TODO should we wait until actually assigning???)
				// NOTE in certain situations tokenizing occurs outside the evaluation environment so it could be marked as new where it will not be when evaluated
				//      therefore I've adapted addVariable() so it won't return false when the variable already exists
				// MDH@08AUG2019: any variable that's marked as new should be added to the top-level environment if it does not exist there
				//                we can make that happen by passing in NULL for getEnvironment() in which case it should check getEnvironment() only (and not all the parents as well)
				// MDH@09AUG2019: I suppose only explicit local variables (in special function calls) should not be checked to exist in parent environments, but otherwise they should
				//                we could give a warning if this variable is defined inside a special function call and is not a local variable
				////////if(amVerbose())
				if(amVerbose())output("Will add%s variable '%s'.\n",(expressionToken->argument==1?" local":""),_significantTokenText);
				if(!addVariable(expressionToken->argument==1?NULL:getEnvironment(),_significantTokenText,VT_UNDEFINED,false)){
					Mstring* _environmentName=_getEnvironmentName();
					output("%sFailed to add%s variable '%s' to environment '%s'.\n",ERROR_PREFIX,(expressionToken->argument!=1&&expressionToken->envid?" implicitly declared local":""),_significantTokenText,string(_environmentName));
					free_string(_environmentName);
					break; // NO retrieves the undefined value subsequently!!
				}
				if(amVerbose())if(expressionToken->argument!=1&&expressionToken->envid)output("WARNING: Not explicitly declared local variable '%s' encountered.\n",_significantTokenText);
			case TT_VARIABLE: // a value reference
				_valueReference->_name=_significantTokenText;_significantTokenText=NULL; // store a copy of the name of the variable being referenced
				if(amVerbose())output("Value reference variable name: '%s'.\n",_valueReference->_name);
				// NOTE do NOT assign the value of an indexed expression because it we did (as we done) the value would be returned as result and not the value at the given index
				///////////////////assignValue(&_valueReference->_value,getValue(_Menvironment,_valueReference->_name)); // store a reference to the value
				/////////////////incrementReferenceCount(_valueReference->_value); // TODO combine this with getValue to something called storeValue
				// a variable can be followed by an index that we should store in the value reference's itemid field
				if(expressionToken->next&&expressionToken->next->type==TT_LIST){
					expressionToken=nextEnvironmentExpressionToken(); // MDH@16OCT2019: why was this commented out???????
					if(amVerbose()){output("Extracting the indices.\n");}
					// MDH@02NOV2019 TODO should this be a weak or strong list????
					Mvalue* indexListValue=getValueOfList(TT_END_OF_LIST,0,0,false); // typically allow for any number of indices (although perhaps we should check!!)
					if(amVerbose()){output("XXXXXXX Index value of list '%s'",_valueReference->_name);outputValue("'",indexListValue,"'.\n");}
					expressionToken=getEnvironmentExpressionToken(); // essential after calling a function that might advance the current expression token
					if(amVerbose()){if(expressionToken){output("End of list index token: ");outputToken(expressionToken);}else output("No end of list index token!");outputChar('\n');}
					// using the indexValue we should now update the value represented up until the last index (in case we have an assignment)
					// which means that only the last index value has to be stored and the container of that last index (map or list)
					// MDH@18OCT2019: let's allow a NULL value to allow for appending to a list (as with Python)
					//                how should we treat an empty list???????? differently I guess
					//                the problem with NULL is that _itemid is NULL by itself, so this poses a problem it can't be NULL
					//                I think we'd get an empty list in return not a NULL value (which is a problem if we do!!!!)
					//                for now allow an empty list
					if(indexListValue&&indexListValue->type==VT_LIST /*&&indexListValue->value._list->_first*/){ // a non-empty list
						if(amVerbose())outputValue("Index id: '",indexListValue,"'.\n");
						// MDH@15OCT2019: apparently there is enlisting too many: we can take the first element to unlist what we received BUT this must mean there's a mistake somewhere
						// _valueReference->_itemid=indexListValue; // MDH@02NOV2019 replacing: 
						assignValue(&_valueReference->_itemid,indexListValue); //////////// NOT SURE... indexListValue->value._list->_first->_value); // now storing the entire index/attribute name list
						/* replacing (storing only the last index/attribute name):
						Mlist* indexList=indexListValue->value._list;
						Mlistelement* indexListelement=indexList->_first; // must be there!!!
						// as long as there are successors we haven't reach the last index yet!!!!
						// TODO what if someone does not specify ALL indices??????
						while(indexListelement->_next){
							// replace the current value with the value in the list (TODO map) at the current index
							assignValue(&_valueReference->_value,getValueAtIndex(_valueReference->_value->value._list,indexListelement->_value));
							indexListelement=indexListelement->_next;
						}
						if(amVerbose())outputValue("Last index: ",indexListelement->_value,"'.");
						// TODO what is going to happen to indexListValue?????? it should be discarded as its reference count will remain zero but all elements that are used elsewhere (like the last index stored in _valueReference will persist a little longer!!)
						assignValue(&_valueReference->_itemid,indexListelement->_value); // store the last index value in the _itemid field
						*/
					}else
					if(indexListValue)
						output("%sIndex of variable '%s' not a list!\n",ERROR_PREFIX,_valueReference->_name);
					else
						output("%sIndex of variable '%s' undefined!\n",ERROR_PREFIX,_valueReference->_name);	
				}else
				if(amVerbose())output("Unindexed variable '%s'!\n",_valueReference->_name);
				// MDH@29MAY2019: if we do NOT have an indexed value, retrieve the value...
				// TODO as a side-effect getReferencedValue() will bind the added value to the value reference (as result) BUT I don't think that is how it should be!!! no the assignment takes care of that
				if(!_valueReference->_itemid){
					if(amVerbose())output("Retrieving the value of '%s' when no item id was specified.\n",_valueReference->_name);
					// MDH@18OCT2019: TODO this is dangerous?!
					_valueReference->_value=getValue(getEnvironment(),_valueReference->_name);
					// MDH@02NOV2019: replacing: assignValue(&_valueReference->_value,getValue(getEnvironment(),_valueReference->_name));
				}
				if(amVerbose())outputValuereference("YYYYYYYYYYYYY Completed variable value reference: '",_valueReference,"'.\n");
				break;
			case TT_REFERENCE:
				// MDH@04NOV2019: a reference is an interesting little bugger which we unfortunately need for certain function calls like settype()
				//                for now we only allow referencing FULL variables i.e. not parts of variables like array or map elements although that seems to be a straightforward extension
				//                so it's much similar to an unindexed variable at the moment
				//                for now the only thing we're going to do is store the name of the reference (i.e. starting with @) (without value) so that whoever uses it will know how to resolve it!!!
				_valueReference->_name=_significantTokenText;_significantTokenText=NULL; // store a copy of the name of the variable being referenced
				if(amVerbose())output("Value reference referenced variable name: '%s'.\n",_valueReference->_name);
				break;			
			case TT_INTEGER: // an integer possibly followed by a real (fractional) part
				// MDH@20JUN2019: some error in the following part because every now and then we get a segmentation fault!!!!
				if(expressionToken->next&&expressionToken->next->type==TT_REAL){ // the integer part of a real
					// TODO fix this
					// first compose the full real text (with the integer text prepended to it)
					Mstring* _realText=_getString(_significantTokenText); // the integer part
					expressionToken=nextEnvironmentExpressionToken(); // now pointing to the real fraction part text following the given integer!!!!
					// OOPS do NOT add a '0' character to the token itself (as this would go wrong showing the tokens) TODO check why this goes wrong!!!
					Mstring* pRealText=_realText;
					if(pRealText){
						char* _realSignificantTokenText=_stringstart(expressionToken->text,expressionToken->significantCharacterCount); // free asap
						if(amVerbose())output("Integer part of decimal text: '%s'.\n",string(pRealText));
						pRealText=string_append(pRealText,_realSignificantTokenText);
						if(amDebugging())outputLine("Fractional part appended!");
						if(strlen(_realSignificantTokenText)==1)pRealText=string_append_char(pRealText,'0'); // a single period is NOT considered equal to zero apparently!!!!
						if(amVerbose())output("Parsing '%s' to a decimal.\n",string(pRealText));
						// MDH@13JUN2019: instead of using a rational we can now use a decimal
						//                the problem is that we need a context, and therefore a decimal precision 
						//                to this purpose I've added an integer variable in which the actual decimal precision can be set
						uint32_t l=strlen(_realSignificantTokenText); // replacing: string_length(expressionToken->text);
						free(_realSignificantTokenText); // freed!!!
						if(amDebugging())output("Real part string length: %u.\n",l);
						if(getDP()<l)output("WARNING: More decimals present in literal than expected. Rounding may occur.\n");
						if(amDebugging())outputLine("Decimal precision checked!");
						Mdecimal* _decimal=__decimal(get_default_mpd_context(),0,0);
						if(amDebugging())outputLine("Decimal created!");
						if(_decimal){
							mpd_set_string(_decimal->mpd,string(pRealText),get_default_mpd_context());
							if(amDebugging())outputLine("Decimal initialized.");
							if(!mpd_isnan(_decimal->mpd))
								assignValue(&_valueReference->_value,_getDecimalValue(_decimal,true));
							else
								outputErrorAndText("The decimal value of %s is undefined",string(pRealText));
						}else
							outputError("Failed to create a decimal");
						/* replacing:
						// MDH@07JUN2019: instead of converting the text representation to a long double 'real' we convert the decimal text representation to a rational
						Mrational* _rational=_getDecimalTextRational(string(pRealText));assignValue(&_valueReference->_value,_getRationalValue(_rational));
						*/
						// replacing: assignValue(&_valueReference->_value,_getFloatValue(_strtold(string(pRealText),getNAR())));
						if(amDebugging())outputLine("Releasing decimal text.");
						free_string(_realText);
						if(amDebugging())outputLine("Decimal text released.");
					}else
						outputError("Failed to initialize the text representation of a decimal");
				}else{ // just an integer
					// first we make a big integer, and if it fits into a VT_INTEGER that's where we put it
					Mbiginteger* _biginteger=__biginteger();
					if(mp_read_radix(_biginteger,_significantTokenText,10)==MP_OKAY){
						if(mp_cmp(_biginteger,getBigintegerLLMin())!=MP_LT&&mp_cmp(_biginteger,getBigintegerLLMax())!=MP_GT){
							_valueReference->_value=_getIntegerValue(mp_get_i64(_biginteger));
              				// MDH@02NOV2019 replacing: assignValue(&_valueReference->_value,_getIntegerValue(mp_get_i64(_biginteger)));
							free_biginteger(_biginteger);
						}else
							_valueReference->_value=_getBigintegerValue(_biginteger,true);
							// MDH@02NOV2019 replacing:	assignValue(&_valueReference->_value,_getBigintegerValue(_biginteger,true));
					}else{
						free_biginteger(_biginteger);
						outputErrorAndText("Failed to create the big integer to store integer ",_significantTokenText);
					}
				}
				break;
			case TT_REAL: // unlikely without integer part in front of it though
				_valueReference->_value=_getFloatValue(_strtold(_significantTokenText,getNAR()));
				// MDH@02NOV2019 replacing: assignValue(&_valueReference->_value,_getFloatValue(_strtold(_significantTokenText,getNAR())));
				///////////////incrementReferenceCount(_valueReference->_value); // TODO combine this with getValue to something called storeValue
				break;
			case TT_DQSTRING:
			case TT_SQSTRING: // a string literal
				_valueReference->_value=_getTextValue(_significantTokenText,false);
				// MDH@02NOV2019 replacing: assignValue(&_valueReference->_value,_getTextValue(_significantTokenText,false));
				////////////////incrementReferenceCount(_valueReference->_value); // TODO combine this with getValue to something called storeValue
				break;
			case TT_LIST: // a list literal
				_valueReference=_getValuereference(getValueOfList(TT_END_OF_LIST,0,0,false));
				break;
			case TT_MAP: // a map literal
			{
				Mvalue* _mapValue=getValueOfMap();
				expressionToken=getEnvironmentExpressionToken(); // essential after calling a function that might advance the current expression token
				if(amVerbose())outputValue("Map extracted: '",_mapValue,"'.\n");
				_valueReference=_getValuereference(_mapValue);
				break;
			}
			case TT_EXPRESSION: // an expression wrapped in parentheses which ends with a TT_END_OF_FUNCTION_CALL (although theoretically it's not an end of function call of course)
			{
				Mvalue* _expressionListValue=getValueOfList(TT_END_OF_FUNCTION_CALL,1,0,false);
				expressionToken=getEnvironmentExpressionToken(); // essential after calling a function that might advance the current expression token
				if(amVerbose())outputLine("Going to wrap the list extracted!");
				// well, actually, we need the first element of the list that is returned!!!
				// use only the first element if the list only has one element, otherwise use the list itself
				if(_expressionListValue->value._list->numberOfElements==1){
					_valueReference=_getValuereference(_expressionListValue->value._list->_first->_value);
				}else
					_valueReference=_getValuereference(_expressionListValue);
				if(amVerbose())outputLine("Extracted list wrapped!");
				break;
			}
			default:
				break;
		}
		if(_significantTokenText)free(_significantTokenText); // free the (duplicated significant) token text
		if(amVerbose()){if(_valueReference&&_valueReference->_value)outputValue("Value: `",_valueReference->_value,"`.\n");else output("No result!\n");}
		// apply the unary operators (backwards)
		if(unaryOperators){
			if(amVerbose())output("Applying unary operators: '%s'.\n",string(unaryOperators));
			uint16_t l=string_length(unaryOperators);
			while(l>0&&_valueReference->_value){
				/////////////decrementReferenceCount(_valueReference->_value);
				_valueReference->_value=applyUnaryOperator(string_char(unaryOperators,--l),_valueReference->_value);
				// MDH@02NOV2019 replacing:	assignValue(&_valueReference->_value,applyUnaryOperator(string_char(unaryOperators,--l),_valueReference->_value));
				///////////////////////if(_valueReference->_value)incrementReferenceCount(_valueReference->_value);
			}
			if(amVerbose())outputValue("Result after applying unary operators: '",_valueReference->_value,"'.\n");
		}else
			if(amVerbose())outputLine("No unary operators to apply!");
		
		// move over to the next expression token (following the end token)
		if(expressionToken)expressionToken=nextEnvironmentExpressionToken();

	}

	if(amVerbose()){
		if(_valueReference->_value){
			outputValue("Value result: '",_valueReference->_value,"'");
			output(" of type '%s'.\n",VALUETYPENAMES[_valueReference->_value->type]);
		}else
			outputLine("No value result!");
	}
	
	return _valueReference;

	/*
		// it could be an assignment in which case we remove the assignee and assigned value
		if(firstToken->type==TT_VARIABLE||firstToken->type==TT_NEW_VARIABLE){ // something that can be assigned to
			// an index might be defined on a variable that is a list
			if(secondToken->type==TT_LIST){
				// TODO locate the end of list token at the same level????
			}
			// TODO there might be a binary operator behind (in front of the assignment operator)
			char* shortcutBinaryOperator=NULL;
			if(secondToken->type==TT_BINARY_AeRu||secondToken->type==TT_BINARY_Aeru){
				shortcutBinaryOperator=string(secondToken->text);
				secondToken=secondToken->next;
			}
			if(secondToken&&secondToken->type==TT_ASSIGNMENT){
				if(firstToken->type==TT_NEW_VARIABLE)addVariable(_Menvironment,firstTokenText,VT_UNDEFINED,false);
				Mvalue* _expressionValue=getValueOfExpression("assignment",'a',endTokenTypes,endTokenTypeCount);
				if(_expressionValue){
					if(shortcutBinaryOperator){ 
						// TODO apply the shortcut binary operator to the current value of the assignee before assigning
					}
					// if we failed to create the variable (see above), the following obviously will fail!!! (or of course when the type of the value is wrong)
					//if(amVerbose())output("Storing value '%s' in variable '%s'.",string(_getValueText(_expressionvalue->_value)),firstTokenText);
					if(!setValue(_Menvironment,firstTokenText,_valuereference->_variable->_value)){
						//output("ERROR: Value '%s' not stored.",string(_getValueText(_expressionvalue->_value)));
						// no need to ever free a value ourselves, the 'garbage collection' takes care of that (see removedValues())
						///free_value(_expressionvalue->_value);
						///_expressionvalue->_value=NULL;
					}else
					if(amVerbose())
						output("Variable '%s' set to '%s'.",firstTokenText,string(_getValueText(getValue(_Menvironment,firstTokenText))));
				}
			}
		}
	}
	return _valuereference;
	*/
}

// BINARY OPERATOR + functions

Mlist* _appliedToLists(Mlist* _list1,Mlist* _list2,TwoArgumentFunction binaryoperator){
	if(!_list1)return _list2;if(!_list2)return _list1;
	// MDH@30OCT2019: ALWAYS apply the binary operator i.e. do NOT just return the value!!! (which makes perfect sense for equality / unequality)
	//                TODO if the result equals NULL, should we then NOT add the given element?????
	// ASSERT neither are NULL
	Mlist* _result=_getListOfType(_list1->valuetype==_list2->valuetype?_list1->valuetype:VT_UNDEFINED); // TODO if the types are the same use that?
	// elements with the same index are to be added and stored under that index
	Mlistelement* _listelement1=_list1->_first;
	Mlistelement* _listelement2=_list2->_first;
	bool consumed1,consumed2;
	while(_listelement1||_listelement2){
		consumed1=false;
		consumed2=false;
		if(_listelement1&&_listelement2){
			if(_listelement1->index==_listelement2->index){
				if(appendedToList(_result,binaryoperator(_listelement1->_value,_listelement2->_value),_listelement1->index))consumed1=consumed2=true;
			}else
			if(_listelement1->index<_listelement2->index){
				if(appendedToList(_result,binaryoperator(_listelement1->_value,NULL),_listelement1->index))consumed1=true;
			}else{
				if(appendedToList(_result,binaryoperator(NULL,_listelement2->_value),_listelement2->index))consumed2=true;
			}
		}else
		if(_listelement1){
			if(appendedToList(_result,binaryoperator(_listelement1->_value,NULL),_listelement1->index))consumed1=true;
		}else
			if(appendedToList(_result,binaryoperator(NULL,_listelement2->_value),_listelement2->index))consumed2=true;
		// done?????
		if(!consumed1&&!consumed2)break; // if neither consumed done
		if(consumed1)_listelement1=_listelement1->_next;
		if(consumed2)_listelement2=_listelement2->_next;
	}
	return _result;
}
// we can use a single function to apply a certain binary operator because the functions have the same signature as a TwoArgumentFunction!!
Mvalue* _appliedToList(Mlist* _list,Mvalue* _value,TwoArgumentFunction binaryoperator){
	// scalars are to be added to each element of the original list
	// lists are to be added to the elements at the same position, so listwise
	Mlist* _result=NULL;
	if(_value->type!=VT_LIST){
		_result=_getListOfType(_list->valuetype);
		Mlistelement* _listelement=_list->_first;
		while(_listelement&&appendedToList(_result,binaryoperator(_listelement->_value,_value),_listelement->index))_listelement=_listelement->_next;
	}else
		_result=_appliedToLists(_list,_value->value._list,binaryoperator);
	return _getValueOfList(_result,true);
}
Mvalue* _appliedToList2(Mvalue* _value,Mlist* _list,TwoArgumentFunction binaryoperator){
	// scalars are to be added to each element of the original list
	// lists are to be added to the elements at the same position, so listwise
	Mlist* _result=NULL;
	if(_value->type!=VT_LIST){
		_result=_getListOfType(_list->valuetype);
		Mlistelement* _listelement=_list->_first;
		while(_listelement&&appendedToList(_result,binaryoperator(_value,_listelement->_value),_listelement->index))_listelement=_listelement->_next;
	}else
		_result=_appliedToLists(_value->value._list,_list,binaryoperator);
	return _getValueOfList(_result,true);
}

// two-argument arithmetic
// helper functions
// NOTE the following takes a lot of precision because we should never return the originals always copies which should be freed if they are not used anymore
/* see Mexecution.c
Mbiginteger* _getBigintegerCopy(Mbiginteger* _biginteger){
	Mbiginteger* _bigintegerCopy=new_Mbiginteger();if(mp_copy(_biginteger,_bigintegerCopy)!=MP_OKAY){free_biginteger(_bigintegerCopy);return NULL;}return _bigintegerCopy;
}
*/
// rational number addition
// generic addition
Mdecimal* getValueDecimal(Mvalue* _value);
Mvalue* add(Mvalue* _value1,Mvalue* _value2){
	if(!_value1||!_value2)return NULL; // MDH@24OCT2019: propagate NULL
	// if either is a list apply 'add' to the list (NOTE scalar addition is NOT the same as list addition)
	if(_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,add);
	if(_value2->type==VT_LIST)return _appliedToList(_value2->value._list,_value1,add);
	// MDH@24OCT2019: isValueZero() can now also return M_LL_INVALID and we do NOT want the value to be considered a 'true' zero when that happens!!!!!
	if(isValueZero(_value1)==M_TRUE)return _value2;
	if(isValueZero(_value2)==M_TRUE)return _value1;
	// MDH@24OCT2019: integer operations should be done using big integers, if the result is to be integer we map to M_LL_INVALID if the result is out of range!!!!
	//                we first do this for the add() binary operator, after this we're going to role this procedure out on the other binary operations!!!
	//                this means:
	//                1. comment out the next block
	//                2. function getBigintegerInteger() was created in order to map the sum big integer into the valid range of integers (if possible)
	/* no longer treat differently here
	// if both are integers, the result should be integer as well!!!
	if(_value1->type==VT_INTEGER&&_value2->type==VT_INTEGER){
			if(amVerbose())output("Adding integers '%lld' and '%lld'.\n",_value1->value._integer->ll,_value2->value._integer->ll);
			return _getIntegerValue(_value1->value._integer->ll+_value2->value._integer->ll);
	}
	*/
	// the other integer one could be a big integer in which case we return a big integer
	if((_value1->type==VT_INTEGER||_value1->type==VT_BIGINTEGER)&&(_value2->type==VT_INTEGER||_value2->type==VT_BIGINTEGER)){
		Mbiginteger* _sumBiginteger=NULL;
		// no need to use _getValueBiginteger because we know the source will be integer
		// NOTE _getBiginteger() was adjusted to return NULL in case ll equals M_LL_INVALID because in that case the result should also be M_LL_INVALID
		// CORRECTION: _getBiginteger() is also used in the I() function and it's a problem if NOT allowing to actually use NAI in computations (as the smallest possible integer)
		// DISCUSSION: M_LL_INVALID is a single long long value that is considered invalid like dividing by zero, or asking for the sign of an undefined real (= long double)
		//             
		bool smallinteger1=(_value1->type==VT_INTEGER),smallinteger2=(_value2->type==VT_INTEGER);
		bool invalidinteger1=(smallinteger1&&_value1->value._integer->ll==M_LL_INVALID),invalidinteger2=(smallinteger2&&_value2->value._integer->ll==M_LL_INVALID);
		if(invalidinteger1||invalidinteger2)return _getIntegerValue(M_LL_INVALID); // if either integer is invalid return an invalid integer (which per definition will be small)
		// ASSERT both integers are considered valid (i.e. not invalid)
		Mbiginteger *_biginteger1=(smallinteger1?_getBiginteger(_value1->value._integer->ll):_value1->value._biginteger);
		Mbiginteger *_biginteger2=(smallinteger2?_getBiginteger(_value2->value._integer->ll):_value2->value._biginteger);
		// replacing: Mbiginteger *_biginteger1=_getValueBiginteger(_value1),*_biginteger2=_getValueBiginteger(_value2); // OOPS careful here, _getValueDecimal would make a copy which we do not want here!!!!
		if(_biginteger1&&_biginteger2){
			if(amVerbose()){outputBiginteger("Adding big integers '",_biginteger1,"'");outputBiginteger(" and '",_biginteger2,"'");}
			_sumBiginteger=__biginteger();
			if(_sumBiginteger&&mp_add(_biginteger1,_biginteger2,_sumBiginteger)!=MP_OKAY){free_biginteger(_sumBiginteger);_sumBiginteger=NULL;} // _dmul replaced by _getDecimalProduct which should be able to multiply any two decimals (not just the pure decimals)
			if(amVerbose()){outputBiginteger(" - Sum: '",_sumBiginteger,"'.\n");}
		}else
			outputError("Failed to convert an integer to a big integer");
		if(smallinteger1)free_biginteger(_biginteger1);
		if(smallinteger2)free_biginteger(_biginteger2);
		// MDH@24OCT2019: now we're going to try to convert the sum back to an integer if we can
		//                but if we can't don't
		if(smallinteger1||smallinteger2){ // we could decide to try to keep the value in range if at least one of the integers is small (instead of demanding both are small integers)
			// if computing the sum failed return the invalid (small) integer (to indicate a missing result)
			if(!_sumBiginteger)return _getIntegerValue(M_LL_INVALID);
			long long llsum=getBigintegerInteger(_sumBiginteger); // will return M_LL_INVALID when _sumBiginteger equals NULL (which we want to exclude)
			// if we do NOT have a sum big integer or the sum big integer is in range ()
			if(llsum!=M_LL_INVALID){free_biginteger(_sumBiginteger);return _getIntegerValue(llsum);}
			output("WARNING: Small integer sum out of range, will continue using big integer sum.\n");
		}
		return _getBigintegerValue(_sumBiginteger,true);
	}
	// if the first value is a text we should always do concatenation!!!!
	if(_value1->type==VT_TEXT){ // force string concatenation using the quote character in the Mvalue in the resulting text
		Mstring* _valueText=__string();
		if(!_valueText)return NULL;
		Mstring* p=_valueText;
		p=string_append_char(p,_value1->value._text->presuffix);
		p=string_append(p,_value1->value._text->_c);
		// MDH@17OCT2019: we can't use _getValueText() here, because _getValueText() will resolve escape sequences which we do NOT want here
		// MDH@28OCT2019: think twice this is only true when _value2 is also of type text
		if(_value2->type!=VT_TEXT){
			Mstring* _value2Text=_getValueText(_value2,true); // get the text representation of the second argument without quotes
			if(_value2Text){p=string_append(p,string(_value2Text));free_string(_value2Text);}
		}else // second argument also of type text
			p=string_append(p,_value2->value._text->_c);
		Mvalue* _value=(p?_getTextValue(string(_valueText),false):NULL);
		free_string(_valueText);
		return _value;
	}
	// if either is a rational, compute the sum rational (NOTE or rationals disguised as decimals)
	if((_value1->type==VT_RATIONAL||(_value1->type==VT_DECIMAL&&_value1->value._decimal->repeating>0))||(_value2->type==VT_RATIONAL||(_value2->type==VT_DECIMAL&&_value2->value._decimal->repeating>0))){
		Mrational *_rational1=getValueRational(_value1),*_rational2=getValueRational(_value2); // OOPS careful here, _getValueRational might construct a new rational or what????
		/////outputLine("Adding two rationals.");
		Mrational* _sumRational=_getRationalSum(_rational1,_rational2); // _qsum replaced by _getRationalSum that takes the deltas into account as well
		/////outputLine("Rationals added!");
		if(_value1->type!=VT_RATIONAL)free_rational(_rational1);else if(_value2->type!=VT_RATIONAL)free_rational(_rational2); // after adding the two rationals we do not need the newly created rationals anymore
		outputLine("Rational copies released.");
		Mvalue* _sumValue=NULL;
		if(_sumRational){
			if(_value1->type==VT_DECIMAL&&_value2->type==VT_DECIMAL){
				_sumValue=_getDecimalValue(_getRationalDecimal(_sumRational),true);
				free_rational(_sumRational);
			}else
				_sumValue=_getRationalValue(_sumRational,true);
		}
		return _sumValue;
	}
	// if either is a decimal, compute the sum decimal
	if(_value1->type==VT_DECIMAL||_value2->type==VT_DECIMAL){
		Mdecimal *_decimal1=getValueDecimal(_value1),*_decimal2=getValueDecimal(_value2); // OOPS careful here, _getValueDecimal would make a copy which we do not want here!!!!
		Mdecimal* _sumDecimal=_getDecimalSum(_decimal1,_decimal2); // _dadd replaced by _getDecimalSum that takes the repeating decimal digits into account as well
		if(_value1->type!=VT_DECIMAL)free_decimal(_decimal1);else if(_value2->type!=VT_DECIMAL)free_decimal(_decimal2); // after adding the two rationals we do not need the newly created rationals anymore
		if(!_sumDecimal)return NULL; // failed to create the sum for whatever reason
		return _getDecimalValue(_sumDecimal,true);
	}
	// if either is a real
	if(_value1->type==VT_FLOAT||_value2->type==VT_FLOAT){
		if(amVerbose()){outputValue("Adding integer/reals '",_value1,"'");outputValue(" and '",_value2,"'.\n");}
		long double ld1=getValueLongDouble(_value1),ld2=getValueLongDouble(_value2);
		return _getFloatValue(isLongDoubleUndefined(ld1)==M_FALSE&&isLongDoubleUndefined(ld2)==M_FALSE?ld1+ld2:M_LD_NAN);
	}
	/* MDH@28OCT2019: either real already dealt with above
	if((_value1->type==VT_INTEGER||_value1->type==VT_FLOAT)&&(_value2->type==VT_INTEGER||_value2->type==VT_FLOAT||_value2->type==VT_TEXT)){
		// if the second argument is text, convert it to a real or integer number
		if(_value2->type==VT_TEXT){
			// are we going to convert it to an integer or a real????
			// NOTE a real has a period in the text, so use that
			char* valueText=_value2->value._text->_c;
			if(strchr(valueText,'.')!=NULL){ // a period 
				////////if(strlen(_valueText)==1)return _value1; // if a single period no need to actually add it unless someone want to change an integer in a real????
				////// we can use _strtold!!! long double ld=0;if(strlen(valueText)>1){char *endPtr=NULL;ld=strtold(valueText,&endPtr);if(endPtr==valueText){output("ERROR: Can't add '%s'.",valueText);return NULL;}} // failure
				_value2=_getFloatValue(_strtold(valueText,getNAR()));
			}else{ // no period
				_value2=_getIntegerValue(_strtoll(valueText,getNAI()));
			}
		}
		if(_value1->type==VT_INTEGER&&_value2->type==VT_INTEGER)return _getIntegerValue(_value1->value._integer->ll+_value2->value._integer->ll);
		return _getFloatValue((_value1->type==VT_INTEGER?_value1->value._integer->ll:_value1->value._float->ld)+(_value2->type==VT_INTEGER?_value2->value._integer->ll:_value2->value._float->ld));
	}
	*/
	return NULL;
}

Mvalue* subtract(Mvalue* _value1,Mvalue* _value2){
	if(!_value1||!_value2)return NULL;
	if(amVerbose()){outputValue("Subtracting '",_value2,"'");outputValue(" from '",_value1,"'.\n");}
	if(_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,subtract);
	if(_value2->type==VT_LIST)return _appliedToList2(_value1,_value2->value._list,subtract);
	// if either is zero, result is easy to determine
	if(isValueZero(_value1)==M_TRUE)return Mneg(_value2);
	if(isValueZero(_value2)==M_TRUE)return _value1;
	if(amVerbose()){outputValue("Subtracting scalar '",_value2,"'");outputValue(" from scalar '",_value1,"'.\n");}
	/*
	// if both are integers, the result should be integer as well!!!
	if(_value1->type==VT_INTEGER&&_value2->type==VT_INTEGER){
			if(amVerbose())output("Subtracting integers '%lld' and '%lld'.\n",_value1->value._integer->ll,_value2->value._integer->ll);
			return _getIntegerValue(_value1->value._integer->ll-_value2->value._integer->ll);
	}
	*/
	// the other integer one could be a big integer in which case we return a big integer
	// the other integer one could be a big integer in which case we return a big integer
	if((_value1->type==VT_INTEGER||_value1->type==VT_BIGINTEGER)&&(_value2->type==VT_INTEGER||_value2->type==VT_BIGINTEGER)){
		Mbiginteger* _differenceBiginteger=NULL;
		// no need to use _getValueBiginteger because we know the source will be integer
		// NOTE _getBiginteger() was adjusted to return NULL in case ll equals M_LL_INVALID because in that case the result should also be M_LL_INVALID
		// CORRECTION: _getBiginteger() is also used in the I() function and it's a problem if NOT allowing to actually use NAI in computations (as the smallest possible integer)
		// DISCUSSION: M_LL_INVALID is a single long long value that is considered invalid like dividing by zero, or asking for the sign of an undefined real (= long double)
		//             
		bool smallinteger1=(_value1->type==VT_INTEGER),smallinteger2=(_value2->type==VT_INTEGER);
		bool invalidinteger1=(smallinteger1&&_value1->value._integer->ll==M_LL_INVALID),invalidinteger2=(smallinteger2&&_value2->value._integer->ll==M_LL_INVALID);
		if(invalidinteger1||invalidinteger2)return _getIntegerValue(M_LL_INVALID); // if either integer is invalid return an invalid integer (which per definition will be small)
		// ASSERT both integers are considered valid (i.e. not invalid)
		Mbiginteger *_biginteger1=(smallinteger1?_getBiginteger(_value1->value._integer->ll):_value1->value._biginteger);
		Mbiginteger *_biginteger2=(smallinteger2?_getBiginteger(_value2->value._integer->ll):_value2->value._biginteger);
		if(_biginteger1&&_biginteger2){
			if(amVerbose()){outputBiginteger("Subtracting big integers '",_biginteger1,"'");outputBiginteger(" and '",_biginteger2,"'");}
			_differenceBiginteger=__biginteger();
			if(_differenceBiginteger&&mp_sub(_biginteger1,_biginteger2,_differenceBiginteger)!=MP_OKAY){free_biginteger(_differenceBiginteger);_differenceBiginteger=NULL;} // _dmul replaced by _getDecimalProduct which should be able to multiply any two decimals (not just the pure decimals)
			if(amVerbose()){outputBiginteger(" - Difference: '",_differenceBiginteger,"'.\n");}
		}else
			outputError("Failed to convert an integer to a big integer");
		if(smallinteger1)free_biginteger(_biginteger1);
		if(smallinteger2)free_biginteger(_biginteger2);
		// MDH@24OCT2019: now we're going to try to convert the sum back to an integer if we can
		//                but if we can't don't
		if(smallinteger1||smallinteger2){ // we could decide to try to keep the value in range if at least one of the integers is small (instead of demanding both are small integers)
			// if computing the sum failed return the invalid (small) integer (to indicate a missing result)
			if(!_differenceBiginteger)return _getIntegerValue(M_LL_INVALID);
			long long llsum=getBigintegerInteger(_differenceBiginteger); // will return M_LL_INVALID when _sumBiginteger equals NULL (which we want to exclude)
			// if we do NOT have a sum big integer or the sum big integer is in range ()
			if(llsum!=M_LL_INVALID){free_biginteger(_differenceBiginteger);return _getIntegerValue(llsum);}
			output("WARNING: Small integer difference out of range, will continue using big integer difference.\n");
		}
		return _getBigintegerValue(_differenceBiginteger,true);
	}
	if((_value1->type==VT_RATIONAL||(_value1->type==VT_DECIMAL&&_value1->value._decimal->repeating>0))||(_value2->type==VT_RATIONAL||(_value2->type==VT_DECIMAL&&_value2->value._decimal->repeating>0))){
		Mrational *_rational1=getValueRational(_value1),*_rational2=getValueRational(_value2); // OOPS careful here, _getValueRational might construct a new rational or what????
		if(amVerbose()){outputRational("Computing the difference of rational '",_rational1,"'");outputRational(" and rational '",_rational2,"'.\n");}
		Mrational* _differenceRational=_getRationalDifference(_rational1,_rational2); // _qsubtract replaced by _getRationalDifference() which takes deltas into account as well
		if(_value1->type!=VT_RATIONAL)free_rational(_rational1);else if(_value2->type!=VT_RATIONAL)free_rational(_rational2); // after adding the two rationals we do not need the newly created rationals anymore
		Mvalue* _differenceValue=NULL;
		if(_differenceRational){
			if(_value1->type==VT_DECIMAL&&_value2->type==VT_DECIMAL){
				_differenceValue=_getDecimalValue(_getRationalDecimal(_differenceRational),true);
				free_rational(_differenceRational);
			}else
				_differenceValue=_getRationalValue(_differenceRational,true);
		}
		return _differenceValue;
	}
	// if either is a decimal, compute the difference decimal
	if(_value1->type==VT_DECIMAL||_value2->type==VT_DECIMAL){
		Mdecimal *_decimal1=getValueDecimal(_value1),*_decimal2=getValueDecimal(_value2); // OOPS careful here, _getValueDecimal would make a copy which we do not want here!!!!
		Mdecimal* _differenceDecimal=_getDecimalDifference(_decimal1,_decimal2); // _dsub replaced by _getDecimalDifference which takes repeating decimal digits into account as well
		if(_value1->type!=VT_DECIMAL)free_decimal(_decimal1);else if(_value2->type!=VT_DECIMAL)free_decimal(_decimal2); // after adding the two rationals we do not need the newly created rationals anymore
		if(!_differenceDecimal)return NULL; // failed to create the sum for whatever reason
		return _getDecimalValue(_differenceDecimal,true);
	}
	// if either is a real
	if(_value1->type==VT_FLOAT||_value2->type==VT_FLOAT){
		if(amVerbose()){outputValue("Subtracting integer/reals '",_value1,"'");outputValue(" and '",_value2,"'.\n");}
		long double ld1=getValueLongDouble(_value1),ld2=getValueLongDouble(_value2);
		return _getFloatValue(isLongDoubleUndefined(ld1)==M_FALSE&&isLongDoubleUndefined(ld2)==M_FALSE?ld1-ld2:M_LD_NAN);
	}
	/* MDH@28OCT2019: now obsolete
	if((_value1->type==VT_INTEGER||_value1->type==VT_FLOAT)&&(_value2->type==VT_INTEGER||_value2->type==VT_FLOAT)){
		if(_value1->type==VT_INTEGER&&_value2->type==VT_INTEGER)return _getIntegerValue(_value1->value._integer->ll-_value2->value._integer->ll);
		return _getFloatValue((_value1->type==VT_INTEGER?_value1->value._integer->ll:_value1->value._float->ld)-(_value2->type==VT_INTEGER?_value2->value._integer->ll:_value2->value._float->ld));
	}
	*/
	return NULL;
}

Mvalue* multiply(Mvalue* _value1,Mvalue* _value2){
	if(!_value1||!_value2)return NULL;
	if(_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,multiply);if(_value2->type==VT_LIST)return _appliedToList(_value2->value._list,_value1,multiply);
	if(isValueZero(_value1)==M_TRUE||isValueOne(_value2)==M_TRUE)return _value1;
	if(isValueZero(_value2)==M_TRUE||isValueOne(_value1)==M_TRUE)return _value2;
	// MDH@26OCT2019: adapted from dealing with any integer type from add()
	if((_value1->type==VT_INTEGER||_value1->type==VT_BIGINTEGER)&&(_value2->type==VT_INTEGER||_value2->type==VT_BIGINTEGER)){
		Mbiginteger* _productBiginteger=NULL;
		// no need to use _getValueBiginteger because we know the source will be integer
		// NOTE _getBiginteger() was adjusted to return NULL in case ll equals M_LL_INVALID because in that case the result should also be M_LL_INVALID
		// CORRECTION: _getBiginteger() is also used in the I() function and it's a problem if NOT allowing to actually use NAI in computations (as the smallest possible integer)
		// DISCUSSION: M_LL_INVALID is a single long long value that is considered invalid like dividing by zero, or asking for the sign of an undefined real (= long double)
		//             
		bool smallinteger1=(_value1->type==VT_INTEGER),smallinteger2=(_value2->type==VT_INTEGER);
		bool invalidinteger1=(smallinteger1&&_value1->value._integer->ll==M_LL_INVALID),invalidinteger2=(smallinteger2&&_value2->value._integer->ll==M_LL_INVALID);
		if(invalidinteger1||invalidinteger2)return _getIntegerValue(M_LL_INVALID); // if either integer is invalid return an invalid integer (which per definition will be small)
		// ASSERT both integers are considered valid (i.e. not invalid)
		Mbiginteger *_biginteger1=(smallinteger1?_getBiginteger(_value1->value._integer->ll):_value1->value._biginteger);
		Mbiginteger *_biginteger2=(smallinteger2?_getBiginteger(_value2->value._integer->ll):_value2->value._biginteger);
		// replacing: Mbiginteger *_biginteger1=_getValueBiginteger(_value1),*_biginteger2=_getValueBiginteger(_value2); // OOPS careful here, _getValueDecimal would make a copy which we do not want here!!!!
		if(_biginteger1&&_biginteger2){
			if(amVerbose()){outputBiginteger("Multiplying big integers '",_biginteger1,"'");outputBiginteger(" and '",_biginteger2,"'");}
			_productBiginteger=__biginteger();
			if(_productBiginteger&&mp_mul(_biginteger1,_biginteger2,_productBiginteger)!=MP_OKAY){free_biginteger(_productBiginteger);_productBiginteger=NULL;} // _dmul replaced by _getDecimalProduct which should be able to multiply any two decimals (not just the pure decimals)
			if(amVerbose()){outputBiginteger(" - Product: '",_productBiginteger,"'.\n");}
		}else
			outputError("Failed to convert a small integer to a big integer");
		if(smallinteger1)free_biginteger(_biginteger1);
		if(smallinteger2)free_biginteger(_biginteger2);
		// MDH@24OCT2019: now we're going to try to convert the sum back to an integer if we can
		//                but if we can't don't
		if(smallinteger1||smallinteger2){ // we could decide to try to keep the value in range if at least one of the integers is small (instead of demanding both are small integers)
			// if computing the sum failed return the invalid (small) integer (to indicate a missing result)
			if(!_productBiginteger)return _getIntegerValue(M_LL_INVALID);
			long long llproduct=getBigintegerInteger(_productBiginteger); // will return M_LL_INVALID when _sumBiginteger equals NULL (which we want to exclude)
			// if we do NOT have a sum big integer or the sum big integer is in range ()
			if(llproduct!=M_LL_INVALID){free_biginteger(_productBiginteger);return _getIntegerValue(llproduct);}
			output("WARNING: Small integer product out of range, will continue using big integer product.\n");
		}
		return _getBigintegerValue(_productBiginteger,true);
	}
	/* replacing:
	// if both are integers, the result should be integer as well!!!
	if(_value1->type==VT_INTEGER&&_value2->type==VT_INTEGER){
			if(amVerbose())output("Multiplying integers '%lld' and '%lld'.\n",_value1->value._integer->ll,_value2->value._integer->ll);
			return _getIntegerValue(_value1->value._integer->ll*_value2->value._integer->ll);
	}
	// the other integer one could be a big integer in which case we return a big integer
	if((_value1->type==VT_INTEGER||_value1->type==VT_BIGINTEGER)&&(_value2->type==VT_INTEGER||_value2->type==VT_BIGINTEGER)){
		Mbiginteger* _productBiginteger=NULL;
		Mbiginteger *_biginteger1=_getValueBiginteger(_value1),*_biginteger2=_getValueBiginteger(_value2); // OOPS careful here, _getValueDecimal would make a copy which we do not want here!!!!
		if(_biginteger1&&_biginteger2){
			if(amVerbose()){outputBiginteger("Multiplying big integers '",_biginteger1,"'");outputBiginteger(" and '",_biginteger2,"'.\n");}
			_productBiginteger=__biginteger();
			if(!_productBiginteger)outputError("Failed to create the product big integer");else
			if(mp_mul(_biginteger1,_biginteger2,_productBiginteger)!=MP_OKAY){
				free_biginteger(_productBiginteger);_productBiginteger=NULL;outputError("Failed to multiply two big integers");
			}else
			if(amVerbose())outputBiginteger("Big integer product: '",_productBiginteger,"'.\n");
			 // _dmul replaced by _getDecimalProduct which should be able to multiply any two decimals (not just the pure decimals)
		}else
			outputError("Failed to create two helper big integers");
		if(_value1->type!=VT_BIGINTEGER)free_biginteger(_biginteger1);else if(_value2->type!=VT_BIGINTEGER)free_biginteger(_biginteger2); // after adding the two rationals we do not need the newly created rationals anymore
		return _getBigintegerValue(_productBiginteger,true);
	}
	*/
	// if either is rational do a rational multiplication
	if((_value1->type==VT_RATIONAL||(_value1->type==VT_DECIMAL&&_value1->value._decimal->repeating>0))||(_value2->type==VT_RATIONAL||(_value2->type==VT_DECIMAL&&_value2->value._decimal->repeating>0))){
		if(amVerbose()){outputValue("Multiplying rationals '",_value1,"'");outputValue(" and '",_value2,"'.\n");}
		Mrational *_rational1=getValueRational(_value1),*_rational2=getValueRational(_value2);
		Mrational* _productRational=_getRationalProduct(_rational1,_rational2); // _qproduct replaced by _getRationalProduct as defined in Mrational.h/c
		if(_value1->type!=VT_RATIONAL)free_rational(_rational1);else if(_value2->type!=VT_RATIONAL)free_rational(_rational2); // after dividing the two rationals we do not need the newly created rationals anymore
		Mvalue* _productValue=NULL;
		if(_productRational){
			if(_value1->type==VT_DECIMAL&&_value2->type==VT_DECIMAL){
				_productValue=_getDecimalValue(_getRationalDecimal(_productRational),true);
				free_rational(_productRational);
			}else
				_productValue=_getRationalValue(_productRational,true);
		}
		return _productValue;
	}
	// if either is a decimal, compute the product decimal
	if(_value1->type==VT_DECIMAL||_value2->type==VT_DECIMAL){
		if(amVerbose()){outputValue("Multiplying decimals '",_value1,"'");outputValue(" and '",_value2,"'.\n");}
		Mdecimal *_decimal1=getValueDecimal(_value1),*_decimal2=getValueDecimal(_value2); // OOPS careful here, _getValueDecimal would make a copy which we do not want here!!!!
		Mdecimal* _productDecimal=_getDecimalProduct(_decimal1,_decimal2); // _dmul replaced by _getDecimalProduct which should be able to multiply any two decimals (not just the pure decimals)
		if(_value1->type!=VT_DECIMAL)free_decimal(_decimal1);else if(_value2->type!=VT_DECIMAL)free_decimal(_decimal2); // after adding the two rationals we do not need the newly created rationals anymore
		if(!_productDecimal)return NULL; // failed to create the sum for whatever reason
		return _getDecimalValue(_productDecimal,true);
	}
	// if either is a real
	if(_value1->type==VT_FLOAT||_value2->type==VT_FLOAT){
		if(amVerbose()){outputValue("Multiplying integer/reals '",_value1,"'");outputValue(" and '",_value2,"'.\n");}
		long double ld1=getValueLongDouble(_value1),ld2=getValueLongDouble(_value2);
		return _getFloatValue(isLongDoubleUndefined(ld1)==M_FALSE&&isLongDoubleUndefined(ld2)==M_FALSE?ld1*ld2:M_LD_NAN);
	}
	return NULL;
}

Mvalue* _getValueOneOfType(Mvaluetype valuetype){
	switch(valuetype){
		case VT_INTEGER: return _getIntegerValue(1);
		case VT_BIGINTEGER: return _getBigintegerValue(_getBiginteger(1),true);
		case VT_FLOAT: return _getFloatValue(1.0);
		case VT_RATIONAL: return _getRationalValue(_getRational(_getBiginteger(1),NULL,M_LD_NAN,false,true),true);
		case VT_DECIMAL: return _getDecimalValue(_getDecimal(__mpd(get_default_mpd_context(),1),M_DP,0,true),true);
		default:break;
	}
	return NULL;
}

long double getRealPowerValue(long double base,Mvalue* _powerValue){
	// ASSERT assuming power does not equal 0
	if(isLongDoubleUndefined(base)==M_FALSE){ // TODO might still be infinite though
		if(_powerValue)
		switch(_powerValue->type){
			case VT_INTEGER:return powl(base,_powerValue->value._integer->ll); // very easy, as we can expext to be able to convert the integer to a long double
			case VT_BIGINTEGER:return powl(base,mp_get_long_double(_powerValue->value._biginteger));
			case VT_DECIMAL:return powl(base,getDecimalLongDouble(_powerValue->value._decimal));
			case VT_RATIONAL:
				{
					long double power=powl(mp_get_long_double(_powerValue->value._rational->num),power);
					if(_powerValue->value._rational->den)power/=powl(mp_get_long_double(_powerValue->value._rational->den),power);
					return powl(base,power);
				}
			case VT_FLOAT:return powl(base,_powerValue->value._float->ld);
			default:break;
		}
	}
	return M_LD_NAN; // uncomputable
}
long double getFloatValuePower(Mvalue* _baseValue,long double power){
	// ASSERT assuming power does not equal 0
	if(isLongDoubleUndefined(power)==M_FALSE){ // TODO might still be infinite though
		if(_baseValue)
		switch(_baseValue->type){
			case VT_INTEGER:return powl(_baseValue->value._integer->ll,power); // very easy, as we can expext to be able to convert the integer to a long double
			case VT_BIGINTEGER:return powl(mp_get_long_double(_baseValue->value._biginteger),power);
			case VT_DECIMAL:return powl(getDecimalLongDouble(_baseValue->value._decimal),power);
			case VT_RATIONAL:
				{ // transform the base value rational to a long double
					long double base=powl(mp_get_long_double(_baseValue->value._rational->num),power);
					if(_baseValue->value._rational->den)base/=powl(mp_get_long_double(_baseValue->value._rational->den),power);
					return powl(base,power);
				}
			case VT_FLOAT:return powl(_baseValue->value._float->ld,power);
			default:break;
		}
	}
	return M_LD_NAN; // uncomputable
}

Mdecimal* _getDecimalPower(mpd_t* base,mpd_t* exponent,mpd_context_t* mpd_context){
	Mdecimal* _decimalPower=NULL;
	if(base&&exponent&&mpd_context){
		_decimalPower=__decimal(mpd_context,0,0);
		if(_decimalPower){
			uint32_t status=0;
			mpd_qpow(_decimalPower->mpd,base,exponent,mpd_context,&status);
			if((status&0xEFBF)!=0){outputError("Failed to apply the decimal power function");free_decimal(_decimalPower);_decimalPower=NULL;}
		}else
			outputError("Failed to create decimal power function result");
	}else
		outputError("Insufficient input for computing a decimal power");
	return _decimalPower;
}

// computing the integer power of some value, can be performed more exact than when the exponent is not an integer
Mbiginteger* _getBigintegerPowerWithPositiveBigintegerExponent(Mbiginteger* baseBiginteger,Mbiginteger* exponentBiginteger){
	// ASSERT assuming exponentBiginteger is positive (so never zero!!!)
	Mbiginteger* _resultBiginteger=NULL;
	if(baseBiginteger&&exponentBiginteger){
		////////////outputBiginteger("Computing big integer ",baseBiginteger,NULL);outputBiginteger(" ** ",exponentBiginteger,".\n");
		if(isBigintegerZero(exponentBiginteger))
			_resultBiginteger=_getBiginteger(1);
		else
		if(!isBigintegerOne(exponentBiginteger)){
			// determine half the exponent
			Mbiginteger* _halfexponentBiginteger=__biginteger();
			if(mp_div_2(exponentBiginteger,_halfexponentBiginteger)==MP_OKAY){
				Mbiginteger* _halfresultBiginteger=_getBigintegerPowerWithPositiveBigintegerExponent(baseBiginteger,_halfexponentBiginteger);
				if(_halfresultBiginteger){
					Mbiginteger* _doublehalfresultBiginteger=__biginteger();
					if(mp_sqr(_halfresultBiginteger,_doublehalfresultBiginteger)==MP_OKAY){
						if(!mp_isodd(exponentBiginteger)||mp_mul(_doublehalfresultBiginteger,baseBiginteger,_doublehalfresultBiginteger)==MP_OKAY)
							_resultBiginteger=_doublehalfresultBiginteger;
						else
							free_biginteger(_doublehalfresultBiginteger);
					}else
						free_biginteger(_doublehalfresultBiginteger);
					free_biginteger(_halfresultBiginteger);
				}
			}
			free_biginteger(_halfexponentBiginteger);
		}else
			_resultBiginteger=_getBigintegerCopy(baseBiginteger);
	}
	return _resultBiginteger;
}
Mvalue* _getBigintegerBigintegerPowerValue(Mbiginteger* baseBiginteger,Mbiginteger* exponentBiginteger){
	Mbiginteger* _bigintegerPower=NULL;
	bool neg=false;
	if(baseBiginteger&&exponentBiginteger){
		//////////////outputBiginteger("Computing big integer ",baseBiginteger,NULL);outputBiginteger(" ** ",exponentBiginteger,".\n");
		if(mp_iszero(baseBiginteger)==MP_NO){ // non-zero base
			neg=(mp_isneg(exponentBiginteger)==MP_YES);
			if(mp_iszero(exponentBiginteger)!=MP_YES){ // not zero
				exponentBiginteger->sign=MP_ZPOS; // sneaky, sneaky!! ascertaining to use a positive exponent!
				_bigintegerPower=_getBigintegerPowerWithPositiveBigintegerExponent(baseBiginteger,exponentBiginteger);
			}else
				_bigintegerPower=_getBiginteger(1);
		}else // base is zero, so power is zero as well
			_bigintegerPower=_getBiginteger(0);
	}
	return(_bigintegerPower?(neg?_getRationalValue(_getRational(NULL,_bigintegerPower,0,false,true),true):_getBigintegerValue(_bigintegerPower,true)):NULL);
}
Mrational* _getRationalBigintegerPower(Mrational* baseRational,Mbiginteger* exponentBiginteger){
	// the result is the rational of the power of the numerator and the power of the denominator
	// if the exponent is negative we simply exchange the numerator and the denominator!!	
	Mrational* _rationalPower=NULL;
	if(baseRational&&exponentBiginteger){
		bool neg=(mp_isneg(exponentBiginteger)==MP_YES);
		exponentBiginteger->sign=MP_ZPOS;
		Mbiginteger *baseNumerator=(neg?baseRational->den:baseRational->num),*baseDenominator=(neg?baseRational->num:baseRational->den);
		Mbiginteger *_numerator=_getBigintegerPowerWithPositiveBigintegerExponent(baseNumerator,exponentBiginteger);
		Mbiginteger *_denominator=_getBigintegerPowerWithPositiveBigintegerExponent(baseDenominator,exponentBiginteger);
		_rationalPower=_getRational(_numerator,_denominator,M_LD_NAN,true,true);
		if(!_rationalPower||!_rationalPower->num)free_biginteger(_numerator);
		if(!_rationalPower||!_rationalPower->den)free_biginteger(_denominator);
	}
	return _rationalPower;
}
mpd_context_t* getContextOfDecimals(Mdecimal* d1,Mdecimal* d2){
	Mdecimalcontext* decimalcontext=_getDecimalcontext(MAX((d1?d1->prec:0),(d2?d2->prec:0)));
	return(decimalcontext?decimalcontext->mpd_context:get_default_mpd_context());
}
Mvalue* _getBigintegerPowerValue(Mvalue* baseValue,Mbiginteger* exponentBiginteger){
	// the general idea is to recursively half the exponent until we end up with having to compute the square which is easy to do
	// but perhaps we should delegate further to functions that deal with specific base value types
	switch(baseValue->type){
		case VT_INTEGER:
			{
				Mvalue* _resultValue=NULL;
				Mbiginteger* _baseBiginteger=_getBiginteger(baseValue->value._integer->ll);
				if(_baseBiginteger){
					_resultValue=_getBigintegerBigintegerPowerValue(_baseBiginteger,exponentBiginteger);
					free_biginteger(_baseBiginteger);
				}
				return _resultValue;
			}
		case VT_BIGINTEGER:
			return _getBigintegerBigintegerPowerValue(baseValue->value._biginteger,exponentBiginteger);
		case VT_RATIONAL:
			return _getRationalValue(_getRationalBigintegerPower(baseValue->value._rational,exponentBiginteger),true);
		case VT_DECIMAL:
			if(baseValue->value._decimal->repeating>0){
				Mrational* _decimalRational=_getDecimalRational(baseValue->value._decimal);
				if(_decimalRational){
					Mrational* _decimalRationalPower=_getRationalBigintegerPower(_decimalRational,exponentBiginteger);
					free_rational(_decimalRational);
					return _getRationalValue(_decimalRationalPower,true);
				}
			}else{ // base is a 'true' decimal
				Mdecimal* _exponentDecimal=_getBigintegerDecimal(exponentBiginteger);
				Mdecimal* _decimalPower=_getDecimalPower(baseValue->value._decimal->mpd,_exponentDecimal->mpd,getContextOfDecimals(baseValue->value._decimal,_exponentDecimal));
				free_decimal(_exponentDecimal);
				return _getDecimalValue(_decimalPower,true);
			}
		default:
			break;
	}
	return NULL;
}

// for finding the decimal root with an integer root degree we need to be able to compute any power of a decimal
// TODO more convenient to work with raw mpd_t instances directly
Mdecimal* _getDecimalPowerWithPositiveBigintegerExponent(Mdecimal* baseDecimal,Mbiginteger* exponentBiginteger){
	// ASSERT assuming exponentBiginteger is positive (so never zero!!!)
	Mdecimal* _resultDecimal=NULL;
	if(baseDecimal&&exponentBiginteger){
		////////////outputBiginteger("Computing big integer ",baseBiginteger,NULL);outputBiginteger(" ** ",exponentBiginteger,".\n");
		if(isBigintegerZero(exponentBiginteger))
			_resultDecimal=__decimal(NULL,1,0);
		else
		if(!isBigintegerOne(exponentBiginteger)){
			// get a decimal context
			Mdecimalcontext* decimalcontext=_getDecimalcontext(baseDecimal->prec);
			mpd_context_t* mpd_context=(decimalcontext?decimalcontext->mpd_context:get_default_mpd_context());
			// determine half the exponent
			Mbiginteger* _halfexponentBiginteger=__biginteger();
			if(mp_div_2(exponentBiginteger,_halfexponentBiginteger)==MP_OKAY){
				Mdecimal* _halfresultDecimal=_getDecimalPowerWithPositiveBigintegerExponent(baseDecimal,_halfexponentBiginteger);
				if(_halfresultDecimal){
					Mdecimal* _doublehalfresultDecimal=__decimal(NULL,1,0);
					if(_doublehalfresultDecimal){
						uint32_t status=0;
						mpd_qmul(_doublehalfresultDecimal->mpd,_halfresultDecimal->mpd,_halfresultDecimal->mpd,mpd_context,&status);
						if((status&0xEFBF)==0){
							if(mp_isodd(exponentBiginteger)){
								_resultDecimal=__decimal(mpd_context,0,0);
								if(_resultDecimal){
									mpd_qmul(_resultDecimal->mpd,_doublehalfresultDecimal->mpd,baseDecimal->mpd,mpd_context,&status);
									if((status&0xEFBF)!=0){free_decimal(_resultDecimal);_resultDecimal=NULL;}
								}else
									outputError("Failed to create a decimal in computing the integer power of a decimal");
							}else
								_resultDecimal=_getDecimalCopy(_doublehalfresultDecimal);
						}else
							outputError("Failed to compute the square of a decimal in computing the integer power of a decimal");
						free_decimal(_doublehalfresultDecimal);
					}
					free_decimal(_halfresultDecimal);
				}
			}
			free_biginteger(_halfexponentBiginteger);
		}else
			_resultDecimal=_getDecimalCopy(baseDecimal);
	}
	return _resultDecimal;
}

// MDH@11OCT2019: until we know a better way I stick to using squared exponentation
mp_err computeBigintegerPower(Mbiginteger const * const baseBiginteger,Mbiginteger const * const exponentBiginteger,Mbiginteger * const powerBiginteger){
	mp_err result=(baseBiginteger&&exponentBiginteger&&powerBiginteger?MP_OKAY:MP_ERR);
	if(result==MP_OKAY){
		if(!isBigintegerOne(baseBiginteger)&&!isBigintegerZero(exponentBiginteger)){
			Mbiginteger *_multiplierBiginteger=_getBigintegerCopy(baseBiginteger),*_exponentBiginteger=_getBigintegerCopy(exponentBiginteger);
			if(_multiplierBiginteger&&_exponentBiginteger){
				if(mp_isodd(_exponentBiginteger)!=MP_YES)mp_set_i32(powerBiginteger,1);else result=mp_copy(baseBiginteger,powerBiginteger); // initialize powerBiginteger to 1
				// can we do this iteratively???
				while(result==MP_OKAY){
					if(mp_iszero(_exponentBiginteger)==MP_YES)break;
					// half the exponent
					if((result=mp_div_2(_exponentBiginteger,_exponentBiginteger))!=MP_OKAY)break;
					// square the multiplier
					if((result=mp_sqr(_multiplierBiginteger,_multiplierBiginteger))!=MP_OKAY)break;
					if(mp_isodd(_exponentBiginteger)==MP_YES)if((result=mp_mul(powerBiginteger,_multiplierBiginteger,powerBiginteger))!=MP_OKAY)break;
				}
			}
			free_biginteger(_multiplierBiginteger);free_biginteger(_exponentBiginteger);
		}else
			result=mp_copy(baseBiginteger,powerBiginteger);
	}
	return result;
}

// MDH@10OCT2019: if both the argument and the degree is rational we can use rational approximations of the (Newtonian) (decimal) algorithm used in _getBigintegerRootValue()
// MDH@15OCT2019: how about checking whether the root approximation is near the actual root????
Mrational* _getRationalBigintegerRootRational(Mrational* rootArgumentRational,Mbiginteger* rootDegreeBiginteger){
	// ASSERT root degree big integer must NOT be negative, and use a single mp_digit (otherwise computing the function value computation is too hard)
	Mrational* _rationalBigintegerRootRational=NULL;
	if(rootArgumentRational&&rootDegreeBiginteger){
		// if the degree is zero, return 1
		if(!isBigintegerZero(rootDegreeBiginteger)){
			// if either rational is one, return a copy of the root argument rational
			if(!isBigintegerOne(rootDegreeBiginteger)&&!isRationalOne(rootArgumentRational)){ // neither equals 1
				if(rootDegreeBiginteger->used==1){ // should ALWAYS be the case!!!!
					outputBiginteger("Computing the rational approximation to the ",rootDegreeBiginteger,"th root");
					outputRational(" of ",rootArgumentRational,".\n");
					Mbiginteger *p_a=rootArgumentRational->num,*q_a=(rootArgumentRational->den?rootArgumentRational->den:_getBiginteger(1)); // helpers that will contain the numerator and denominator of A (the root argument)
					Mbiginteger *_pk=__biginteger(),*_qk=_getBiginteger(1); // initialize the solution to the root argument allowing that q_k equals NULL to indicate it is equal to 1
					if(_pk&&_qk){
						// TODO how to check whether rootDegreeBiginteger is nottoo large????
						mp_err result=MP_OKAY;
						// MDH@13OCT2019: TODO if there's exactly one mp_digit being used in the root degree we can improve on the initial approximation
						//                NOTE assuming that 
						int64_t rootDegreeDigit=mp_get_i64(rootDegreeBiginteger);
						// let's change the initial approximation of the root using the n root method on the big integer numerator and denominator
						if((result=mp_n_root(p_a,(mp_digit)rootDegreeDigit,_pk))==MP_OKAY&&
							(!q_a||(result=mp_n_root(q_a,(mp_digit)rootDegreeDigit,_qk))==MP_OKAY)){
							// TODO we could have a match already (currently discovered in the first step of the iterations below)
							// _pk now not above n root of p_a
							// _qk now not above n root of q_a
							// TODO knowing that the root argument rational is now between p_a/(q_a+1) and (p_a+1)/q_a
							/* let's not do this and allow the first initial solution to be on the wrong side
							// if we have a denominator that is smaller than p_a (i.e. the argument rational is larger than 1) go over 
							if(q_a&&mp_cmp(p_a,q_a)>0)result=mp_incr(_pk);
							*/
						}
						if(result==MP_OKAY){
							// try to initialize root degree times the denominator of the root argument (which could be NULL when it equals 1)
							Mbiginteger* _np_a=_getBigintegerCopy(rootDegreeBiginteger);
							if(_np_a&&q_a&&mp_mul(_np_a,q_a,_np_a)!=MP_OKAY){free_biginteger(_np_a);_np_a=NULL;}
							if(_np_a){
								// we need some additional helper big integers
								Mbiginteger *_pktothepowern=__biginteger(),*_qktothepowern=_getBiginteger(1),*_delta1=__biginteger(),*_delta2=__biginteger(),*_distancenumerator=__biginteger(),*_pktothepowernminus1=__biginteger(),*_divremainder=__biginteger(),*_gcd=__biginteger();
								Mbiginteger *_num1=__biginteger(),*_num=__biginteger(),*_den=__biginteger(),*_nextpk=__biginteger(),*_nextqk=__biginteger(); // initially the same as _pk and _qk
								Mbiginteger *_distancedenominator=__biginteger(); // the distance to the root
								Mbiginteger *_pkctothepowern=__biginteger();
								Mbiginteger *_pkonthisside=__biginteger(),*_pkontheotherside=__biginteger(),*_deltapk=__biginteger(),*_distanceonthisside=__biginteger(),*_distanceontheotherside=__biginteger(),*_pkdifference=__biginteger(),*_pkhalfway=__biginteger(),*_distancehalfway=__biginteger(),*_one=_getBiginteger(1); // what we'll use for determining a value below the root
								if(_pktothepowern&&_qktothepowern&&_delta1&&_delta2&&_distancenumerator&&_pktothepowernminus1&&_divremainder&&_gcd&&_nextpk&&_nextqk&&_num1&&_num&&_den&&_distancedenominator&&_pkctothepowern&&_pkonthisside&&_pkontheotherside&&_deltapk&&_distanceonthisside&&_distanceontheotherside&&_pkdifference&&_pkhalfway&&_distancehalfway&&_one){
									char c;
									unsigned long long iter=0;
									Mrational* _rational;
									Mdecimal* _decimal;
									while(++iter){
										output("\nRational root approximation #%lld: ",iter);outputBiginteger("(",_pk,NULL);outputBiginteger("/",_qk,")");
										// let's show the decimal representation of this value
										_rational=_getRational(_getBigintegerCopy(_pk),_getBigintegerCopy(_qk),M_LD_NAN,false,true);
										if(_rational){_decimal=_getRationalDecimal(_rational);free_rational(_rational);if(_decimal){outputDecimal("=",_decimal,NULL);free_decimal(_decimal);}}
										output(".\n");
										// update the delta
										outputBiginteger("\tNumerator ",_pk," to power");outputBiginteger(" ",rootDegreeBiginteger,":");
										if(computeBigintegerPower(_pk,rootDegreeBiginteger,_pktothepowern)!=MP_OKAY){outputError("Failed to compute the power of the numerator of the rational approximation");break;}
										outputBiginteger(" ",_pktothepowern,".\n");
										outputBiginteger("\tDenominator ",_qk," to power");outputBiginteger(" ",rootDegreeBiginteger,":");
										if(computeBigintegerPower(_qk,rootDegreeBiginteger,_qktothepowern)!=MP_OKAY){outputError("Failed to compute the power of the numerator of the rational approximation");break;}
										outputBiginteger(" ",_qktothepowern,".\n");

										/* replacing:
										if(mp_exptmod(_pk,rootDegreeBiginteger,NULL,_pktothepowern)!=MP_OKAY){outputError("Failed to compute the power of the numerator of the rational approximation");break;}
										if(mp_exptmod(_qk,rootDegreeBiginteger,NULL,_qktothepowern)!=MP_OKAY){outputError("Failed to compute the power of the denominator of the rational approximation");break;}
										*/
										if(!q_a||!_delta1||mp_mul(_pktothepowern,q_a,_delta1)!=MP_OKAY){outputError("Failed to compute delta1 in the rational approximation to the root of a rational");break;}
										outputBiginteger("\tDelta 1: ",_delta1,".\n");
										if(!p_a||!_delta2||mp_mul(_qktothepowern,p_a,_delta2)!=MP_OKAY){outputError("Failed to compute delta1 in the rational approximation to the root of a rational");break;}
										outputBiginteger("\tDelta 2: ",_delta2,".\n");
										if(!_delta2||!_delta1||!_distancenumerator||mp_sub(_delta2,_delta1,_distancenumerator)!=MP_OKAY){outputError("Failed to compute the delta in the rational approximation of the root of a rational");break;}
										// we can compute the denominator of the distance as well which is q_a times _qktothepowern
										if(!_qktothepowern||!q_a||!_distancedenominator||mp_mul(_qktothepowern,q_a,_distancedenominator)!=MP_OKAY){outputError("Failed to compute the denominator of the distance to the rational root argument");break;}

										outputBiginteger("\tDistance from (",_pk,"/");outputBiginteger(NULL,_qk,")");outputBiginteger("**",rootDegreeBiginteger," to ");
										outputBiginteger("root argument (",p_a,"/");outputBiginteger(NULL,q_a,"): ");
										outputBiginteger("(",_distancenumerator,"/");outputBiginteger(NULL,_distancedenominator,")");
										_rational=_getRational(_getBigintegerCopy(_distancenumerator),_getBigintegerCopy(_distancedenominator),M_LD_NAN,false,true);
										if(_rational){_decimal=_getRationalDecimal(_rational);free_rational(_rational);if(_decimal){outputDecimal("=",_decimal,NULL);free_decimal(_decimal);}}
										outputChar('\n');

										if(mp_iszero(_distancenumerator))break; // if delta is zero, exact hit (which I think can only happen when)
										
										if(mp_div(_pktothepowern,_pk,_pktothepowernminus1,_divremainder)!=MP_OKAY){outputError("Failed to compute a helper big integer in the rational approximation of the root of a rational");break;}
										// update _pk (next) and _qk (next)
										if(mp_mul(_pktothepowern,_np_a,_nextpk)!=MP_OKAY){outputError("Failed to update the numerator of the rational approximation to the root of a rational");break;}
										if(mp_add(_nextpk,_distancenumerator,_nextpk)!=MP_OKAY){outputError("Failed to update the numerator of the rational approximation to the root of a rational");break;}
										if(mp_mul(_qk,_pktothepowernminus1,_nextqk)!=MP_OKAY){outputError("Failed to update the denominator of the rational approximation to the root of a rational");break;}
										if(mp_mul(_nextqk,_np_a,_nextqk)!=MP_OKAY){outputError("Failed to update the denominator of the rational approximation to the root of a rational");break;}
										// that's neat isn't it?
										// how about normalizing _pk and _qk here, which might help
										if(mp_gcd(_nextpk,_nextqk,_gcd)!=MP_OKAY){outputError("Failed to compute the greatest common denominator of the numerator and denominator approximation to the root of a rational");break;}
										if(!isBigintegerOne(_gcd)&&(mp_div(_nextpk,_gcd,_nextpk,_divremainder)!=MP_OKAY||mp_div(_nextqk,_gcd,_nextqk,_divremainder)!=MP_OKAY)){outputError("Failed to normalize the numerator and denominator approximation to the root of a rational");break;}

										// do the bracketing here (on the next pk and qk) 
										// ASSERT we have to ascertain that the denominator remains the same!!!!!
										// the sign of distance numerator tells us on which side of the root we are
										// how about using two big integers???? starting out with 
										if(computeBigintegerPower(_nextpk,rootDegreeBiginteger,_pktothepowern)!=MP_OKAY){outputError("Failed to initialize the distance numerator for bracketing.");break;}
										if(computeBigintegerPower(_nextqk,rootDegreeBiginteger,_qktothepowern)!=MP_OKAY){outputError("Failed to initialize the distance denominator for bracketing.");break;}
										if(mp_mul(_pktothepowern,q_a,_delta1)!=MP_OKAY){outputError("Failed to compute delta1 in the rational approximation to the root of a rational");break;}
										if(mp_mul(_qktothepowern,p_a,_delta2)!=MP_OKAY){outputError("Failed to compute delta1 in the rational approximation to the root of a rational");break;}
										if(mp_sub(_delta2,_delta1,_distancenumerator)!=MP_OKAY){outputError("Failed to compute the new distance numerator in the rational approximation of the root of a rational");break;}
										if(mp_mul(_qktothepowern,q_a,_distancedenominator)!=MP_OKAY){outputError("Failed to compute new distance denominator of the rational approximation of the root of a rational");break;}
										outputBiginteger("\n\tDistance of the next Newtonian approximation (",_nextpk,"/");
										outputBiginteger(NULL,_nextqk,"):");outputBiginteger("(",_distancenumerator,"/");outputBiginteger(NULL,_distancedenominator,").\n");

										if(mp_copy(_nextpk,_pkonthisside)==MP_OKAY&&mp_copy(_nextpk,_pkontheotherside)==MP_OKAY&&mp_copy(_distancenumerator,_distanceonthisside)==MP_OKAY){
											output("\tWill use the Newtonian approximation to bracket the rational root with two successive rationals");outputBiginteger(" with denominator ",_nextqk,".\n");
											/* show the starting point of bracketing!!!
											outputBiginteger("\tBracketing initialized starting at (",_nextpk,"/");outputBiginteger(NULL,_nextqk,")");
											outputBiginteger(" with distance (",_distancenumerator,"/");outputBiginteger(NULL,_distancedenominator,").\n");
											*/
											mp_set_i64(_deltapk,(mp_isneg(_distancenumerator)==MP_YES?-1:1));
											unsigned long long halvingiterations=0,bracketingiterations=0;
											while(1){
												if(mp_iszero(_deltapk)){ // we have two solutions, one on this side and one on the other side
													// the difference could be one between pkonthisside and pkontheotherside in which case we're done
													if(mp_sub(_pkonthisside,_pkontheotherside,_pkdifference)!=MP_OKAY)break;
													if(mp_cmp_mag(_pkdifference,_one)<=0)break;
													if(mp_add(_pkonthisside,_pkontheotherside,_pkhalfway)!=MP_OKAY)break;
													if(mp_div_2(_pkhalfway,_pkhalfway)!=MP_OKAY)break;
													if(computeBigintegerPower(_pkhalfway,rootDegreeBiginteger,_distancehalfway)!=MP_OKAY)break;
													if(mp_mul(_distancehalfway,q_a,_distancehalfway)!=MP_OKAY)break;
													if(mp_sub(_delta2,_distancehalfway,_distancehalfway)!=MP_OKAY)break;
													//////outputBiginteger("\tDistance of half way numerator (",_pkhalfway,"/");outputBiginteger(NULL,_qk,"):");outputBiginteger(" ",_distancehalfway,".\n");
													// replace the pk on the same side with the half way one, so soon the bracketing will end
													if(mp_copy(_pkhalfway,(mp_isneg(_distancehalfway)==mp_isneg(_distanceontheotherside)?_pkontheotherside:_pkonthisside))!=MP_OKAY)break;
													halvingiterations++;
												}else{
													if(mp_add(_pkontheotherside,_deltapk,_pkontheotherside)!=MP_OKAY)break; // keep going 
													// as we are computing _distanceontheotherside we can use it to store intermediate results
													if(computeBigintegerPower(_pkontheotherside,rootDegreeBiginteger,_distanceontheotherside)!=MP_OKAY)break;
													// what is the distance now???? NOTE _delta2 remains the same because _qk won't change!!!!
													if(mp_mul(_distanceontheotherside,q_a,_distanceontheotherside)!=MP_OKAY)break;
													if(mp_sub(_delta2,_distanceontheotherside,_distanceontheotherside)!=MP_OKAY)break;
													///////outputBiginteger("\tDistance of corrected numerator (",_pkontheotherside,"/");outputBiginteger(NULL,_qk,"):");outputBiginteger(" ",_distanceontheotherside,".\n");
													if(mp_isneg(_distanceonthisside)==mp_isneg(_distanceontheotherside)){ // still on this side
														if(mp_mul_2(_deltapk,_deltapk)!=MP_OKAY)break; // double _deltapk otherwise we're going to slow!!!
													}else // yes we're on the other side now, so make _deltapk 0
														mp_set_i64(_deltapk,0);
													bracketingiterations++;
												}
											}
											// how about showing the brackets
											output("\tNumber of bracketing iterations=%llu - number of halving iterations=%llu.\n",bracketingiterations,halvingiterations);
											outputBiginteger("\tNumerator of approximation on this side of the root: ",_pkonthisside,NULL);outputBiginteger(" with distance ",_distanceonthisside,".\n");
											outputBiginteger("\tNumerator of approximation on the other side of the root: ",_pkontheotherside,NULL);outputBiginteger(" with distance ",_distanceontheotherside,".\n");
											// we need the one with a negative distance
											if(mp_copy((mp_isneg(_distanceontheotherside)?_pkontheotherside:_pkonthisside),_nextpk)!=MP_OKAY)break;
											outputBiginteger("\tAccepted approximation numerator from bracketing: ",_nextpk,".\n");
										}else
											output("\t%sFailed to perform rational root bracketing.\n",ERROR_PREFIX);


										// what's the change in approximation?
										if(mp_mul(_pk,_nextqk,_num)!=MP_OKAY){outputError("Failed to initialize the numerator of the change to the rational root approximation");break;}
										if(mp_mul(_qk,_nextpk,_num1)!=MP_OKAY){outputError("Failed to initialize the change to the rational root approximation");break;}
										if(mp_sub(_num,_num1,_num)!=MP_OKAY){outputError("Failed to compute the numerator of the change to the rational root approximation");break;}
										if(mp_mul(_nextqk,_qk,_den)!=MP_OKAY){outputError("Failed to compute the denominator of the change to the rational root approximation");break;}
										if(mp_gcd(_num,_den,_gcd)!=MP_OKAY){outputError("Failed to compute the greatest common denominator of the change in rational approximation to the root of a rational");break;}
										if(!isBigintegerOne(_gcd)&&(mp_div(_num,_gcd,_num,_divremainder)!=MP_OKAY||mp_div(_den,_gcd,_den,_divremainder)!=MP_OKAY)){outputError("Failed to normalize the change in the rational approximation to the root of a rational");break;}
										output("\tChange in rational approximation: ",iter);outputBiginteger("(",_num,NULL);outputBiginteger("/",_den,")");
										bool decimalprecisionreached=false;
										_rational=_getRational(_getBigintegerCopy(_num),_getBigintegerCopy(_den),M_LD_NAN,false,true);
										if(_rational){
											_decimal=_getRationalDecimal(_rational);free_rational(_rational);
											if(_decimal){
												if(mpd_iszero(_decimal->mpd)==MP_YES)decimalprecisionreached=true;
												outputDecimal("=",_decimal,NULL);
												free_decimal(_decimal);
											}
										}
										output(".\n");
										if(decimalprecisionreached)break; // decimal precision reached

										output("\t%s...","Press Ctrl-C to stop, or any other key to continue");inputCharRead(&c);outputChar('\n'); // wait for any key
										if(c==3)break;

										if(mp_copy(_nextpk,_pk)!=MP_OKAY){outputError("Failed to update the numerator of the rational root approximation");break;}
										if(mp_copy(_nextqk,_qk)!=MP_OKAY){outputError("Failed to update the denominator of the rational root approximation");break;}

									}
									free_biginteger(_pktothepowern);free_biginteger(_qktothepowern);free_biginteger(_delta1);free_biginteger(_delta2);free_biginteger(_distancenumerator);
									free_biginteger(_pktothepowernminus1);free_biginteger(_divremainder);free_biginteger(_gcd);
									free_biginteger(_num1);free_biginteger(_num);free_biginteger(_den);free_biginteger(_nextpk);free_biginteger(_nextqk);
									free_biginteger(_distancedenominator);
									free_biginteger(_pkctothepowern);
									free_biginteger(_pkonthisside);free_biginteger(_pkontheotherside);free_biginteger(_deltapk);free_biginteger(_distanceonthisside);free_biginteger(_distanceontheotherside);
									free_biginteger(_pkhalfway);free_biginteger(_distancehalfway);free_biginteger(_one);
								}
								free_biginteger(_np_a);
								_rationalBigintegerRootRational=_getRational(_pk,_qk,M_LD_NAN,true,false);
							}
						}else
							outputError("Failed to initialize the rational root approximation");
					}else
						outputError("Failed to initialize the rational rational root approximation");
					if(!rootArgumentRational->den)free_biginteger(q_a); // MDH@30OCT2019: if the root argument denominator equals 1 i.e. the rational is actually a (big) integer...
					// take care of freeing the result numerator and denominator when we do not have a rational root rational
					if(!_rationalBigintegerRootRational){free_biginteger(_pk);free_biginteger(_qk);}
				}else
					outputError("The root degree is too large (which should never happen though, as it should have been prevented)");
			}else
				_rationalBigintegerRootRational=_getRationalCopy(rootArgumentRational);
		}else // the root degree equals 0
			_rationalBigintegerRootRational=_getRational(NULL,NULL,M_LD_NAN,false,false);
	}
	return _rationalBigintegerRootRational;
}
// MDH@10OCT2019: better to return a decimal instead of already wrapping the result in a value (so we can do postprocessing!!!!)
//                wait we're wrapping it because the result could be different from a decimal!!!!
Mvalue* _getBigintegerRootValue(Mvalue* rootArgumentValue,Mbiginteger* rootDegreeBiginteger){
	Mvalue* _bigintegerRootValue=NULL;
	if(rootArgumentValue&&rootDegreeBiginteger){
		////if(amVerbose())
		{outputValue("Determining the root of ",rootArgumentValue,NULL);outputBiginteger(" with degree ",rootDegreeBiginteger,".\n");}
		// TODO check for special values like 0 or 1 or negatives...
		// computing with true decimals is fine, but with a decimal that is a rational approximation (i.e. with repeating) we're in trouble
		// a rational with a delta should be purified
		// we can do the decimal approximation first
		Mdecimal* _rootArgumentDecimal=_getValueDecimal(rootArgumentValue);
		if(_rootArgumentDecimal){
			uint32_t status=0;
			outputDecimal("Root argument decimal: '",_rootArgumentDecimal,"'.\n");
			// we need an mpd_context for use in the decimal computations!!
			Mdecimalcontext* _decimalcontext=_getDecimalcontext(_rootArgumentDecimal->prec);
			mpd_context_t* mpd_context=(_decimalcontext?_decimalcontext->mpd_context:get_default_mpd_context());
			if(mpd_context){
				Mdecimal* _rootDegreeDecimal=_getBigintegerDecimal(rootDegreeBiginteger);
				if(_rootDegreeDecimal){
					outputDecimal("Root degree decimal: '",_rootDegreeDecimal,"'.\n");
					// MDH@10OCT2019: to anticipate on root arguments smaller than 1 of which the root will be larger instead of smaller we use the square root as first approximation
					// MDH@10OCT2019: because we are approaching the root from above, as soon as the next approximation is equal to or larger than the previous approximation we're done
					//                this means not using the distance anymore because e.g. 2**(7/9) with decimal precision 20 failed to converge (resulted in toggling between two decimals that different by the final digit)
					Mdecimal *_bigintegerRootDecimal=__decimal(mpd_context,1,0),*_nextBigintegerRootDecimal=__decimal(mpd_context,0,0); // let's use 1 as first approximation for any decimal that is below 1
					if(_bigintegerRootDecimal&&_nextBigintegerRootDecimal){
						outputLine("Root computation result decimals created...");
						uint32_t status=0;
						// let's determine on which side of one the root argument is located!!!!
						int rootArgumentComparison=mpd_qcmp(_rootArgumentDecimal->mpd,_bigintegerRootDecimal->mpd,&status);
						// if the root argument is equal to 1, the solution is 1 of course, and no need to continue
						if(rootArgumentComparison>0)mpd_qsqrt(_bigintegerRootDecimal->mpd,_rootArgumentDecimal->mpd,mpd_context,&status);
						// if the root argument does not equal one and we managed to initialize the root argument (to either 1 or the square root), we may continue
						if(rootArgumentComparison&&!(status&0xEFBF)){
							outputDecimal("Root computation result decimals initialized to ",_bigintegerRootDecimal,".\n");
							// TODO only when the root degree is larger than 2 do we do the iterative process
							// 0. preparations: we need (root degree - 1 ) regularly
							Mdecimal* _rootDegreeMinus1Decimal=__decimal(mpd_context,0,0); /////_getDecimalCopy(_rootDegreeDecimal);
							if(_rootDegreeMinus1Decimal){
								outputLine("Root computation helper decimal created...");
								// can't I use getDecimalOne() here?????? apparently not!!
								mpd_t* _decimalOne=__mpd(mpd_context,1);
								mpd_qsub(_rootDegreeMinus1Decimal->mpd,_rootDegreeDecimal->mpd,_decimalOne,mpd_context,&status);
								free_mpd(_decimalOne);
								if((status&0xEFBF)==0){
									outputLine("Root computation helper decimal initialized...");
									// we need the root degree minus 1 as big integer as well
									Mbiginteger* _rootDegreeMinus1Biginteger=_getBigintegerCopy(rootDegreeBiginteger);
									if(_rootDegreeMinus1Biginteger){
										outputLine("Root computation helper big integer created...");
										if(mp_decr(_rootDegreeMinus1Biginteger)==MP_OKAY){
											outputLine("Root computation helper big integer initialized...");
											// we need a product, a quotient and an addition help decimal
											/*
											Mdecimal *_distance=__decimal(mpd_context,0,0),*_prevdistance=__decimal(mpd_context,0,0);
											*/
											Mdecimal *_product=__decimal(mpd_context,0,0),*_quotient=__decimal(mpd_context,0,0),*_productplusquotient=__decimal(mpd_context,0,0),*_power=__decimal(mpd_context,0,0);
											// initial value of the quotient denominator that we need for checking whether we're done and in the computation
											if(/*_distance&&_prevdistance&&*/_product&&_quotient&&_productplusquotient&&_power){
												outputLine("Root computation helper decimals created...");
												// ready to rock 'n' roll, eh iterate
												// NOTE iterating until the next value is the same wasn't working, it might be better to compute the power value itself and to compare with the root argument value, if match stop!!
												unsigned long long iter=0;
												Mdecimal *_quotientdenominator=NULL;
												char c;
												while((status&0xEFBF)==0){
													// 'update' the quotient denominator, so we can use it in checking whether we are already there yet, and if not in the computation
													// TODO might it be a good idea to compute the quotient and compare the quotient with the current solution??????
													_quotientdenominator=_getDecimalPowerWithPositiveBigintegerExponent(_bigintegerRootDecimal,_rootDegreeMinus1Biginteger);
													if(!_quotientdenominator){status=0xFFFFFFFF;break;}
													/* MDH@10OCT2019: not using the distance anymore!!!
													// are we there yet?????
													// compute the current power value
													mpd_qmul(_power->mpd,_quotientdenominator->mpd,_bigintegerRootDecimal->mpd,mpd_context,&status);
													if((status&0xEFBF)!=0)break;
													// if we like to know the distance to the goal we have to compute the difference
													mpd_qsub(_distance->mpd,_power->mpd,_bigintegerRootDecimal->mpd,mpd_context,&status);
													if((status&0xEFBF)!=0)break;
													// if the distance hasn't changed we're done (as we noticed the distance won't be zero in general)
													if(mpd_qcmp(_distance->mpd,_prevdistance->mpd,&status)==0)break; // a match, so done
													free_mpd(_prevdistance->mpd);_prevdistance->mpd=mpd_qncopy(_distance->mpd);
													if(!_prevdistance->mpd){outputError("Failed to copy the distance!");break;}
													*/
													// replacing: if(mpd_iszero(_distance->mpd))break;
													// if the product of the quotient denominator and the root decimal equals the root argument
													// replacing: if(mpd_qcmp(_power->mpd,_bigintegerRootDecimal->mpd,&status)==0)break; // a match, so done
													// next iteration!!!!
													iter++;
													if(amVerbose()){
														output("Root approximation at iteration #%" PRIu32 ":",iter);
														outputDecimal(" ",_bigintegerRootDecimal,".");
														////////outputDecimal(" Distance: ",_distance,".");
														output(" %s...","Press any key to continue");
														inputCharRead(&c);
														outputChar('\n');
													}
													mpd_qdiv(_quotient->mpd,_rootArgumentDecimal->mpd,_quotientdenominator->mpd,mpd_context,&status);
													free_decimal(_quotientdenominator); // don't need it anymore
													mpd_qmul(_product->mpd,_rootDegreeMinus1Decimal->mpd,_bigintegerRootDecimal->mpd,mpd_context,&status);
													mpd_qadd(_productplusquotient->mpd,_product->mpd,_quotient->mpd,mpd_context,&status);
													// update the solution
													mpd_qdiv(_nextBigintegerRootDecimal->mpd,_productplusquotient->mpd,_rootDegreeDecimal->mpd,mpd_context,&status);
													// check whether done or not which is when the next approximation is not smaller than the previous approximation
													if(mpd_qcmp(_nextBigintegerRootDecimal->mpd,_bigintegerRootDecimal->mpd,&status)>=0)break;
													// update _bigintegerRootDecimal to _nextBigintegerRootDecimal
													free_mpd(_bigintegerRootDecimal->mpd);_bigintegerRootDecimal->mpd=mpd_qncopy(_nextBigintegerRootDecimal->mpd);
												}
												if((status&0xEFBF)!=0){
													output("%sRoot computation ended with error code " PRIu32 ".\n",ERROR_PREFIX,status);
													free_decimal(_bigintegerRootDecimal);
												}else{
													_bigintegerRootValue=_getDecimalValue(_bigintegerRootDecimal,true);
													///////if(amVerbose())
													outputDecimal("Root computation result decimal: '",_bigintegerRootDecimal,"'.\n");
												}
											}else
												outputError("Failed to create helper decimals in computing a root decimal");
											/*
											free_decimal(_distance);free_decimal(_prevdistance);
											*/
											free_decimal(_product);free_decimal(_quotient);
											free_decimal(_productplusquotient);free_decimal(_power);
										}else
											outputError("Failed to compute a helper big integer in computing a root decimal");
										free_biginteger(_rootDegreeMinus1Biginteger);
										outputLine("Root computation helper big integer released...");
									}else
										outputError("Failed to copy the root degree in computing a root decimal");
								}else
									outputError("Failed to compute a helper decimal in computing a root decimal");
							}else
								outputError("Failed to create a helper decimal in computing a root decimal");
							free_decimal(_rootDegreeMinus1Decimal);
							outputLine("Root computation helper decimal released...");
						}else
						if(rootArgumentComparison)
							outputError("Failed to initialize the result of the root computation to the square root");
						else // wrap the result (which is 1)
							_bigintegerRootValue=_getDecimalValue(_bigintegerRootDecimal,true);
					}
					free_decimal(_nextBigintegerRootDecimal);
					free_decimal(_rootDegreeDecimal);
					outputLine("Root computation degree decimal released...");
				}else{
					output("%s",ERROR_PREFIX);outputBiginteger("Failed to convert root degree '",rootDegreeBiginteger,"' to a decimal.\n");
				}
			}else
				outputError("Failed to create a decimal context for computing a decimal root");
			if(rootArgumentValue->type!=VT_DECIMAL)free_decimal(_rootArgumentDecimal);
		}else{
			output("%s",ERROR_PREFIX);outputValue("Failed to convert root argument '",rootArgumentValue,"' to a decimal.\n");
		}
	}
	return _bigintegerRootValue;
}
Mvalue* power(Mvalue* _value1,Mvalue* _value2){
	if(!_value1||!_value2)return NULL;
	if(_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,power);if(_value2->type==VT_LIST)return _appliedToList2(_value1,_value2->value._list,power);
	if(isValueZero(_value1)==M_TRUE)return _value1;
	if(isValueZero(_value2)==M_TRUE)return _getValueOneOfType(_value1->type); // if the power is zero, we return the value 1 with the same type as 
	// MDH@26OCT2019: TODO same approach with any integer as in the other binary operators??????
	// MDH@27OCT2019: let's deal with if either is a real first
	// I suppose if the base or exponent is real, the result should also be real (because it will be approximate)
	if(_value2->type==VT_FLOAT)return _getFloatValue(getFloatValuePower(_value1,_value2->value._float->ld));
	// ASSERT exponent is NOT a real
	if(_value1->type==VT_FLOAT)return _getFloatValue(getRealPowerValue(_value1->value._float->ld,_value2));
	// ASSERT neither is real
	// MDH@27OCT2019: typically for integers with an expoonent that is positive the result should also be integer
	//                and we deal with that separatately
	if((_value1->type==VT_INTEGER||_value1->type==VT_BIGINTEGER)&&
		((_value2->type==VT_INTEGER&&isIntegerPositive(_value2->value._integer)==M_TRUE)||
		 (_value2->type==VT_BIGINTEGER&&isBigintegerPositive(_value2->value._biginteger)==M_TRUE))){
		bool smallinteger1=(_value1->type==VT_INTEGER),smallinteger2=(_value2->type==VT_INTEGER);
		bool invalidinteger1=(smallinteger1&&_value1->value._integer->ll==M_LL_INVALID),invalidinteger2=(smallinteger2&&_value2->value._integer->ll==M_LL_INVALID);
		if(invalidinteger1||invalidinteger2)return _getIntegerValue(M_LL_INVALID); // if either integer is invalid return an invalid integer (which per definition will be small)
		Mbiginteger* _powerBiginteger=NULL;
		// ASSERT both integers are considered valid (i.e. not invalid)
		Mbiginteger *_biginteger1=(smallinteger1?_getBiginteger(_value1->value._integer->ll):_value1->value._biginteger);
		Mbiginteger *_biginteger2=(smallinteger2?_getBiginteger(_value2->value._integer->ll):_value2->value._biginteger);
		// replacing: Mbiginteger *_biginteger1=_getValueBiginteger(_value1),*_biginteger2=_getValueBiginteger(_value2); // OOPS careful here, _getValueDecimal would make a copy which we do not want here!!!!
		if(_biginteger1&&_biginteger2){
			if(amVerbose()){outputBiginteger("Exponentiating big integers '",_biginteger1,"'");outputBiginteger(" and '",_biginteger2,"'");}
			_powerBiginteger=_getBigintegerPowerWithPositiveBigintegerExponent(_biginteger1,_biginteger2);
			if(amVerbose()){outputBiginteger(" - Power: '",_powerBiginteger,"'.\n");}
		}else
			outputError("Failed to convert a small integer to a big integer");
		if(smallinteger1)free_biginteger(_biginteger1);
		if(smallinteger2)free_biginteger(_biginteger2);
		// MDH@24OCT2019: if the base is integer, we're going to try to return a small integer
		if(smallinteger1){ // we could decide to try to keep the value in range if at least one of the integers is small (instead of demanding both are small integers)
			// if computing the sum failed return the invalid (small) integer (to indicate a missing result)
			if(!_powerBiginteger)return _getIntegerValue(M_LL_INVALID);
			long long llpower=getBigintegerInteger(_powerBiginteger); // will return M_LL_INVALID when _sumBiginteger equals NULL (which we want to exclude)
			// if we do NOT have a sum big integer or the sum big integer is in range ()
			if(llpower!=M_LL_INVALID){free_biginteger(_powerBiginteger);return _getIntegerValue(llpower);}
			output("WARNING: Small integer power out of range, will continue using big integer power.\n");
		}
		return _getBigintegerValue(_powerBiginteger,true);
	}
	if((_value1->type==VT_INTEGER||_value1->type==VT_BIGINTEGER||_value1->type==VT_DECIMAL||_value1->type==VT_RATIONAL)&&
		(_value2->type==VT_INTEGER||_value2->type==VT_BIGINTEGER||_value2->type==VT_DECIMAL||_value2->type==VT_RATIONAL)){
		// computing the power is not so easy for certain value type combinations
		// ASSERT base and exponent are not reals
		// given that the way to compute the power might be different depending on the type of the exponent if differentiate between that
		Mvalue* _returnValue=NULL;
		// 1. when the exponent is integer
		if(_value2->type==VT_INTEGER||_value2->type==VT_BIGINTEGER||(_value2->type==VT_RATIONAL&&(!_value2->value._rational->den||isBigintegerOne(_value2->value._rational->den)))){
			Mbiginteger* _exponentBiginteger=_getValueBiginteger(_value2);
			_returnValue=_getBigintegerPowerValue(_value1,_exponentBiginteger);
			if(_value2->type!=VT_BIGINTEGER)free_biginteger(_exponentBiginteger);
		}else{ // non-integer exponent, only decimals and rationals remaining
			// if the exponent is inherently rational we should use 
			Mrational* _exponentRational=NULL;
			if(_value2->type==VT_RATIONAL)_exponentRational=_value2->value._rational;
			else 
			if(_value2->type==VT_DECIMAL&&_value2->value._decimal->repeating>0)_exponentRational=_getDecimalRational(_value2->value._decimal);
			// MDH@14OCT2019: let's only do a rational approximation if the exponent is rational but the denominator is not too large i.e. using at most a single mp_digit (which might be large enough as it is though)
			if(_exponentRational&&(!_exponentRational->den||_exponentRational->den->used==1)){ // the exponent is rational and the exponent denominator (which results in root finding is not too large)
				Mvalue* _rootValue=NULL; // the result of the computation of taking the power of a decimal to a rational exponent
				//if(amVerbose())
				outputRational("Computing a power with rational exponent ",_exponentRational,".\n");
				bool neg=(_exponentRational->num->sign==MP_NEG);
				Mbiginteger* _positiveExponentNumerator=(neg?_getBigintegerNeg(_exponentRational->num):_exponentRational->num);
				Mbiginteger* exponentDenominator=_exponentRational->den;
				if(_positiveExponentNumerator){ // we 
					// TODO now we are testing whether the denominator does not equal one, but in the future all rationals with denominator 1 should have a NULL denominator!!!
					if(exponentDenominator&&!isBigintegerOne(exponentDenominator)){ // a 'real' rational (i.e. not simply pretending to be one)
						// MDH@10OCT2019: we can improve on the computation of the power by computing the integer quotient of the rational and the remainder
						// MDH@10OCT2019: we can even improve even more by choosing the smallest of the numerator and denominator to be used in the power computation
						//                NO we can't because 2**(x/y) is NOT equal to 1/2**(y/x) as I conjectured, so we have to stick to the original approximation for now
						// MDH@13OCT2019: if the numerator is negative we will have to invert the solution
						
						mp_ord numdencomp=mp_cmp(_positiveExponentNumerator,exponentDenominator);
						if(numdencomp!=MP_EQ){ // numerator and denominator are not equal
							Mbiginteger *_integerdividend=__biginteger(),*_remainder=__biginteger(); // the defaults when the denominator equals NULL
							// we divide the maximum of the numerator and the denominator by the minimum of the numerator and the denominator (which typically means that _integerdividend will always be nonzero essentially)
							if(_integerdividend&&_remainder&&mp_div(_positiveExponentNumerator,exponentDenominator,_integerdividend,_remainder)==MP_OKAY){
								// if _integerdividend is not zero we may compute the multiplier
								Mvalue* _multiplierValue=(mp_iszero(_integerdividend)!=MP_YES?_getBigintegerPowerValue(_value1,_integerdividend):NULL);
								// MDH@10OCT2019: we have a special situation when the root argument (_value1) is rational itself in which case we are computing the 
								// instead of computing the power of the numerator we use the _remainder instead
								Mvalue* rootArgumentValue=_getBigintegerPowerValue(_value1,_remainder); // NOTE will be released by the value garbage collector
								// if the root argument is rational, we should use pure big integer computations and have all rational approximations to the root
								// MDH@30OCT2019: wait a minute, if the root argument value is a big integer we would still like to do a rational root instead of decimal root finding
								//                and also when the it's a rational in disguise (stored as a decimal with repeating digits) CAREFUL use a copy of the big integer calling _getRational!!!
								Mrational* _rootArgumentRational=NULL;
								if(rootArgumentValue->type==VT_BIGINTEGER)_rootArgumentRational=_getRational(_getBigintegerCopy(rootArgumentValue->value._biginteger),NULL,M_LD_NAN,false,false);else
								if(rootArgumentValue->type==VT_RATIONAL&&floatIsUndefinedOrZero(rootArgumentValue->value._rational->delta))_rootArgumentRational=rootArgumentValue->value._rational;else
								if(rootArgumentValue->type==VT_DECIMAL&&rootArgumentValue->value._decimal->repeating>0)_rootArgumentRational=_getDecimalRational(rootArgumentValue->value._decimal);
								if(_rootArgumentRational){ // the root argument is supposedly a rational
									// TODO if the base is not a pure rational, we could of course purify it
									_rootValue=_getRationalValue(_getRationalBigintegerRootRational(_rootArgumentRational,exponentDenominator),true);
									if(rootArgumentValue->type!=VT_RATIONAL)free_rational(_rootArgumentRational);
								}else // base NOT a pure rational, so we're goint go stick with using decimal root approximation i.e. the decimal approximation to the base will be used 
									_rootValue=_getBigintegerRootValue(rootArgumentValue,exponentDenominator);
								// now apply the multiplier if need be
								//if(amVerbose())outputValue("The value to take the root of: '",_rootArgumentValue,"'.\n");
								if(_multiplierValue){ // have to multiply
									_rootValue=multiply(_multiplierValue,_rootValue);
									outputValue("Rational exponent root equals the product of multiplier ",_multiplierValue," and ");
									outputBiginteger("the ",exponentDenominator,"th ");
									outputValue("root of ",rootArgumentValue," ");
									outputValue("which is ",_rootValue,".\n");
								}else{ // no need to multiply
									outputBiginteger("The ",exponentDenominator,"th ");
									outputValue("root of ",rootArgumentValue," "); // TODO what happened to rootArgumentValue????
									outputValue("equals ",_rootValue,".\n");
								}
								///// wrong: if(numdencomp==MP_LT)_rootValue=Mreciprocal(_rootValue); // the numerator is smaller than the denominator, so we need to invert the value
								/* replacing NOT splitting up the rational exponent in an integer and remainder part (under 1)
								// base to the power of a rational is the denominatorth root of the numerators power of the base
								Mvalue* _rootArgumentValue=_getBigintegerPowerValue(_value1,_exponentRational->num);// NOTE will be released by the value garbage collector
								//if(amVerbose())outputValue("The value to take the root of: '",_rootArgumentValue,"'.\n");
								Mvalue* _rootValue=_getBigintegerRootValue(_rootArgumentValue,_exponentRational->den);
								outputValue("Rational exponent root of ",_rootArgumentValue,NULL);outputValue(": ",_rootValue,".\n");
								*/
							}else
								outputError("Failed to determine the integer and fractional part of a rational exponent");
							free_biginteger(_integerdividend);free_biginteger(_remainder);
						}else // the numerator equals the denominator meaning that _value1 is the value to return
							_rootValue=_value1;
					}else // an integer rational, so no need to take the root at all!!!
						_rootValue=_getBigintegerPowerValue(_value1,_positiveExponentNumerator); // that's all folks
					// don't forget the delta (if any)
					if(!floatIsUndefinedOrZero(_exponentRational->delta)) // a defined delta
						// multiply the result with base to the power of delta
						// TODO the base should determine what the type of the power computation should be???????
						_returnValue=multiply(_rootValue,_getFloatValue(getFloatValuePower(_value1,getReal(_exponentRational->delta))));
					else
						_returnValue=_rootValue;
					if(neg){free_biginteger(_positiveExponentNumerator);if(_returnValue)_returnValue=Mreciprocal(_returnValue);}
				}else
					outputError("Failed to reverse the sign of the rational exponent");
				if(_value2->type!=VT_RATIONAL)free_rational(_exponentRational);
			}else{ // not integer based exponent (so if the exponent is a decimal is does not have a repeating part), so use decimals
				// there's a mpd_pow() methods that we technically use on anything that convertable to a decimal
				// converting a rational to a decimal is difficult unless the rational represents a decimal (i.e. the denominator is a power of 10 or we can make it a power of 10 somehow)
				Mdecimal *_baseDecimal=getValueDecimal(_value1),*_exponentDecimal=getValueDecimal(_value2);
				if(_baseDecimal&&_exponentDecimal){
					Mdecimal* _powerDecimal=NULL;
					mpd_context_t* mpd_context=getContextOfDecimals(_baseDecimal,_exponentDecimal);
					if(!mpd_context)outputError("No decimal context for use in the power function");
					// the decimal library has a function to compute the power of two decimals and we can use that for most of the value pairs
					// if either has a repeating part we have a problem
					if(_baseDecimal->repeating>0){
						if(amVerbose())outputLine("Computing the power of a rational.");
						// the result is the quotient of the power of the numerator divided by the power of the denominator of the associated rational
						Mrational* _baseRational=getValueRational(_value1);
						Mdecimal* _baseNumDecimal=_getBigintegerDecimal(_baseRational->num);
						Mdecimal* _numPowerDecimal=_getDecimalPower(_baseNumDecimal->mpd,_exponentDecimal->mpd,mpd_context);
						Mdecimal* _baseDenDecimal=_getBigintegerDecimal(_baseRational->den);
						Mdecimal* _denPowerDecimal=_getDecimalPower(_baseDenDecimal->mpd,_exponentDecimal->mpd,mpd_context);
						_powerDecimal=__decimal(mpd_context,0,0);
						// the quotient of the numerator and denominator power is the end result
						if(_powerDecimal){
							uint32_t status=0;
							mpd_qdiv(_powerDecimal->mpd,_numPowerDecimal->mpd,_denPowerDecimal->mpd,mpd_context,&status);
							if((status&0xEFBF)!=0){outputError("Failed to divide the numerator and denominator powers");free_decimal(_powerDecimal);_powerDecimal=NULL;}
						}else
							outputError("Failed to create the decimal result of applying the power function to a rational");
						free_decimal(_baseNumDecimal);free_decimal(_baseDenDecimal);
						free_decimal(_numPowerDecimal);free_decimal(_denPowerDecimal);
						if(_value1->type!=VT_RATIONAL)free_rational(_baseRational);
					}else{ // base and exponent decimals is true
						_powerDecimal=_getDecimalPower(_baseDecimal->mpd,_exponentDecimal->mpd,mpd_context);
						outputLine("Power decimal computed!");
						// if the exponent is integer typed, the base type determines what to return
					}
					if(_powerDecimal)_returnValue=_getDecimalValue(_powerDecimal,true);else outputError("Failed to create the power function result decimal");
				}
				// if the originals weren't decimals, free the created decimals!!!!
				if(_value1->type!=VT_DECIMAL)free_decimal(_baseDecimal);
				if(_value2->type!=VT_DECIMAL)free_decimal(_exponentDecimal);
			}
		}
		return _returnValue;
	}
	return NULL;
}

// MDH@15AUG2019: given that epower means base times 10 to the power of exponent, it makes sense to actually compute epower as mul(base,power(10,exponent)) where for 10 we use a single integer
Mvalue* epower(Mvalue* _value1,Mvalue* _value2){
	// we can still use the shortcuts
	if(!_value1||!_value2)return NULL;
	if(isValueZero(_value1)==M_TRUE||isValueZero(_value2)==M_TRUE)return _value1; // NOTE if the power is zero, the multiplication factor will be 1
	return multiply(_value1,power(_getIntegerValue(10),_value2)); // TODO check whether _getIntegerValue(10) actually gets freed by the 'gc'
	/* replacing:
	if(_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,epower);if(_value2->type==VT_LIST)return _appliedToList2(_value1,_value2->value._list,epower);

	// if both values are numeric (somehow) we can do the computation
	if((_value1->type==VT_INTEGER||_value1->type==VT_FLOAT||_value1->type==VT_BIGINTEGER||_value1->type==VT_DECIMAL||_value1->type==VT_RATIONAL)&&
		(_value2->type==VT_INTEGER||_value2->type==VT_FLOAT||_value2->type==VT_BIGINTEGER||_value2->type==VT_DECIMAL||_value2->type==VT_RATIONAL)){
		// if the epower exponent is zero _value1 is the result
		// see above: if(isValueZero(_value2))return _value1;
		if(_value2->type==VT_INTEGER){ // an integer exponent
			long long exponentOf10=_value2->value._integer->ll;
			// if the power value is 0, _value1 is the result
			// multiplication or division by an integer power of 10 which is an integer therefore
			if(_value1->type==VT_DECIMAL){
				Mdecimal* _decimal=_getDecimalCopy(_value1->value._decimal);
				_decimal->mpd->exp+=exponentOf10; // TODO theoretically we can get overflow here!! the exponent is an int64_t (alternative is using mpd_scaleb)
				return _getDecimalValue(_decimal,true);
			}
			if(_value1->type==VT_RATIONAL){
				// either to multiply the numerator or the denominator with the exponent
				// TODO deal appropriately with any delta!!!
				Mbiginteger* _biginteger10=_getBiginteger(10);
				Mrational* _rational=NULL;
				if(_biginteger10){
					if(exponentOf10>0){
						Mbiginteger* _numerator=_getBigintegerCopy(_value1->value._rational->num);
						while(exponentOf10>0)if(mp_mul(_numerator,_biginteger10,_numerator)==MP_OKAY)exponentOf10--;else break;
						if(exponentOf10==0)
							_rational=_getRational(_numerator,_getBigintegerCopy(_value1->value._rational->den),getReal(_value1->value._rational->delta),true,true);
						else
							outputError("Failed to multiply the numerator of the rational by an integer power of 10.");
					}else{
						Mbiginteger* _denominator=(!_value1->value._rational->den?_getBiginteger(1):_getBigintegerCopy(_value1->value._rational->den));
						while(exponentOf10<0)if(mp_mul(_denominator,_biginteger10,_denominator)==MP_OKAY)exponentOf10++;else break;
						if(exponentOf10==0)
							_rational=_getRational(_getBigintegerCopy(_value1->value._rational->num),_denominator,getReal(_value1->value._rational->delta),true,true);
						else
							outputError("Failed to multiply the denominator of the rational by an integer power of 10.");
					}
					free_biginteger(_biginteger10);
				}
				return _getRationalValue(_rational,true);
			}
		}
		return multiply(_value1,power(_getIntegerValue(10),_value2)); // temp. value like the power result and _getIntegerValue(10) will be garbage collected if not bound somewhere!!!
		// replacing: return _getFloatValue((_value1->type==VT_INTEGER?_value1->value._integer->ll:_value1->value._float->ld*pow(10.,(_value2->type==VT_INTEGER?_value2->value._integer->ll:_value2->value._float->ld))));
	}
	*/
	return NULL;
}

// MDH@07JUN2019: when two integers are presented to divide instead of actually computing the division we can store the division as a rational (so we kind of have a slow evaluation of the division, and we maintain accuracy as long as possible)
Mvalue* divide(Mvalue* _value1,Mvalue* _value2){
	if(!_value1||!_value2)return NULL;
	if(_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,divide);if(_value2->type==VT_LIST)return _appliedToList2(_value1,_value2->value._list,divide);
	if(isValueZero(_value1)==M_TRUE||isValueOne(_value2)==M_TRUE)return _value1;
	if(isValueZero(_value2)==M_TRUE)return NULL; // TODO shouldn't we return infinity?????
	// MDH@26OCT2019: dealing with any integer conform as we did in the other binary operators
	//                NO dividing integers should result in a rational so we can keep the accuracy
	// NOT replacing:
	// integer divisions are not computed but stored in rational format (without a delta to not suggest that the division is decimal)
	if((_value1->type==VT_INTEGER||_value1->type==VT_BIGINTEGER)&&(_value2->type==VT_INTEGER||_value2->type==VT_BIGINTEGER)){ // both are integer
		Mbiginteger* _numerator=(_value1->type==VT_INTEGER?_getBiginteger(_value1->value._integer->ll):_getBigintegerCopy(_value1->value._biginteger));
		Mbiginteger* _denominator=(_value2->type==VT_INTEGER?_getBiginteger(_value2->value._integer->ll):_getBigintegerCopy(_value2->value._biginteger));
		// if the denominator is negative, both the numerator and denominator should be negated (should this be part of the normalization procedure?), theoretically storing the sign separate from the big integers in a rational could also be the way to go
		// so when the sign of the two big integers is different, the rational is negative, otherwise it is positive and _getRational would store the absolute values of the big integer
		// if _getRational would take care of negating the numerator and denominator it would have to free the passed in big integers (if so requested)
		Mrational* _rational=_getRational(_numerator,_denominator,M_LD_NAN,true,true); // free num/den when failing to bind
		return _getRationalValue(_rational,true); // when failing to bind _rational to a value, free it as well
	}
	// if one of them is a rational do a rational division
	if((_value1->type==VT_RATIONAL||(_value1->type==VT_DECIMAL&&_value1->value._decimal->repeating>0))||(_value2->type==VT_RATIONAL||(_value2->type==VT_DECIMAL&&_value2->value._decimal->repeating>0))){
		Mrational *_rational1=getValueRational(_value1),*_rational2=getValueRational(_value2);
		Mrational* _quotientRational=_getRationalQuotient(_rational1,_rational2); // _qdivide replaced by _getRationalQuotient (as defined in Mrational.h/c)
		if(_value1->type!=VT_RATIONAL)free_rational(_rational1);else if(_value2->type!=VT_RATIONAL)free_rational(_rational2); // after dividing the two rationals we do not need the newly created rationals anymore
		Mvalue* _quotientValue=NULL;
		if(_quotientRational){
			if(_value1->type==VT_DECIMAL&&_value2->type==VT_DECIMAL){
				_quotientValue=_getDecimalValue(_getRationalDecimal(_quotientRational),true);
				free_rational(_quotientRational);
			}else
				_quotientValue=_getRationalValue(_quotientRational,true);
		}
		return _quotientValue;
	}
	// if either is a decimal, compute the quotient decimal
	if(_value1->type==VT_DECIMAL||_value2->type==VT_DECIMAL){
		Mdecimal *_decimal1=getValueDecimal(_value1),*_decimal2=getValueDecimal(_value2); // OOPS careful here, _getValueDecimal would make a copy which we do not want here!!!!
		Mdecimal* _divideDecimal=_getDecimalQuotient(_decimal1,_decimal2); // _ddiv now replaced by _getDecimalQuotient which should be able to divide any two decimals not just the pure once!!!!!
		if(_value1->type!=VT_DECIMAL)free_decimal(_decimal1);else if(_value2->type!=VT_DECIMAL)free_decimal(_decimal2); // after adding the two rationals we do not need the newly created rationals anymore
		return _getDecimalValue(_divideDecimal,true);
	}
	// if either is a real
	if(_value1->type==VT_FLOAT||_value2->type==VT_FLOAT){
		if(amVerbose()){outputValue("Dividing (as) reals '",_value1,"'");outputValue(" and '",_value2,"'.\n");}
		long double ld1=getValueLongDouble(_value1),ld2=getValueLongDouble(_value2);
		return _getFloatValue(isLongDoubleUndefined(ld1)==M_FALSE&&isLongDoubleUndefined(ld2)==M_FALSE?ld1/ld2:M_LD_NAN);
	}
	/* replacing:
	// always real divide
	if((_value1->type==VT_INTEGER||_value1->type==VT_FLOAT)&&(_value2->type==VT_INTEGER||_value2->type==VT_FLOAT)){
		long double ld1=(_value1->type==VT_INTEGER?_value1->value._integer->ll:_value1->value._float->ld); // TODO casting to a long double is perhaps not the best way?
		long double ld2=(_value2->type==VT_INTEGER?_value2->value._integer->ll:_value2->value._float->ld); // TODO casting to a long double is perhaps not the best way?
		return _getFloatValue(ld1/ld2);
	}
	*/
	return NULL;
}
Mvalue* integerdivide(Mvalue* _value1,Mvalue* _value2){
	if(!_value1||!_value2)return NULL;
	if(_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,integerdivide);if(_value2->type==VT_LIST)return _appliedToList2(_value1,_value2->value._list,integerdivide);
	if(isValueZero(_value1)==M_TRUE||isValueOne(_value2)==M_TRUE)return _value1;
	if(isValueZero(_value2)==M_TRUE)return NULL; // TODO or should we return some form of infinity?????
	// MDH@26OCT2019: adapted from dealing with any integer type from add()
	if((_value1->type==VT_INTEGER||_value1->type==VT_BIGINTEGER)&&(_value2->type==VT_INTEGER||_value2->type==VT_BIGINTEGER)){
		Mbiginteger* _integerquotientBiginteger=NULL;
		// no need to use _getValueBiginteger because we know the source will be integer
		// NOTE _getBiginteger() was adjusted to return NULL in case ll equals M_LL_INVALID because in that case the result should also be M_LL_INVALID
		// CORRECTION: _getBiginteger() is also used in the I() function and it's a problem if NOT allowing to actually use NAI in computations (as the smallest possible integer)
		// DISCUSSION: M_LL_INVALID is a single long long value that is considered invalid like dividing by zero, or asking for the sign of an undefined real (= long double)
		//             
		bool smallinteger1=(_value1->type==VT_INTEGER),smallinteger2=(_value2->type==VT_INTEGER);
		bool invalidinteger1=(smallinteger1&&_value1->value._integer->ll==M_LL_INVALID),invalidinteger2=(smallinteger2&&_value2->value._integer->ll==M_LL_INVALID);
		if(invalidinteger1||invalidinteger2)return _getIntegerValue(M_LL_INVALID); // if either integer is invalid return an invalid integer (which per definition will be small)
		// ASSERT both integers are considered valid (i.e. not invalid)
		Mbiginteger *_biginteger1=(smallinteger1?_getBiginteger(_value1->value._integer->ll):_value1->value._biginteger);
		Mbiginteger *_biginteger2=(smallinteger2?_getBiginteger(_value2->value._integer->ll):_value2->value._biginteger);
		// replacing: Mbiginteger *_biginteger1=_getValueBiginteger(_value1),*_biginteger2=_getValueBiginteger(_value2); // OOPS careful here, _getValueDecimal would make a copy which we do not want here!!!!
		if(_biginteger1&&_biginteger2){
			if(amVerbose()){outputBiginteger("Integer dividing big integers '",_biginteger1,"'");outputBiginteger(" and '",_biginteger2,"'");}
			_integerquotientBiginteger=__biginteger();
			if(_integerquotientBiginteger&&mp_div(_biginteger1,_biginteger2,_integerquotientBiginteger,NULL)!=MP_OKAY){free_biginteger(_integerquotientBiginteger);_integerquotientBiginteger=NULL;} // _dmul replaced by _getDecimalProduct which should be able to multiply any two decimals (not just the pure decimals)
			if(amVerbose()){outputBiginteger(" - Integer quotient: '",_integerquotientBiginteger,"'.\n");}
		}else
			outputError("Failed to convert a small integer to a big integer");
		if(smallinteger1)free_biginteger(_biginteger1);
		if(smallinteger2)free_biginteger(_biginteger2);
		// MDH@24OCT2019: now we're going to try to convert the sum back to an integer if we can
		//                but if we can't don't
		if(smallinteger1||smallinteger2){ // we could decide to try to keep the value in range if at least one of the integers is small (instead of demanding both are small integers)
			// if computing the sum failed return the invalid (small) integer (to indicate a missing result)
			if(!_integerquotientBiginteger)return _getIntegerValue(M_LL_INVALID);
			long long llintegerquotient=getBigintegerInteger(_integerquotientBiginteger); // will return M_LL_INVALID when _sumBiginteger equals NULL (which we want to exclude)
			// if we do NOT have a sum big integer or the sum big integer is in range ()
			if(llintegerquotient!=M_LL_INVALID){free_biginteger(_integerquotientBiginteger);return _getIntegerValue(llintegerquotient);}
			output("WARNING: Small integer integer quotient out of range, will continue using big integer integer quotient.\n");
		}
		return _getBigintegerValue(_integerquotientBiginteger,true);
	}
	/* replacing:
	// if both are integers, the result should be integer as well!!!
	if(_value1->type==VT_INTEGER&&_value2->type==VT_INTEGER){
			if(amVerbose())output("Integer dividing integers '%lld' and '%lld'.\n",_value1->value._integer->ll,_value2->value._integer->ll);
			return(_value2->value._integer->ll!=0?_getIntegerValue(lldiv(_value1->value._integer->ll,_value2->value._integer->ll).quot):NULL);
	}
	// the other integer one could be a big integer in which case we return a big integer
	if((_value1->type==VT_INTEGER||_value1->type==VT_BIGINTEGER)&&(_value2->type==VT_INTEGER||_value2->type==VT_BIGINTEGER)){
		Mbiginteger* _integerdivideBiginteger=NULL;
		Mbiginteger *_biginteger1=_getValueBiginteger(_value1),*_biginteger2=_getValueBiginteger(_value2); // OOPS careful here, _getValueDecimal would make a copy which we do not want here!!!!
		if(_biginteger1&&_biginteger2){
			if(amVerbose()){outputBiginteger("Integer dividing big integers '",_biginteger1,"'");outputBiginteger(" and '",_biginteger2,"'.\n");}
			if(mp_iszero(_biginteger2)==MP_NO){
				_integerdivideBiginteger=__biginteger();
				if(_integerdivideBiginteger){
					Mbiginteger* _integerremainderBiginteger=__biginteger();
					if(_integerremainderBiginteger){
						if(mp_div(_biginteger1,_biginteger2,_integerdivideBiginteger,_integerremainderBiginteger)!=MP_OKAY){free_biginteger(_integerdivideBiginteger);_integerdivideBiginteger=NULL;} // _dmul replaced by _getDecimalProduct which should be able to multiply any two decimals (not just the pure decimals)
						free_biginteger(_integerremainderBiginteger);
					}
				}else 
					outputError("Failed to create the integer divide result big integer");
			}
		}else 
			outputError("Failed to create two helper big integers");
		if(_value1->type!=VT_BIGINTEGER)free_biginteger(_biginteger1);else if(_value2->type!=VT_BIGINTEGER)free_biginteger(_biginteger2); // after adding the two rationals we do not need the newly created rationals anymore
		return _getBigintegerValue(_integerdivideBiginteger,true);
	}
	*/
	// MDH@28OCT2019: copied over from divide() and adjusted to return an integer
	if((_value1->type==VT_RATIONAL||(_value1->type==VT_DECIMAL&&_value1->value._decimal->repeating>0))||(_value2->type==VT_RATIONAL||(_value2->type==VT_DECIMAL&&_value2->value._decimal->repeating>0))){
		Mrational *_rational1=getValueRational(_value1),*_rational2=getValueRational(_value2);
		if(amVerbose()){outputRational("Determining the integer part of dividing rational '",_rational1,"'");outputRational(" by '",_rational2,"'.\n");}
		Mrational* _quotientRational=_getRationalQuotient(_rational1,_rational2); // _qdivide replaced by _getRationalQuotient (as defined in Mrational.h/c)
		if(amVerbose())outputRational("Quotient: '",_quotientRational,"'.\n");
		if(_value1->type!=VT_RATIONAL)free_rational(_rational1);else if(_value2->type!=VT_RATIONAL)free_rational(_rational2); // after dividing the two rationals we do not need the newly created rationals anymore
		// we're supposed to return the big integer by dividing the numerator by the denominator and forgetting the remainder
		// this means that we can reuse _getRationalInteger passing in _divisionRational and telling it to return the truncated integer
		if(!_quotientRational)return NULL;
		Mbiginteger* _rationalInteger=_getRationalInteger(_quotientRational,true,true);
		free_rational(_quotientRational); // only used for temporary storage of the division rational
		return _getBigintegerValue(_rationalInteger,true);
	}
	if(_value1->type==VT_DECIMAL||_value2->type==VT_DECIMAL){
		Mdecimal *_decimal1=getValueDecimal(_value1),*_decimal2=getValueDecimal(_value2); // OOPS careful here, _getValueDecimal would make a copy which we do not want here!!!!
		Mdecimal* _divideDecimal=_getDecimalQuotient(_decimal1,_decimal2); // _ddiv now replaced by _getDecimalQuotient which should be able to divide any two decimals not just the pure once!!!!!
		if(_value1->type!=VT_DECIMAL)free_decimal(_decimal1);else if(_value2->type!=VT_DECIMAL)free_decimal(_decimal2); // after adding the two rationals we do not need the newly created rationals anymore
		if(!_divideDecimal)return NULL;
		Mdecimal* _decimalInteger=_getDecimalInteger(_divideDecimal,true,true);
		free_decimal(_divideDecimal); // only used for temporary storage of the division result
		return _getDecimalValue(_decimalInteger,true);
	}
	// MDH@28OCT2019: if either is a real
	if(_value1->type==VT_FLOAT||_value2->type==VT_FLOAT){
		if(amVerbose()){outputValue("Dividing (as) reals '",_value1,"'");outputValue(" and '",_value2,"'.\n");}
		long double ld1=getValueLongDouble(_value1),ld2=getValueLongDouble(_value2);
		return _getFloatValue(isLongDoubleUndefined(ld1)==M_FALSE&&isLongDoubleUndefined(ld2)==M_FALSE?truncl(ld1/ld2):M_LD_NAN); // same as divide, but applying truncl to the result (cutting off the fraction)
	}
	/* MDH@28OCT2019: see above
	if((_value1->type==VT_INTEGER||_value1->type==VT_FLOAT)&&(_value2->type==VT_INTEGER||_value2->type==VT_FLOAT)){
		// if both integer, use lldiv to perform the integer division
		if(_value1->type==VT_INTEGER&&_value2->type==VT_INTEGER)return _getIntegerValue(lldiv(_value1->value._integer->ll,_value2->value._integer->ll).quot);
		// at least one is real, perform floating point division, then trunc!!!
		long double ld1=(_value1->type==VT_INTEGER?_value1->value._integer->ll:_value1->value._float->ld);
		long double ld2=(_value2->type==VT_INTEGER?_value2->value._integer->ll:_value2->value._float->ld);
		return _getIntegerValue(truncl(ld1/ld2));
	}
	*/
	return NULL;
}
Mvalue* divideremainder(Mvalue* _value1,Mvalue* _value2){
	if(!_value1||!_value2)return NULL;
	if(_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,divideremainder);if(_value2->type==VT_LIST)return _appliedToList2(_value1,_value2->value._list,divideremainder);
	if(isValueZero(_value1)==M_TRUE)return _value1;
	if(isValueOne(_value2)==M_TRUE)return(_value2->type==VT_INTEGER?_getIntegerValue(0):_getFloatValue(0));
	// MDH@26OCT2019: adapted from dealing with any integer type from add()
	if((_value1->type==VT_INTEGER||_value1->type==VT_BIGINTEGER)&&(_value2->type==VT_INTEGER||_value2->type==VT_BIGINTEGER)){
		Mbiginteger* _moduloBiginteger=NULL;
		// no need to use _getValueBiginteger because we know the source will be integer
		// NOTE _getBiginteger() was adjusted to return NULL in case ll equals M_LL_INVALID because in that case the result should also be M_LL_INVALID
		// CORRECTION: _getBiginteger() is also used in the I() function and it's a problem if NOT allowing to actually use NAI in computations (as the smallest possible integer)
		// DISCUSSION: M_LL_INVALID is a single long long value that is considered invalid like dividing by zero, or asking for the sign of an undefined real (= long double)
		//             
		bool smallinteger1=(_value1->type==VT_INTEGER),smallinteger2=(_value2->type==VT_INTEGER);
		bool invalidinteger1=(smallinteger1&&_value1->value._integer->ll==M_LL_INVALID),invalidinteger2=(smallinteger2&&_value2->value._integer->ll==M_LL_INVALID);
		if(invalidinteger1||invalidinteger2)return _getIntegerValue(M_LL_INVALID); // if either integer is invalid return an invalid integer (which per definition will be small)
		// ASSERT both integers are considered valid (i.e. not invalid)
		Mbiginteger *_biginteger1=(smallinteger1?_getBiginteger(_value1->value._integer->ll):_value1->value._biginteger);
		Mbiginteger *_biginteger2=(smallinteger2?_getBiginteger(_value2->value._integer->ll):_value2->value._biginteger);
		// replacing: Mbiginteger *_biginteger1=_getValueBiginteger(_value1),*_biginteger2=_getValueBiginteger(_value2); // OOPS careful here, _getValueDecimal would make a copy which we do not want here!!!!
		if(_biginteger1&&_biginteger2){
			if(amVerbose()){outputBiginteger("Moduloing big integers '",_biginteger1,"'");outputBiginteger(" and '",_biginteger2,"'");}
			_moduloBiginteger=__biginteger();
			if(_moduloBiginteger&&mp_div(_biginteger1,_biginteger2,NULL,_moduloBiginteger)!=MP_OKAY){free_biginteger(_moduloBiginteger);_moduloBiginteger=NULL;} // _dmul replaced by _getDecimalProduct which should be able to multiply any two decimals (not just the pure decimals)
			if(amVerbose()){outputBiginteger(" - Integer division remainder: '",_moduloBiginteger,"'.\n");}
		}else
			outputError("Failed to convert a small integer to a big integer");
		if(smallinteger1)free_biginteger(_biginteger1);
		if(smallinteger2)free_biginteger(_biginteger2);
		// MDH@24OCT2019: now we're going to try to convert the sum back to an integer if we can
		//                but if we can't don't
		if(smallinteger1||smallinteger2){ // we could decide to try to keep the value in range if at least one of the integers is small (instead of demanding both are small integers)
			// if computing the sum failed return the invalid (small) integer (to indicate a missing result)
			if(!_moduloBiginteger)return _getIntegerValue(M_LL_INVALID);
			long long llmodulo=getBigintegerInteger(_moduloBiginteger); // will return M_LL_INVALID when _sumBiginteger equals NULL (which we want to exclude)
			// if we do NOT have a sum big integer or the sum big integer is in range ()
			if(llmodulo!=M_LL_INVALID){free_biginteger(_moduloBiginteger);return _getIntegerValue(llmodulo);}
			output("WARNING: Small integer quotient remainder out of range, will continue using big integer quotient remainder.\n");
		}
		return _getBigintegerValue(_moduloBiginteger,true);
	}
	/* replacing:
	if(_value1->type==VT_INTEGER&&_value2->type==VT_INTEGER){
		if(amVerbose())output("Integer remainder of dividing integers '%lld' and '%lld'.\n",_value1->value._integer->ll,_value2->value._integer->ll);
		return(_value2->value._integer->ll!=0?_getIntegerValue(lldiv(_value1->value._integer->ll,_value2->value._integer->ll).rem):NULL);
	}
	// the other integer one could be a big integer in which case we return a big integer
	if((_value1->type==VT_INTEGER||_value1->type==VT_BIGINTEGER)&&(_value2->type==VT_INTEGER||_value2->type==VT_BIGINTEGER)){
		Mbiginteger* _integerremainderBiginteger=NULL; // OOPS careful here, _getValueDecimal would make a copy which we do not want here!!!!
		Mbiginteger *_biginteger1=_getValueBiginteger(_value1),*_biginteger2=_getValueBiginteger(_value2);
		if(_biginteger1&&_biginteger2){
			if(amVerbose()){outputBiginteger("Remainder of dividing big integers '",_biginteger1,"'");outputBiginteger(" and '",_biginteger2,"'.\n");}
			if(mp_iszero(_biginteger2)!=MP_YES){
				_integerremainderBiginteger=__biginteger();
				if(_integerremainderBiginteger){
					Mbiginteger *_integerdivideBiginteger=__biginteger();
					if(_integerdivideBiginteger){
						if(mp_div(_biginteger1,_biginteger2,_integerdivideBiginteger,_integerremainderBiginteger)!=MP_OKAY){free_biginteger(_integerremainderBiginteger);_integerremainderBiginteger=NULL;} // _dmul replaced by _getDecimalProduct which should be able to multiply any two decimals (not just the pure decimals)
						free_biginteger(_integerdivideBiginteger);
					}
				}
			}
		}
		if(_value1->type!=VT_BIGINTEGER)free_biginteger(_biginteger1);else if(_value2->type!=VT_BIGINTEGER)free_biginteger(_biginteger2); // after adding the two rationals we do not need the newly created rationals anymore
		return _getBigintegerValue(_integerremainderBiginteger,true);
	}
	*/
	// MDH@28OCT2019: copied over from integerdivide() and adjusted to return the remainder
	if((_value1->type==VT_RATIONAL||(_value1->type==VT_DECIMAL&&_value1->value._decimal->repeating>0))||(_value2->type==VT_RATIONAL||(_value2->type==VT_DECIMAL&&_value2->value._decimal->repeating>0))){
		Mrational *_rational1=getValueRational(_value1),*_rational2=getValueRational(_value2);
		if(amVerbose()){outputRational("Determining the remainder of dividing rational '",_rational1,"'");outputRational(" by '",_rational2,"'.\n");}
		Mrational* _quotientRational=_getRationalQuotient(_rational1,_rational2); // _qdivide replaced by _getRationalQuotient (as defined in Mrational.h/c)
		if(amVerbose())outputRational("Quotient: '",_quotientRational,"'.\n");
		if(_value1->type!=VT_RATIONAL)free_rational(_rational1);else if(_value2->type!=VT_RATIONAL)free_rational(_rational2); // after dividing the two rationals we do not need the newly created rationals anymore
		// we're supposed to return the big integer by dividing the numerator by the denominator and forgetting the remainder
		// this means that we can reuse _getRationalInteger passing in _divisionRational and telling it to return the truncated integer
		if(!_quotientRational)return NULL;
		Mbiginteger* _rationalInteger=_getRationalInteger(_quotientRational,true,true);
		free_rational(_quotientRational); // only used for temporary storage of the division rational
		if(!_rationalInteger)return NULL;
		return subtract(_value1,multiply(_value2,_getBigintegerValue(_rationalInteger,true))); // it's easiest to simply subtract the result from the first value NOTE the intermediate _getBigintegerValue itself will never be bound, so _rationalInteger will be released when the value wrapper is by the GC
	}
	if(_value1->type==VT_DECIMAL||_value2->type==VT_DECIMAL){
		Mdecimal *_decimal1=getValueDecimal(_value1),*_decimal2=getValueDecimal(_value2); // OOPS careful here, _getValueDecimal would make a copy which we do not want here!!!!
		Mdecimal* _divideDecimal=_getDecimalQuotient(_decimal1,_decimal2); // _ddiv now replaced by _getDecimalQuotient which should be able to divide any two decimals not just the pure once!!!!!
		if(_value1->type!=VT_DECIMAL)free_decimal(_decimal1);else if(_value2->type!=VT_DECIMAL)free_decimal(_decimal2); // after adding the two rationals we do not need the newly created rationals anymore
		if(!_divideDecimal)return NULL;
		Mdecimal* _decimalInteger=_getDecimalInteger(_divideDecimal,true,true);
		free_decimal(_divideDecimal); // only used for temporary storage of the division result
		if(!_decimalInteger)return NULL;
		return subtract(_value1,multiply(_value2,_getDecimalValue(_decimalInteger,true)));
	}
	// MDH@28OCT2019: if either is a real
	if(_value1->type==VT_FLOAT||_value2->type==VT_FLOAT){
		if(amVerbose()){outputValue("Determining what's left after dividing (as) reals '",_value1,"'");outputValue(" and '",_value2,"'.\n");}
		long double ld1=getValueLongDouble(_value1),ld2=getValueLongDouble(_value2);
		return _getFloatValue(isLongDoubleUndefined(ld1)==M_FALSE&&isLongDoubleUndefined(ld2)==M_FALSE?ld1-ld2*truncl(ld1/ld2):M_LD_NAN);
	}
	/* replacing:
	if((_value1->type==VT_INTEGER||_value1->type==VT_FLOAT)&&(_value2->type==VT_INTEGER||_value2->type==VT_FLOAT)){
		// at least one is real, perform floating point division, then trunc!!!
		long double ld1=(_value1->type==VT_INTEGER?_value1->value._integer->ll:_value1->value._float->ld);
		long double ld2=(_value2->type==VT_INTEGER?_value2->value._integer->ll:_value2->value._float->ld);
		return _getFloatValue(ld1-ld2*truncl(ld1/ld2)); // what's left after subtracting the truncated value
	}
	*/
	return NULL;
}

// integer arithmetic 
// TODO yet to complete for big integers, rationals, decimals etc.
long long not(long long boolean){return(boolean==M_LL_INVALID?M_LL_INVALID:(boolean==M_TRUE?M_FALSE:M_TRUE));} // if invalid, stays invalid, otherwise return M_FALSE when M_TRUE and vice versa
// bitwise operators (and, or, xor)
Mvalue* bitwisexor(Mvalue* _value1,Mvalue* _value2){
	if(!_value1||!_value2)return NULL;
	if(_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,bitwisexor);if(_value2->type==VT_LIST)return _appliedToList2(_value1,_value2->value._list,bitwisexor);
	if(_value1->type==VT_INTEGER&&_value2->type==VT_INTEGER)return _getIntegerValue(_value1->value._integer->ll^_value2->value._integer->ll);
	if((_value1->type==VT_INTEGER||_value1->type==VT_BIGINTEGER)&&(_value2->type==VT_INTEGER||_value2->type==VT_BIGINTEGER)){
		Mbiginteger* _xorbiginteger=__biginteger();
		if(_xorbiginteger){
			// creating two intermediate big integers that need to be freed asap
			Mbiginteger* _biginteger1=(_value1->type==VT_INTEGER?_getBiginteger(_value1->value._integer->ll):_getBigintegerCopy(_value1->value._biginteger));
			Mbiginteger* _biginteger2=(_value2->type==VT_INTEGER?_getBiginteger(_value2->value._integer->ll):_getBigintegerCopy(_value2->value._biginteger));
			if(_biginteger1&&_biginteger2&&mp_xor(_biginteger1,_biginteger2,_xorbiginteger)!=MP_OKAY){free_biginteger(_xorbiginteger);_xorbiginteger=NULL;}
			free_biginteger(_biginteger1);free_biginteger(_biginteger2); // free the created copies
			return _getBigintegerValue(_xorbiginteger,true);
		}else
			outputError("Failed to create the xor result big integer");
	}
	if(_value1->type==VT_DECIMAL||_value2->type==VT_DECIMAL){
	}else
	if(_value1->type==VT_RATIONAL||_value2->type==VT_RATIONAL){
	}
	return NULL;
}
Mvalue* bitwiseand(Mvalue* _value1,Mvalue* _value2){
	if(!_value1||!_value2)return NULL;
	if(_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,bitwiseand);if(_value2->type==VT_LIST)return _appliedToList2(_value1,_value2->value._list,bitwiseand);
	if(_value1->type==VT_INTEGER&&_value2->type==VT_INTEGER)return _getIntegerValue(_value1->value._integer->ll&_value2->value._integer->ll);
	if((_value1->type==VT_INTEGER||_value1->type==VT_BIGINTEGER)&&(_value2->type==VT_INTEGER||_value2->type==VT_BIGINTEGER)){
		Mbiginteger* _bitwiseandbiginteger=__biginteger();
		if(_bitwiseandbiginteger){
			// creating two intermediate big integers that need to be freed asap
			Mbiginteger* _biginteger1=(_value1->type==VT_INTEGER?_getBiginteger(_value1->value._integer->ll):_getBigintegerCopy(_value1->value._biginteger));
			Mbiginteger* _biginteger2=(_value2->type==VT_INTEGER?_getBiginteger(_value2->value._integer->ll):_getBigintegerCopy(_value2->value._biginteger));
			if(_biginteger1&&_biginteger2&&mp_and(_biginteger1,_biginteger2,_bitwiseandbiginteger)!=MP_OKAY){free_biginteger(_bitwiseandbiginteger);_bitwiseandbiginteger=NULL;}
			free_biginteger(_biginteger1);free_biginteger(_biginteger2); // free the created copies
			return _getBigintegerValue(_bitwiseandbiginteger,true);
		}else
			outputError("Failed to create the bitwise and result big integer");
	}
	if(_value1->type==VT_DECIMAL||_value2->type==VT_DECIMAL){
	}else
	if(_value1->type==VT_RATIONAL||_value2->type==VT_RATIONAL){
	}
	return NULL;
}
Mvalue* bitwiseor(Mvalue* _value1,Mvalue* _value2){
	if(!_value1||!_value2)return NULL;
	if(_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,bitwiseor);if(_value2->type==VT_LIST)return _appliedToList2(_value1,_value2->value._list,bitwiseor);
	if(_value1->type==VT_INTEGER&&_value2->type==VT_INTEGER)return _getIntegerValue(_value1->value._integer->ll|_value2->value._integer->ll);
	if((_value1->type==VT_INTEGER||_value1->type==VT_BIGINTEGER)&&(_value2->type==VT_INTEGER||_value2->type==VT_BIGINTEGER)){
		Mbiginteger* _bitwiseorbiginteger=__biginteger();
		if(_bitwiseorbiginteger){
			// creating two intermediate big integers that need to be freed asap
			Mbiginteger* _biginteger1=(_value1->type==VT_INTEGER?_getBiginteger(_value1->value._integer->ll):_getBigintegerCopy(_value1->value._biginteger));
			Mbiginteger* _biginteger2=(_value2->type==VT_INTEGER?_getBiginteger(_value2->value._integer->ll):_getBigintegerCopy(_value2->value._biginteger));
			if(_biginteger1&&_biginteger2&&mp_or(_biginteger1,_biginteger2,_bitwiseorbiginteger)!=MP_OKAY){free_biginteger(_bitwiseorbiginteger);_bitwiseorbiginteger=NULL;}
			free_biginteger(_biginteger1);free_biginteger(_biginteger2); // free the created copies
			return _getBigintegerValue(_bitwiseorbiginteger,true);
		}else
			outputError("Failed to create the bitwise or result big integer");
	}
	if(_value1->type==VT_DECIMAL||_value2->type==VT_DECIMAL){
	}else
	if(_value1->type==VT_RATIONAL||_value2->type==VT_RATIONAL){
	}
	return NULL;
}

// logical binary operators
Mvalue* logicaland(Mvalue* _value1,Mvalue* _value2){
	if(!_value1||!_value2)return NULL;
	if(_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,logicaland);if(_value2->type==VT_LIST)return _appliedToList2(_value1,_value2->value._list,logicaland);
	if(_value1->type==VT_INTEGER&&_value2->type==VT_INTEGER)return _getIntegerValue(_value1->value._integer->ll&&_value2->value._integer->ll);
	if((_value1->type==VT_INTEGER||_value1->type==VT_BIGINTEGER)&&(_value2->type==VT_INTEGER||_value2->type==VT_BIGINTEGER)){
		Mbiginteger* _logicalandbiginteger=NULL;
		// creating two intermediate big integers that need to be freed asap
		Mbiginteger* _biginteger1=(_value1->type==VT_INTEGER?_getBiginteger(_value1->value._integer->ll):_getBigintegerCopy(_value1->value._biginteger));
		Mbiginteger* _biginteger2=(_value2->type==VT_INTEGER?_getBiginteger(_value2->value._integer->ll):_getBigintegerCopy(_value2->value._biginteger));
		if(_biginteger1&&_biginteger2)_logicalandbiginteger=_getBiginteger(mp_iszero(_biginteger1)==MP_YES||mp_iszero(_biginteger2)==MP_YES?0:1); // if either is zero, the result is zero otherwise 1
		free_biginteger(_biginteger1);free_biginteger(_biginteger2); // free the created copies
		return _getBigintegerValue(_logicalandbiginteger,true);
	}	
	if(_value1->type==VT_DECIMAL||_value2->type==VT_DECIMAL){
	}else
	if(_value1->type==VT_RATIONAL||_value2->type==VT_RATIONAL){
	}
	return NULL;
}
Mvalue* logicalor(Mvalue* _value1,Mvalue* _value2){
	if(!_value1||!_value2)return NULL;
	if(_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,logicalor);if(_value2->type==VT_LIST)return _appliedToList2(_value1,_value2->value._list,logicalor);
	if(_value1->type==VT_INTEGER&&_value2->type==VT_INTEGER)return _getIntegerValue(_value1->value._integer->ll||_value2->value._integer->ll);
	if((_value1->type==VT_INTEGER||_value1->type==VT_BIGINTEGER)&&(_value2->type==VT_INTEGER||_value2->type==VT_BIGINTEGER)){
		Mbiginteger* _logicalorbiginteger=NULL;
		// creating two intermediate big integers that need to be freed asap
		Mbiginteger* _biginteger1=(_value1->type==VT_INTEGER?_getBiginteger(_value1->value._integer->ll):_getBigintegerCopy(_value1->value._biginteger));
		Mbiginteger* _biginteger2=(_value2->type==VT_INTEGER?_getBiginteger(_value2->value._integer->ll):_getBigintegerCopy(_value2->value._biginteger));
		if(_biginteger1&&_biginteger2)_logicalorbiginteger=_getBiginteger(mp_iszero(_biginteger1)==MP_NO||mp_iszero(_biginteger2)==MP_NO?1:0); // if either is not zero, the result is 1 otherwise 0, NOTE using || is better than using &&???
		free_biginteger(_biginteger1);free_biginteger(_biginteger2); // free the created copies
		return _getBigintegerValue(_logicalorbiginteger,true);
	}
	if(_value1->type==VT_DECIMAL||_value2->type==VT_DECIMAL){
	}else
	if(_value1->type==VT_RATIONAL||_value2->type==VT_RATIONAL){
	}
	return NULL;
}

// binary shift operators
// helper function taking care of shifting integers, taking the invalid integer values (for integer and shift) into account
long long integerShift(long long integer,long long shift){
	long long result=(shift==M_LL_INVALID?M_LL_INVALID:integer);
	if(shift!=0&&result!=M_LL_INVALID){if(shift>0)result<<=shift;else result>>=(-shift);} // always shift left or right by a positive value!!
	return result;
}
Mvalue* shiftleft(Mvalue* _value1,Mvalue* _value2){
	if(!_value1||!_value2)return NULL;
	if(_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,shiftleft);if(_value2->type==VT_LIST)return _appliedToList2(_value1,_value2->value._list,shiftleft);
	if(isValueZero(_value1)==M_TRUE||isValueZero(_value2)==M_TRUE)return _value1; // MDH@25OCT2019: if either value is zero the result is the first value
	// ASSERT neither value zero
	// TODO deal with integers separately
	// do NOT allow shifting by anything that cannot be converted to an integer
	long long shiftleftinteger=getValueInteger(_value2);if(shiftleftinteger==M_LL_INVALID)return NULL;
	///////////if(shiftleftinteger==0)return _value1; // return _value1 if no need to shift!!
	// only need to check the value1 type now
	if(_value1->type==VT_INTEGER)return _getIntegerValue(integerShift(_value1->value._integer->ll,shiftleftinteger));
	if(_value1->type==VT_FLOAT)return _getFloatValue(ldShift(_value1->value._float->ld,shiftleftinteger));
	if(_value1->type==VT_BIGINTEGER){
		Mbiginteger* _shiftleftBiginteger=_getBigintegerCopy(_value1->value._biginteger); // make a copy of the big integer to shift left
		if(_shiftleftBiginteger){
			if((shiftleftinteger<0?mp_div_2d(_value1->value._biginteger,-shiftleftinteger,_shiftleftBiginteger,NULL):mp_mul_2d(_value1->value._biginteger,shiftleftinteger,_shiftleftBiginteger))!=MP_OKAY){
				free_biginteger(_shiftleftBiginteger);_shiftleftBiginteger=NULL;
				output("%s",ERROR_PREFIX);outputBiginteger("Failed to shift '",_value1->value._biginteger,"' to the left.\n");			
			}else 
				outputError("Failed to shift left a big integer");
		}else
			outputError("Failed to create the shift left result big integer");
		return _getBigintegerValue(_shiftleftBiginteger,true);
	}
	if(_value1->type==VT_RATIONAL||(_value1->type==VT_DECIMAL&&_value1->value._decimal->repeating>0)){
		Mrational* _shiftleftRational=NULL;
		Mrational* _rational1=_getValueRational(_value1);
		if(_rational1){
			// shifting to the right means dividing the rational by 2 the given number of times but this means doubling the denominator
			// i.e. we should never divide because we could end up with zero (and loose the precision of exact computations)
			_shiftleftRational=_getRationalCopy(_rational1);
			if(_shiftleftRational){
				///if(amVerbose())
				outputRational("Rational shift left copy: '",_shiftleftRational,"'.\n");
				if(shiftleftinteger<0){ // naughty boy (or girl for that matter)... // actually a shift right
					// multiply the denominator by 2 shiftleftinteger times
					if(!_shiftleftRational->den)_shiftleftRational->den=_getBiginteger(1); // force having a non NULL denominator before trying to shift it
					if(_shiftleftRational->den==NULL||mp_mul_2d(_shiftleftRational->den,-shiftleftinteger,_shiftleftRational->den)!=MP_OKAY){
						free_rational(_shiftleftRational);_shiftleftRational=NULL;
						outputError("Failed to half a rational");
					}
					// force normalization
					if(_shiftleftRational){_shiftleftRational->normalized=false;normalizeRational(_shiftleftRational);}
				}else{ 
					if(mp_mul_2d(_shiftleftRational->num,shiftleftinteger,_shiftleftRational->num)!=MP_OKAY){
						free_rational(_shiftleftRational);_shiftleftRational=NULL;
						outputError("Failed to double a rational");
					}
				}
				if(_shiftleftRational){_shiftleftRational->normalized=false;normalizeRational(_shiftleftRational);}
			}else 
				outputError("Failed to copy a rational");
			if(_value1->type!=VT_RATIONAL)free_rational(_rational1);
		}else 
			outputError("Failed to convert a decimal to a rational");
		return _getRationalValue(_shiftleftRational,true);
	}
	if(_value1->type==VT_DECIMAL){ // a pure decimal 
		// similar approach as with a rational, except decimals have a shiftl and shiftr method
		Mdecimal* _shiftleftDecimal=_getDecimalCopy(_value1->value._decimal);
		if(_shiftleftDecimal){
			uint32_t status=0;
			if(_shiftleftDecimal>0)mpd_qshiftr(_shiftleftDecimal->mpd,_shiftleftDecimal->mpd,shiftleftinteger,&status);else mpd_qshiftl(_shiftleftDecimal->mpd,_shiftleftDecimal->mpd,-shiftleftinteger,&status);
			if((status&0xEFBF)!=0){free_decimal(_shiftleftDecimal);_shiftleftDecimal=NULL;outputError("Failed to shift a decimal to the left");}
		}else 
			outputError("Failed to copy a decimal");
		return _getDecimalValue(_shiftleftDecimal,true);
	}
	return NULL;
}
Mvalue* shiftright(Mvalue* _value1,Mvalue* _value2){
	if(!_value1||!_value2)return NULL;
	if(_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,shiftright);
	if(_value2->type==VT_LIST)return _appliedToList2(_value1,_value2->value._list,shiftright);
	if(isValueZero(_value1)==M_TRUE||isValueZero(_value2)==M_TRUE)return _value1; // MDH@26OCT2019: if either value is zero return _value1
	// ASSERT neither value is zero
	// do NOT allow shifting by anything that cannot be converted to an integer
	long long shiftrightinteger=getValueInteger(_value2);if(shiftrightinteger==M_LL_INVALID)return NULL;
	/////////////if(shiftrightinteger==0)return _value1; // return _value1 if no need to shift!!
	// only need to check the value1 type now
	if(_value1->type==VT_INTEGER)return _getIntegerValue(integerShift(_value1->value._integer->ll,-shiftrightinteger));
	if(_value1->type==VT_FLOAT)return _getFloatValue(ldShift(_value1->value._float->ld,-shiftrightinteger));
	if(_value1->type==VT_BIGINTEGER){
		Mbiginteger* _shiftrightBiginteger=_getBigintegerCopy(_value1->value._biginteger); // make a copy of the big integer to shift right
		if(_shiftrightBiginteger){
			if((shiftrightinteger>0?mp_div_2d(_value1->value._biginteger,shiftrightinteger,_shiftrightBiginteger,NULL):mp_mul_2d(_value1->value._biginteger,-shiftrightinteger,_shiftrightBiginteger))!=MP_OKAY){
				free_biginteger(_shiftrightBiginteger);_shiftrightBiginteger=NULL;
				output("%s",ERROR_PREFIX);outputBiginteger("Failed to shift '",_value1->value._biginteger,"' to the right.\n");			
			}else 
				outputError("Failed to shift right a big integer");
		}else
			outputError("Failed to create the shift right result big integer");
		return _getBigintegerValue(_shiftrightBiginteger,true);
		/* replacing:
		// what we shift by should fit in int64_t!!!!
		if(_value2->type==VT_INTEGER||_value2->value._biginteger->used<=1){
			int64_t shr=(_value2->type==VT_INTEGER?_value2->value._integer->ll:mp_get_i64(_value2->value._biginteger));
			if(shr!=M_LL_INVALID){
				Mbiginteger* _biginteger1=(_value1->type==VT_INTEGER?_getBiginteger(_value1->value._integer->ll):_getBigintegerCopy(_value1->value._biginteger));
				if(_biginteger1){
					if(amVerbose()){outputBiginteger("Shifting big integer '",_biginteger1,"' left");output(" by %" PRIi64 ".\n",shr);}
					if((shr>0?mp_div_2d(_biginteger1,shr,_biginteger1,NULL):mp_mul_2d(_biginteger1,-shr,_biginteger1))==MP_OKAY)return _getBigintegerValue(_biginteger1,true);
					output("%s",ERROR_PREFIX);outputBiginteger("Failed to shift '",_biginteger1,"' to the right.\n");			
					free_biginteger(_biginteger1);
				}
			}
		}else
			outputError("Number of shift positions too large");
		*/
	}
	if(_value1->type==VT_RATIONAL||(_value1->type==VT_DECIMAL&&_value1->value._decimal->repeating>0)){
		Mrational* _shiftrightRational=NULL;
		Mrational* _rational1=_getValueRational(_value1);
		if(_rational1){
			// shifting to the right means dividing the rational by 2 the given number of times but this means doubling the denominator
			// i.e. we should never divide because we could end up with zero (and loose the precision of exact computations)
			_shiftrightRational=_getRationalCopy(_rational1);
			if(_shiftrightRational){
				///if(amVerbose())
				outputRational("Rational shift right copy: '",_shiftrightRational,"'.\n");
				if(shiftrightinteger>0){
					// multiply the denominator by 2 shiftrightinteger times
					if(!_shiftrightRational->den)_shiftrightRational->den=_getBiginteger(1); // force having a non NULL denominator before trying to shift it
					if(_shiftrightRational->den==NULL||mp_mul_2d(_shiftrightRational->den,shiftrightinteger,_shiftrightRational->den)!=MP_OKAY){
						free_rational(_shiftrightRational);_shiftrightRational=NULL;
						outputError("Failed to half a rational");
					}
					// force normalization
					if(_shiftrightRational){_shiftrightRational->normalized=false;normalizeRational(_shiftrightRational);}
				}else{ // naughty boy (or girl for that matter)...
					if(mp_mul_2d(_shiftrightRational->num,-shiftrightinteger,_shiftrightRational->num)!=MP_OKAY){
						free_rational(_shiftrightRational);_shiftrightRational=NULL;
						outputError("Failed to double a rational");
					}
				}
				if(_shiftrightRational){_shiftrightRational->normalized=false;normalizeRational(_shiftrightRational);}
			}else 
				outputError("Failed to copy a rational");
			if(_value1->type!=VT_RATIONAL)free_rational(_rational1);
		}else 
			outputError("Failed to convert a decimal to a rational");
		return _getRationalValue(_shiftrightRational,true);
	}
	if(_value1->type==VT_DECIMAL){ // a pure decimal 
		// similar approach as with a rational, except decimals have a shiftl and shiftr method
		Mdecimal* _shiftrightDecimal=_getDecimalCopy(_value1->value._decimal);
		if(_shiftrightDecimal){
			uint32_t status=0;
			if(_shiftrightDecimal>0)mpd_qshiftr(_shiftrightDecimal->mpd,_shiftrightDecimal->mpd,shiftrightinteger,&status);else mpd_qshiftl(_shiftrightDecimal->mpd,_shiftrightDecimal->mpd,-shiftrightinteger,&status);
			if((status&0xEFBF)!=0){free_decimal(_shiftrightDecimal);_shiftrightDecimal=NULL;outputError("Failed to shift a decimal to the right");}
		}else 
			outputError("Failed to copy a decimal");
		return _getDecimalValue(_shiftrightDecimal,true);
	}
	return NULL;
}

// binary comparison operators
// TODO these should return either TRUE, FALSE or UNDEFINED independent of the input type
Mvalue* smallerthan(Mvalue* _value1,Mvalue* _value2){
	if(!_value1||!_value2)return _getIntegerValue(M_LL_INVALID);
	if(_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,smallerthan);if(_value2->type==VT_LIST)return _appliedToList2(_value1,_value2->value._list,smallerthan);
	if((_value1->type==VT_INTEGER||_value1->type==VT_FLOAT)&&(_value2->type==VT_INTEGER||_value2->type==VT_FLOAT))
		return _getIntegerValue((_value1->type==VT_INTEGER?_value1->value._integer->ll:_value1->value._float->ld)<(_value2->type==VT_INTEGER?_value2->value._integer->ll:_value2->value._float->ld)?1:0);
	if((_value1->type==VT_INTEGER||_value1->type==VT_BIGINTEGER)&&(_value2->type==VT_INTEGER||_value2->type==VT_BIGINTEGER)){
		// creating two intermediate big integers that need to be freed asap
		Mbiginteger* _biginteger1=(_value1->type==VT_INTEGER?_getBiginteger(_value1->value._integer->ll):_getBigintegerCopy(_value1->value._biginteger));
		Mbiginteger* _biginteger2=(_value2->type==VT_INTEGER?_getBiginteger(_value2->value._integer->ll):_getBigintegerCopy(_value2->value._biginteger));
		long long llsmallerthan=(_biginteger1&&_biginteger2?(mp_cmp(_biginteger1,_biginteger2)==MP_LT?M_TRUE:M_FALSE):M_LL_INVALID); // if either is not zero, the result is 1 otherwise 0, NOTE using || is better than using &&???
		free_biginteger(_biginteger1);free_biginteger(_biginteger2); // free the created copies
		return _getIntegerValue(llsmallerthan);
	}
	// MDH@23OCT2019: if we can rationalize at least one of the values, we should work with rationals (so we get the highest possible accuracy in the comparison)
	if((_value1->type==VT_RATIONAL||(_value1->type==VT_DECIMAL&&_value1->value._decimal->repeating>0))||(_value2->type==VT_RATIONAL||(_value2->type==VT_DECIMAL&&_value2->value._decimal->repeating>0))){
		long long result=M_LL_INVALID;
		Mrational *_rational1=_getValueRational(_value1),*_rational2=_getValueRational(_value2);
		if(_rational1&&_rational2){
			Mrational* _rationalDifference=_getRationalDifference(_rational1,_rational2);
			if(_rationalDifference){
				if(amVerbose())outputRational("Difference in determining whether a rational is smaller than another rational: '",_rationalDifference,"'.\n");
				result=isRationalNegative(_rationalDifference);
				free_rational(_rationalDifference);
			}else
				outputError("Failed to compute the difference of two rationals");
		}else
			outputError("Failed to convert comparison operator arguments to rationals");
		if(_value1->type!=VT_RATIONAL)free_rational(_rational1);if(_value2->type!=VT_RATIONAL)free_rational(_rational2);
		return _getIntegerValue(result);
	}
	if(_value1->type==VT_DECIMAL||_value2->type==VT_DECIMAL){
		// creating two intermediate decimals that need to be freed asap
		long long result=M_LL_INVALID;
		Mdecimal *_decimal1=_getValueDecimal(_value1),*_decimal2=_getValueDecimal(_value2);
		if(_decimal1&&_decimal2){
			Mdecimal* _decimalDifference=_getDecimalDifference(_decimal1,_decimal2);
			if(_decimalDifference){
				if(amVerbose())outputDecimal("Difference in determining whether a decimal is smaller than another decimal: '",_decimalDifference,"'.\n");
				result=isDecimalNegative(_decimalDifference);
				free_decimal(_decimalDifference);
			}else
				outputError("Failed to compute the difference of two decimals");
		}else
			outputError("Failed to convert comparison arguments to decimals");
		if(_value1->type!=VT_DECIMAL)free_decimal(_decimal1);if(_value2->type!=VT_DECIMAL)free_decimal(_decimal2);
		return _getIntegerValue(result);
	}
	return NULL;
}
Mvalue* largerthan(Mvalue* _value1,Mvalue* _value2){
	if(!_value1||!_value2)return NULL;
	if(_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,largerthan);if(_value2->type==VT_LIST)return _appliedToList2(_value1,_value2->value._list,largerthan);
	if((_value1->type==VT_INTEGER||_value1->type==VT_FLOAT)&&(_value2->type==VT_INTEGER||_value2->type==VT_FLOAT))
		return _getIntegerValue((_value1->type==VT_INTEGER?_value1->value._integer->ll:_value1->value._float->ld)>(_value2->type==VT_INTEGER?_value2->value._integer->ll:_value2->value._float->ld)?1:0);
	if((_value1->type==VT_INTEGER||_value1->type==VT_BIGINTEGER)&&(_value2->type==VT_INTEGER||_value2->type==VT_BIGINTEGER)){
		// creating two intermediate big integers that need to be freed asap
		Mbiginteger* _biginteger1=(_value1->type==VT_INTEGER?_getBiginteger(_value1->value._integer->ll):_getBigintegerCopy(_value1->value._biginteger));
		Mbiginteger* _biginteger2=(_value2->type==VT_INTEGER?_getBiginteger(_value2->value._integer->ll):_getBigintegerCopy(_value2->value._biginteger));
		long long lllargerthan=(_biginteger1&&_biginteger2?(mp_cmp(_biginteger1,_biginteger2)==MP_GT?M_TRUE:M_FALSE):M_LL_INVALID); // if either is not zero, the result is 1 otherwise 0, NOTE using || is better than using &&???
		free_biginteger(_biginteger1);free_biginteger(_biginteger2); // free the created copies
		return _getIntegerValue(lllargerthan);
	}
	// MDH@23OCT2019: if we can rationalize at least one of the values, we should work with rationals (so we get the highest possible accuracy in the comparison)
	if((_value1->type==VT_RATIONAL||(_value1->type==VT_DECIMAL&&_value1->value._decimal->repeating>0))||(_value2->type==VT_RATIONAL||(_value2->type==VT_DECIMAL&&_value2->value._decimal->repeating>0))){
		long long result=M_LL_INVALID;
		Mrational *_rational1=_getValueRational(_value1),*_rational2=_getValueRational(_value2);
		if(_rational1&&_rational2){
			Mrational* _rationalDifference=_getRationalDifference(_rational1,_rational2);
			if(_rationalDifference){
				if(amVerbose())outputRational("Difference in determining whether a rational is larger than another rational: '",_rationalDifference,"'.\n");
				result=not(isRationalNegative(_rationalDifference));
				free_rational(_rationalDifference);
			}else 
				outputError("Failed to compute the difference of two rationals");
		}else
			outputError("Failed to convert comparison operator arguments to rationals");
		if(_value1->type!=VT_RATIONAL)free_rational(_rational1);if(_value2->type!=VT_RATIONAL)free_rational(_rational2);
		return _getIntegerValue(result);
	}
	if(_value1->type==VT_DECIMAL||_value2->type==VT_DECIMAL){
		// creating two intermediate decimals that need to be freed asap
		long long result=M_LL_INVALID;
		Mdecimal *_decimal1=_getValueDecimal(_value1),*_decimal2=_getValueDecimal(_value2);
		if(_decimal1&&_decimal2){
			Mdecimal* _decimalDifference=_getDecimalDifference(_decimal1,_decimal2);
			if(_decimalDifference){
				if(amVerbose())outputDecimal("Difference in determining whether a decimal is larger than another decimal: '",_decimalDifference,"'.\n");
				result=not(isDecimalNegative(_decimalDifference));
				free_decimal(_decimalDifference);
			}else
				outputError("Failed to compute the difference of two decimals");
		}else
			outputError("Failed to convert comparison arguments to decimals");
		if(_value1->type!=VT_DECIMAL)free_decimal(_decimal1);if(_value2->type!=VT_DECIMAL)free_decimal(_decimal2);
		return _getIntegerValue(result);
	}
	return NULL;
}
Mvalue* largerthanorequalto(Mvalue* _value1,Mvalue* _value2){
	if(!_value1||!_value2)return NULL;
	if(_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,largerthanorequalto);if(_value2->type==VT_LIST)return _appliedToList2(_value1,_value2->value._list,largerthanorequalto);
	if((_value1->type==VT_INTEGER||_value1->type==VT_FLOAT)&&(_value2->type==VT_INTEGER||_value2->type==VT_FLOAT))
		return _getIntegerValue((_value1->type==VT_INTEGER?_value1->value._integer->ll:_value1->value._float->ld)>=(_value2->type==VT_INTEGER?_value2->value._integer->ll:_value2->value._float->ld)?1:0);
	if((_value1->type==VT_INTEGER||_value1->type==VT_BIGINTEGER)&&(_value2->type==VT_INTEGER||_value2->type==VT_BIGINTEGER)){
		// creating two intermediate big integers that need to be freed asap
		Mbiginteger* _biginteger1=(_value1->type==VT_INTEGER?_getBiginteger(_value1->value._integer->ll):_getBigintegerCopy(_value1->value._biginteger));
		Mbiginteger* _biginteger2=(_value2->type==VT_INTEGER?_getBiginteger(_value2->value._integer->ll):_getBigintegerCopy(_value2->value._biginteger));
		long long lllargerthanorequalto=(_biginteger1&&_biginteger2?(mp_cmp(_biginteger1,_biginteger2)==MP_LT?M_FALSE:M_TRUE):M_LL_INVALID); // if either is not zero, the result is 1 otherwise 0, NOTE using || is better than using &&???
		free_biginteger(_biginteger1);free_biginteger(_biginteger2); // free the created copies
		return _getIntegerValue(lllargerthanorequalto);
	}
	// MDH@23OCT2019: if we can rationalize at least one of the values, we should work with rationals (so we get the highest possible accuracy in the comparison)
	if((_value1->type==VT_RATIONAL||(_value1->type==VT_DECIMAL&&_value1->value._decimal->repeating>0))||(_value2->type==VT_RATIONAL||(_value2->type==VT_DECIMAL&&_value2->value._decimal->repeating>0))){
		long long result=M_LL_INVALID;
		Mrational *_rational1=_getValueRational(_value1),*_rational2=_getValueRational(_value2);
		if(_rational1&&_rational2){
			Mrational* _rationalDifference=_getRationalDifference(_rational1,_rational2);
			if(_rationalDifference){
				if(amVerbose())outputRational("Difference in determining whether a rational is larger than or equal to another rational: '",_rationalDifference,"'.\n");
				result=not(isRationalNegative(_rationalDifference));
				free_rational(_rationalDifference);
			}else 
				outputError("Failed to compute the difference of two rationals");
		}else
			outputError("Failed to convert comparison operator arguments to rationals");
		if(_value1->type!=VT_RATIONAL)free_rational(_rational1);if(_value2->type!=VT_RATIONAL)free_rational(_rational2);
		return _getIntegerValue(result);
	}
	if(_value1->type==VT_DECIMAL||_value2->type==VT_DECIMAL){
		// creating two intermediate decimals that need to be freed asap
		long long result=M_LL_INVALID;
		Mdecimal *_decimal1=_getValueDecimal(_value1),*_decimal2=_getValueDecimal(_value2);
		if(_decimal1&&_decimal2){
			Mdecimal* _decimalDifference=_getDecimalDifference(_decimal1,_decimal2);
			if(_decimalDifference){
				if(amVerbose())outputDecimal("Difference in determining whether a decimal is larger than or equal to another decimal: '",_decimalDifference,"'.\n");
				result=not(isDecimalNegative(_decimalDifference));
				free_decimal(_decimalDifference);
			}else
				outputError("Failed to compute the difference of two decimals");
		}else
			outputError("Failed to convert comparison arguments to decimals");
		if(_value1->type!=VT_DECIMAL)free_decimal(_decimal1);if(_value2->type!=VT_DECIMAL)free_decimal(_decimal2);
		return _getIntegerValue(result);
	}
	return NULL;
}
Mvalue* unequalto(Mvalue* _value1,Mvalue* _value2){
	if(!_value1&&!_value2)return _getIntegerValue(M_FALSE); // NULL == NULL
	if(!_value1||!_value2)return _getIntegerValue(M_TRUE); // !NULL != NULL
	if(_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,unequalto);if(_value2->type==VT_LIST)return _appliedToList2(_value1,_value2->value._list,unequalto);
	if((_value1->type==VT_INTEGER||_value1->type==VT_FLOAT)&&(_value2->type==VT_INTEGER||_value2->type==VT_FLOAT))
		return _getIntegerValue((_value1->type==VT_INTEGER?_value1->value._integer->ll:_value1->value._float->ld)!=(_value2->type==VT_INTEGER?_value2->value._integer->ll:_value2->value._float->ld)?1:0);
	if((_value1->type==VT_INTEGER||_value1->type==VT_BIGINTEGER)&&(_value2->type==VT_INTEGER||_value2->type==VT_BIGINTEGER)){
		// creating two intermediate big integers that need to be freed asap
		Mbiginteger* _biginteger1=(_value1->type==VT_INTEGER?_getBiginteger(_value1->value._integer->ll):_getBigintegerCopy(_value1->value._biginteger));
		Mbiginteger* _biginteger2=(_value2->type==VT_INTEGER?_getBiginteger(_value2->value._integer->ll):_getBigintegerCopy(_value2->value._biginteger));
		long long llunequalto=(_biginteger1&&_biginteger2?(mp_cmp(_biginteger1,_biginteger2)==MP_EQ?M_FALSE:M_TRUE):M_LL_INVALID); // if either is not zero, the result is 1 otherwise 0, NOTE using || is better than using &&???
		free_biginteger(_biginteger1);free_biginteger(_biginteger2); // free the created copies
		return _getIntegerValue(llunequalto);
	}
	// MDH@23OCT2019: if we can rationalize at least one of the values, we should work with rationals (so we get the highest possible accuracy in the comparison)
	if((_value1->type==VT_RATIONAL||(_value1->type==VT_DECIMAL&&_value1->value._decimal->repeating>0))||(_value2->type==VT_RATIONAL||(_value2->type==VT_DECIMAL&&_value2->value._decimal->repeating>0))){
		long long result=M_LL_INVALID;
		Mrational *_rational1=_getValueRational(_value1),*_rational2=_getValueRational(_value2);
		if(_rational1&&_rational2){
			Mrational* _rationalDifference=_getRationalDifference(_rational1,_rational2);
			if(_rationalDifference){
				if(amVerbose())outputRational("Difference in determining whether a rational is not equal to another rational: '",_rationalDifference,"'.\n");
				result=not(isRationalZero(_rationalDifference)); 
				free_rational(_rationalDifference);
			}else 
				outputError("Failed to compute the difference of two rationals");
		}else
			outputError("Failed to convert comparison operator arguments to rationals");
		if(_value1->type!=VT_RATIONAL)free_rational(_rational1);if(_value2->type!=VT_RATIONAL)free_rational(_rational2);
		return _getIntegerValue(result);
	}
	if(_value1->type==VT_DECIMAL||_value2->type==VT_DECIMAL){
		// creating two intermediate decimals that need to be freed asap
		long long result=M_LL_INVALID;
		Mdecimal *_decimal1=_getValueDecimal(_value1),*_decimal2=_getValueDecimal(_value2);
		if(_decimal1&&_decimal2){
			Mdecimal* _decimalDifference=_getDecimalDifference(_decimal1,_decimal2);
			if(_decimalDifference){
				if(amVerbose())outputDecimal("Difference in determining whether a decimal is not equal to another decimal: '",_decimalDifference,"'.\n");
				result=not(isDecimalZero(_decimalDifference));
				free_decimal(_decimalDifference);
			}else
				outputError("Failed to compute the difference of two decimals");
		}else
			outputError("Failed to convert comparison arguments to decimals");
		if(_value1->type!=VT_DECIMAL)free_decimal(_decimal1);if(_value2->type!=VT_DECIMAL)free_decimal(_decimal2);
		return _getIntegerValue(result);
	}
	return NULL;
}
Mvalue* equalto(Mvalue* _value1,Mvalue* _value2){
	if(!_value1&&!_value2)return _getIntegerValue(M_TRUE); // NULL == NULL
	if(!_value1||!_value2)return _getIntegerValue(M_FALSE); // !NULL != NULL
	if(_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,equalto);
	if(_value2->type==VT_LIST)return _appliedToList2(_value1,_value2->value._list,equalto);
	if((_value1->type==VT_INTEGER||_value1->type==VT_FLOAT)&&(_value2->type==VT_INTEGER||_value2->type==VT_FLOAT))
		return _getIntegerValue((_value1->type==VT_INTEGER?_value1->value._integer->ll:_value1->value._float->ld)==(_value2->type==VT_INTEGER?_value2->value._integer->ll:_value2->value._float->ld)?1:0);
	if((_value1->type==VT_INTEGER||_value1->type==VT_BIGINTEGER)&&(_value2->type==VT_INTEGER||_value2->type==VT_BIGINTEGER)){		Mbiginteger* _equaltobiginteger=NULL;
		// creating two intermediate big integers that need to be freed asap
		Mbiginteger* _biginteger1=(_value1->type==VT_INTEGER?_getBiginteger(_value1->value._integer->ll):_getBigintegerCopy(_value1->value._biginteger));
		Mbiginteger* _biginteger2=(_value2->type==VT_INTEGER?_getBiginteger(_value2->value._integer->ll):_getBigintegerCopy(_value2->value._biginteger));
		long long llequalto=(_biginteger1&&_biginteger2?(mp_cmp(_biginteger1,_biginteger2)==MP_EQ?M_TRUE:M_FALSE):M_LL_INVALID); // if either is not zero, the result is 1 otherwise 0, NOTE using || is better than using &&???
		free_biginteger(_biginteger1);free_biginteger(_biginteger2); // free the created copies
		return _getIntegerValue(llequalto);
	}
	if((_value1->type==VT_RATIONAL||(_value1->type==VT_DECIMAL&&_value1->value._decimal->repeating>0))||(_value2->type==VT_RATIONAL||(_value2->type==VT_DECIMAL&&_value2->value._decimal->repeating>0))){
		long long result=M_LL_INVALID;
		Mrational *_rational1=_getValueRational(_value1),*_rational2=_getValueRational(_value2);
		if(_rational1&&_rational2){
			Mrational* _rationalDifference=_getRationalDifference(_rational1,_rational2);
			if(_rationalDifference){
				if(amVerbose())outputRational("Difference in determining whether a rational is equal to another rational: '",_rationalDifference,"'.\n");
				result=isRationalZero(_rationalDifference);
				free_rational(_rationalDifference);
			}else 
				outputError("Failed to compute the difference of two rationals");
		}else
			outputError("Failed to convert comparison operator arguments to rationals");
		if(_value1->type!=VT_RATIONAL)free_rational(_rational1);if(_value2->type!=VT_RATIONAL)free_rational(_rational2);
		return _getIntegerValue(result);
	}
	if(_value1->type==VT_DECIMAL||_value2->type==VT_DECIMAL){
		// creating two intermediate decimals that need to be freed asap
		long long result=M_LL_INVALID;
		Mdecimal *_decimal1=_getValueDecimal(_value1),*_decimal2=_getValueDecimal(_value2);
		if(_decimal1&&_decimal2){
			Mdecimal* _decimalDifference=_getDecimalDifference(_decimal1,_decimal2);
			if(_decimalDifference){
				if(amVerbose())outputDecimal("Difference in determining whether a decimal is equal to another decimal: '",_decimalDifference,"'.\n");
				result=isDecimalZero(_decimalDifference);
				free_decimal(_decimalDifference);
			}else
				outputError("Failed to compute the difference of two decimals");
		}else
			outputError("Failed to convert comparison arguments to decimals");
		if(_value1->type!=VT_DECIMAL)free_decimal(_decimal1);if(_value2->type!=VT_DECIMAL)free_decimal(_decimal2);
		return _getIntegerValue(result);
	}
	return NULL;
}
// MDH@21OCT2019: first comparison method dealing with decimals and rationals from which the rest was produced
Mvalue* smallerthanorequalto(Mvalue* _value1,Mvalue* _value2){
	if(!_value1||!_value2)return NULL;
	if(_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,smallerthanorequalto);if(_value2->type==VT_LIST)return _appliedToList2(_value1,_value2->value._list,smallerthanorequalto);
	if((_value1->type==VT_INTEGER||_value1->type==VT_FLOAT)&&(_value2->type==VT_INTEGER||_value2->type==VT_FLOAT))
		return _getIntegerValue((_value1->type==VT_INTEGER?_value1->value._integer->ll:_value1->value._float->ld)<=(_value2->type==VT_INTEGER?_value2->value._integer->ll:_value2->value._float->ld)?1:0);
	if((_value1->type==VT_INTEGER||_value1->type==VT_BIGINTEGER)&&(_value2->type==VT_INTEGER||_value2->type==VT_BIGINTEGER)){
		// creating two intermediate big integers that need to be freed asap
		Mbiginteger* _biginteger1=(_value1->type==VT_INTEGER?_getBiginteger(_value1->value._integer->ll):_getBigintegerCopy(_value1->value._biginteger));
		Mbiginteger* _biginteger2=(_value2->type==VT_INTEGER?_getBiginteger(_value2->value._integer->ll):_getBigintegerCopy(_value2->value._biginteger));
		long long llsmallerthanorequalto=(_biginteger1&&_biginteger2?(mp_cmp(_biginteger1,_biginteger2)==MP_GT?M_FALSE:M_TRUE):M_LL_INVALID); // if either is not zero, the result is 1 otherwise 0, NOTE using || is better than using &&???
		free_biginteger(_biginteger1);free_biginteger(_biginteger2); // free the created copies
		return _getIntegerValue(llsmallerthanorequalto);
	}
	// MDH@23OCT2019: if we can rationalize at least one of the values, we should work with rationals (so we get the highest possible accuracy in the comparison)
	if((_value1->type==VT_RATIONAL||(_value1->type==VT_DECIMAL&&_value1->value._decimal->repeating>0))||(_value2->type==VT_RATIONAL||(_value2->type==VT_DECIMAL&&_value2->value._decimal->repeating>0))){
		long long result=M_LL_INVALID;
		Mrational *_rational1=_getValueRational(_value1),*_rational2=_getValueRational(_value2);
		if(_rational1&&_rational2){
			Mrational* _rationalDifference=_getRationalDifference(_rational1,_rational2);
			if(_rationalDifference){
				if(amVerbose())outputRational("Difference in determining whether a rational is smaller than or equal to another rational: '",_rationalDifference,"'.\n");
				result=not(isRationalPositive(_rationalDifference)); // i.e. if difference is NOT positive, we should return M_TRUE
				free_rational(_rationalDifference);
			}else 
				outputError("Failed to compute the difference of two rationals");
		}else
			outputError("Failed to convert comparison operator arguments to rationals");
		if(_value1->type!=VT_RATIONAL)free_rational(_rational1);if(_value2->type!=VT_RATIONAL)free_rational(_rational2);
		return _getIntegerValue(result);
	}
	if(_value1->type==VT_DECIMAL||_value2->type==VT_DECIMAL){
		// creating two intermediate decimals that need to be freed asap
		long long result=M_LL_INVALID;
		Mdecimal *_decimal1=_getValueDecimal(_value1),*_decimal2=_getValueDecimal(_value2);
		if(_decimal1&&_decimal2){
			Mdecimal* _decimalDifference=_getDecimalDifference(_decimal1,_decimal2);
			if(_decimalDifference){
				if(amVerbose())outputDecimal("Difference in determining whether a decimal is smaller than or equal to another decimal: '",_decimalDifference,"'.\n");
				result=not(isDecimalPositive(_decimalDifference));
				free_decimal(_decimalDifference);
			}else
				outputError("Failed to compute the difference of two decimals");
		}else
			outputError("Failed to convert comparison arguments to decimals");
		if(_value1->type!=VT_DECIMAL)free_decimal(_decimal1);if(_value2->type!=VT_DECIMAL)free_decimal(_decimal2);
		return _getIntegerValue(result);
	}
	return NULL;
}
// end comparison operator implementation

// MDH@18OCT2019: we can get the range of integers between two values
Mvalue* integerrange(Mvalue* _value1,Mvalue* _value2){
	if(!_value1||!_value2)return NULL;
	if(_value1->type==VT_MAP||_value2->type==VT_MAP)return NULL; // neither operand can be a map for sure
	if(_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,integerrange);
	if(_value2->type==VT_LIST)return _appliedToList2(_value1,_value2->value._list,integerrange);
	// now we're dealing with scalars
	Mvalue* upValue=smallerthanorequalto(_value1,_value2); // the direction we'll be going
	if(upValue&&upValue->type==VT_INTEGER){
		bool up=(upValue->value._integer->ll!=0);
		// if going up the first value is the ceil of _value1, otherwise it's the floor of _value1
		// I suppose there's no need to determine the last integer because we can use _value2 itself in the comparisons!!!
		Mvalue* firstIntegerrangeValue=(up?Mceil(_value1):Mfloor(_value1));
		if(firstIntegerrangeValue){
			long long rangeInteger=getValueInteger(firstIntegerrangeValue);
			if(rangeInteger!=M_LL_INVALID){
				Mvalue* integerrangeValue=_getIntegerValue(rangeInteger);
				if(integerrangeValue){
					if(amVerbose()){
						Mvalue* lastIntegerrangeValue=(up?Mfloor(_value2):Mceil(_value2));
						outputValue("Determining the integers in [",integerrangeValue,",");outputValue(NULL,lastIntegerrangeValue,"].\n");
						char c;output("Press Ctrl-C to stop or any other key to continue...");inputCharRead(&c);if(c==3)return NULL;
					}
					Mlist* integerrangeValueList=_getListOfType(VT_INTEGER);
					Mvalue* inrangeValue;
					while(integerrangeValue){
						// determine whether this value does not exceed the last value
						inrangeValue=(up?smallerthanorequalto(integerrangeValue,_value2):largerthanorequalto(integerrangeValue,_value2));
						if(!inrangeValue||inrangeValue->type!=VT_INTEGER||inrangeValue->value._integer->ll==M_LL_INVALID){outputError("Unable to determine whether the integer is inside the integer range");break;}
						if(inrangeValue->value._integer->ll==0)break; // not in range
						if(appendedToList(integerrangeValueList,integerrangeValue,M_LL_INVALID)==0){outputError("Failed to add an integer to an integer range");break;}
						// determine the next value to insert into the integer range
						if(up)rangeInteger++;else rangeInteger--;
						integerrangeValue=_getIntegerValue(rangeInteger);
					}
					return _getValueOfList(integerrangeValueList,true);
				}else
					outputError("Failed to initialize the first candidate range integer");
			}else
				outputError("Failed to extract the lower bound of the integer range");
		}else
			outputError("Failed to determine the first integer range value");
	}else
		outputError("Unable to determine whether to go up or down in the integer range");
	return NULL;
	// it depends on whether _value1 is smaller than _value2 whether we'll be going up or down
}

Mvalue* applyBinaryOperator(char* operator,Mvalue* _value1,Mvalue* _value2){
	Mvalue* result=NULL;
	if(_value1&&_value2){
		if(amVerbose()){outputValue("Computing '",_value1,NULL);output("' %s '",operator);outputValue(NULL,_value2,"'.\n");}
		switch(operator[0]){
			// real arithmetic
			case '+' :result=add(_value1,_value2);break;
			case '-' :result=subtract(_value1,_value2);break;
			case '*' :result=(strlen(operator)-1?power(_value1,_value2):multiply(_value1,_value2));break;
			case 'e' :result=epower(_value1,_value2);break;
			case '/' :result=(strlen(operator)-1?integerdivide(_value1,_value2):divide(_value1,_value2));break;
			case '\\':result=integerdivide(_value1,_value2);break;
			case '%' :result=divideremainder(_value1,_value2);break;
			// integer arithmetic
			case '^' :result=bitwisexor(_value1,_value2);break;
			case '&' :result=(strlen(operator)-1?logicaland(_value1,_value2):bitwiseand(_value1,_value2));break;
			case '|' :result=(strlen(operator)-1?logicalor(_value1,_value2):bitwiseor(_value1,_value2));break;
			// comparison operators
			case '<' :result=(strlen(operator)-1?(operator[1]=='<'?shiftleft(_value1,_value2):smallerthanorequalto(_value1,_value2)):smallerthan(_value1,_value2));break;
			case '>' :result=(strlen(operator)-1?(operator[1]=='>'?shiftright(_value1,_value2):largerthanorequalto(_value1,_value2)):largerthan(_value1,_value2));break;
			case '!' :result=unequalto(_value1,_value2);break;
			case '=' :result=equalto(_value1,_value2);break;
			case ':' :result=integerrange(_value1,_value2);break; // MDH@18OCT2019: added the 'range' binary operator to generate a list with all integers between _value1 and _value2
			default:output("%sUnknown binary operator '%s'.\n",ERROR_PREFIX,operator);
		}
		if(amVerbose()){if(result)outputValue("Result of applying binary operator: '",result,"'.\n");else outputLine("No result!");}
	}
	return result;
}
// MDH@14OCT2019: using (almost the) same precedence as used in C (except I have power operators as well ** and e)
char getOperatorPrecedence(Mstring* operator){
	if(operator){
		char c;
		switch(c=string_char(operator,0)){
			// real arithmetic
			case 'e' :return 11;
			case '*' :return 9+string_length(operator); // ** has precedence 11, * 10
			case '\\':
			case '%' :
			case '/' :return 10;
			case '+':
			case '-' :return 9;
			case '>' :
			case '<' :return (c==string_char(operator,1)?8:7); // shift operators have precedence 8, comparison operators (<, <=, > and >=) 7
			// bitwise (or logical) operators
			case '^' :return 4;
			case '&' :return (string_char(operator,1)?2:5);
			case '|' :return (string_char(operator,1)?1:3);
			case ':' : // MDH@18OCT2019: lowest priority right now but have to check on this!!!!
			// not-equal/equal operator
			case '!' :
			case '=' :return 6;
		}
	}
	return 0;
}

typedef struct Mformulaelement{
	Mvaluereference* _operand; // an operand to apply the binary operator to
	Mstring* _operator; // a (shortcut) binary operator 
	struct Mformulaelement* _next;
	struct Mformulaelement* _prev; // MDH@21MAY2019: unfortunately needed for moving back!!
}Mformulaelement;
/* any formula starts with
typedef struct Mformula{
	Mvaluereference* _operand;
	Mformulaelement* _next;
}Mformula;
*/
// once we have constructed a formula it needs to be computed

/* MDH@12JUL2019: we need the significant part of the token text
char* getSignificantTokenText(Mtoken* token){
	Mstring* tokenText=token->text; // get a reference to the token's text
	uint32_t tokenTextLength;char firstInsignificantTokenCharacter;
	if(token->significantCharacterCount){
		tokenTextLength=string_length(tokenText);
		firstInsignificantTokenCharacter=string_char(tokenText,token->significantCharacterCount);
		if(!string_shorten(tokenText,expressionToken->significantCharacterCount)){
			outputError("Failed to shorten token text");
			return NULL;
		}
	}
	// we can't call string() as that will write the '\0' overwriting the character we need back
	char* significantTokenText=string(tokenText);
	if(token->significantCharacterCount){
		if(!string_setlength(tokenText,tokenTextLength)||!string_setchar(tokenText,firstInsignificantTokenCharacter,token->significantCharacterCount)){
			// this would be very serious but also very unlikely because we're resetting the length, and writing a character in front of that length
			output("BUG: Failed to restore the token text.\n"); 
			significantTokenText=NULL;
		}
	}
	return significantTokenText;
}
*/
/**
 * an expression is the top-level element of the M language hierarchy
 * which optionally starts with an assignment to a single variable, BUT it makes sense to allow for multiple assignments in a row?????
 * typically this assignee can be composite referencing indices or attributes in maps, obviously we can put this variable in some sort of structure
 * it composes a list of value references to which operators are to be applied
 */
Mvalue* getValueOfExpression(const char* info,char resulttype,TokenType endTokenTypes[],uint8_t endTokenTypeCount){
	Mvalue* _expressionValue=NULL;

	Mtoken* expressionToken=getEnvironmentExpressionToken();
	// typically the offset token determines what the expression ends with!!
	// e.g. ( ends with , or )    [ ends with ]     { ends with }    etc.   
	/////////_expressionvalue->_valuereference=(Mvaluereference*)calloc(1,sizeof(Mvaluereference)); // create a value reference that is to hold a single value reference as result
	
	if(expressionToken){
		if(amVerbose()){
			output("getValueOfExpression() interpreting %s expression starting with token '%s' of type '%s'",info,string(expressionToken->text),TOKENTYPE_STRING[expressionToken->type]);
			if(endTokenTypeCount){
				output(" that ends");
				uint8_t endTokenTypeIndex=0;
				while(endTokenTypeIndex<endTokenTypeCount){output(endTokenTypeIndex?" or ":" with ");output(TOKENTYPE_STRING[endTokenTypes[endTokenTypeIndex++]]);}
			}
			output(".\n");
		}
		
		Mvaluereference* _valuereference;
		Mformulaelement* formula=CALLOC(1,sizeof(Mformulaelement),'F');
		Mformulaelement* _formulaelement=formula;

		int8_t endTokenTypeIndex; // max. 127 token types should suffice!!!

		while(expressionToken){
			if(amVerbose())output("getValueOfExpression() processing %s expression token '%s' of type %s.\n",info,string(expressionToken->text),TOKENTYPE_STRING[expressionToken->type]);
			// does this token end the expression????
			endTokenTypeIndex=endTokenTypeCount;
			// OOPS operators shouldn't break here (and end the expression)
			while(endTokenTypeIndex>0&&expressionToken->type!=endTokenTypes[endTokenTypeIndex-1])endTokenTypeIndex--; // replacing: &&expressionToken->type>=8)endTokenTypeIndex--;
			if(endTokenTypeIndex>0){if(amVerbose())output("End of %s expression.\n",info);break;}
			
			if(_formulaelement){
				_formulaelement->_operand=getValueReference("operand",endTokenTypes,endTokenTypeCount);
				expressionToken=getEnvironmentExpressionToken(); // essential after calling a function that might advance the current expression token
				if(amVerbose())outputValue("Operand: ",getReferencedValue(_formulaelement->_operand),"'.\n");
			}

			// the next token(s) should be a binary operator
			// NOTE some binary operators are stored in a couple of tokens!!!
			if(expressionToken)
				if(expressionToken->type==TT_END_OF_DQSTRING||expressionToken->type==TT_END_OF_SQSTRING)
					expressionToken=nextEnvironmentExpressionToken();

			// MDH@16MAY2019: can't end an expression with an operator BRO'
			if(expressionToken){
				if(amVerbose())output("Does '%s' of type '%s' end the expression? ",string(expressionToken->text),TOKENTYPE_STRING[expressionToken->type]);
				endTokenTypeIndex=endTokenTypeCount;
				while(endTokenTypeIndex&&expressionToken->type!=endTokenTypes[endTokenTypeIndex-1]/*&&expressionToken->type>=8*/)endTokenTypeIndex--;
				if(endTokenTypeIndex){
					if(amVerbose())outputLine("YES"); // replacing: output("Token '%s' of type %s ends the %s expression.\n",string(expressionToken->text),TOKENTYPE_STRING[expressionToken->type],info);
					break;
				}
				if(amVerbose())outputLine(" NO");

				if(amVerbose())output("Interpreting operator token '%s' of type '%s'.\n",string(expressionToken->text),TOKENTYPE_STRING[expressionToken->type]);
				// MDH@12JUL2019: 'remove' non-significant characters
				_formulaelement->_operator=_stringCopy(expressionToken->text,expressionToken->significantCharacterCount); // replacing: _stringCopy(expressionToken->text);
				if(!_formulaelement->_operator){outputError("Failed to copy the operator");break;}
				// MDH@12JUL2019 no need for this anymore: string_setlength(_formulaelement->_operator,expressionToken->significantCharacterCount); // cut off the nonsignificant stuff
				// append any other binary operator behind it (like a continuation or assignment operator)
				while(expressionToken->next&&expressionToken->next->type>2&&expressionToken->next->type<=8){ // OOPS exclude unary operators AND allow for an assignment operator as well
					expressionToken=nextEnvironmentExpressionToken();
					string_append_char(_formulaelement->_operator,string_char(expressionToken->text,0)); // CHECK works for assignment operator but not per se for any operator!!!
				}
				if(expressionToken->next&&expressionToken->next->type==TT_ASSIGNMENT){
					expressionToken=nextEnvironmentExpressionToken();
					string_append_char(_formulaelement->_operator,string_char(expressionToken->text,0)); // CHECK works for assignment operator but not per se for any operator!!!
				}
				if(amVerbose())output("Formula element operator: '%s'.\n",string(_formulaelement->_operator));
				_formulaelement->_next=(Mformulaelement*)CALLOC(1,sizeof(Mformulaelement),'f');
				_formulaelement=_formulaelement->_next;
				expressionToken=nextEnvironmentExpressionToken();
			}else
			if(amVerbose())outputLine("No further formula elements!");
		}

		// evaluate the formula
		if(formula){

			if(amVerbose())outputValue("First formula value: '",formula->_operand->_value,"'.\n");

			// skip all assignments
			// MDH@11AUG2019: how about creating ALL new variables IMMEDIATELY BEFORE evaluating the right-hand-side therefore allowing the use of these new variables in the right-hand-side in formulas as we have accepted??????
			//                the main advantage being that you can use it directly even in the same expression, so as such it won't harm and it has benefits e.g. you can use a local variable immediately
			uint16_t numberOfAssignments=0;
			Mformulaelement* _lastAssignmentFormulaelement=NULL;
			_formulaelement=formula;
			while(_formulaelement){
				if(string_last_char(_formulaelement->_operator)!='=')break; // not ending with assignment operator character to start with
				if(string_char(_formulaelement->_operator,0)=='<'||string_char(_formulaelement->_operator,0)=='>'||string_char(_formulaelement->_operator,0)=='!')break; // break on <=, >= and !=
				if(string_length(_formulaelement->_operator)>1&&string_char(_formulaelement->_operator,0)=='=')break; // break on ==
				if(_lastAssignmentFormulaelement)_formulaelement->_prev=_lastAssignmentFormulaelement; // MDH@21MAY2019: in order to be able to traverse back!!!
				// MDH@11AUG2019: should we force existence?????? I don't think so because creation is done when the value reference is actually created, but I need to make certain that this is the case!!!!
				/*
				if(!addVariable(expressionToken->argument==1?NULL:getEnvironment(),_significantTokenText,VT_UNDEFINED,false)){
					Mstring* _environmentName=_getEnvironmentName();
					output("%sFailed to add%s variable '%s' to environment '%s'.\n",ERROR_PREFIX,(expressionToken->argument!=1&&expressionToken->envid?" implicitly declared local":""),_significantTokenText,string(_environmentName));
					free_string(_environmentName);
					break; // NO retrieves the undefined value subsequently!!
				}
				if(amVerbose())if(expressionToken->argument!=1&&expressionToken->envid)output("WARNING: Not explicitly declared local variable '%s' encountered.\n",_significantTokenText);
				*/
				_lastAssignmentFormulaelement=_formulaelement;
				numberOfAssignments++;
				_formulaelement=_formulaelement->_next;
			}
			if(amVerbose())output("Number of assignments: %u.\n",numberOfAssignments);
			
			// MDH@14OCT2019: applying binary operators typically is done taking operator precedence into account which means we cannot apply lower precedence binary operators until higher precedence binary operators are applied first
			//                which again means that you can apply an operator as soon as the next one does not have a higher priority which means that after applying the highest order operators we have apply the next highest order operator
			//                we always need to compare two successive operators if the precedence of the first is not below the precedence of the second you may apply the first operator, otherwise you skip applying the operator
			//                perhaps it's best to immediately consume formula elements we no longer need!!!!!
			Mvalue* _result=(_formulaelement->_next?NULL:getReferencedValue(_formulaelement->_operand)); // bit of a nuisance though!!!
			Mformulaelement* nextformulaelement;
			char operatorprecedence,nextoperatorprecedence;
			while(_formulaelement->_next){
				operatorprecedence=getOperatorPrecedence(_formulaelement->_operator);
				nextoperatorprecedence=getOperatorPrecedence(_formulaelement->_next->_operator);
				if(operatorprecedence>=nextoperatorprecedence){ // current operator has higher or the same precedence which means we can apply it
					_result=applyBinaryOperator(string(_formulaelement->_operator),getReferencedValue(_formulaelement->_operand),getReferencedValue(_formulaelement->_next->_operand));
					// if we replace any value stored in the value reference of the first operand, we can reuse that formula element
					_formulaelement->_operand->_value=_result;
					// MDH@02NOV2019 replacing: assignValue(&(_formulaelement->_operand->_value),_result);
					// but because _result could be NULL we have to force _name to be NULL just in case 
					if(_formulaelement->_operand->_name){free(_formulaelement->_operand->_name);_formulaelement->_operand->_name=NULL;}
					// we need to point the formula operand to the next of the consumed formula element, so the consumed formula element won't be used again in computations
					nextformulaelement=_formulaelement->_next;
					// point the formula element now storing the result to the next of the consumed formula element
					_formulaelement->_next=nextformulaelement->_next;
					// release the applied operator, and replace it by the successor operator
					free_string(_formulaelement->_operator);_formulaelement->_operator=nextformulaelement->_operator;
					// can't reach the consumed formula element anymore, so release whatever it contains (except for the operator which we have retained)
					free_valuereference(nextformulaelement->_operand); // free the consumed operand
					FREE(nextformulaelement,'F'); // NOTE although it's operator is still pointing to something, it is still pointed to that Mstring (as we took that over), so it should NOT be released!!!!!!
					// if we have a formula element behind us of which the operator has not yet been applied we go back there (because my operator has changed!!!!!)
					if(_formulaelement->_prev)_formulaelement=_formulaelement->_prev;
					// is there a formula element in front of it that has not yet been applied?????
					if(amVerbose())outputValue("Result: '",_result,"'.\n");
				}else{ // we have to apply the next operator BEFORE applying this operator
					_formulaelement->_next->_prev=_formulaelement; // point the next formula element to me, so it's knows that the operator behind it has not yet been applied
					_formulaelement=_formulaelement->_next; // skip applying the current operator for now
				}
			}
			/* replacing:
			Mvalue* _result=getReferencedValue(_formulaelement->_operand); // the first result computed
			if(amVerbose())outputValue("First result: '",_result,"'.\n");
			// 'applying' the binary operators left-to-right remembering the intermediate result in _result
			// NOTE because all formula-elements are freed afterwards (see below) there's no need to so while applying the binary operators
			while(_formulaelement->_next){ // a binary operator to apply
				if(amVerbose())output("Binary operator to apply: '%s'.\n",string(_formulaelement->_operator));
				_result=applyBinaryOperator(string(_formulaelement->_operator),_result,getReferencedValue(_formulaelement->_next->_operand));
				if(amVerbose())outputValue("Next result: '",_result,"'.\n");
				_formulaelement=_formulaelement->_next;
			}
			*/
			if(amVerbose())outputValue("Result: '",_result,"'.\n");
			
			// perform assignments right-to-left (which is a little problematic though)
			if(numberOfAssignments){
				if(amVerbose())output("Performing %u assignments.\n",numberOfAssignments);
				_formulaelement=_lastAssignmentFormulaelement;
				while(_formulaelement){
					_valuereference=_formulaelement->_operand;
					if(amVerbose()){
						Mstring* _indexidText=_getValueText(_valuereference->_itemid,false);
						output("Assignment to %s%s using operator %s!\n",_valuereference->_name,(_indexidText?string(_indexidText):""),string(_formulaelement->_operator));
						if(_indexidText)free_string(_indexidText);
					}
					string_shorten(_formulaelement->_operator,1); // cutting off the assignment operator is fine, as we do not need it anymore!!!
					if(string_length(_formulaelement->_operator)){ // _result will change due to applying the shortcut binary operator
						// we have to be a bit careful here if the value reference uses an index id
						// does the _value field already contain the current value of the variable, if so we may immediately use that here instead of getValue()
						_result=applyBinaryOperator(string(_formulaelement->_operator),getReferencedValue(_valuereference),_result);
						// MDH@02NOV2019 replacing: assignValue(&_result,applyBinaryOperator(string(_formulaelement->_operator),getReferencedValue(_valuereference),_result));
						// replacing:	assignValue(&_result,applyBinaryOperator(string(_formulaelement->_operator),getValue(_Menvironment,_valuereference->_name),_result));
					}
					if(amVerbose())outputValue("Result to store in the value reference: '",_result,"'.\n");
					setReferencedValue(_valuereference,_result);
					if(amVerbose())outputValue("Stored in the value reference: '",_valuereference->_value,"'.\n");
					Mvalue* referencedValue=getReferencedValue(_valuereference);
					if(amVerbose())outputValue("Referenced value to use as result: '",referencedValue,"'.\n");
					// MDH@02NOV2019: we still didn't get a change to a list argument so here also we need to prevent copying the list/map
					_result=referencedValue; // MDH@02NOV2019: replacing: assignValue(&_result,referencedValue); // should we do this???? well, in case the assignment failed!!!
					/* replacing:
					if(_valuereference->_itemid){
						// TODO check whether all the items are of the right type!!!
						appendedToList(_valuereference->_value->value._list,_result,_valuereference->_itemid->value._integer->ll);
						// TODO typically the list will be mutable, but the point here is that we need the full index list to get the right value (which we didn't store!!!!)
					}else{
						setValue(_Menvironment,_valuereference->_name,_result);
						// use the current value of the variable assigned to as new result!!
						assignValue(&_result,getValue(_Menvironment,_valuereference->_name)); // CHECK assign??
					}
					*/
					///////////if(!(--numberOfAssignments))break; // no more assignments???
					_formulaelement=_formulaelement->_prev;
				}
			}

			// the expression value is the value of the first operand!!!
			if(amVerbose()){outputValue("Storing '",_result,"'");output(" as value of expression '%s'.\n",info);}
			_expressionValue=_result; // MDH@02NOV2019 replacing: assignValue(&_expressionValue,_result); // MDH@21MAY2019: this will increment the reference count of _result so it makes sense to actually decrement its reference count after being used

			// free the formula
			if(amVerbose())outputLine("Freeing formula elements.");
			Mformulaelement* _nextformulaelement;
			_formulaelement=formula;
			while(_formulaelement){
				free_string(_formulaelement->_operator);
				free_valuereference(_formulaelement->_operand);
				_nextformulaelement=_formulaelement->_next;
				free(_formulaelement);
				_formulaelement=_nextformulaelement;
			}
			if(amVerbose())outputLine("Formula elements freed.");
		}else
		if(amVerbose())output("No result of expression '%s' to store.",info);
	}
	if(amVerbose()){output("'%s' expression evaluates to",info);outputValue(": '",_expressionValue,"'.\n");}
	return _expressionValue;
}
/**
 * getValueOfExpression() is the work horse for evaluating individual (simple i.e. non composite expressions) expressions 
 * the first token is being passed in which of course should represent a value somehow, evaluateExpression needs 
 */
/*
Mexpressionvalue* getFunctionValue(Mtoken* _offsetToken,char* functionName){
	if(_offsetToken&&functionName){
		Mexpressionvalue* _functionExpressionvalue=(Mexpressionvalue*)calloc(1,sizeof(Mexpressionvalue*));
		_functionExpressionvalue->_token=_offsetToken->next;
		// pFirstToken is the first token in the argument list, arguments are separated by commas
		// 1. compose the list of arguments
		Mlist* arguments=(Mlist*)calloc(1,sizeof(Mlist*));
		char* variableName=NULL; // keep track of the current variable name (that we might need when we run into an assignment)
		char* variableOperator=NULL; // the variable operator applicable to the variable name (right behind the variable and possibly in front of the assignment operator)
		while(pToken!=NULL){
			if(pToken->type==TT_VARIABLE){
				variableName=
			}
			pToken=pToken->next;
		}
		// 2. apply the function to the arguments and return it's result
		if(functionName){
		
		}
		// ASSERT if we get here arguments is the result
		if(arguments->numberOfElements){ 
			// we have arguments left
			if(arguments->numberOfElements==1)return arguments->_first->_value; // the first argument's value is the result
			// wrap the list in an Mvalue!!
			return getListValue(arguments);
	}
	// no result!!!
	return NULL;
}
*/

Mstring* _getCommandText(bool color){
	Mstring* commandText=__string();
	Mtoken* commandToken=_userInputCommand->_firstToken; // TODO can we get rid of using commandcount-1 here????
	while(commandToken){
		// if we bump into a comment we're done!!!
		if(commandToken->type==TT_COMMENT)break;
		// TODO there must be a better way to do the coloring!!!
		if(color){string_append(commandText,ES"38;5;");string_append(commandText,getTokenColor(commandToken->type));string_append_char(commandText,'m');} // assuming the same back color is used on ALL tokens, so we won't have to pass that along
		// MDH@31OCT2019: for now decided NOT to show the whitespace inside the tokens (by replacing the first whitespace character with the end-of-string marker)
		char firstWhitespaceTokenCharacter=(commandToken->significantCharacterCount>0?string_replacedchar(commandToken->text,'\0',commandToken->significantCharacterCount):'\0');
		string_append(commandText,string(commandToken->text));
		if(firstWhitespaceTokenCharacter)string_setchar(commandToken->text,firstWhitespaceTokenCharacter,commandToken->significantCharacterCount); // put first whitespace character (if any) back
		// MDH@03MAY2019: place an asterisk in front of the type to indicate that expr is NOT null!!
		if(amAssisting()){
			if(color){string_append(commandText,ES"38;5;");string_append(commandText,getInfoColor());string_append_char(commandText,'m');}
			string_append_char(commandText,'(');if(commandToken->expr)string_append_char(commandText,'*');string_append(commandText,TOKENTYPE_STRING[commandToken->type]);string_append(commandText,") ");
		}
		commandToken=commandToken->next;
	}
	if(color)if(!amAssisting()){string_append(commandText,ES"38;5;");string_append(commandText,getInfoColor());string_append_char(commandText,'m');} // reset to info color
	return commandText;
}

void clearCommand(){
	free_command(_userInputCommand);_userInputCommand=NULL; // MDH@28OCT2019: using the command now...
	/* replacing:
	pLastCommandToEvaluate=NULL;
	// a small precaution here!!!
	if(_userInputCommand->_firstToken){freeToken(_userInputCommand->_firstToken);_userInputCommand->_firstToken=NULL;}
	*/
}
void outputValueColored(Mvalue* _value){
	if(_value)
	switch(_value->type){
		case VT_UNDEFINED:outputTokenTypeColor(TT_DQSTRING);output("%s",M_NULL_VALUE_TEXT_REPRESENTATION);break; // let's use the same color as for double quotes string (for now)
		case VT_TOKEN:outputTokenTypeColor(_value->value._token->type);output("%s",string(_value->value._token->text));break; // easy the token type determines the color to use!!!
		case VT_INTEGER:outputTokenTypeColor(TT_INTEGER);outputValue(NULL,_value,NULL);break;
		case VT_BIGINTEGER:outputTokenTypeColor(TT_INTEGER);outputBiginteger(NULL,_value->value._biginteger,NULL);break;
		case VT_DECIMAL:outputTokenTypeColor(TT_REAL);outputDecimal(NULL,_value->value._decimal,NULL);break;
		case VT_RATIONAL:
			if(_value->value._rational){
				output("(");
				outputTokenTypeColor(TT_INTEGER);outputBiginteger(NULL,_value->value._rational->num,NULL);resetOutputColor();
				output("/");outputTokenTypeColor(TT_INTEGER);
				if(_value->value._rational->den)outputBiginteger(NULL,_value->value._rational->den,NULL);else outputChar('1'); // a missing denominator means it's equal to 1
				resetOutputColor();
				output(")");
				if(_value->value._rational->delta){
					outputTokenTypeColor(TT_REAL);
					if(_value->value._rational->delta->ld>=0)output("+");
					Mstring* _realValueText=__string();
					if(_realValueText){
						appendld(_realValueText,_value->value._rational->delta->ld);
						output("%s",string(_realValueText));
						free_string(_realValueText);
					}
					// replacing:	output("%.*Lf",LDBL_DIG,_value->value._rational->delta->ld);
					resetOutputColor();
				}
			}
			break;
		case VT_FLOAT:outputTokenTypeColor(TT_REAL);outputValue(NULL,_value,NULL);break;
		case VT_TEXT:outputTokenTypeColor(_value->value._text->presuffix=='"'?TT_DQSTRING:TT_SQSTRING);outputValue(NULL,_value,NULL);break;
		case VT_LIST:
			// TODO not using _getListText() as defined in Mexecution
			/////////if(amVerbose())outputValue("List value '",_value,"'.");
			outputChar('[');
			Mlist* _list=_value->value._list;
			if(_list&&_list->numberOfElements){
				Mlistelement* _listelement=_list->_first;
				unsigned long long listitemindex=1;
				while(_listelement){
					if(_listelement->index)while(listitemindex<_listelement->index){listitemindex++;output(",");} // missing elements
					outputValueColored(_listelement->_value);
					_listelement=_listelement->_next;
				}
			}
			outputChar(']');
			break;
		case VT_MAP:
			outputChar('{');
			Mmap* _map=_value->value._map;
			if(_map&&_map->numberOfElements){
				Mvariable* _mapelementvariable;
				Mmapelement* _mapelement=_map->_first;
				while(_mapelement){
					_mapelementvariable=_mapelement->_variable;
					// TODO are we coloring the name?????
					// quoting the name to indicate it is alphanumeric!!
					output("%c%s%c%c",'\'',_mapelementvariable->_name,'\'',':');
					outputValueColored(_mapelementvariable->_value);
					if(!_mapelement->_next)break;
					outputChar(',');
					_mapelement=_mapelement->_next;
				}
			}
			outputChar('}');
			break;
		case VT_REFERENCE:
			outputChar('@');
            if(_value->value._reference){
                 output("%s",_value->value._reference->variable->_name);
				 outputChar(':');
                 output("%zu",_value->value._reference->referenceindex);
			}
		default:
			break;
	}
	resetOutputColor();
}

void unfinishToken(Mtoken* lastCommandToken){
	// MDH@03SEP2019: adjusted so that not only the unary operators are left finished but all one character token types (which are the only tokens that are immediately finished once a single character is entered!!)
	//                NOTE unfinishing of the current token is done so that the token can be continued, so technically we should unfinish all tokens that can be continued after a character is removed from them
	// for all non-unary token that we are in now that is finished, unfinish 
	// TODO there are other one-character tokens
	if(lastCommandToken)
		if(!isOneCharacterTokenType(lastCommandToken->type)) // not a unary operator (of length 1) we ended up in
			if(string_length(lastCommandToken->text)==lastCommandToken->significantCharacterCount) // the current length equals the number of significant characters (i.e. we remove the first whitespace in the token)
				lastCommandToken->significantCharacterCount=0;
}

bool isAValidCommand(Mcommand* command,bool report){

	// 1. if no command nothing evaluated TODO don't call when this is the case though
	if(!command->_firstToken){if(report)outputError("Undefined command");return false;}
	
	Mtoken* lastCommandToken=command->_lastToken;
	if(!lastCommandToken){if(report)outputError("Unfinished command");return false;}
	
	// 2. if the last token is a comment, remove it before further evaluation TODO should we unfinish the token??????
	//    as a result _userInputCommand->_lastToken and _userInputCommand->_firstToken could now both be NULL, that's why we test this first
	if(lastCommandToken->type==TT_COMMENT)if(!removeToken()){if(report)outputError("Failed to remove the comment");return false;}

	// 3. any command always has two significant tokens TODO could compare _userInputCommand->_firstToken with _userInputCommand->_lastToken which should be different!!!
	//    in this case we clear the command, so that the command won't be repeated, and the user can switch to control mode immediately with the Enter key!!
	/// TODO fix: if(firstCommandToken==lastCommandToken->expr){if(report)outputError("Empty command");/*clearCommand(firstCommandToken);*/return false;} // TODO do we need clearCommand() here at all???????

	// 2. if the last token is an error, can't evaluate (well, better not)
	// TODO it makes sense to remove the error token
	if(lastCommandToken->type==TT_ERROR){if(report)outputError("Can't evaluate erroneous command");if(!removeToken()){if(report)outputError("Failed to remove the error");}unfinishToken(lastCommandToken);return false;}

	// 3. if the last token is an operator of sorts the command is incomplete
	if(lastCommandToken->type<=8){if(report)outputError("Value behind operator at end of command missing");return false;}

	// MDH@03MAY2019: this is new, if expr is not NULL apparently we have missing parentheses!!!!
	//                BUT given that the first token always is of type TT_EXPRESSION and the last token will be pointing to it when complete we'd have to check for that too
	//                    this actually means that if expr is NULL there's one parentheses too many!!!
	/*
	if(!_userInputCommand->_lastToken->expr){outputError("Too many parentheses!");return false;}
	if(_userInputCommand->_lastToken->expr!=_userInputCommand->_firstToken){outputError("Not enough parentheses!");return false;}
	*/
	// MDH@22MAY2019: the following is complex because we might be right behind the closing of a list, map or function call, in which case the command is still complete!!!
	// MDH@27MAY2019: the last token should now either point to the first token in the command, or to something that does point to the first token in the command
	//////////// already noticed while entering the expression!!!!: if(!_userInputCommand->_lastToken->expr){outputError("Too many parentheses!");return false;}
	Mtoken* expressionToken=lastCommandToken->expr; // the token pointed to by the last command token
	if(expressionToken)if(lastCommandToken->type==TT_END_OF_LIST||lastCommandToken->type==TT_END_OF_FUNCTION_CALL||lastCommandToken->type==TT_END_OF_MAP)expressionToken=expressionToken->expr;
	if(expressionToken){ // could be a problem
		// MDH@16OCT2019: I made ] ) and } again point to the associated [ ( and {, which of course should be pointing to NULL if it does not the command is incomplete
		if(amVerbose())if(report)output("First token in last expression pointed to: '%s' of type '%s' at offset '%" PRIu16 "'.\n",string(expressionToken->text),TOKENTYPE_STRING[expressionToken->type],expressionToken->offset);
		if(report)
		switch(expressionToken->type){
			case TT_LIST:outputError("Missing end of list");break;
			case TT_FUNCTION_CALL:outputError("Missing end of function call");break;
			case TT_MAP:outputError("Missing end of map");break;
			default:output("%sUnknown expression with first token of type %s left unfinished.\n",ERROR_PREFIX,TOKENTYPE_STRING[expressionToken->expr->type]);break;
		}
		return false;
		/* replacing:
		// MDH@23JUL2019: we can now be very strict
		//                the last token should point to the first expression which only contains whitespace, whereas all other expression tokens start with ()
		if(_userInputCommand->_lastToken->expr->type!=TT_EXPRESSION||(string_length(_userInputCommand->_lastToken->expr->text)&&string_char(_userInputCommand->_lastToken->expr->text,0)!=' ')){
			outputError("Incomplete command");
			return false;
		}
		*/
		/* replacing:
		// this is allowed if this token ends something that points to NULL
		if((_userInputCommand->_lastToken->type!=TT_END_OF_LIST&&_userInputCommand->_lastToken->type!=TT_END_OF_FUNCTION_CALL&&_userInputCommand->_lastToken->type!=TT_END_OF_MAP)||_userInputCommand->_lastToken->expr->expr){
			switch(_userInputCommand->_lastToken->expr->expr->type){
				case TT_LIST:outputError("Missing end of list.");break;
				case TT_FUNCTION_CALL:outputError("Missing end of function call!");break;
				case TT_MAP:outputError("Missing end of map!");break;
				default:outputError("Not enough parentheses.");break;
			}
			return false;
		}
		*/
	}

	// 4. can't end with function of function call
	// MDH@20JUL2019: BUT we can treat the function as (new) variable, although new variables should not occur at the end of a command???
	if(lastCommandToken->type==TT_FUNCTION){if(report)outputError("Function call missing at end of command");return false;}
	if(lastCommandToken->type==TT_FUNCTION_CALL){if(report)outputError("Unfinished function call");return false;}
	if(lastCommandToken->type==TT_LIST||lastCommandToken->type==TT_LISTELEMENT){if(report)outputError("Unfinished list");return false;}
	if(lastCommandToken->type==TT_DQSTRING||lastCommandToken->type==TT_SQSTRING){if(report)outputError("Unfinished string literal");return false;}
	if(lastCommandToken->type==TT_EXPRESSION){if(report)outputError("Unfinished expression");return false;}
	if(lastCommandToken->type==TT_MAP||lastCommandToken->type==TT_MAP_VALUE){if(report)outputError("Unfinished map");return false;}
	
	return true;

}

void outputCommandInfo(Mcommand* command);

// if a sequence of tokens needs to be evaluated to a value, call getCommandValue()
Mvalue* getCommandValue(Mcommand* command,char commandType){
	if(amVerbose())outputCommandInfo(command);
	if(!isAValidCommand(command,amVerbose()))return NULL;
	getEnvironment()->expressionToken=command->_firstToken->next; // prepare the current environment for executing the command
	if(amVerbose())outputLine("Evaluating...");
	return getValueOfExpression(getEnvironment()->_name,commandType,(TokenType[]){},0);
}

// anything the user types is a sequence of tokens which we can store in a linked list
bool evaluateCommand(){
	
	/// NOT HERE!! outputChar('\n'); // indicating that the command is being evaluated!!!
	if(!isAValidCommand(_userInputCommand,true))return false;

	// evaluating means getting the value of the expression that _userInputCommand->_firstToken points to
	// NOTE that the first token is always a dummy token (which will at most contain the whitespace at the start of the command)
	Mstring* commandText=_getCommandText(true);
	// plug the token following the dummy starting token of the command into the current execution environment (typically _Menvironment I suppose)
	getEnvironment()->expressionToken=_userInputCommand->_firstToken->next; // initialize the (current) expression token
	clock_t then=clock();
	Mvalue* _commandExpressionValue=getValueOfExpression("command",'e',(TokenType[]){},0);
	resetOutputColor(); // MDH@02OCT2019: given that the out() might've been used to write stuff to the console in weird colorings TODO doesn't seem to help	
	long long elapsed=(clock()-then)/1000;if(elapsed>0)output("The evaluation took %d ms.\n",elapsed);/////////else output("less than 1 ms.");
	// output the commandText
	output("%s = ",string(commandText));
	// if the result is a null value, show the NULL_value
	outputValueColored(isValueNull(_commandExpressionValue)?NULL_value:_commandExpressionValue);
	
	///////////////decrementReferenceCount(_commandExpressionValue); if(amVerbose())outputLine("Result released!"); // TODO do we need to do this?????

	///////if(amVerbose())outputLine("Command to release!");
	free_string(commandText);
	////////if(amVerbose())outputLine("Command released!");
	return true;

}

void prepareForUserInput(){
	//enableRawMode();
	// disable output buffering on printf (as in raw input mode it would not write at all)
	setbuf(stdout,NULL);
	initSession();
}

// MDH@24APR2019: writeCommand() writes the command to evaluate, and sets _userInputCommand->_lastToken in the process
// MDH@31OCT2019: there's a complication when the last character in the token is the newline character
void writeCommand(Mcommand * const command){
	Mtoken* token=(command?command->_firstToken:NULL);
	while(token){
		numberOfBehindPromptCharactersWritten+=outputToken(command->_lastToken=token);
		// are we supposed to generate a newline?
		// NOTE we're assuming there that a string can never end with this character which is also the text escape character (which requires another character following it)
		if(string_last_char(command->_lastToken->text)==M_NEWLINE_CHARACTER)showContinuedPrompt();
		token=token->next;
	}
}

uint32_t commandPage=0; // the command page to show (when 0 not paging through the commands)
uint32_t commandPages=0; // the total number of command pages
void setCommandPage(uint32_t createUserInputCommandPage){
	commandPage=createUserInputCommandPage;
	int32_t commandToShowIndex=10,lastCommandToShowIndex=commandCount-(commandPage*10);
	while(--commandToShowIndex>=0&&lastCommandToShowIndex+commandToShowIndex>=0){
		resetOutputColor();
		output("%d. ",lastCommandToShowIndex+commandToShowIndex+1);
		Mtoken* token=commands[lastCommandToShowIndex+commandToShowIndex]->_firstToken;
		while(token){outputToken(token);token=token->next;}
	}
	resetOutputColor();
	output("%s","Select the last digit of the command to use, or the up/down key to show the next/previous page.");
	output("%s",">> "); // TODO what kind of prompting do we want to do???
}
void showNextCommandPage(){
	if(commandPage<commandPages)
		setCommandPage(commandPage+1);
	else
		output("%s","No further commands to show.");
}
void showPreviousCommandPage(){
	if(commandPage>1)
		setCommandPage(commandPage-1);
	else
		output("%s","No further commands to show.");
}
/*
// when the user tries to insert a character we need to cut off the rest of the command and append it afterwards
char* removedRestOfCommand(){
	if(getUserInputLength()<getCommandLength()){
		Mstring* restOfCommand=__string();
		if(restOfCommand!=NULL){
			uint16_t tokenPosition=getUserInputLength()-_userInputCommand->_lastToken->offset;
			if(tokenPosition)string_append(restOfCommand,string_remainder(_userInputCommand->_lastToken->text,tokenPosition));
			string_setlength(_userInputCommand->_lastToken->text,tokenPosition); // the new length of the token (cutting off what's behind it)
			// now to append the text in the rest of the tokens
			Mtoken* token=_userInputCommand->_lastToken->next;
			if(token!=NULL){
				while(token!=NULL){string_append(restOfCommand,string(_userInputCommand->_lastToken->text));token=token->next;}
				freeToken(token); // we'll free all the token starting at the successor of _userInputCommand->_lastToken
				_userInputCommand->_lastToken->next=NULL;
			}
			outputInfo("Rest of command: '%s'.",string(restOfCommand));
			return string(restOfCommand);
		}
	}
	return NULL;
} 
*/
/*
void writeRestOfCommand(){ // writes rest of command assuming _userInputCommand->_lastToken is not NULL and we are to return to the current cursor position adterwards!!
	uint16_t leftToWrite=getCommandLength()-getUserInputLength();
	if(leftToWrite>0){ // something left to write
		// something of the current token to write?
		if(getUserInputLength()>_userInputCommand->_lastToken->offset){ // part of current token to write
			outputTokenColor(_userInputCommand->_lastToken);printf("%s",string_remainder(_userInputCommand->_lastToken->text,getUserInputLength()-_userInputCommand->_lastToken->offset));
		}
		// write the rest of the tokens
		writeTokens(_userInputCommand->_lastToken->next);
		moveCursorLeft(leftToWrite);
	}
}
*/
void setInputMode(enum INPUTMODE_ENUM newInputMode){
	inputMode=newInputMode;
	deleteTokenautocompletiontexts();
	// in command mode the behind cursor text is determined JIT, whereas it is actively used in the other modes!!!
	if(_suggestedText)string_setlength(_suggestedText,0);
}
char switchToControlMode(char* message){
	if(inputMode!=IM_CONTROL){
		if(inputMode==IM_COMMAND)clearCommand();
		if(message!=NULL){setColor(getErrorColor());output("\n%s\n",message);} // MDH@01OCT2019: message will typically be an error so
		resetOutputColor();
		setInputMode(IM_CONTROL);
		outputVariables(); // immediately show the list of available variables (so we can then use v for verbose flag, AND s is available for Shell again!!!!)
	}
	///////////outputFlags(); // show the user the current flags!!
	return 'o'; // to make the loop know to quit
	//output("%s\n >> ","Control mode: Flags: Assist Debug - Options: eXit History Shell");
}

size_t getNumberOfAutocompletionCharacters(){return string_length(_autoCompletionText);} // TODO assuming _autoCompletionText has actually been constructed!!!!
size_t getNumberOfTokenAutocompletionTexts(){
	size_t numberOfTokenAutocompletionTexts=0;
	Mtokenautocompletiontext* tokenautocompletiontext=_firstTokenautocompletiontext;
	while(tokenautocompletiontext){numberOfTokenAutocompletionTexts++;tokenautocompletiontext=tokenautocompletiontext->_next;}
	return numberOfTokenAutocompletionTexts;
}

size_t getNumberOfManualFeedforwardCharactersWritten(){
	// MDH@27SEP2019 doesn't update the identifier continuation anymore (as it might be optional and is moved over to writeBehindCursorText) removing: updateUserInputCommandIdentifierContinuation();
	size_t numberOfManualFeedforwardCharactersWritten=0;
	// MDH@04OCT2019: append it to the suggested text
	if(string_length(_manualFeedforwardText)>0){
		if(numberOfIdentifierContinuationManualFeedforwardCharacters>0){
			char c=string_replacedchar(_manualFeedforwardText,'\0',numberOfIdentifierContinuationManualFeedforwardCharacters);
			setColor(getIdentifierContinuationTextColor());
			numberOfManualFeedforwardCharactersWritten=output("%s",string(_manualFeedforwardText));
			string_setchar(_manualFeedforwardText,c,numberOfIdentifierContinuationManualFeedforwardCharacters); // OOPS put it back before showing not after showing!!!
		}
		setColor(getManualFeedforwardTextColor());
		numberOfManualFeedforwardCharactersWritten+=output("%s",string(_manualFeedforwardText)+numberOfManualFeedforwardCharactersWritten);
		if(numberOfManualFeedforwardCharactersWritten){
			string_setlength(_manualFeedforwardText,numberOfManualFeedforwardCharactersWritten); // just in case not all characters were written!!!
			if(!string_append(_suggestedText,string(_manualFeedforwardText)))numberOfManualFeedforwardCharactersWritten=0;
		}
	}
	return numberOfManualFeedforwardCharactersWritten; // one less character written than the computed length!!!
}
size_t getNumberOfIdentifierContinuationTextCharactersWritten(){
	// MDH@27SEP2019 doesn't update the identifier continuation anymore (as it might be optional and is moved over to writeBehindCursorText) removing: updateUserInputCommandIdentifierContinuation();
	size_t numberOfIdentifierContinuationCharactersWritten=(_identifierContinuationCharacters?strlen(_identifierContinuationCharacters):0);
	// MDH@04OCT2019: append it to the suggested text
	if(numberOfIdentifierContinuationCharactersWritten>0)if(!string_append(_suggestedText,_identifierContinuationCharacters))numberOfIdentifierContinuationCharactersWritten=0; // append to suggested text
	if(numberOfIdentifierContinuationCharactersWritten>0){
		setColor(getIdentifierContinuationTextColor());
		output("%s",_identifierContinuationCharacters);
		/*
		size_t numberOfIdentifierContinuationCharactersToWrite=numberOfIdentifierContinuationCharactersWritten;
		while(numberOfIdentifierContinuationCharactersToWrite){outputChar(' ');numberOfIdentifierContinuationCharactersToWrite--;}
		*/
	}
	return numberOfIdentifierContinuationCharactersWritten; // one less character written than the computed length!!!
}
size_t getNumberOfImmediateFeedforwardCharactersWritten(){
	size_t numberOfImmediateFeedforwardCharactersWritten=(_immediateFeedforwardText?string_length(_immediateFeedforwardText):0);
	if(numberOfImmediateFeedforwardCharactersWritten>0)if(!string_append(_suggestedText,string(_immediateFeedforwardText)))numberOfImmediateFeedforwardCharactersWritten=0; // append to suggested text
	if(numberOfImmediateFeedforwardCharactersWritten>0){setColor(getFeedForwardTextColor());output(string(_immediateFeedforwardText));}
	return numberOfImmediateFeedforwardCharactersWritten;
}
size_t getNumberOfAutocompletionCharactersWritten(){
	size_t numberOfAutocompletionCharactersWritten=(_autoCompletionText?string_length(_autoCompletionText):0);
	if(numberOfAutocompletionCharactersWritten>0)if(!string_append(_suggestedText,string(_autoCompletionText)))numberOfAutocompletionCharactersWritten=0; // append to suggested text
	if(numberOfAutocompletionCharactersWritten>0){ // additional auto completion text to write
		/////debugWrite("Auto completion characters to write: '%s'.",string(_autoCompletionText));
		setColor(getFeedForwardTextColor());
		output("%s",string(_autoCompletionText));
	}
	return numberOfAutocompletionCharactersWritten;
}
/*
// MDH@27SEP2019: when the user just deleted the identifier continuation we would not want it to be generated immediately
void writeSuggestedText(bool updateUserInputCommandIdentifierContinuationText){
	// MDH@26SEP2019: behind cursor text now consists of two parts now: identifier continuation text and feed forward text
	// 0. preparation
	resetOutputColor();
	// we're going to compute the total number of characters written behind the prompt (), so we will know how many blanks we need to append
	// MDH@27SEP2019: the best idea today is to update the identifier continuation text just before showing it
	// 1. write the identifier continuation text and feed forward text
	size_t commandLength=getCommandLength();
	size_t newNumberOfBehindPromptCharactersWritten=commandLength;
	if(updateUserInputCommandIdentifierContinuationText)updateUserInputCommandIdentifierContinuation(); // force an update of the identifier continuation (could have been already done in updateLastTokenAutocompletionText()!)
	newNumberOfBehindPromptCharactersWritten+=getNumberOfIdentifierContinuationTextCharactersWritten();
	newNumberOfBehindPromptCharactersWritten+=getNumberOfFeedforwardCharactersWritten();
	////////inputInfo("Number of written suggested characters: %zu.",newNumberOfBehindPromptCharactersWritten);
	// 2. write additional blanks overwriting what we had before
	while(newNumberOfBehindPromptCharactersWritten<numberOfBehindPromptCharactersWritten){newNumberOfBehindPromptCharactersWritten++;outputChar(' ');}
	numberOfBehindPromptCharactersWritten=newNumberOfBehindPromptCharactersWritten;
	outputChar(' ');  // one extra to be on the safe size TODO why?????????
	moveCursorLeft(newNumberOfBehindPromptCharactersWritten+1-commandLength); // return to where the command ends
	// 3. finalize: remember the actual number of characters written (and therefore will not be blanks)
	// update numberOfBehindPromptCharactersWritten (we do not need to remember blanks written!!!å)
	if(_userInputCommand->_lastToken)outputTokenColor(_userInputCommand->_lastToken); // return to the color of the current token
}
*/

// MDH@01OCT2019: it's better to show the suggested text JIT i.e. just before asking the user for input
//                this is also better because at that moment we know the user should be seeing it
size_t numberOfSuggestedCharactersWritten=0;
void showSuggestedText(){
	// ASSERT _suggestedText should not be NULL
	// MDH@26SEP2019: behind cursor text now consists of two parts now: identifier continuation text and feed forward text
	// 0. preparation
	string_setlength(_suggestedText,0); // clear the suggested text!!!
	resetOutputColor();
	// 1. write the identifier continuation first, then the manual feed forward, the immediate feed forward characters and finally the auto completion text
	if(string_length(_manualFeedforwardText)>0)
		numberOfSuggestedCharactersWritten=getNumberOfManualFeedforwardCharactersWritten();
	else
		numberOfSuggestedCharactersWritten=getNumberOfIdentifierContinuationTextCharactersWritten();
	numberOfSuggestedCharactersWritten+=getNumberOfImmediateFeedforwardCharactersWritten()+getNumberOfAutocompletionCharactersWritten();
	// 2. and back to where the cursor is supposed to be
	moveCursorLeft(numberOfSuggestedCharactersWritten); // return to where the command ends
	// 3. finalize: remember the actual number of characters written (and therefore will not be blanks)
	// update numberOfBehindPromptCharactersWritten (we do not need to remember blanks written!!!å)
	outputUserInputCommandTokenColor(); // replacing: if(_userInputCommand->_lastToken)outputTokenColor(_userInputCommand->_lastToken); // return to the color of the current token
}
void hideSuggestedText(){
	int32_t numberOfBlanksToWrite=numberOfSuggestedCharactersWritten;
	while(--numberOfBlanksToWrite>=0)outputChar(' ');
	///////outputChar(' ');
	moveCursorLeft(numberOfSuggestedCharactersWritten);
}

// MDH@20SEP2019: whenever a token changes the associated feed forward text might change, so it makes sense to update the feed forward text accordingly
//                assuming that the given type is correct (e.g. an identifier of which has been determined whether it is a function or variable name)
//MDH@26SEP2019: due to the separation of the identifier continuation text and the other feed forward text we separate getting the identifier continution text from getting the other feed forward text
void updateLastTokenAutocompletionText(){
	// MDH@27SEP2019: updating the identifier continuation now moved to writeSuggestedText(true), so JIT update
	setLastTokenAutocompletionText(_getLastTokenAutoCompletionText()); // MDH@24SEP2019: the bool forces setting the identifier continuation characters when available, so we can show them to the user
	inputInfo("Last token auto completion text updated.");
}

void backToPrompt(){
	// MDH@27SEP2019: assuming for now that whatever is written next will determine the number of characters written behind the prompt
	numberOfBehindPromptCharactersWritten=0; // assume no text characters written so far
	// this will be more complicated if the command occupies multiple lines
	// therefore we need to move the cursor left, write a single blank and move the cursor one left again and so on
	// replacing: restoreCursor();clearScreenFromCursor();

	// MDH@31OCT2019: with a clearScreenFromCursor() following it suffices to first move to the initial prompt line
	size_t linesfreed=free_userinputline();
	while(linesfreed>0){linesfreed--;oneLineUp();}
	toStartOfLine();moveCursorRight(promptLength); // should now be at the right position for clearing
	/* replacing:
	uint16_t cp=getUserInputLength();
	while(cp--)backspace(); // MDH@24APR2019 replacing: while(getUserInputLength()>0){getUserInputLength()--;backspace();}
	*/
	/*
	if(getUserInputLength()>0){moveCursorLeft(getUserInputLength());getUserInputLength()=0;}
	clearScreenFromCursor();
	*/
	/* replacing:
	while(characterCount>0){
		characterCount--;
		moveCursorLeft(1);resetOutputColor();outputChar(' ');moveCursorLeft(1);
	}
	*/
}

void setUserInputCommand(Mcommand* command){
	_userInputCommand=command;
	// replacing: _userInputCommand->_lastToken=_userInputCommand->_firstToken=pCommand;
	writeCommand(_userInputCommand);
	// MDH@30OCT2019: userInputCommandIdentifierContinuationNeedsUpdating=(_userInputCommand?inIdentifierToken(_userInputCommand->_lastToken):false); // MDH@02OCT2019 because we're setting _userInputCommand->_lastToken but not calling setLastUserInputCommandToken()
	//////////writeSuggestedText(true);
	// MDH@06AUG2019 TODO: determine the initializations associated with a stored command!!!
	////////// removing: determineCommandInitializations();
}
/**
 * setCommandIndex() accepts \p createUserInputCommandIndex between 0 and commandCount at most
 * but 0 is now also accepted, returning to show _userInputCommand->_firstToken (if any)
 */
void setCommandIndex(uint32_t createUserInputCommandIndex){
	commandIndex=createUserInputCommandIndex;
	////if(amVerbose())inputInfo("Command index %lld.",commandIndex);
	// it's easier to go to the beginning of the line although we could be on the line below!!!!
	// replacing: 
	backToPrompt();
	clearScreenFromCursor();
	// MDH@29OCT2019: we have to do the following because otherwise inputInfo() will jump back to the end of the command instead of right behind the prompt!!!!
	//                NOTE typically _userInputCommand will not be NULL when we're scrolling through the list of previous commands!!!!
	if(_userInputCommand)setUserInputCommand(NULL); 
	clearInfo();
	// MDH@24APR2019 obsolete: getCommandLength()=getUserInputLength()=0; // do we need this????
	// TODO do we need to do this: clear the behind cursor text (in any situation)
	deleteAutocompletionText(); // MDH@20SEP2019 replacing: string_setlength(feedforwardText,0); 
	if(commandIndex){
		// MDH@29OCT2019 should already have _userInputCommand equal to NULL: _userInputCommand->_lastToken=NULL; 
		// replacing: _userInputCommand->_lastToken=NULL; // MDH@03SEP2019: I have to do this otherwise inputInfo() won't work the way we want it to
		Mcommand* command=commands[commandCount-commandIndex];
		// MDH@19APR2019: if not to accept the history command we use the previous command as behind cursor text
		// TODO this construction (with a return in the middle is a bit unclear)
		if(amAcceptinghistorycommand()){ // use the history command as autocompletion text instead of accepting it immediately as command!!!
			/////if(amVerbose())
			inputInfo("Showing registered command #%lld.",commandCount-commandIndex+1);
			setUserInputCommand(command);
			return;
		}
		if(amVerbose())inputInfo("Showing command #%lld as suggested text.",commandCount-commandIndex+1);
		// the previous command will be used as behind cursor text, and not immediately as command
		// MDH@20SEP2019
		// this is a bit of a nuisance as there will be no associated tokens for the entire behind cursor text
		// theoretically this means we could store the entire behind cursor text in a single feed forward text
		// we can abuse feedforwardText, by using it to compose it, and afterwards use it to create a single feed forward text instance
		Mstring* _commandFeedforward=__string(); // free asap
		if(_commandFeedforward){
			Mtoken* token=command->_firstToken;
			while(token){
				///////outputText("(%s)",tokenText);
				if(!string_append(_commandFeedforward,string(token->text)))break;
				token=token->next;
			}
			// because the last token is NULL, we can use setLastTokenAutocompletionText to register _commandFeedforward used as the entire feed forward text
			setLastTokenAutocompletionText(string(_commandFeedforward));
			free_string(_commandFeedforward); // get rid of the feed forward text we constructed
		}
	}
	/* MDH@29OCT2019 removed to the start:
	setUserInputCommand(NULL); // TODO probably shouldn't have to do this??????
	clearInfo(); // TODO do we need this when showing a previous command as behind cursor text??????
	*/
	/////////////printf("(%d)",getCommandLength());
}
bool commandDown(){
	if(commandCount==0)return false;
	setCommandIndex(commandIndex<commandCount?commandIndex+1:0);
	return true;
}
bool commandUp(){
	// MDH@26FEB2019: instead of stopping at the start of the commands it's better to move back to the new command which is at commandCount
	if(commandCount==0)return false;
	setCommandIndex(commandIndex>0?commandIndex-1:commandCount);
	return true;
}

// MDH@23SEP2019: whenever the type of the current token (_userInputCommand->_lastToken) changes (possibly with the start of a new token), so will the feed forward text associated with that token
//                therefore it is best to set the last token type using a separate function
// MDH@03OCT2019: every time the token type changes we need to sync the immediate feed forward text as well!!!!
void setTokenType(Mtoken* token,TokenType tokenType/*,bool endOfInput*/){
	if(token){
		if(tokenType!=token->type){
			// MDH@04OCT2019 moved to input loop removing: if(!deleteLastTokenImmediateFeedforwardText())inputError("Failed to remove the current token immediate feed forward text.");
			token->type=tokenType;
			// MDH@04OCT2019 moved to input loop removing: if(!updateImmediateFeedforwardTextOfUserInputCommand())inputError("Failed to add the current token immediate feed forward text.");
		}
	}
	///////// MDH@29OCT2019 probably don't need this here anymore: if(endOfInput)updateLastTokenAutocompletionText();
}

// MDH@23SEP2019: prudent to replace all calls to _getToken that simply append a new token to the command, by a method that will always call setLastTokenType() 
// command generic (i.e. it does not need to be the user input command, it could be some command that is being parsed)
Mtoken* _getNewCommandToken(Mtoken* lastCommandToken,TokenType tokenType){
	// MDH@01OCT2019: because the current token is NOT removed from the command, we should NOT delete its associated feed forward text
	//                but we should remove any identifier continuation
	// MDH@02OCT2019 no need for this anymore here: if(endOfInput)deleteIdentifierContinuation(); // remove whatever feed forward text that was associated with the now finished last command token as it will no longer be applicabld
	Mtoken* _newCommandToken=_getToken(lastCommandToken,tokenType);
	if(_newCommandToken)setTokenType(_newCommandToken,tokenType);else if(amDebugging())inputError("Failed to create a command token");
	return _newCommandToken;
}

void createUserInputCommand(){
	// MDH@24APR2019 obsolete: getCommandLength()=string_length(feedforwardText); // MDH@21APR2019: oops was 0 before...
	resetOutputColor(); // TODO do we need this here?????
	if(amDebugging())inputInfo("Creating the new user input command.");
	// MDH@23SEP2019: createUserInputCommandToken() added to take care of updating _userInputCommand->_lastToken (should be NULL as it is used to represent the previous last token)
	_userInputCommand=_getNewCommand(true);
	// MDH@29OCT2019: the following is absolutely silly although how about updating 
	if(_userInputCommand){
		// MDH@30OCT2019: userInputCommandIdentifierContinuationNeedsUpdating=false; // MDH@29OCT2019: instead of calling setLastUserInputCommandToken()
		updateLastTokenAutocompletionText(); // TODO perhaps we do not need this after all here????? NOTE used to do that in setTokenType() when endInput was true but not doing that anymore
		if(amDebugging())inputInfo("New user input command created.");
	}else
		inputError("Failed to create a new user input command.");
	/* replacing: 
	_userInputCommand->_firstToken=_getNewCommandToken(NULL,TT_EXPRESSION,true,true);
	setLastUserInputCommandToken(_userInputCommand->_firstToken); // so updating identifierContinuationIsDirty is guaranteed!!!
	if(!_userInputCommand->_firstToken)outputError("Failed to create a new command");else _userInputCommand->_firstToken->expr=NULL;
	*/
	/* replacing:
	_userInputCommand->_lastToken=_userInputCommand->_firstToken=_getToken(NULL,TT_EXPRESSION);
	if(!_userInputCommand->_firstToken){outputError("Failed to create a new command");return;}
	setLastTokenType(TT_EXPRESSION,true); // MDH@23SEP2019: endOfInput set to true, although we know there will be no feed forward on an expression
	////////// removing: removeInitializations(); // MDH@06AUG2019: ready for new initializations at the start of a new command
	// MDH@27MAY2019: NO let's just keep expr NULL!!!
	_userInputCommand->_firstToken->expr=NULL; // TODO do I need this???? YES, because we used _getToken()! PERHAPS NOT as prevToken is NULL???????
	*/
}

// TODO copyUserInputCommand() should set ->expr correctly
// MDH@29OCT2019: TODO caller should check whether or not _userInputCommand is NULL if it is copying failed!!!!
void copyUserInputCommand(){
	// ASSERT _userInputCommand must NOT be NULL and we're assuming that _userInputCommand now points to one of the remembered commands (that needs to be duplicated in order to allow editing it)
	//        it's probably best to first create a new command, copy the tokens over from _userInputCommand and set the user input command to that new command
	Mcommand* _newUserInputCommand=_getNewCommand(false); // get a new command without tokens (should NEVER fail unless memory shortage)
	if(_newUserInputCommand){
		// if fails to copy _userInputCommand->_firstToken _userInputCommand->_lastToken should end up as NULL
		if(amDebugging())inputInfo("Preparing the user input command for editing.");
		///// NOT NEEDED using the false flag in _getNewCommand()!!!! _newUserInputCommand->_lastToken=NULL;_newUserInputCommand->_firstToken=NULL;
		Mtoken* _tokenToCopy=_userInputCommand->_firstToken;
		// the essence is that _userInputCommand->_lastToken points to the last token in _userInputCommand->_firstToken
		// NOTE theoretically _userInputCommand->_lastToken could be NULL due to _getToken() failing to create a new token
		while(_tokenToCopy){
			// MDH@28OCT2019: because we adapted _getNewCommandToken to receive the last command token as argument, and returning the new command token, we need to assign the result to _userInputCommand->_lastToken!!!
			_newUserInputCommand->_lastToken=_getNewCommandToken(_newUserInputCommand->_lastToken,_tokenToCopy->type);
			if(!_newUserInputCommand->_lastToken)break;
			// replacing: if(!setLastUserInputCommandToken(_getNewCommandToken(_userInputCommand->_lastToken,_tokenToCopy->type)))break; // MDH@23SEP2019: TODO should we do something to _userInputCommand->_firstToken when this happens? or show some error???
			/* MDH@23SEP2019 replacing:
			_userInputCommand->_lastToken=_getToken(_userInputCommand->_lastToken,_tokenToCopy->type);
			///////// MDH@23SEP2019: moved out of _getToken() using false for endOfInput to prevent adding/changing the associated feed forward text
			setLastTokenType(_tokenToCopy->type,false);
			*/
			/* TODO check whether the following is correct!!! guess not!!
			if(_userInputCommand->_lastToken->type==TT_END_OF_FUNCTION_CALL||_userInputCommand->_lastToken->type==TT_END_OF_LIST||_userInputCommand->_lastToken->type==TT_END_OF_MAP){
				if(_tokenToCopy->expr)
					_userInputCommand->_lastToken->expr=_userInputCommand->_lastToken->expr->expr;
				else
					outputLine("BUG: End of argument list or map encountered, but not started.");
			}
			*/

			// MDH@29OCT2019: if we want to do it right we should check who's referencing back to _tokenToCopy
			Mtoken *referencedToken=_tokenToCopy->expr;
			if(_tokenToCopy->expr){ // some token referenced
				Mtoken *referencedToken=_tokenToCopy,*newReferencedToken=_newUserInputCommand->_lastToken;
				// move back until we find the token referenced (and we should find it)
				while(referencedToken!=_tokenToCopy->expr){referencedToken=referencedToken->prev;newReferencedToken=newReferencedToken->prev;}
				// ASSERT referencedToken now equals the token in the original command being referenced (which could be itself obviously), and newReferencedToken is a token in the new user input command that should be pointed to!!!
				if(newReferencedToken)_newUserInputCommand->_lastToken->expr=newReferencedToken;else inputError("BUG: Failed to synchronize a token reference.");
			}else // nothing pointed to, so just in case
				_newUserInputCommand->_lastToken->expr=NULL;
			// replacing: _newUserInputCommand->_lastToken->expr=_tokenToCopy->expr; // MDH@20MAY2019: just copy the expr over!!!!
			
			_newUserInputCommand->_lastToken->significantCharacterCount=_tokenToCopy->significantCharacterCount;
			// if failing to copy the text over get rid of the command constructed so far, and break
			_newUserInputCommand->_lastToken->text=_stringCopy(_tokenToCopy->text,0); // copies the entire Mstring over
			if(!_newUserInputCommand->_lastToken->text){_newUserInputCommand->_lastToken=NULL;break;} // TODO perhaps we'd have to do a little more than just this?????
			// MDH@24APR2019 obsolete: getCommandLength()+=string_length(_userInputCommand->_lastToken->text);
			// some additional fields to copy over (NOT the offset is that is set automatically)
#ifdef __DEBUG__
			printf("%d:%s",_userInputCommand->_lastToken->type,string(_userInputCommand->_lastToken->text));
#endif
			if(!_newUserInputCommand->_firstToken)_newUserInputCommand->_firstToken=_newUserInputCommand->_lastToken; // TODO=DONE will never happen???? it does here
			// get the next token to copy...
			_tokenToCopy=_tokenToCopy->next;
		}
		if(!_newUserInputCommand->_lastToken){free_command(_newUserInputCommand);_newUserInputCommand=NULL;}
	}else
		inputError("Failed to prepare the command for editing");
	// OOPS do NOT call setUserInputCommand() here as it will write the command once more so it might suffice to assign
	_userInputCommand=_newUserInputCommand; // replacing: setUserInputCommand(_newUserInputCommand); // testing whether successful: inputInfoCommand(_userInputCommand);
}

// NEWYEAR'S DAY 2019: It's a nuisance to show a command without copying it into an actual createUserInputCommand
/**
 * setCommand() creates a new (empty) command (in _userInputCommand->_firstToken) and initializes it to the token in pNewCommand (the command pointed to by commandIndex)
 *              which is supposedly showing behind the cursor!!!
 * ASSUMPTION should only be called when at the prompt (getUserInputLength()=0) ready for starting or changing a command
 * setCommand() won't show the command anymore as we assume that any registered command passed in is already showing!!!
 */
/*
Mtoken* getCommand(){
	return(commandIndex&&getUserInputLength()?commands[commandCount-commandIndex]:_userInputCommand->_firstToken);
}
void echoCommand(){
	Mtoken* token=_userInputCommand->_firstToken;
	resetOutputColor();
	while(token){printf("%s",string(token->text));token=token->next;}
}
void setCommand(Mtoken* pNewCommand){
	// ASSERT let's assume we're at the prompt (i.e. getUserInputLength()==0 and _userInputCommand->_firstToken==NULL)
	// NO we cannot assume that because there might be a command currently showing at the prompt
	if(_userInputCommand->_firstToken){clearCommand();backToPrompt();} // if we have a command get rid of it and ascertain to be at the prompt!!
	// the problem is that we do NOT want to actually change the new command, so we have to copy it somehow
	createUserInputCommandToEvaluate(); // NOTE might fail, in which case _userInputCommand->_lastToken will be NULL!!
	if(pNewCommand){ // something to copy
		// at least once we need to set _userInputCommand->_lastToken!!!
		Mtoken* pNewToken=pNewCommand; // first token to copy!!
		// NOTE theoretically _userInputCommand->_lastToken could be NULL due to _getToken() failing to create a new token
		while(_userInputCommand->_lastToken){
			// if failing to copy the text over get rid of the command constructed so far, and break
			if(!_stringCopy(pNewToken->text,_userInputCommand->_lastToken->text)){clearCommand();break;}
			// MDH@24APR2019 obsolete: getCommandLength()+=string_length(_userInputCommand->_lastToken->text);
			// some additional fields to copy over (NOT the offset is that is set automatically)
			_userInputCommand->_lastToken->type=pNewToken->type;
#ifdef __DEBUG__
            printf("%d:%s",_userInputCommand->_lastToken->type,string(_userInputCommand->_lastToken->text));
#endif
			pNewToken=pNewToken->next;
			if(!pNewToken)break;
			// we're going to need another token!!!
			_userInputCommand->_lastToken=_getToken(_userInputCommand->_lastToken);
		}
		// if the user decides to start typing ascertain to show it in the right color!!
		if(_userInputCommand->_lastToken)outputTokenColor(_userInputCommand->_lastToken);
#ifdef __DEBUG__
		echoCommand();
#endif
	}
}
*/
/* MDH@11AUG2019: no longer required now that all environments get a unique id in tokenization
// every non-function call token can be inside a call to a special function that can have local variables
Mtoken* getSpecialFunctionCallToken(const Mtoken* const token){
	Mtoken* container=(token?token->expr:NULL);
	// NOTE if the type is not a function call (like a list or map) or its argument is 0 or below it is not a special function
	while(container&&(container->type!=TT_FUNCTION_CALL||container->argument<=0))container=container->expr;
	return container;
}
*/
// an identifier with a certain name in a certain special function call (to which it might be local)
// instead of requiring a specialFunctionCallToken it suffices to know the environment id
bool existsInCommand(char* identifierName,uint64_t identifierEnvironmentId){ // replacing: const Mtoken* const specialFunctionCallToken){
	// every token contains a reference to its previous identifier (or name of the function being called), basically this means we can find all identifiers present in the current command
	// but we have to be careful because variables declared locally should be skipped unless they are in the same function call i.e. expr
	bool found=false;
	size_t l=strlen(identifierName);
	Mtoken* commandIdentifier=_userInputCommand->_lastToken->prevIdentifier;
	char *match,*commandIdentifierName;
	uint64_t commandIdentifierEnvironmentId,commandIdentifierEnvironmentLevels,ander=(1<<M_BITS_PER_ENV_LEVEL)-1;
	while(!found&&commandIdentifier){
		// if a function call or end of function call identifier, no need to check!!
		if(commandIdentifier->type!=TT_FUNCTION&&commandIdentifier->type!=TT_END_OF_FUNCTION_CALL){ // a (new) variable
			commandIdentifierName=string(commandIdentifier->text); // I have to do this to get the closing '\0' placed!!!
			if(strlen(commandIdentifierName)>=l){ // a match is only possible if identifierName is at least as long as 
				// TODO using strstr for now, but it would be better to find the position of the first non-matching character and if that is at least l we're good
				match=strstr(commandIdentifierName,identifierName);
				if(match==commandIdentifierName)if(commandIdentifierName[l]=='\0'||commandIdentifierName[l]==' '){ // the names match
					if(commandIdentifier->argument==1){ // the identifier is local to one of the special function calls (which is present in `do`, `for` and `function` function calls)
						// we can't tell for sure that this local identifier is in the same special function call unless `expr` field matches imagine the situation where multiple do's are in the same command following each other
						// the local variables in the first are not local to the second do call it's all about scope meaning we have to mark the end of a scope as well so we know which identifiers to skip i.e. those identifiers local to another special function call
						// so if we stored `( f g , h ) ( x, g` the second g is not in the first call and therefore does not exist in the command, so in going back you have to keep track of the level which should be the same as level of the caller
						// the special function call associated with the two identifiers must match!!
						// BUT a local variable of a special function call could be used in which the special function call of the identifier is nested within (like a do inside a do) in which case we should keep going up
						// so: identifier is local to its own special function call but the presented identifier might not i.e. it might be defined in a outer special function call
						if(identifierEnvironmentId){ // defined inside a subenvironment
							commandIdentifierEnvironmentId=commandIdentifier->envid;
							commandIdentifierEnvironmentLevels=(commandIdentifierEnvironmentId&15);
							// it's all about environmentid subclassing the environment id of identifier
							// i.e. environment id level should be at least the identifier's environment id
							if(commandIdentifierEnvironmentLevels<=(identifierEnvironmentId&15)){ // the registered identifier is defined at a level equal to or above that of the identifier
								commandIdentifierEnvironmentId>>=4;identifierEnvironmentId>>=4; // shift out the number of levels
								// all environment ids of the local identifier (commandIdentifier) should match those in identifierEnvironmentId
								while(commandIdentifierEnvironmentLevels>0&&((commandIdentifierEnvironmentId&ander)==(identifierEnvironmentId&ander))){
									commandIdentifierEnvironmentLevels--;
									commandIdentifierEnvironmentId>>=M_BITS_PER_ENV_LEVEL;
									identifierEnvironmentId>>=M_BITS_PER_ENV_LEVEL;
								}
								if(commandIdentifierEnvironmentId==0)found=true;
							}
						}
						/* replacing:
						if(specialFunctionCallToken){ // the given identifier exists inside a special function call therefore it might be the local identifier with the same name!!
							Mtoken *localIdentifierSpecialFunctionCallToken=getSpecialFunctionCallToken(identifier),*needleSpecialFunctionCallToken=specialFunctionCallToken; // which MUST exist i.e. will NOT be NULL
							while(needleSpecialFunctionCallToken&&needleSpecialFunctionCallToken!=localIdentifierSpecialFunctionCallToken)needleSpecialFunctionCallToken=getSpecialFunctionCallToken(needleSpecialFunctionCallToken);
							if(needleSpecialFunctionCallToken)found=true;
						}
						*/
					}else
						found=true;
				} // TODO will blank always be the only possible whitespace character????? 
			}
		}
		// get the next identifier
		commandIdentifier=commandIdentifier->prevIdentifier;
	}
	//////////if(found)inputInfo("%s",identifierName);else inputInfo("NOT %s",identifierName);
	return found;
}

// MDH@30APR2019: if the current token is a variable/function check whether it still is
//                call whenever the current token changes (in removePreviousTokenCharacter() and commandCharacterAccepted())
// MDH@01OCT2019: aSuggestedCharacter is actually not used anymore, so no need to pass it in anymore
bool tokenCheckedForBeingAFunction(Mtoken* lastCommandToken,bool endOfInput/*,bool aSuggestedCharacter*/){
	// only identifiers should be checked...
	if(lastCommandToken->type!=TT_VARIABLE&&lastCommandToken->type!=TT_NEW_VARIABLE&&lastCommandToken->type!=TT_FUNCTION)return false;
	// non-existing variables should be assigned to so it's a good idea to put the assignment operator behind it, although it might be hard to remove it though
	char* _identifierName=_stringstart(lastCommandToken->text,lastCommandToken->significantCharacterCount); // free asap
	if(lastCommandToken->type!=TT_FUNCTION){ // is it a function (now)?
		if(getFunction(getEnvironment(),_identifierName)){ // yes, it is
			// if a new variable before (now a function), remove the (assignment) character in the behind cursor text
			// MDH@20SEP2019: I suppose we need to ascertain that an opening parenthesis is associated with the token now, and no longer anything else
			/* MDH@20SEP2019: removing:
			if(amMatchingparentheses())if(_userInputCommand->_lastToken->type==TT_NEW_VARIABLE)if(string_char(feedforwardText,0)=='=')string_removed_char(feedforwardText,0);
			*/
			// the minimum we can do is put an opening parenthesis in the behind cursor text
			setTokenType(lastCommandToken,TT_FUNCTION/*,endOfInput*/);if(endOfInput)updateLastTokenAutocompletionText();
			reoutputToken(lastCommandToken);
			// insert an opening parenthesis for the function call
			// MDH@23SEP2019 take care of by setLastTokenType, so removed: if(endOfInput&&amMatchingparentheses())setLastTokenAutocompletionText("(");else deleteAutocompletionTextOfToken(_userInputCommand->_lastToken); // MDH@20SEP2019: either force the feedforward text to match an opening parenthesis or nothing TODO does endOfInput matter?????
			/* MDH@20SEP2019 replacing:
			if(endOfInput)if(amMatchingparentheses())if(string_char(feedforwardText,0)!='(')string_insert_char(feedforwardText,0,'(');
			*/
		}
	}else{ // is it (still) a function?
		if(!getFunction(getEnvironment(),_identifierName)){ // no, it ain't
			// the minimum we can do is remove the opening parenthesis behind it (if it is still there!!!!!)
			setTokenType(lastCommandToken,TT_VARIABLE/*,endOfInput*/);if(endOfInput)updateLastTokenAutocompletionText();
			reoutputToken(lastCommandToken);
			//////////outputInfo("Variable redrawn!");
			// remove any opening parenthesis from the behind cursor text
			// MDH@23SEP2019 take care of by setLastTokenType, so removed: deleteAutocompletionTextOfToken(_userInputCommand->_lastToken); // MDH@20SEP2019: I suppose when TT_VARIABLE changes to TT_NEW_VARIABLE later on, an equal sign might be added!!!
			/* MDH@20SEP2019 replacing:
			if(endOfInput)if(amMatchingparentheses())if(getNumberOfSuggestedCharacters())if(string_char(feedforwardText,0)=='(')string_removed_char(feedforwardText,0);
			*/
		}
	}
	// check whether the variable exists or not
	// MDH@07AUG2019: this variable could exist in this command, which we should check
	if(lastCommandToken->type==TT_VARIABLE||lastCommandToken->type==TT_NEW_VARIABLE){ // might not exist after all both in the command and in the current environment
		// MDH@08AUG2019 WARNING: all variables assigned to in the local variable declaration argument of the special functions should ALWAYS be considered new, but of course we cannot see that until they are assigned to
		//                        unless we do not require them to be assigned to (and we can just use them by name itself without assigning a value to them) in which case they are local but uninitialized...
		bool variableExists=(lastCommandToken->argument!=1&&(existsInCommand(_identifierName,lastCommandToken->envid/*replacing:getSpecialFunctionCallToken(_userInputCommand->_lastToken)*/)||containsVariable(getEnvironment(),_identifierName)));
		if(lastCommandToken->type==TT_VARIABLE){ // might not exist after all both in the command and in the current environment
			if(!variableExists){ // apparently does NOT exist
				setTokenType(lastCommandToken,TT_NEW_VARIABLE/*,endOfInput*/);if(endOfInput)updateLastTokenAutocompletionText();
				reoutputToken(lastCommandToken);
				// suggested characters should make = show (probably already present in the behind cursor text)
				// MDH@23SEP2019 take care of by setLastTokenType, so removed: setLastTokenAutocompletionText("="); // MDH@20SEP2019 replacing: if(endOfInput&&!aSuggestedCharacter)if(amMatchingparentheses())if(string_char(feedforwardText,0)!='=')string_insert_char(feedforwardText,0,'=');
			}
		}else{ // a new variable
			if(variableExists){ // now an existing variable
				setTokenType(lastCommandToken,TT_VARIABLE/*,endOfInput*/);if(endOfInput)updateLastTokenAutocompletionText();
				reoutputToken(lastCommandToken);
				///// MDH@23SEP2019 removed: deleteAutocompletionTextOfToken(_userInputCommand->_lastToken); // MDH@20SEP2019 replacing: if(endOfInput)if(amMatchingparentheses())if(string_length(feedforwardText)&&string_char(feedforwardText,0)=='=')string_removed_char(feedforwardText,0);
			}
		}
	}
	free(_identifierName); // freed
	return true;
}

// MDH@14AUG2019: cancelCommand() takes care of removing everything in the current command
void cancelCommand(){ // in response to Ctrl-C or backspace on the first character
	if(amVerbose())inputInfo("Cancelling the command.");
	// MDH@31OCT2019: with a command now possibly covering multiple lines we have to do a little more than we did before but we can put that in backToPrompt()
	backToPrompt();
	clearScreenFromCursor(); // inserting doing this otherwise (in the case of backspace) we would apparently still see the behind cursor text
	clearCommand();
	// by removing the behind cursor text, we ascertain that when the Enter key is pressed, we will switch to control mode, as otherwise we wouldn't, on the other hand, if _userInputCommand->_firstToken is NULL we should always switch to control mode (even if)
	if(_manualFeedforwardText){free_string(_manualFeedforwardText);_manualFeedforwardText=NULL;}
	deleteTokenautocompletiontexts(); // MDH@20SEP2019 replacing: string_setlength(feedforwardText,0); 
	// it's a good idea to inform the user that the command was cleared
	if(amVerbose())inputInfo("Command cleared!");
}
void updateOnTokenCharacterRemoved(char removedCharacter){
	// adapt screen
	clearScreenFromCursor(); // will clear what's behind the cursor
	// MDH@14AUG2019: shouldn't we ALWAYS write the behind cursor text, because I think we should, and if we want do not want to see it we should delete it beforehand!!!!! which is much preferred over not showing it when it is still there!!!!!
	// MDH@24APR2019 obsolete: getCommandLength()--; // decrement the total command length
	if(getUserInputLength()){ // still something left of the command (that we might check for being a function or not)
		// on screen as well please
		// before writing the behind cursor text we're going to check whether the current token still is a function or variable
		tokenCheckedForBeingAFunction(_userInputCommand->_lastToken,true/*,false*/); // MDH@14AUG2019: no, not a suggested character (as called on the backspace user action)
		// MDH@27FEB2019: if what's behind the cursor is NOT in the command but in feedforwardText that's what we should now write
		//// MDH@14AUG2019 moving to execute always: writeSuggestedText(true);
		// replacing: if(_userInputCommand->_firstToken)writeRestOfCommand(); // write all characters at and after the cursor (will reset the cursor!!)
	}/* removedTokenCharacter() calls removeToken which will NULL the _userInputCommand->_firstToken and _userInputCommand->_lastToken when the first command character is removed, in which case we do not need:
		else clearCommand();*/
	
	// MDH@20SEP2019: the following is about removing the feed forward characters that were added when a certain token started but as you can see 
	//                it is all about feed forward associated with the start of a token, so removing the associated feed forward can also be done at the moment the token is actually removed
	//                so for now we remove the following block and simply write the behind cursor text
	////////writeSuggestedText(true);
	/* replacing:
	// MDH@14AUG2019: how about removing any matching character?????
	if(string_length(getFeedforwardCharacters())){ // MDH@20SEP2019: will reconstruct feedforwardText if need be
		if(amMatchingparentheses()){
			// TODO are there any other characters that we might need to remove
			// NOTE assuming the character removed is ALWAYS present in INPUTCHARACTERTYPES so we can find its associated input type!!!!!
			//////////if(amVerbose()){inputInfo("Adapting the suggested text.");}
			char characterToRemove='\0';	
			switch(INPUTCHARACTERTYPES[removedCharacter]){
				case '[':characterToRemove=']';break;
				case '{':characterToRemove='}';break;
				case '(':characterToRemove=')';break;
				case '!':characterToRemove='=';break;
				case 'D':case 'S':characterToRemove=removedCharacter;break;
			}
			// if what can be removed matches the first character of the suggested text, remove it
			if(!characterToRemove&&string_char(feedforwardText,0)==characterToRemove){
				if(!string_removed_char(feedforwardText,0)){
					inputError("Failed to remove the first matching character of the suggested text");
				}
			}
		}
		if(string_length(feedforwardText)){
			//////////if(amVerbose())inputInfo("Writing suggested text.");
			writeSuggestedText(true);
		}
	}
	*/
}

// MDH@01OCT2019: updating the type of an identifier token due to the removal of the last token character is now done in removeTokenCharacter() just as commandCharacterAccepted() does!!!
//                also uint16_t behindCursor (as it is always called with constant value 1) removed as formal parameter, and endOfInput (typically true) added!!!
char removedTokenCharacter(bool endOfInput){
	// MDH@30OCT2019: ASSERT there's something to 'remove'
	// MDH@03SEP2019: 
	char tokenCharacterRemoved='\0';
	if(_userInputCommand){ // should ALWAYS be the case
		uint16_t tokenCharacterPosition;
		// find the token that we should remove a character from (either the current token or the one in front of it (if all tokens are non-empty!))
		while(_userInputCommand->_lastToken){
			tokenCharacterPosition=string_length(_userInputCommand->_lastToken->text); // MDH@24APR2019 replacing (what is essentially the same): getUserInputLength()-_userInputCommand->_lastToken->offset;
#ifdef __DEBUG__
			printf("%d",tokenCharacterPosition);
#endif
			if(tokenCharacterPosition>=1)break;
#ifdef __DEBUG__
			outputChar('.');
#endif		
			_userInputCommand->_lastToken=_userInputCommand->_lastToken->prev;
		}
		// MDH@30OCT2019: userInputCommandIdentifierContinuationNeedsUpdating=inIdentifierToken(_userInputCommand->_lastToken); // MDH@02OCT2019: should be called whenever _userInputCommand->_lastToken changes...
		if(_userInputCommand->_lastToken)tokenCharacterRemoved=string_removed_char(_userInputCommand->_lastToken->text,tokenCharacterPosition-1);
#ifdef __DEBUG__
			outputChar(tokenCharacterRemoved);
#endif
		if(tokenCharacterRemoved){
			if(endOfInput){
				// MDH@30OCT2019: with multiline user input it sometimes is a little harder than calling moveCursorLeft(1)
				//                if the offset of the current user input line is beyond the total command length apparently we've 'removed' the last command character on the previous line
				if(_userinputline&&_userinputline->offset>getUserInputLength()){
					toStartOfLine();clearScreenFromCursor(); // clear this user input line and what's beyond it
					removeUserinputline();
					oneLineUp();toUserInputCursorPosition(0); // one line up and to the proper position (not sure what will happen to the suggested text though)
				}else
					moveCursorLeft(1); // MDH@01OCT2019: this ought to be done BEFORE tokenCheckedForBeingAFunction() is called so we moved it over here!!!
			}
			// MDH@01OCT2019: whenever the last token does not change but the last token character is removed, we should check the type 
			//                HOWEVER we're assuming that we're dealing with an end of input situation
			bool tokenRemoved=(string_empty(_userInputCommand->_lastToken->text)?removeToken():false);
			unfinishToken(_userInputCommand->_lastToken); // we need to do this to allow appending characters to the token again
			if(endOfInput)if(!tokenRemoved)tokenCheckedForBeingAFunction(_userInputCommand->_lastToken,endOfInput);
		}
	}else
		inputInfo("BUG: No command to remove characters from!");
	return tokenCharacterRemoved;
}

// in response to backspace the previous token character is to be removed
void removePreviousTokenCharacter(){ // NOTE always due to a backspace!
	if(commandIndex){commandIndex=0;copyUserInputCommand();} // MDH@03SEP2019 BUG FIX: I have to do this if scrolling through the list of previous commands!!!
	char removedCharacter=removedTokenCharacter(true); // MDH@01OCT2019: will now also perform moveCursorLeft(1) when the argument is true and success
	if(removedCharacter){
		// MDH@01OCT2019: moveCursorLeft(1); // TODO check if this is necessary also when cancelling the command
		if(amVerbose()){inputInfo("Character '%c' removed.",removedCharacter);}
		// if no text is left in the command we cancel the command (as a service to the user who wouldn't understand that Enter wouldn't switch to Control mode on an otherwise empty command!!!)
		if(_userInputCommand){ // we still have a command being evaluated (NOTE that removedTokenCharacter() can actually set _userInputCommand->_firstToken to NULL)
			if(_userInputCommand->_lastToken==_userInputCommand->_firstToken&&string_length(_userInputCommand->_firstToken->text)==0){
				if(amDebugging()){inputInfo("%s","Cancelling the command.");}
				cancelCommand();
				if(amDebugging()){inputInfo("%s","Command cancelled.");}
				/* replacing:
				if(string_length(feedforwardText))inputInfo("Use Ctrl-C to clear the text suggestion as well.");else cancelCommand();
				*/
			}else{
				if(amDebugging()){inputInfo("%s","Updating.");}
				updateOnTokenCharacterRemoved(removedCharacter);
				if(amDebugging()){inputInfo("%s","Updated.");}
			}
		}else
		if(amVerbose())
			inputInfo("Command cleared.");
	}else // MDH@03MAY2019: can't switch to control mode here (so we just report the error!!!)
		inputError("%s","Failed to remove the last entered character.");
}

void outputCommandInfo(Mcommand* command){
	if(!command||!command->_lastToken)return;
	// MDH@12AUG2019: identifiers first
	Mtoken* identifierToken=command->_lastToken->prevIdentifier;
	if(identifierToken){
		output("%s","Identifiers:");
		while(1){
			//if(identifierToken==TT_VARIABLE||identifierToken==TT_NEW_VARIABLE){
				// all identier tokens with argument equal to 1 should be considered new, if not it is a bug
				if(identifierToken->argument==1&&identifierToken->type!=TT_NEW_VARIABLE)setColor(getErrorColor());else outputTokenColor(identifierToken);
				output(" %s",string(identifierToken->text));
				resetOutputColor();
				output("(%u)",identifierToken->offset);
			//}
			identifierToken=identifierToken->prevIdentifier;
			if(!identifierToken)break;
		}
		outputChar('\n');
	}
	// tokens
	Mtoken* token=command->_firstToken;
	uint16_t tokenIndex=0;
	output("%s:\n%s\t%s\t%s\t%s\t%s\t%s\t%s\t\t\t%s\n","Tokens","#","OFFSET","USED","LENGTH","ARG","ENV DEPTH/INDEX","TYPE","TEXT");
	while(token!=NULL){
		tokenIndex++;
		output("%u\t%u\t%u\t%u\t%" PRId32 "\t%x/%x\t\t%-24s`%s`",tokenIndex,token->offset,token->significantCharacterCount,string_length(token->text),token->argument,(token->envid&15),(token->envid>>4),TOKENTYPE_STRING[token->type],string(token->text));
		if(token->expr)
			output("\n%s\t%u\t%s\t%s\t%-24s\n"," part of",token->expr->offset,"","",TOKENTYPE_STRING[token->expr->type]);
		else
			output("\t%s\n","Not part of another expression!");
		if(token->prevIdentifier)
			output("%s\t%u\t%s\t%s\t%-24s\n"," points to",token->prevIdentifier->offset,"","",TOKENTYPE_STRING[token->prevIdentifier->type]);
		/* removing:
		if(token->type==TT_VARIABLE||token->type==TT_NEW_VARIABLE){
			Mtoken* specialFunctionCallToken=getSpecialFunctionCallToken(token);
			if(specialFunctionCallToken){
				output("%s\t%u\n"," local to",specialFunctionCallToken->offset);
			}
		}
		*/
		token=token->next;
	}
}

// MDH@09JUL2019: count the number of list elements in front of the current token
uint32_t getListElementCount(){
	uint32_t listElementCount=0;
	Mtoken* token=_userInputCommand->_lastToken;
	Mtoken* startToken=_userInputCommand->_lastToken->expr;
	while(token!=startToken){if(token->expr==startToken&&token->type==TT_LISTELEMENT)listElementCount++;token=token->prev;}
	return listElementCount;
}
// TODO we could call the following function from tokenCheckedForBeingAFunction
void changeFunctionTokenToAVariable(bool endOfInput){
	char* _identifierName=_stringstart(_userInputCommand->_lastToken->text,_userInputCommand->_lastToken->significantCharacterCount); // free asap
	// MDH@07AUG2019: here we also need to exclude explicit local variables (with argument equal to 1) as possibly existing i.e. those variables are always non-existing so they will get created in the function call execution environment!!!
	_userInputCommand->_lastToken->type=(_userInputCommand->_lastToken->argument!=1&&(existsInCommand(_identifierName,_userInputCommand->_lastToken->envid/* replacing:getSpecialFunctionCallToken(_userInputCommand->_lastToken)*/)||containsVariable(getEnvironment(),_identifierName))?TT_VARIABLE:TT_NEW_VARIABLE); // MDH@07AUG2019: the function might have been created (and used) in the current command
	free(_identifierName);
	reoutputToken(_userInputCommand->_lastToken);
	/* MDH@01OCT2019 because the token isn't actually removed the feed forward text associated with the token does not need to be deleted actually
	// MDH@20SEP2019: this function is called when a function name changes into a variable name (because the user did not enter ( behind a function name)
	//                and it makes sense to simply remove the associated feed forward of the token
	deleteAutocompletionTextOfToken(_userInputCommand->_lastToken);
	*/
	/* replacing:
	// I think we should remove ( from the behind cursor text if it was inserted
	if(endOfInput)if(amMatchingparentheses())
	if(string_length(feedforwardText)&&string_char(feedforwardText,0)=='(')
	if(!string_removed_char(feedforwardText,0))inputError("Failed to remove the function argument list opening parenthesis from the feed forward text."); // TODO is there a better way???
	*/
	// ready to redetermine the new token type!!!!
}

// MDH@28OCT2019: in order to implement the eval function the part in commandCharacterAccepted() that can work with any command is moved over to commandCharacterAppended()
//                and is called from commandCharacterAccepted() passing _userInputCommand->_lastToken in as first argument!!
//                NOTE that commandCharacterAccepted() keeps the part of the code that has to do with the endOfInput and aSuggestedCharacter flag
//                NOTE we have to use the pointer to the last command token because if we used the last command token itself, we wouldn't be able to change the last command token!!!!
//                NOTE instead we're returning the last command token (which will change if starting a new token!!!!)
Mtoken* commandCharacterAppended(Mcommand* command,char inputChar,char *inputCharacterType,bool endOfInput){
	// determine the token type associated with the newly inputted character
	// MDH@28MAR2019: if we're in a binary token type with the repeatable flag set AND the user has repeated the previous first token character the inputCharacterType should become R to get the right transition
	Mtoken* lastCommandToken=(command?command->_lastToken:NULL);
	// TODO shouldn't be outputting to the console if the command is not the user input command
	if(!lastCommandToken){inputError("BUG: No last command token.");return NULL;}
	if(amDebugging())inputInfo("Appending '%c'.",inputChar);
	/* MDH@31OCT2019: for now not allowing special TT_WHITESPACE tokens BUT returning to the original idea of appending whitespace to the current token
	// MDH@31OCT2019: by allowing dummy i.e. TT_WHITESPACE tokens in the command the type of the token to consider isn't that of lastCommandToken per se
	//                so it's actually best if we create a new token that points to the last non-whitespace command token
	//                and we should not allow an R input character type to continue an operator like that on the previous line, or checking whether someone entered a whitespace where it's not an whitespace ending a token
	Mtoken* lastNonwhitespaceCommandToken=lastCommandToken;while(lastNonwhitespaceCommandToken->type==TT_WHITESPACE)lastNonwhitespaceCommandToken=lastNonwhitespaceCommandToken->prev;
	if(lastNonwhitespaceCommandToken==lastCommandToken){ // not behind a whitespace (newline) token
	*/
		if((TOKENTYPE_IDS[lastCommandToken->type]&0x62)==0x62)if(inputChar==string_char(lastCommandToken->text,0))*inputCharacterType='r'; // MDH@04NOV2019: changed into lowercase r as we're now using R for token of type reference!!!
		// MDH@16APR2019: W indicates a whitespace character BUT it is NOT a functional whitespace character in a comment, an error, or a string literal
		// MDH@31OCT2019: until now only a blank was identified as a whitespace character, but now I've adapted the backtick as newline character which is also treated as whitespace
		//                there's no need to act differently here, we can simply check whether the last character in the returned token is a backtick
		if(*inputCharacterType=='W'){ // whitespace isn't always 'functional' whitespace (i.e. they can be part of the actual command)
			if(lastCommandToken->type==TT_ERROR||lastCommandToken->type==TT_COMMENT||lastCommandToken->type==TT_DQSTRING||lastCommandToken->type==TT_SQSTRING)*inputCharacterType='w';
		}else
		if(*inputCharacterType==' '){ // indicating a new line request (but not in a string)
			if(lastCommandToken->type==TT_DQSTRING||lastCommandToken->type==TT_SQSTRING)*inputCharacterType='w';
		}
	/*
	}
	*/
	int16_t newTokenType=0; // MDH@05JUN2019: we need newTokenType AFTER appending the last character allowed in a token (like q behind a integer or real)
	if(*inputCharacterType!='W'){ // only characters that are not whitespace can start a new token
		// MDH@31OCT2019: if we decide to always insert an empty TT_NEWLINE token on a backtick (`) newline character
		//                we have to be careful here though because if we're in a TT_WHITESPACE (dummy) token, we should look at the one before that (so essentially any TT_WHITESPACE should end immediately)
		if(*inputCharacterType=='`'){ // a (functional) new line request character
			// not acceptable when not end of input or behind another new line token
			if(!endOfInput||lastCommandToken->type==TT_WHITESPACE)return NULL;
			newTokenType=TT_WHITESPACE;
		}else // not the newline character (currently also `)
			newTokenType=nextTokenType(lastCommandToken->type,*inputCharacterType); // MDH@22MAR2019: this is a bit of a quick fix, so whitespace never ends up in nextTokenType() as whitespace never ends the current token, or changes its type
#ifdef __DEBUG__
	resetOutputColor();
	printf("[%d+%c->%d]",_userInputCommand->_lastToken->type,inputCharacterType,newTokenType);
	outputTokenColor(_userInputCommand->_lastToken);
#endif
		/////if(amDebugging())inputInfo("B");
		//MDH@17JUL2019: typically we'd get an error immediately when NOT entering a function call character ( behind a function identifier
		// MDH@02OCT2019: we need some additional corrections in certain situations i.e. do NOT end a single/double quoted string if ' or " was entered behind the escape character
		switch(newTokenType){
			case TT_ERROR:
				if(lastCommandToken->type==TT_FUNCTION){
					// we should assume that the identifier represents a (new) variable (identifier)
					changeFunctionTokenToAVariable(endOfInput);
					newTokenType=nextTokenType(lastCommandToken->type,*inputCharacterType);
				}
				break;
			case TT_END_OF_DQSTRING:
				if(string_last_char(lastCommandToken->text)=='\\')newTokenType=TT_DQSTRING;
				break;
			case TT_END_OF_SQSTRING:
				if(string_last_char(lastCommandToken->text)=='\\')newTokenType=TT_SQSTRING;
				break;
		}

		/////if(amDebugging())inputInfo("C");
		// TODO just like unary operators expressions, maps and list end immediately
		// some combinations are (still) not allowed...
		if(newTokenType<0||newTokenType==lastCommandToken->type){
			/* 
			   MDH@27MAY2019: most of the time we do allow the same one-character token behind another!!!
			   MDH@12JUL2019: BUT NOT ALWAYS (values and binary operator e.g.) I have to think this through again 
			   MDH@14AUG2019: start of list i.e. [ is allowed behind another [ always, also ( behind ( is also allowed, 
			*/
			if(newTokenType==lastCommandToken->type&&lastCommandToken->significantCharacterCount>0){
				// MDH@16APR2019: most tokens cannot follow each other directly except for unary and TODO ternary operators and list element tokens (although undefined list element cells do not need to be inserted!!)
				// MDH@23JUL2019: and TT_END_OF_FUNCTION_CALL and all the other end of something tokens!!
				if(lastCommandToken->type!=TT_LIST&&lastCommandToken->type!=TT_FUNCTION_CALL&&lastCommandToken->type!=TT_UNARY&&lastCommandToken->type!=TT_TERNARY_aeru&&lastCommandToken->type!=TT_LISTELEMENT&&lastCommandToken->type!=TT_END_OF_FUNCTION_CALL&&lastCommandToken->type!=TT_END_OF_MAP&&lastCommandToken->type!=TT_END_OF_LIST){
					newTokenType=TT_ERROR;
					if(amVerbose())inputError("Token already finished!");
				}
			}
		}else{ // different token types
			// a shortcut assignment can NOT be turned into a equality comparison
			if(*inputCharacterType=='='&&lastCommandToken->type==TT_ASSIGNMENT&&(lastCommandToken->prev->type==TT_BINARY_AeRu||lastCommandToken->prev->type==TT_BINARY_Aeru)){
				newTokenType=TT_ERROR;
				if(amVerbose())inputError("A shortcut operator assignment cannot change into an equality.");
			}
		}
		/////if(amDebugging())inputInfo("D");
		// MDH@03MAY2019: no matter what the new token type is, any token of type TT_EXPRESSION always ends immediately...
		//                this is because the first (offset) token in a command is always of type TT_EXPRESSION which should end immediately on any next token although significantCharacterCount will still be zero
		//                this way it will always be there!!
		if(newTokenType<0){

		}else
		if(newTokenType!=lastCommandToken->type||lastCommandToken->type==TT_EXPRESSION||lastCommandToken->significantCharacterCount>0){
			///////////if(amVerbose())outputLine("!");/////inputInfo("New token!");
			// MDH@10APR2019: NOT every new token type starts a new token:
			//                if we're in a binary operator and move to another binary operator type it's an extension
			//                NO we decide NOT to do this when the command is evaluated we should compose the values and apply the operators
			///////if(!isBinaryOperatorTokenType(_userInputCommand->_lastToken->type)||!isBinaryOperatorTokenType(newTokenType))

			// MDH@16APR2019: a character that is assumed to indicate the assignment operator has to be checked because it could well be the = that starts the binary equality operator
			//                which means we have to switch from assignment token to BearU token (which is unfinished)
			if(newTokenType==TT_ASSIGNMENT){
				// checking for validity of accepting as assignment is not that easy
				// we can allow a binary operator in front of the assignment of course in that case it definitely is an assignment if it is not the = is an error!!
				bool behindBinaryOperator=(lastCommandToken->type==TT_BINARY_AeRu||lastCommandToken->type==TT_BINARY_Aeru);
				// NOTE if behind binary operator there must always be a token in front of it, so lastTokenToCheck cannot be NULL!!
				// MDH@21MAY2019: possibly we have multiple tokens representing a binary operator (like ** << and >> which are allowed!!!) so we need to skip all binary operators in front of the assignment character
				Mtoken* lastTokenToCheck=lastCommandToken;
				if(behindBinaryOperator)while(lastTokenToCheck->type>=3&&lastTokenToCheck->type<=7)lastTokenToCheck=lastTokenToCheck->prev;
				if(amVerbose())inputInfo("Type of token to check: %s.",TOKENTYPE_STRING[lastTokenToCheck->type]);
				// ASSERT lastTokenToCheck should either represent a variable or the end of a list element to allow for operator
				if(lastTokenToCheck->type==TT_END_OF_LIST){ // end of a list
					// we have to find the associated start of the list, and the token in front of that (which should be a variable!!!)
					// which is easy because the expr tells us the start of the list BUT 
					lastTokenToCheck=lastTokenToCheck->expr;
					///////////if(amVerbose())inputInfo("Presumed list start token");
					if(lastTokenToCheck)lastTokenToCheck=lastTokenToCheck->prev;else inputError("%s","Start of index list not found!");
				}
				// two options: = behind a binary operator without variable (or list) in front of it is not allowed, i.e. an error, otherwise we assume that = represents the first = of == the equality operator...
				if(lastTokenToCheck==NULL||(lastTokenToCheck->type!=TT_VARIABLE&&lastTokenToCheck->type!=TT_NEW_VARIABLE)){
					if(behindBinaryOperator){
						newTokenType=TT_ERROR;
						//if(amVerbose())inputError("No variable to assign to.");
					}else
						newTokenType=TT_BINARY_aErU;
				}
			}
			/////if(amDebugging())inputInfo("E");
			// MDH@23JUL2019: _getToken() will now also use newTokenType to set the (initial) type of the new token
			// MDH@23SEP2019: replacing _getToken() call by createUserInputCommandToken (and generating an error when this goes wrong somehow)
			lastCommandToken=_getNewCommandToken(lastCommandToken,newTokenType/*,endOfInput*/);
			if(endOfInput)updateLastTokenAutocompletionText();
			if(!lastCommandToken)return NULL;
			/* replacing:
			_userInputCommand->_lastToken=_getToken(_userInputCommand->_lastToken,newTokenType);
			// MDH@23SEP2019: moved out of _getToken (because not always will we need to update the feed forward text when new tokens are created, e.g. in copyUserInputCommand()!)
			setLastTokenType(newTokenType,endOfInput);
			*/
			/////if(amDebugging())inputInfo("F");
/*
#ifdef __DEBUG__
			printf("@%p=%p?:%s",_userInputCommand->_firstToken,_userInputCommand->_lastToken,string(_userInputCommand->_firstToken->text));
#endif
*/
			/* MDH@23JUL2019 TODO check what we still need of the following!!!!: replacing:
			// ending a function call, list or map is only allowed with expr defined
			if(_userInputCommand->_lastToken->type==TT_END_OF_FUNCTION_CALL||_userInputCommand->_lastToken->type==TT_LISTELEMENT||_userInputCommand->_lastToken->type==TT_END_OF_LIST||_userInputCommand->_lastToken->type==TT_END_OF_MAP){
				if(_userInputCommand->_lastToken->expr){ // i.e. pointing to some token that should be of the right type!!!
					// check whether the match is correct
					switch(newTokenType){
						case TT_END_OF_FUNCTION_CALL:
							if(_userInputCommand->_lastToken->expr->type!=TT_FUNCTION_CALL&&_userInputCommand->_lastToken->expr->type!=TT_EXPRESSION){
								/////inputError("%s","No function call or expression to end here!");
								inputError("End of function call/expression character does not match '%s' of type '%s'!",string(_userInputCommand->_lastToken->expr->text),TOKENTYPE_STRING[_userInputCommand->_lastToken->expr->type]);
								_userInputCommand->_lastToken->type=TT_ERROR;
							}
							break;
						case TT_LISTELEMENT:
							// MDH@09JUL2019: a comma (starting a list element) is allowed in a function call accepting multiple parameters)
							if(_userInputCommand->_lastToken->expr->type!=TT_LIST&&_userInputCommand->_lastToken->expr->type!=TT_MAP){
								// so if it's a function call it might be allowed
								if(_userInputCommand->_lastToken->expr->type!=TT_FUNCTION_CALL){
									inputError("First expression token '%s' of type '%s' does not start a list or map!",string(_userInputCommand->_lastToken->expr->text),TOKENTYPE_STRING[_userInputCommand->_lastToken->expr->type]);
									_userInputCommand->_lastToken->type=TT_ERROR;
								}else{
									// the token in front of the function call token should denote a function
									char* functionName=string(_userInputCommand->_lastToken->expr->prev->text);
									Mfunction* function=getFunction(getEnvironment(),functionName);
									/////////////if(amVerbose())output("Function '%s'.\n",functionName);
									// TODO this works for two-argument functions but can we tell which argument this is?????
									// we have to count the parameters by counting the list elements
									uint32_t listElementCount=getListElementCount()+1;
									if(!function||listElementCount>=function->_parameterMap->numberOfElements){
										if(function)
											inputError("Function '%s' does not allow for more than %u argument(s).",functionName,listElementCount);
										else
											inputError("Cannot tell whether function '%s' allows for more than %u argument(s).",functionName,listElementCount);
										_userInputCommand->_lastToken->type=TT_ERROR;
									}
								}
							}
							break;
						case TT_END_OF_LIST:
							if(_userInputCommand->_lastToken->expr->type!=TT_LIST){
								inputError("First token '%s' of type '%s' does not start a list!",string(_userInputCommand->_lastToken->expr->text),TOKENTYPE_STRING[_userInputCommand->_lastToken->expr->type]);
								_userInputCommand->_lastToken->type=TT_ERROR;
							}
							break;
						case TT_END_OF_MAP:
							if(_userInputCommand->_lastToken->expr->type!=TT_MAP){
								inputError("First expression token '%s' of type '%s' does not start a map!",string(_userInputCommand->_lastToken->expr->text),TOKENTYPE_STRING[_userInputCommand->_lastToken->expr->type]);
								//inputError("No map to end here!");
								_userInputCommand->_lastToken->type=TT_ERROR;
							}
							break;
					}
					// if we do the following we need to move 
					// TODO we can do the following even on error but what if we didn't???
					///////////////////_userInputCommand->_lastToken->expr=_userInputCommand->_lastToken->expr->expr;
				}else{
					inputError("%s","Can't end a (function argument) list or map here!");
					_userInputCommand->_lastToken->type=TT_ERROR;
				}
			}
			// taken over in _getToken()
			_userInputCommand->_lastToken->type=newTokenType;
			*/

			// MDH@27MAY2019: a lot of tokens are one-character tokens
	
			// MDH@15APR2019: there are some other characters as well, that immediately end the token like parentheses, comma's and semicolons and ? and : TODO are there more??????
			if(lastCommandToken->significantCharacterCount==0){
				if(isOneCharacterTokenType(lastCommandToken->type)){
					lastCommandToken->significantCharacterCount=1;
					////////bool initializationsChanged=false;
					// MDH@06AUG2019: these are also the tokens we need to recognize for keeping track of the initialized variables (and the level)
					switch(lastCommandToken->type){
						case TT_ASSIGNMENT:
							if(lastCommandToken->argument==1)lastCommandToken->argument=-1; // indicating that whatever comes next, should not be considered local variables, i.e. should NOT be marked 'automatically' as new variables because they should exist!!!
							/* replacing:
							if(_userInputCommand->_lastToken->prev->type==TT_NEW_VARIABLE)if(initializable()){
								char* newVariableName=string(_userInputCommand->_lastToken->prev->text);
								if(pushInitialization(newVariableName)){
									initializationsChanged=true;
									if(amVerbose())inputInfo("New variable '%s' initialization registered.",newVariableName);
								}else
									inputError("Failed to register the initialization of new variable '%s'.",newVariableName);
							}
							*/
							break;
						case TT_FUNCTION_CALL:
							if(lastCommandToken->prev->type==TT_FUNCTION){
								/* replacing:
								char* functionName=string(_userInputCommand->_lastToken->prev->text);
								// we do not need to store the function name itself, just the argument that will contain the local variable initializations
								if(pushInitialization("(")){
									_lastInitialization->argument=((!strcmp(functionName,DOFUNCTION_NAME)||!strcmp(functionName,FORFUNCTION_NAME)?0:(!strcmp(functionName,DEFINEUSERFUNCTION_NAME)?1:-1)));
									initializationsChanged=true;
									if(amVerbose())inputInfo("Function '%s' registered.",functionName);
								}else
									inputError("Failed to register function call '%s'.",functionName);
								*/
							}else
								inputError("No function in front of function call.");
							break;
						case TT_END_OF_FUNCTION_CALL:
							if(lastCommandToken->argument==-1)lastCommandToken->argument=1;
							/* replacing:
							if(pushInitialization(")")){ // will set the argument count appropriately...
								if(amVerbose())inputInfo("End of function call registered.");
								initializationsChanged=true;
							}else
								inputError("Failed to register the end of a function call.");
							*/
							break;
						case TT_LISTELEMENT:
							if(lastCommandToken->expr&&lastCommandToken->expr->type==TT_FUNCTION_CALL){ // TODO is this correct?
								if(lastCommandToken->argument==-1)lastCommandToken->argument=1;
								/* replacing:
								// not any comma is a function call argument separator!!!
								if(pushInitialization(",")){
									_lastInitialization->argument--; // decrement the argument count (once it is zero any initialization is local to the function call)
									if(amVerbose())inputInfo("End of function argument with count set to %lld.",_lastInitialization->argument);
									initializationsChanged=true;
								}else
									inputError("Failed to register a next function call argument!");
								*/
							}
						default:
							break;
					}
					/* removing:
					if(amDebugging()){
						if(!initializationsChanged)
							inputInfo("Token with text '%c' of type %s considered to be a one character token.",inputChar,TOKENTYPE_STRING[_userInputCommand->_lastToken->type]);
						else 
						if(!amVerbose())showInitializations();
					}
					*/
				}
				/* replacing:
				if(_userInputCommand->_lastToken->type!=TT_ERROR&&_userInputCommand->_lastToken->type!=TT_COMMENT&&_userInputCommand->_lastToken->type!=TT_DQSTRING&&_userInputCommand->_lastToken->type!=TT_SQSTRING)
					if(inputCharacterType=='('||inputCharacterType=='['||inputCharacterType=='{'||inputCharacterType==','||inputCharacterType==';'||inputCharacterType==':'||inputCharacterType=='?')
						_userInputCommand->_lastToken->significantCharacterCount=1;
				*/
			}
			// TODO should we write the associated colors here?????
			/////// moved over to commandCharacterAccepted because it's definitely not part of an inline command (evaluated by Meval!!!) outputTokenColor(lastCommandToken);
			/////if(amDebugging())inputInfo("H");
		}
	}else{ // a functional whitespace character, ends a current token!!
		if(lastCommandToken->significantCharacterCount==0&&lastCommandToken->type!=TT_EXPRESSION) // MDH@22MAR2019: first whitespace character in a non-whitespace token ends the current token (but should never change its type (see NO_TRANSITIONS))
			lastCommandToken->significantCharacterCount=string_length(lastCommandToken->text);
		if(inputChar==' ')inputChar=M_WHITESPACE_CHARACTER; // MDH@31OCT2019: so we can make the blanks visible!!
	}
	/////if(amDebugging())inputInfo("I");
	// append the typed character at getUserInputLength() minus current token offset in _userInputCommand->_lastToken->text
	string_append_char(lastCommandToken->text,inputChar);
	/////if(amDebugging())inputInfo("J");

	if(newTokenType<0)lastCommandToken->significantCharacterCount=string_length(lastCommandToken->text);

	return lastCommandToken;

}

// this is a fun method, allowing us to parse and evaluate any command (which we're gonna need when running M starting with commands to execute from a file)
Mvalue* Mevalfunction(Mvalue* value){
	Mvalue* _evalValue=NULL;
	Mstring* _evalValueText=_getValueText(value,true);
	if(_evalValueText){
		if(amVerbose())output("To evaluate: '%s'.\n",string(_evalValueText));
		Mcommand* _evalCommand=_getNewCommand(true);
		if(_evalCommand){
			Mtoken* _evalCommandToken=_evalCommand->_firstToken;
			/* already set: 
			_evalCommandToken->expr=NULL; // MDH@28OCT2019: essential bto'
			Mtoken* _lastEvalCommandToken=_evalCommandToken;
			*/
			uint32_t pos=0;
			char evalInputChar,evalInputCharType;
			if(amVerbose())output("Parsing '");
			Mtoken* newLastEvalCommandToken=NULL;
			while(pos<string_length(_evalValueText)){
				evalInputChar=string_char(_evalValueText,pos++);
				if(amVerbose())outputChar(evalInputChar);
				evalInputCharType=INPUTCHARACTERTYPES[evalInputChar];
				newLastEvalCommandToken=commandCharacterAppended(_evalCommand,evalInputChar,&evalInputCharType,false); // MDH@29OCT2019: we have to pass false all the time TODO not this way please
				if(newLastEvalCommandToken!=_evalCommand->_lastToken)_evalCommand->_lastToken=newLastEvalCommandToken; // update our eval command's last token TODO do we need to test here????
				if(!_evalCommand->_lastToken)break;
			}
			if(amVerbose())outputLine("'.");
			if(_evalCommand->_lastToken){
				// just like with do() we have to evaluate the command in a subenvironment
				Menvironment* _evalEnvironment=__environment();
				if(_evalEnvironment){
					_evalEnvironment->_name=_strdup("eval");
					if(pushExecutionEnvironment(_evalEnvironment)){
						_evalValue=getCommandValue(_evalCommand,'e');
						popExecutionEnvironment(); // pop the eval environment we successfully pushed
					}else
						output("%sUnable to setup the evaluation of '%s'.\n",ERROR_PREFIX,string(_evalValueText));
				}else
					output("%sUnable to evaluate '%s'.\n",ERROR_PREFIX,string(_evalValueText));
			}else
				output("%sUnable to evaluate the invalid command '%s'.\n",ERROR_PREFIX,string(_evalValueText));
			free_token(_evalCommandToken); // clean up the command
		}
		free_string(_evalValueText);
	}
	return _evalValue;
}

// MDH@12APR2019: in order to implement the Tab character we have to delegate entering a character (typed) to a separate function
//       		  ASSERTION _userInputCommand->_firstToken and _userInputCommand->_lastToken are  NOT  NULL
//                the endOfInput flag is used to indicate whether this is the end of the input
//                the aSuggestedCharacter flag tells commandCharacterAccepted() that the input character came from feedforwardText (the feed forward), so it will in that case not alter feedforwardText (by removing the same character that was entered)
bool commandCharacterAccepted(char inputChar,char *inputCharacterType,bool endOfInput,bool aSuggestedCharacter){
	bool initializationsChanged=false;
	// MDH@21APR2019: there are two situation where we need to get a command
	/////outputChar('1');
	//                1. we haven't got one 2. we have got a registered command which hasn't changed yet (in which case commandIndex will still be positive)
	if(!_userInputCommand) // no current command
		createUserInputCommand(); // we need to make a new token (to start the command to evaluate)
	else // we have a current command BUT 
	if(commandIndex)
		copyUserInputCommand();
	/////outputChar('2');
	// if _userInputCommand->_lastToken is now NULL something went wrong (in copyUserInputCommand or createUserInputCommand most likely)
	if(!_userInputCommand){inputError("BUG: No user input command.");return false;}
	commandIndex=0; // to indicate we are now working with a NEW command (even if we fail to accept the character!!!)
	/////outputChar('3');
	clearInfo();
	/////outputChar('4');
	/////////if(amDebugging())inputInfo("A");
	/* MDH@28MAR2019: if the user enters the comment character we should toggle the token type's highest bit (bit 7)
	if(inputCharType=='C'){
		_userInputCommand->_lastToken->type^=0x70; // toggling bit 7
		// a comment character will NEVER change the (actual) token type but it should change the color to use
		if(_userInputCommand->_lastToken->type&0x70){commenting=true;outputTokenColor(_userInputCommand->_lastToken);}else notCommenting=true; // if a comment was started, switch to the comment token color
	}else // not a comment character
	if((_userInputCommand->_lastToken->type&0x70)==0){ // not in a comment
		if(notCommenting){notCommenting=false;outputTokenColor(_userInputCommand->_lastToken);} // if behind coming out of a comment, we have to reset the output token color
	*/
	/*
	// MDH@26FEB2019: when a user starts inserting characters instead of appending them we can cut off the rest of the characters in the command
	//                and put it in a single Mstring instance and append these one at a time 
	char* removed=removedRestOfCommand();
	*/

	// MDH@28OCT2019: all the code that deals with updating the tokens 
	//                NOTE passing in the address of _userInputCommand->_lastToken, so it can be changed!!!!
	Mtoken* newLastCommandToEvaluateToken=commandCharacterAppended(_userInputCommand,inputChar,inputCharacterType,endOfInput);
	if(!newLastCommandToEvaluateToken)return false;
	if(newLastCommandToEvaluateToken!=_userInputCommand->_lastToken){
		_userInputCommand->_lastToken=newLastCommandToEvaluateToken;
		outputUserInputCommandTokenColor();
	}else{
		// MDH@31OCT2019: show whitespace in the standard info color!!
		if(_userInputCommand->_lastToken->significantCharacterCount>0){
			resetOutputColor();
			if(*inputCharacterType=='W'&&inputChar==M_NEWLINE_CHARACTER)*inputCharacterType=' '; // convert the newlinecharacter (which type should be W to the blank)
		}
		/*
		if(string_last_char(_userInputCommand->_lastToken->text)=='`'&&_userInputCommand->_lastToken->type!=TT_DQSTRING&&_userInputCommand->_lastToken->type!=TT_SQSTRING){
			resetOutputColor();
		}
		*/
	}

	/////if(amDebugging())inputInfo("J");
#ifdef __DEBUG__
	printf("[%s]",string(_userInputCommand->_lastToken->text));
#endif
	// MDH@24APR2019 obsolete: getCommandLength()++; // increment total command length
	outputChar(inputChar); ///////// replacing: outputLastTokenChar(_userInputCommand->_lastToken); // echo the last token character
	
	if(endOfInput){
		//////if(amAssisting())output(":%c",*inputCharacterType);
		debugWrite("Command length after inserting %c: %zu.",inputChar,getCommandLength());
	}

	// MDH@24APR2019 obsolete: getUserInputLength()++; // increment the current cursor position
	// MDH@07AUG2019: after a character is input by the user (or some other source) the identifier type will be checked...
	//                BUT 
	// MDH@01OCT2019: argument aSuggestedCharacter is no longer used in tokenCheckedForBeingAFunction and consequently by this function, so it is removed as argument and replaced by updateidentifiercontinuation (which we do need)
	bool notCheckedForBeingAFunction=!tokenCheckedForBeingAFunction(_userInputCommand->_lastToken,endOfInput/*,aSuggestedCharacter*/); // MDH@28MAY2019: ALWAYS check for being a function!!!!
	/////if(amDebugging())inputInfo("K");
	if(endOfInput){
		/* MDH@20SEP2019: because I created updateLastTokenAutocompletionText which should take care of adding the right token feed forward I do not need to do the following
		// MDH@29APR2019: I'd like to detect when a variable becomes a function or vice versa
		if(notCheckedForBeingAFunction){
			/////if(amDebugging())inputInfo("L");
			// MDH@16APR2019: we can check for an unfinished binary operator in which case we should show = behind 
			// MDH@15APR2019: it seems like a good idea to adapt the behind cursor text if we entered the start character of a list (element), map or expression opening parenthesis
			if(_userInputCommand->_lastToken->type!=TT_ERROR){ // MDH@29APR2019: don't add closing bracket to autocompletion text when in error!!!
				// MDH@20SEP2019: typically the token type should determine what the feed forward text of the token should be
				//                so it is less optimal to explicitly check for certain input characters instead of simply looking at the type of token that started!!!
				//                even so, knowing that each of the input characters that insert a feed forward character actually started a new token and there's at that point no token feed forward yet
				//                it's easiest to simply set the feed forward text of the last token
				if(!aSuggestedCharacter){ // MDH@14AUG2019: using this flag here means no changes to the suggested text are done, when this character was consumed from the suggested text
					if(_userInputCommand->_lastToken->type!=TT_BINARY_aErU){
						// MDH@14AUG2019: whether or not to insert a feed-forward matching parenthesis is open for debate
						//                this is a bit of an issue because string literal start and ends is the same, and how do we know if a string is started or ended????? solution: test the type
						if(amMatchingparentheses()){
							// MDH@14AUG2019: I think we should always insert the character we need to close the bracket no matter what
							//                NOTE we're using inputCharType here, not inputChar but as you may notice in removeCharacter there it's not using the input character type, I suppose we should
							//                with strings is important only to insert the same character when the inserted character started the string
							// removing: if(!string_length(feedforwardText)) // MDH@28MAY2019: if there's nothing behind the cursor yes we do append closing stuff
							switch(inputCharacterType){
								case '[':
									setLastTokenAutocompletionText("]"); // MDH@20SEP2019 replacing: string_insert_char(feedforwardText,0,']');
									break;
								case '{':
									setLastTokenAutocompletionText("}"); // MDH@20SEP2019 replacing: string_insert_char(feedforwardText,0,'}');
									break;
								case '(':
									setLastTokenAutocompletionText(")"); // MDH@20SEP2019 replacing: string_insert_char(feedforwardText,0,')');
									break;
								case 'D':
									if(_userInputCommand->_lastToken->type==TT_DQSTRING&&string_length(_userInputCommand->_lastToken->text)==1)
									setLastTokenAutocompletionText(string(_userInputCommand->_lastToken->text)); // MDH@20SEP2019 replacing: string_insert_char(feedforwardText,0,inputChar);
									break;
								case 'S':
									if(_userInputCommand->_lastToken->type==TT_SQSTRING&&string_length(_userInputCommand->_lastToken->text)==1)
									setLastTokenAutocompletionText(string(_userInputCommand->_lastToken->text)); // MDH@20SEP2019 replacing: string_insert_char(feedforwardText,0,inputChar);
									break;
								// MDH@14AUG2019: if the user typed the same character as is currently behind the cursor let's remove that character
								//                but only when the character entered is NOT consumed because in that case it was already removed (and shouldn't be removed again if there's another such a character which can happen a lot with matching parentheses)
								default:
									// MDH@20SEP2019: it is well possible that the user will type the first character of the feed forward associated with the token in which case I suppose that character should be removed from the feed forward text
									removeFirstFeedforwardCharacterFromLastTokenWhenMatching(inputChar);
									// replacing: if(string_length(feedforwardText)&&string_char(feedforwardText,0)==inputChar)if(!string_removed_char(feedforwardText,0))inputError("Failed to remove the matching first feed forward character");
									break;
							}
						}
					}else
						setLastTokenAutocompletionText("="); // MDH@20SEP2019 replacing: string_insert_char(feedforwardText,0,'=');
				}
			}
			/////if(amDebugging())inputInfo("M");
		}
		*/
		bool acceptedFirstSuggestedCharacterDeleted=(!aSuggestedCharacter||(_identifierContinuationCharacters?deleteFirstIdentifierContinuationCharacter():deleteFirstAutocompletionCharacter(inputChar,true)));
		if(!acceptedFirstSuggestedCharacterDeleted)inputError("Failed to remove the accepted first suggested character."); // i.e. we're NOT switching to control mode or returning false
		updateLastTokenAutocompletionText(/*acceptedFirstSuggestedCharacterDeleted*/); // it makes sense to update the current token feed forward text just before actually showing it AND to update the identifier continuation first
		/////////writeSuggestedText(false); // just in case we removed some character (see TT_FUNCTION->TT_VARIABLE)
		/////if(amDebugging())inputInfo("N");
		debugWrite("Command length after writing behind cursor text: %zu.",getCommandLength());
		//////////if(!initializationsChanged)outputStatus(inputChar,*inputCharacterType);
		/////if(amDebugging())inputInfo("O");
	}
	/////if(amDebugging())inputInfo("P");
	return true;
}

bool COMMAND_PROCESSOR_AVAILABLE=0;
void clearShellCommand(){
	string_setlength(_suggestedText,0);
	string_setlength(shellCommand,0);
	// MDH@24APR2019 obsolete: getUserInputLength()=0;
}
void executeShellCommand(){
	// ASSERTION string_length(shellCommand) should be positive
	output(""); // get a new line before we see the result of executing this command!!
	int result=system(string(shellCommand));
	if(result)output("Result: %d.\n",result); // non-zero result
	clearShellCommand(); // ready for the next execution
}
char switchToShellMode(char* message){
	clearCommand();
	resetOutputColor();
	if(message!=NULL)output("%s",message);
	setInputMode(IM_SHELL);
	clearShellCommand();
	return 's';
	//output("%s\n $ ","Enter your shell command, and press the Return button to execute.");
}
void switchToCommandMode(){
	if(inputMode==IM_COMMAND)return;
	setInputMode(IM_COMMAND);
	// ascertain to not have autocompletion text
	deleteTokenautocompletiontexts(); // MDH@20SEP2019 replacing: string_setlength(feedforwardText,0); 
}

char getFirstSuggestedCharacter(bool autogenerated){return(_identifierContinuationCharacters&&strlen(_identifierContinuationCharacters)>0?getFirstIdentifierContinuationCharacter():getFirstAutocompletionCharacter(autogenerated));}

// MDH@01OCT2019: I created consumeFirstSuggestedCharacter() today to consume the first suggested character but initially called commandCharacterAccepted with endOfInput flag equal to false
//                I suppose that was wrong because there's exactly one character being accepted and that's also the end of input character 
//                the result will be that updateLastTokenAutocompletionText(true) and writeSuggestedText(true) are executed already by commandCharacterAccepted() so I won't have to do that here anymore
char getFirstSuggestedCharacterConsumed(char firstSuggestedCharacter,bool endOfInput){
	// if the first suggested character is provided use that, otherwise get any
	if(!firstSuggestedCharacter)firstSuggestedCharacter=getFirstSuggestedCharacter(false); // any first suggested character will do
	if(firstSuggestedCharacter){
		char firstSuggestedCharacterInputType=INPUTCHARACTERTYPES[firstSuggestedCharacter];
		if(!commandCharacterAccepted(firstSuggestedCharacter,&firstSuggestedCharacterInputType,endOfInput,true))firstSuggestedCharacter='\0';
	}
	return firstSuggestedCharacter;
}

void removeFirstSuggestedCharacter(char firstSuggestedCharacter,bool consumed){
	if(!_identifierContinuationCharacters||strlen(_identifierContinuationCharacters)==0){ // a feed forward text character was consumed
		deleteFirstAutocompletionCharacter(firstSuggestedCharacter,consumed);
	}
}

int main(int argc, char **argv){

	COMMAND_PROCESSOR_AVAILABLE=system(NULL); // check if there's a command processor available

#ifdef __DEBUG__
	printf("\n%s","Operator token types:");
	printf("\nUnary operator                            : %d.",TT_UNARY);
	printf("\nAssignment operator                       : %d.",TT_ASSIGNMENT);
	printf("\nBinary operator                           : %d.",TT_BINARY_aeru);
	printf("\nAssignable repeatable binary operator     : %d.",TT_BINARY_AeRu);
	printf("\nEqualizable repeatable binary operator    : %d.",TT_BINARY_aERu);
	printf("\nTEqualizable unfinished binary operator   : %d.",TT_BINARY_aErU);
	printf("\nAssignable binary operator                : %d.",TT_BINARY_Aeru);
	printf("\nTernary operator                          : %d.",TT_TERNARY_aeru);
	printf("\n%s","Unfinishable token types:");
	printf("\nComment (allowed when command is complete): %d.",TT_COMMENT);
	printf("\nError                                     : %d.",TT_ERROR);

#endif

	// MDH@23FEB2019: how about being able to continue with commands stored in a file, or perhaps allow for -log <logfile> or log=
	// whereas any filename without prefix is the file to execute at the start
	if(argc>1){
		printf("%s\n","Arguments");
		for(int arg=1;arg<argc;arg++){
			printf("%i. %s\n",arg,argv[arg]);
			if(argv[arg][0]=='-'){ // a flag (or flags)
				int i=0;
				while(argv[arg][++i]){
					if(argv[arg][i]=='v')setVerbose(false);else
					if(argv[arg][i]=='V')setVerbose(true);else
					if(argv[arg][i]=='d')setDebugging(false);else
					if(argv[arg][i]=='D')setDebugging(true);else
					if(argv[arg][i]=='a')setAssisting(false);else
					if(argv[arg][i]=='A')setAssisting(true);else
					if(argv[arg][i]=='m')setMatchingparentheses(false);else
					if(argv[arg][i]=='M')setMatchingparentheses(true);else
					if(argv[arg][i]=='C')setColorscheme(0);else // light color scheme
					if(argv[arg][i]=='c')setColorscheme(1);else // dark color scheme
					if(argv[arg][i]=='W')setWrapping(true);else
					if(argv[arg][i]=='w')setWrapping(false);else
					if(argv[arg][i]>='0'&&argv[arg][i]<='9')setColorscheme(argv[arg][i]-'0'); // a digit indicating the color scheme to use
				}
			}
		}
	}

	prepareForUserInput(); // AFTER using the command-line parameters (will effectuate wrap mode and color scheme)

	resetOutputColor(); // just in case
	outputLine("Welcome to M.");
	output("\nVersion: %s - Build: %s - Date: %s\n",M_VERSION,M_BUILD,M_DATE);
	outputLine("");
	displayFlags();
	outputLine("");
	
	// tell user whether allocation recording is active!!!
	outputLine(allocationrecordinginitialized()?"Allocation recording ready!":"No allocation recording!");

	if(!initEnvironment()){ // ascertain to have an execution environment!!!
		setColor(getErrorColor());setBackColor(getBackgroundColor());
		outputLine("Exiting: due to failing to initialize the M execution environment!");
		resetOutputColor();
		exit(1);
	}
	if(amVerbose())output("M environment initialized with %llu predefined values.\n",getNumberOfValues());

	Mstring* predefinedVariableNames=_getVariableNames(getEnvironment(),", ");
	if(predefinedVariableNames){
		output("Predefined variables: %s.\n",string(predefinedVariableNames));
		free_string(predefinedVariableNames); // no get rid of it!!!
	}else
		outputLine("No predefined variables!");
	//////////output("Number of predefined variables: %d.",getNumberOfVariables(mEnvironment));
	
	// initialize commands and input mode
	shellCommand=__string(); // MDH@12APR2019: allow executing shell commands (calling system())
	// MDH@20SEP2019 removing: feedforwardText=__string(); // MDH@27FEB2019: create the behind cursor text (to be cleared whenever we start a new command)
	_userInputCommand=NULL; // the current (user input) command

	///// writeCommand() will take care of this!!!! getCommandLength()=0; // keep track of the total command length...
	inputMode=IM_COMMAND; // TODO should this go into promptForUserInput()?

	// TODO shouldn't we do this in initEnvironment? (or its alternative initM() yet to be created)
	_immediateFeedforwardText=__string();if(!_immediateFeedforwardText)outputError("Failed to allow immediate feed forward"); // TODO we can do better than this!!
	_suggestedText=__string();if(!_suggestedText)outputError("Failed to allow suggested text");
	/* do not initialize _manualFeedforwardText because there's now a difference between manual feed forward being NULL or empty (not blocking vs blocking identifier continuation)
	_manualFeedforwardText=__string();if(!_manualFeedforwardText)outputError("Failed to allow manual feed forward");
	*/
	
	char inputChar,inputCharType;

	outputLine("");
	outputLine("Use Ctrl-Z to exit M immediately at any time.");
	outputLine("In any mode press the Enter key on an empty line to switch modes.");

	// let's mark the allocations BEFORE we start looping
	addallocationtype('!');

	while(1){ // command loop

		// if we're supposed to start a new command (i.e. it's not a command continuation)
		promptForUserInput();

		/* MDH@16MAR2019: we're behind the prompt now and should start out without a current command (in pCommnad)
		//                if _userInputCommand->_firstToken is NOT null, we have to make it NULL
		commandIndex=0; // MDH@16MAR2019: pretty essential otherwise it would keep evaluating previous commands
		if(_userInputCommand->_firstToken) // if we still have a command to free, free it entirely
			if(!clearCommand())
				switchToControlMode("Switching to control mode, due to failing to remove the command.");
		*/
		/* replacing (there shouldn't be a command to write right now, unless perhaps when someone entered an invalid command???? to be continued)
		   point: an evaluated command should be discarded??? in which case a user cannot correct it and has to type it in again
		   so it makes sense to be allowed to complete a command (that failed to evaluate)
		*/
		// TODO what if we're not in inputMode here??????
		if(inputMode==IM_COMMAND){
			commandIndex=0; // TODO should we do this always (even if we have an incomplete command?????)
			// MDH@24APR2019: _userInputCommand->_firstToken could be non-null if we failed to evaluate it (e.g. when being imcomplete), and we allow a retry
			//                NOTE registered commands should always be successfully evaluated, so do NOT get rid of any pending command!!!!
			if(_userInputCommand)writeCommand(_userInputCommand);else deleteTokenautocompletiontexts(); // MDH@20SEP2019 replacing: string_setlength(feedforwardText,0);
			/////////////// if(_userInputCommand->_firstToken)clearCommand(); // TODO do we need this????
			/* replacing:
			if(_userInputCommand->_firstToken==NULL)if(!string_setlength(feedforwardText,0))output("??"); // TODO should we be loosing feedforwardText here????
			getCommandLength()=getUserInputLength()=writeTokens(_userInputCommand->_firstToken);
			*/
			/////////outputStatus();
		}
		// which used to be: writeCommand(); // write the current command (if any)

		/* replacing:
		_userInputCommand->_lastToken=_userInputCommand->_firstToken;
		// an existing command to show
		while(_userInputCommand->_lastToken!=NULL){
			outputToken(_userInputCommand->_lastToken);
			// the cursor will move along with every printf()
			getUserInputLength()+=string_length(_userInputCommand->_lastToken->text);
			_userInputCommand->_lastToken=_userInputCommand->_lastToken->next;
		}
		*/
		// we do NOT need a command until after the first character which makes sense because we allow ` and arrow up and down to switch to option mode or select another command
		// now we need to read characters one at a time and echo them from the command line
		// Ctrl-D to exit M
		while(1){ // command input loop
			
			if(inputMode==IM_COMMAND){
				// MDH@07OCT2019: the newly created feed forward category (manual) takes precedence over the identifier continuation and immediate feed forward text
				//                although it is shown in the same color
				// MDH@07OCT2019: now decided to ALWAYS update the identifier continuation BUT it will be merged with the manual feed forward text
				updateUserInputCommandIdentifierContinuation();
				// if we have manual feed forward starting with the given identifier continuation, the identifier continuation will remain
				// and the identifier continuation will be removed from the manual feed forward
				if(_manualFeedforwardText){ // existing manual feed forward text that may block identifier continuation characters
					if(amDebugging())inputInfo("Determining manual feed forward text.");
					// if _manualFeedforwardText is empty ANY identifier continuation will be blocked (e.g. when a single identifier continuation character is removed)
					numberOfIdentifierContinuationManualFeedforwardCharacters=string_number_of_matching_chars(_manualFeedforwardText,_identifierContinuationCharacters);
					// MDH@08OCT2019: when the manual feed forward matches the start of the identifier continuation use the latter
					//                this poses a problem though, the alternative being to always show the identifier continuation behind the manual feed forward
					//                which means that we have to get rid of the identifier continuation if we are not showing it
					//                the point is we cannot simply get rid of the manual feed forward text being the result of a number of left arrow actions
					//                we can only augment it with identifier continuation although the identifier continuation would reappear automatically when we do
					/*
					if(numberOfIdentifierContinuationManualFeedforwardCharacters==string_length(_manualFeedforwardText)){ // the entire manual feed forward text starts the identifier continuation
						// we want to keep the manual feed forward but also show the remainder of the identifier continuation
						// there's no need to delete the identifier continuation because the manual feed forward takes precendence over showing the identifier continuation
						if(strlen(_identifierContinuationCharacters)>numberOfIdentifierContinuationManualFeedforwardCharacters){
							string_append(_manualFeedforwardText,_identifierContinuationCharacters+numberOfIdentifierContinuationManualFeedforwardCharacters);
						}
						// using the identifier continuation instead of the manual feed forward text replacing: free_string(_manualFeedforwardText);_manualFeedforwardText=NULL;
					}
					*/
					///// replacing: if(_identifierContinuationCharacters){
						/* replacing:
						// MDH@06OCT2019: if they are the same we should prefer the identifier continuation
						if(strcmp(string(_manualFeedforwardText),_identifierContinuationCharacters)) // manual feed forward text starts with the identifier continuation
							deleteIdentifierContinuation();
						else
							string_setlength(_manualFeedforwardText,0);
						*/
						/* replacing:
						size_t numberOfIdentifierContinuationCharacters=strlen(_identifierContinuationCharacters);
						if(numberOfIdentifierContinuationCharacters>0&&numberOfIdentifierContinuationCharacters<=string_length(_manualFeedforwardText)){
							char c=string_replacedchar(_manualFeedforwardText,'\0',numberOfIdentifierContinuationCharacters);
							if(strcmp(string(_manualFeedforwardText),_identifierContinuationCharacters)==0){ // manual feed forward text starts with the identifier continuation
								numberOfIdentifierContinuationCharacters-=string_removed(_manualFeedforwardText,0,numberOfIdentifierContinuationCharacters);
								// NOTE the \0 that we put in will end up at position 0 if successful removal of the identifier continuation from the manual feed forward text
								// if not all removed that should've been removed report!!
								if(numberOfIdentifierContinuationCharacters>0)inputError("Failed to merge manual and identifier continuation feed forward text");
							}else // no match
								deleteIdentifierContinuation();
							string_replacedchar(_manualFeedforwardText,c,numberOfIdentifierContinuationCharacters);
						}else // no match
							deleteIdentifierContinuation();
						*/
					//////////}
				}
				////////////if(amVerbose()||!amDebugging())inputInfo("Manual feed forward: '%s'.",string(_manualFeedforwardText));
				// if we do NOT have manual feed forward text, 'update' the immediate feed forward text i.e. only show immediate feed forward text when there's no manual feed forward text!!!
				// get rid of the current immediate feed forward text and update it
				string_setlength(_immediateFeedforwardText,0);
				////outputChar('A');
				if(!_manualFeedforwardText||string_length(_manualFeedforwardText)==0)updateImmediateFeedforwardTextOfUserInputCommand();
				////outputChar('B');
				// MDH@03OCT2019: some feed forward texts are also current token specific, therefore we need to sync the feed forward texts
				//                TODO perhaps we should distinguish between feed forward and auto completion (as with the brackets)
				//                DONE solved this by taking care of immediate feed forward texts whenever the current token (type) changes
				///////////updateFeedforwardTexts();
				updateAutoCompletionText(); // to force it being reconstructed!!! TODO if we decide to always do that we do not need to do this here!!!
				////outputChar('C');
				showSuggestedText();
				////outputChar('D');
				if(amDebugging())outputDebugInfo();
				/////outputChar('E');
			}
			////////outputChar('X');

			// ask the user for input
			if(!inputCharRead(&inputChar))break;

			// hide the suggested text again before processing the character read
			if(inputMode==IM_COMMAND){
				hideSuggestedText();
			}

			////////inputChar=getInputChar();

			if(inputChar>127)continue; // undefined input character

			inputCharType=INPUTCHARACTERTYPES[inputChar];
			
			/* something terribly going wrong when the following code is executed!!!
			if(inputMode==IM_COMMAND){
				if(!amDebugging())outputStatus(inputChar,inputCharType);
			}
			*/
			////////printf("(%d)",inputCharType);

			// if not in control mode, and the switch to control mode character is entered, switch to control mode if first character (NOTE getUserInputLength() is only defined in the other two modes)
			// MDH@16APR2019: I want to use the Enter key (ASCII 13) to switch to the next mode, because the associated input character type is n which will ALWAYS break
			//                in that case we do NOT need the o input character type!!!
			if(inputCharType=='o'){
				if(inputMode==IM_CONTROL){
					switchToCommandMode();
					break;
				}
				// not in control mode, go to control mode if first character on line
				if(!getUserInputLength()){
					inputCharType=switchToControlMode(NULL);
					break;
				}
				// accept (might be an acceptable character in string literal in commands or in shell commands)
			}

			/////if(inputChar!=ESCAPE_CHARACTER)printf("(%d)",inputChar);

			// special (control) input character types
			// first the ones that will break in any input mode!!!!
			if(inputCharType=='i')continue; // insignificant input character without specific purpose

			if(inputCharType=='n'){ // end-of-line (CR of LF) character
				break; // MDH@31OCT2019: changed to using ` as newline character which is considered whitespace so it's appended to the end of a token and ends that token as well, so that a token cannot be split over two lines (which would be ackward)
				/* replacing using it as a newline character:
				// MDH@30OCT2019: if not in command mode or (in command mode) we do not have an input command or it is not valid
				if(inputMode!=IM_COMMAND||!_userInputCommand||isAValidCommand(_userInputCommand,false))break;
				// ASSERT in command mode with an invalid (i.e. unfinished) command
				// ignore if at the start position on the line!!!
				if(!_userinputline||getUserInputLength()>_userinputline->offset){
					showContinuedPrompt();
					showSuggestedText(); // we have to rewrite the suggested text though
					outputTokenColor(_userInputCommand->_lastToken); // and show the right color
				}else // at start of user input line, do not allow having empty user input lines!!!
					beep();
				continue;
				*/
			}

			if(inputCharType=='x')break; // eXit (Ctrl-C or Ctrl-Z) character

			// from now on no continue's anymore, because at the end of the loop we want to check for inputCharType equaling o
			if(inputMode==IM_COMMAND){
#ifdef __DEBUG__
				outputChar(inputCharType);
#endif
				// MDH@03SEP2019: any input character that somehow changes the command needs to ascertain that no previous command is being used (i.e. when commandIndex is not zero)
				//////////outputStatus(inputChar,inputCharType);
				if(inputCharType=='d'){ // MDH@18APR2019: delete now always deletes the first character in the behind cursor text
					/////debugWrite("DELETE");
					// MDH@20SEP2019: equivalent to removing ANY first character in the first feed forward text (if any)
					// MDH@27SEP2019: removing the identifier continuation text takes precedence!!!
					// MDH@07OCT2019: manual feed forward now comes first (in showing)
					if(!_manualFeedforwardText&&string_length(_manualFeedforwardText)>0){
						// remove one character at a time (TODO might be confusing with the removal of the entire identifier continuation on Delete)
						if(!getFirstManualFeedforwardCharacterRemoved())inputCharType=switchToControlMode("Failed to delete the first suggested character.");
					}else
					if(_identifierContinuationCharacters){
						// MDH@30OCT2019: by blocking the subsequent update (the next time), we loose the identifier continuation automatically
						userInputCommandIdentifierContinuationNeedsUpdating=false; // ascertain to block or not block accordingly (i.e. if we're in an identifier don't block, either wise block)
						/// replacing: deleteIdentifierContinuation();
						///////writeSuggestedText(false);
					}else
					if(_firstTokenautocompletiontext){ // replacing: string_length(feedforwardText)){ // something to delete
						if(!getFirstAutocompletionCharacterRemoved())inputCharType=switchToControlMode("Failed to remove the first character in the suggested text.");
					}else // no first feed forward text (and character)!
						beep();
				}else
				if(inputCharType=='b'){ // backspace
					///////debugWrite("BACKSPACE");
					// something to remove?
					if(getUserInputLength()) // TODO _userInputCommand->_firstToken should be NULL at the same time getCommandLength() becomes 0!!!
						removePreviousTokenCharacter();
					else // nothing to remove
						beep();
				}else
				if(inputCharType=='h'){
					if(_userInputCommand)
						inputInfo("Press Enter to execute the command, Ctrl-C to clear the command, Ctrl-Z to exit M immediately.");
					else
						inputInfo("Use Ctrl-Z to exit M immediately, press the Enter key to switch to Control mode.");
				}else
				if(inputCharType=='c'){ // cancel command (Ctrl-C)
					// replacing: if(_userInputCommand->_firstToken!=NULL){clearCommand();break;}beep(); 
					if(!_userInputCommand){
						inputInfo("Nothing to clear. (Use Ctrl-H for some help.)");
						beep();
					}else
					if(_userInputCommand->_firstToken){
						/////////if(amWrapping()())break; // if in amWrapping()() can't guarantee backspace() to move into the previous line which means just prompt again...
						// MDH@03SEP2019: what to do when Ctrl-C is called on a previous command????? i.e. when _userInputCommand->_firstToken points to a previous command, I'd say that we should return to the current command
						if(commandIndex)
							setCommandIndex(0);
						else
							cancelCommand();
					} else
						beep();
				}else
				if(inputCharType=='t'){ // Tab character
					// MDH@03SEP2019: here commandCharacterAccepted() will take care of copying the command if commandIndex is not (yet) zero
					// if there's a preview (well, code completion by way of a feedforwardText)
					// MDH@20SEP2019: I suppose we can simply get the characters from the (constructed) feedforwardText
					//                but that would cause problems in case something goes wrong
					// MDH@25SEP2019: the following goes wrong in those case where the feed forward text is generated, so it should not generate feed forward text until after all feed forward characters are consumed!!!!
					//                we solve this by always passing false for the endOfInput argument!!!
					/* replacing:
					if(_firstTokenautocompletiontext){
						char newInputChar;
						while((newInputChar=getFirstAutocompletionCharacterRemoved())){
							if(!commandCharacterAccepted(newInputChar,INPUTCHARACTERTYPES[newInputChar],false,true)){
								beep();
								inputCharType=switchToControlMode("Failed to accept a suggested character.");
								break;
							}
						}
						// MDH@25SEP2019: we now need to generate the completion text again
						deleteTokenautocompletiontexts(); // APPARENTLY I need to do this to force writeSuggestedText(true) to reconstruct it (although I thought updateLastTokenAutocompletionText() would call deleteTokenautocompletiontexts())
						updateLastTokenAutocompletionText();
						writeSuggestedText(true); // TODO can we not find a better way to do this??????
					}else
						beep();
					*/
					// the original way of doing this (by computing the behind cursor text and consuming it)
					// MDH@26SEP2019: how about only consuming the identifier continuation text if we have it as a service to the user????? I think that makes sense doesn't it
					//                updateBehindCursorText() adjusted to return (if available) a pointer to the characters in the feed forward text (TODO change behind cursor into feed forward when appropriate)
					// MDH@04OCT2019: are we going to consume in parts????? NOTE if there's no identifier continuation text, the immediate feed forward and auto completion texts to consume are present in _suggestedText (thank god)
					// MDH@07OCT2019: manual feed forward text (as a whole) takes precedence
					size_t numberOfCharactersToConsume=string_length(_suggestedText);
					if(numberOfCharactersToConsume>0){
						// if there are manual feed forward characters it can start with a number of identifier continuation characters to consume as a whole using Tab
						size_t numberOfManualFeedforwardCharacters=(_manualFeedforwardText?string_length(_manualFeedforwardText):0);
						size_t numberOfIdentifierContinuationCharacters=(numberOfManualFeedforwardCharacters?numberOfIdentifierContinuationManualFeedforwardCharacters:(_identifierContinuationCharacters?strlen(_identifierContinuationCharacters):0));
						// the feed forward text that is colored differently (identifier continuation and manual feed forward text is to be accepted as a whole when Tab is used)
						// it's easiest to do that by specifying the number of suggested characters to consume
						if(numberOfIdentifierContinuationCharacters>0)numberOfCharactersToConsume=numberOfIdentifierContinuationCharacters;
						else
						if(numberOfManualFeedforwardCharacters>0)numberOfCharactersToConsume=numberOfManualFeedforwardCharacters;
						char newInputChar='\0',newInputCharType='\0';
						size_t numberOfSuggestedCharactersAccepted=0;
						while(numberOfSuggestedCharactersAccepted<numberOfCharactersToConsume){
							newInputChar=string_char(_suggestedText,numberOfSuggestedCharactersAccepted);
							if(!newInputChar){
								inputCharType=switchToControlMode("Suggested characters vanishing somehow.");
								break;
							}
							// MDH@24APR2019 obsolete: getCommandLength()--; // until we manage to insert the character removed, we have one less character in the total command length
							// MDH@14AUG2019: suggestedCharacter is set to true now, this makes perfect sense as I'm consuming all characters here and we do not want to remove them, NOTE that characters may still be inserted but only when bc=0 obviously
							newInputCharType=INPUTCHARACTERTYPES[newInputChar];
							if(newInputChar!='#'&&!commandCharacterAccepted(newInputChar,&newInputCharType,false,true)){
								inputCharType=switchToControlMode("Failed to consume a suggested character.");
								newInputChar='\0'; // to indicate some error occurred
								break;
							}
							if(newInputCharType==' ')showContinuedPrompt(); // MDH@31OCT2019: whenever a newline (request) character is consumed, make a new line
							numberOfSuggestedCharactersAccepted+=1;
						}
						// remove at most numberOfSuggestedCharactersAccepted from the suggested text
						if(inputMode==IM_COMMAND){
							// if identifier continuation characters were consumed nothing to do, otherwise either to clear the manual
							// MDH@08OCT2019: have to be careful here because identifier continuation characters might come out of the manual feed forward text
							if(numberOfSuggestedCharactersAccepted){ // at least one character consumed
								if(numberOfSuggestedCharactersAccepted==numberOfCharactersToConsume){
									if(numberOfManualFeedforwardCharacters){ // some of the manual feed forward characters were consumed
										if(numberOfSuggestedCharactersAccepted>=string_length(_manualFeedforwardText)){ // all manual feed forward characters were consumed
											free_string(_manualFeedforwardText);_manualFeedforwardText=NULL;
										}else{ // not all manual feed forward characters were consumed 
											// remove numberOfSuggestedCharactersAccepted from the start of the manual feed forward text
											if(!string_removed(_manualFeedforwardText,0,numberOfSuggestedCharactersAccepted))
												inputCharType=switchToControlMode("Not all accepted suggested characters removed from the suggested text.");
										}
									}else
										deleteTokenautocompletiontexts();
									updateLastTokenAutocompletionText(); // TODO do we need the following????
								}else
									inputCharType=switchToControlMode("Not all suggested characters accepted.");
								// we might end up behind some character that produces auto completion stuff like [ or {
							}else
								inputCharType=switchToControlMode("No suggested characters accepted.");					
						}
					}else // no consumable text
						beep();
				}else
				if(inputCharType=='m'){ // Esc character...
					if(inputCharRead(&inputChar)){
						///printf("(%d)",inputChar);
						if(inputChar==91){
							if(inputCharRead(&inputChar)){//inputChar=getInputChar();
								///printf("(%d)",inputChar);
								if(inputChar==51){
									if(inputCharRead(&inputChar)){//inputChar=getInputChar();
										if(inputChar==126){ // delete
											/////////inputInfo("Delete");
											// we should have suggested (identifier continuation or feed forward (autocompletion)) text
											// MDH@07OCT2019: manual feed forward text goes first
											if(_suggestedText&&string_length(_suggestedText)){ // there is suggested text with parts to delete
												// any identifier continuation characters precede manual feed forward text
												if(string_length(_manualFeedforwardText)){
													if(getFirstManualFeedforwardCharacterRemoved()){
														if(string_length(_manualFeedforwardText)==0){free_string(_manualFeedforwardText);_manualFeedforwardText=NULL;}
													}else
														inputCharType=switchToControlMode("Failed to delete the first suggested character.");
												}else
												if(_identifierContinuationCharacters&&strlen(_identifierContinuationCharacters)>0){
													// if there's immediate feed forward token, there's no manual feed forward
													// PROBLEM can't remove the identifier continuation characters as a whole because if I do it will be recreated
													if(!prependedToManualFeedforwardText(_identifierContinuationCharacters+1))
														inputCharType=switchToControlMode("Failed to delete the first suggested character.");
												}else{
													// MDH@06OCT2019: as long as the user is manually deleting identifier continuation characters
													//                we prevent the automatic update of the identifier continuation?????
													// MDH@06OCT2019: it makes sense to let Delete consume only a single character, otherwise the user might get confused
													//                because sometimes only one character is consumed, and sometimes more
													//                so to keep it simple we consume a single character at a time
													//                BUT if there are characters left we move these into the manual feed forward
													//                that way no identifier continuation will show until the user consumed the entire manual feed forward
													//                this makes sense because the identifier continuation itself won't change while using Delete only
													//                HOWEVER what should happen if the user types another character thus changing what's in front of the
													//                manual feed forward?????
													if(string_length(_immediateFeedforwardText)==0){ // MDH@20SEP2019 replacing: string_length(feedforwardText)){
														if(!getFirstAutocompletionCharacterRemoved()) // MDH@20SEP2019 replacing: string_removed_char(feedforwardText,0))
															inputCharType=switchToControlMode("Failed to delete the first character of the suggested text.");	
													}else{ // remove the immediate feed forward text
														string_setlength(_immediateFeedforwardText,0);
														/////////////immediateFeedforwardToBeUpdated=false; // do not update next time
													}
												}
											}else
												beep();
										}
									}
								}else
								if(inputChar==65){ // up arrow 
									if(inputMode==IM_COMMAND){ // i.e. show previous command if any
										if(!commandIndex&&_userInputCommand)
											inputError("%s","Won't show previous commands when one is being entered.");
										else
										if(!commandDown())
											beep();
									}else
									if(!commandPage)
										showPreviousCommandPage();
									/*
									else
									if(!commandDown())
										outputInfo("%s","No previous command!");
									*/
								}else
								if(inputChar==66){ // down arrow
									if(inputMode==IM_COMMAND){									
										if(!commandIndex&&_userInputCommand)
											inputError("%s","Won't show next commands when one is being entered!");
										else 
										if(!commandUp())
											beep();
									}else
									if(!commandPage)
										showNextCommandPage();
									/*
									else
									if(!commandUp())
										outputInfo("%s","No next command!");
									*/
								}else
								if(inputChar==67){ // right arrow
									// MDH@27SEP2019: don't forget the continuation text as well!!!
									if(string_length(_suggestedText)){
										char c=string_char(_suggestedText,0);
										if(c){
											// MDH@31OCT2019: this might well be a newline character!!!
											char suggestedInputCharType=INPUTCHARACTERTYPES[c];
											if(commandCharacterAccepted(c,&suggestedInputCharType,true,false)){
												inputCharType=suggestedInputCharType; // MDH@31OCT2019: because might have changed!!!
												if(inputCharType==' ')showContinuedPrompt(); // MDH@31OCT2019: we just consumed a newline (request) character
												// where to remove it from????
												// NOTE identifier continuation and immediate feed forward are redetermined automatically so do not need to be adjusted here
												if(string_length(_manualFeedforwardText)){
													if(getFirstManualFeedforwardCharacterRemoved()!=c)
														inputCharType=switchToControlMode("Wrong first suggested character accepted!");
												}else
												if(!_identifierContinuationCharacters||strlen(_identifierContinuationCharacters)==0){
													if(string_length(_immediateFeedforwardText)==0){
														if(!getFirstAutocompletionCharacterRemoved())
															inputCharType=switchToControlMode("Failed to remove the accepted first auto completion character.");
													}
												}												
											}else
												inputCharType=switchToControlMode("First suggested character not accepted.");
										}else
											inputCharType=switchToControlMode("First suggested character vanished.");
									}else
										beep();
								}else
								if(inputChar==68){ // left arrow
									if(getUserInputLength()){
										// TODO apparently _userInputCommand->_firstToken will still be NULL when we're scrolling through the list of previous commands...
										// MDH@03SEP2019: BUG FIX forgot to make commandIndex 0 when copying the command (as copyUserInputCommand() itself does not seem to do that!!!)
										if(commandIndex){commandIndex=0;copyUserInputCommand();} // will also set getCommandLength()!!!
										// MDH@27FEB2019: we should remove the last character of the current token (and command) and move it into feedforwardText
										// MDH@20SEP2019: we do NOT want the character removed to disappear when the token it came from disappears, therefore the addition to the feed forward text should be anonymous
										// MDH@25SEP2019: because we're going to prepend c to the feed forward text, we have to determine the associated token i.e. the token that generated c
										//                i.e. if the character being moved would have been auto-generated we want to know the associated token
										//                the problem now is that removedTokenCharacter() will already remove the token when its a single-character token
										//                TODO something is not correct because if removedTokenCharacter() takes care of removing the token which check again below????????
										//                it's easiest to check whether the token will be removed: this will be the case if there's only one character in the token
										//                in which case that will be the originating token but only in the situation where c matches the feed forward character of that expression
										///////Mtoken* startOfExpressionToken=(string_length(_userInputCommand->_lastToken->text)==1?_userInputCommand->_lastToken->expr:NULL); // MDH@25SEP2019: remember what the start of expression token associated with the current last command token is
										char c=removedTokenCharacter(true); // passing true will force removedTokenCharacter() to actually check an identifier token type
										if(c){ // removing the character behind the cursor succeeded
											debugWrite("Character '%c' removed.",c);
											// MDH@01OCT2019 removed as now done by removedTokenCharacter(): moveCursorLeft(1); // MDH@01OCT2019: TODO will this be sufficient when the identifier token type changed????? probably
											/* MDH@01OCT2019: no need to perform a tokenChecked... here anymore as we moved that functionality over to removedTokenCharacter(true)
											// TODO is the following necessary at all? given that removedTokenCharacter should/could have done so??????
											// TODO doing the following is better moved over to removedTokenCharacter() because if 
											if(string_length(_userInputCommand->_lastToken->text)>0)
												tokenCheckedForBeingAFunction(true);
											*/
											/*
											else // nothing left in current token // MDH@23SEP2019: it seems better to call removeToken() here as removeToken() will also remove the token's feed forward text
												removeToken();
											*/
											// MDH@30SEP2019: this is the only call to getAutocompletionTextOfCharacterPrepended() therefore we can simply adjust that function to check whether this character matches a consumed character!!!
											//                but I guess failing to do so is not that terrible that we should switch to control mode
											// MDH@01OCT2019: if this character also starts the new identifier continuation, it should not be anonymously prepended 
											//                let's construct the identifier continuation text that we would get 
											//                CORRECTION checking against the first identifier continuation text suffices (instead of comparing with the entire new identifier continuation)
											// MDH@04OCT2019: there was so much going wrong doing the following that I simply replaced it by prepending the consumed character to the feed forward texts and leaving it
											//                to the input loop part to deal with identifier continuations that match the start of the feed forward text(s)
											// MDH@07OCT2019: I've introduced a new feed forward element (manualFeedforwardText) to contain the part of the text
											//                that the user took out of the (tokenized) command to e.g. correct a command
											if(!manualFeedforwardCharacterPrepended(c)) // replacing: if(!getAutocompletionTextOfCharacterPrepended(c,true))
												inputCharType=switchToControlMode("Failed to accept the removed command character as suggested text.");
											/*
											else
											if(amVerbose())
											inputInfo("Manual feed forward: '%s'.",string(_manualFeedforwardText));
											*/
											/* replacing:
											updateUserInputCommandIdentifierContinuation();
											///outputChar('1');
											// prepend only anonymously when not matching the identifier continuation character!!
											if(!_identifierContinuationCharacters||_identifierContinuationCharacters[0]!=c)
											if(!getAutocompletionTextOfCharacterPrepended(c,false))
											inputError("Failed to accept the removed command character as suggested text.");
											///outputChar('2');
											updateLastTokenAutocompletionText(); // if we haven't updated the identifier continuation text do it again	
											///outputChar('3');
											*/
											////////writeSuggestedText(false);
											/* replacing:
											bool removedTokenCharacterMatchesFirstNewIdentifierContinuationTextCharacter=false;
											Mstring* identifierContinuationText=NULL;
											if(_userInputCommand->_lastToken->type==TT_FUNCTION||_userInputCommand->_lastToken->type==TT_VARIABLE||_userInputCommand->_lastToken->type==TT_NEW_VARIABLE){
												identifierContinuationText=__string();
												if(identifierContinuationText){
													string_append_char(identifierContinuationText,c);
													if(_identifierContinuationCharacters)string_append(identifierContinuationText,_identifierContinuationCharacters);
													updateUserInputCommandIdentifierContinuation(); // determine the new identifier continuation
													if(_identifierContinuationCharacters&&strcmp(_identifierContinuationCharacters,string(identifierContinuationText))==0)
														removedTokenCharacterMatchesFirstNewIdentifierContinuationTextCharacter=true;
												}
											}
											if(!removedTokenCharacterMatchesFirstNewIdentifierContinuationTextCharacter)
											if(!getAutocompletionTextOfCharacterPrepended(c))
											inputError("Failed to accept the removed command character as suggested text.");
											updateLastTokenAutocompletionText(identifierContinuationText==NULL); // if we haven't updated the identifier continuation text do it again	
											writeSuggestedText(false);
											if(identifierContinuationText)free_string(identifierContinuationText);
											*/
										}else
											inputCharType=switchToControlMode("Failed to remove the last command character.");
									}else
										beep();
								}else
									beep();
							}
						}
					}
				}else{
					// MDH@21APR2019: creating a command if need be is delegated to commandCharacterAccepted() which we know
					//                we always need a command (being edited)
					// MDH@01OCT2019: if the input character matches the first non-anonymous i.e. autogenerated feed forward character same functionality as right arrow (except now we know the character entered)
					if(inputChar==getFirstSuggestedCharacter(true)){
						char firstSuggestedCharacterConsumed=getFirstSuggestedCharacterConsumed(inputChar,true);
						if(firstSuggestedCharacterConsumed)removeFirstSuggestedCharacter(firstSuggestedCharacterConsumed,false);else inputCharType=switchToControlMode("Failed to accept the matching first suggested character.");
					}else{
						// MDH@31OCT2019: when the accepted character is the backtick in whitespace we should continue with the command on the next line
						//                OOPS a backtick inside a string is also recognized as such which shouldn't happen
						//                ALSO because the backtick will be visible it's probably better to insert an empty token for it of type TT_NEWLINE or something like that
						//                it's probably best to check whether to accept a backtick here???? NOTE we could have backticks in commands read from files as well????
						if(commandCharacterAccepted(inputChar,&inputCharType,true,false)){
							if(inputCharType==' '){ // a newline request (whenever M_NEWLINE_CHARACTER is input at a functional position)
								showContinuedPrompt();
								//////////showSuggestedText(); // we have to rewrite the suggested text though
								///////////outputTokenColor(_userInputCommand->_lastToken); // and show the right color
							}
						}else
						if(inputCharType!='`') // not a new line request character
							inputCharType=switchToControlMode(_userInputCommand->_firstToken?"Failed to accept the character.":"Failed to create a new command.");
						else
							inputError("New line request character not allowed here");
					}
					/*
					// we need to have a token (to append the input character to) which initializes to _userInputCommand->_firstToken
					if(_userInputCommand->_firstToken==NULL) // no first command token
						// if commandIndex we should one of the registered commands
						setCommand(commandIndex?commands[commandCount-commandIndex]:NULL); // will also set getCommandLength()!!!
					// if still NULL (also when we fail to actually create a new first command token)
					if(_userInputCommand->_lastToken!=NULL){
						if(!commandCharacterAccepted(inputChar,inputCharType,true))
							switchToControlMode("Failed to accept the character.");
					}else
						switchToControlMode("Failed to create a new command!");
					*/
				}
				///////////// MDH@06AUG2019 NOT AGAIN: outputStatus(inputChar,inputCharType);
			}else
			if(inputMode==IM_CONTROL){ // inputChar received in control mode
				outputChar(inputChar); // nice to see the character we typed...
				// might be paging through the commands
				if(!commandPage){ // not currently paging through the commands
					// single character responses (and out again)
					if(inputChar=='a'||inputChar=='A'){setAssisting(inputChar=='A');inputCharType='n';break;}
					if(inputChar=='d'||inputChar=='D'){setDebugging(inputChar=='D');inputCharType='n';break;}
					if(inputChar=='m'||inputChar=='M'){setMatchingparentheses(inputChar=='M');inputCharType='n';break;}
					if(inputChar=='v'||inputChar=='V'){setVerbose(inputChar=='V');inputCharType='n';break;}
					if(inputChar=='u'||inputChar=='U'){setAcceptinghistorycommand(inputChar='U');inputCharType='n';break;}
					if(inputChar>='0'&&inputChar<='9'){setColorscheme(inputChar-'0');inputCharType='n';break;}
					if(inputChar=='w'||inputChar=='W'){setWrapping(inputChar=='W');inputCharType='n';break;}
					////////////if(inputChar=='v'||inputChar=='V'){outputVariables();inputCharType='n';break;}
					if(inputChar=='b'||inputChar=='B'){beep();inputCharType='n';break;} // MDH@23SEP2019: so we can test whether beep() is working... TODO for toggling beeping????
					if(inputChar=='f'||inputChar=='F'){outputFunctions();inputCharType='n';break;}
					if(inputChar=='r'||inputChar=='R'){reset();inputCharType='n';break;}
					// options
					if(inputChar=='x'||inputChar=='X'){inputCharType='x';break;}
					if(inputChar=='s'||inputChar=='S')inputCharType=switchToShellMode(NULL);
					if(inputChar=='h'||inputChar=='H'){
						// are we showing the history 5 commands at a time, or 9 at a time? we want the user to be able to select a command quickly
						// we could call them a, b, c etc.
						if(commandCount){
							commandPages=1+(commandCount-1)/10;
							showNextCommandPage(); // as soon as commandPage>0 we are paging...
						}else
							output("%s\n","No previous commands to show.");
					}
				}else
					// user might have selected one of the commands (letter a through j)
					commandPage=0; // stop paging
			}else{ // Shell command input mode
				// we still allow using certain 'special' characters for composing the command (much like we did with a command)
				if(inputCharType=='b'){ // backspace
					uint16_t cp=getUserInputLength();
					if(cp){
						if(string_removed_char(shellCommand,cp-1)){
							moveCursorLeft(1);
							////////writeSuggestedText(true);
						}else
							inputCharType=switchToControlMode("Failed to remove the shell command character!");
					}else // nothing to remove
						beep();
				}else
				if(inputCharType=='d'){
					if(string_length(_suggestedText)){ // something behind the cursor that we can remove
						if(string_removed_char(_suggestedText,0))
							///////writeSuggestedText(true)
							;
						else
							inputCharType=switchToControlMode("Failed to remove the first autocompletion character!");
					}else // nothing to remove
						beep();
				}else
				if(inputCharType=='c'){ // cancel command (Ctrl-C)
					if(_userInputCommand->_firstToken!=NULL){
						backToPrompt();
						clearShellCommand();
						//////////if(amWrapping()())break;
					}else
						beep();
				}else
				if(inputCharType=='t'){ // Tab character
					// if there's a preview (well, code completion by way of a feedforwardText)
					// here we have a serious problem in that we now have identifier continuation text, immediate feed forward text and permanent feed forward texts
					uint16_t bc=getNumberOfSuggestedCharacters();
					if(bc){
						while(bc--){
							char newInputChar=string_removed_char(_suggestedText,0);
							if(!newInputChar){
								///////writeSuggestedText(true);
								inputCharType=switchToControlMode("Failed to accept all suggested characters.");
								break;
							}
							if(newInputChar!='#')string_append_char(shellCommand,newInputChar);
						}
					}else
						beep();
				}else
				if(inputCharType=='m'){ // Esc character...
					if(inputCharRead(&inputChar)){////inputChar=getInputChar();
						if(inputChar==91){
							if(inputCharRead(&inputChar)){/////inputChar=getInputChar();
								if(inputChar==51){
									if(inputCharRead(&inputChar)){///////inputChar=getInputChar();
										if(inputChar==126){ // delete
											// TODO FIX this does not seem to be right!!!!!
											if(getNumberOfSuggestedCharacters()){
												// we could go one to the right and do a backspace!!
												moveCursorRight(1);
												// TODO what to do here??? removePreviousTokenCharacter();
											}else // nothing under the cursor to delete
												beep();
										}
									}
								}else
								if(inputChar==65){ // up arrow 
									beep();
								}else
								if(inputChar==66){ // down arrow
									beep();
								}else
								if(inputChar==67){ // right arrow
									if(getNumberOfSuggestedCharacters()){
										char newInputChar=string_removed_char(_suggestedText,0);
										if(!newInputChar){
											////////writeSuggestedText(true);
											inputCharType=switchToControlMode("Failed to accept the suggested characters.");
										}else
											string_insert_char(shellCommand,getUserInputLength(),newInputChar);
									}else
										beep();
								}else
								if(inputChar==68){ // left arrow
									uint16_t cp=getUserInputLength();
									if(cp){
										bool success=false;
										char c=string_removed_char(shellCommand,cp-1);
										if(c){ // removing the character behind the cursor succeeded
											// prefix it to _suggestedText
											string_insert_char(_suggestedText,0,c);
											moveCursorLeft(1);
											///////writeSuggestedText(true);
										}else
											inputCharType=switchToControlMode("Failed to move the cursor left.");
									}else
										beep();
								}else
									beep();
							}
						}
					}
				}else{
					string_append_char(shellCommand,inputChar);
					outputChar(inputChar);
					// MDH@24APR2019: getUserInputLength()++;
				}
			}
			// if switched to control mode, inputCharType will be equal to 'o' and we break out of this input loop!!!
			if(inputCharType=='o'||inputCharType=='s')break;
		} // end of character input 

		// MDH@07OCT2019: there are several places where we might get rid of any manual feed forward text still pending
		//                but this seems to be a very good place for it
		//                _manualFeedforwardText is only used in command and shell mode, not in control mode
		//                perhaps it's better done at the prompt
		//                TODO consider doing the same for feed forward texts?????
		if(_manualFeedforwardText){free_string(_manualFeedforwardText);_manualFeedforwardText=NULL;}

		// if eXit input character(s) received...
		if(inputCharType=='x'){
			// we should only exit M when not entering a function body
			if(!_currentFunctionBodyInput)break; // break out of user input loop
			// switch back to command mode
			endFunctionBodyInput();
			switchToCommandMode();
		}else
		// MDH@16APR2019: now if we use n to switch modes as well, we can do that if there's no command
		if(inputCharType=='n'){
			if(inputMode==IM_COMMAND){
				// MDH@21JUL2019: if the last token appears to be a function identifier change it to a variable
				//                so we won't end up with refusal of evaluation
				if(_userInputCommand&&_userInputCommand->_lastToken)if(_userInputCommand->_lastToken->type==TT_FUNCTION)changeFunctionTokenToAVariable(false);
				inputInfo("%s",""); // so that line will be empty
				resetOutputColor(); // prevent showing subsequent output in the wrong colors
				clearScreenFromCursor(); // so we won't see the behind cursor text anymore
			}
			outputChar('\n');
			if(inputMode==IM_COMMAND){ // the newline character ends the command to be evaluated!!
				// if _userInputCommand->_firstToken is set, we have a command to evaluate
				/*
				Mtoken* _userInputCommand->_firstTokenToEvaluate=NULL; // this would be the command to register if we succeed in evaluating it!!!
				if(_userInputCommand->_firstToken){ // a current command being edited
					// MDH@22MAR2019: currently the first token is an EXPRESSION token
					if(getUserInputLength()>string_length(_userInputCommand->_firstToken->text)){
						// finish the last token???
						if(_userInputCommand->_lastToken->significantCharacterCount==0)_userInputCommand->_lastToken->significantCharacterCount=string_length(_userInputCommand->_lastToken->text);
						_userInputCommand->_firstTokenToEvaluate=_userInputCommand->_firstToken; // but only when not at start of command!!!
						if(amDebugging())outputTokenInfo();
					}
				}else // no command yet, although we might be looking at a previous command
				if(commandIndex&&getUserInputLength()) // NOTE using getUserInputLength() is better than using amAcceptinghistorycommand() (causing it!!)
					_userInputCommand->_firstTokenToEvaluate=commands[commandCount-commandIndex];
				*/

				// if we succeeded in evaluating a command we should register it
				if(_userInputCommand&&_userInputCommand->_firstToken){ // technically something to evaluate
					if(amVerbose())outputCommandInfo(_userInputCommand);
					size_t mark=allocationmark();
					if(amVerbose())output("Mark: %zu.\n",mark);
					bool commandEvaluated=evaluateCommand();
					outputChar('\n');
					if(mark>0){
						if(amVerbose())output("Left after unmarking: %zu.\n",unmarkallocation(mark));
						allocationreport(1); //syncallocations(); // will also do allocationreport(1)
					}else
					if(amVerbose())outputLine("No allocations to unmark.");
					if(!commandEvaluated){
						Mstring* commandText=_getCommandText(false);
						if(!string_length(commandText)){
							clearCommand();
							outputLine("Nothing to evaluate!");
						}else // MDH@16MAY2019: no need to tell the user that evaluation failed, because an error message would have been shown to indicate what went wrong (see evaluateCommand())
							outputLine("Please complete, correct or cancel the command.");
						free_string(commandText);
						continue;
					}
					resetOutputColor();
					if(amVerbose())outputLine("Command evaluated!");
					deleteTokenautocompletiontexts(); // MDH@20SEP2019 replacing: string_setlength(feedforwardText,0); // clear the autocompletion text NOTE if we fail to evaluate the command it will not be cleared!!!!
					// if we succeed in registering the command the command tokens should NOT be freed, BUT if we fail to register the command we should free ALL command tokens
					if(!registerCommand(_userInputCommand)){
						// if commandIndex (>0) we have evaluated a previous command which should also NEVER be freed
						if(!commandIndex){ // a new command being registered!!!
							free_command(_userInputCommand); // MDH@29OCT2019 replacing: freeToken(_userInputCommand->_firstToken);
							outputError("Failed to register the command! Probable cause: out of memory");
						}else
							outputError("Failed to register the command again! Probable cause: out of memory");
					}else
					if(amVerbose())outputLine("Command registered!");

					// start anew (without a current command to evaluate!!!!) NOTE the memory is either still pointed to in `commands` or freed because it failed to bind it in commands so we're free to NULL the pointer here!!!
					_userInputCommand=NULL; // MDH@29OCT2019 replacing non Mcommand style (before today): _userInputCommand->_lastToken=_userInputCommand->_firstToken=NULL; // remove reference to current command

					// garbage collection: remove any values not used anymore...
					size_t removedValueCount=getNumberOfRemovedValues();
					if(amDebugging()){if(removedValueCount)output("Number of garbage collected values: %lu.\n",removedValueCount);else outputLine("No garbage collected values.");}

					// switch to function body input mode when this command contained at least one user function definition
					// (even when dealing with currently inputting function body commands)
					if(_firstFunctionBodyRequest&&!startFunctionBodyInput())outputError("Failed to start requesting the body of a new function");

				}else{
					// MDH@14AUG2019: if a user presses Enter when there's no command but still feedforwardText it looses feedforwardText but we do switch to the control mode as I think that is what the user wants (if only to look at the list of variables)
					//                NOTE that I might consider keeping feedforwardText, so it will be redisplayed when the user returns to the command mode
					switchToControlMode(NULL); // replacing: if(getNumberOfSuggestedCharacters()==0)switchToControlMode(NULL);else outputError("Still suggested text");
				}
			}else
			if(inputMode==IM_SHELL){
				if(string_length(shellCommand))
					executeShellCommand();
				else // MDH@16APR2019: back to command mode
					switchToCommandMode();
			}else // Return key in control mode, always to return to command input!!
				switchToCommandMode();
		}
	}
	// 'normal' exit
	exit(0);
}