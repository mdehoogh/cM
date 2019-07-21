// remove the line below when not in debug mode
//#define __DEBUG__

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

// used externally in Mexecution.h, Mvalue.h, Menvironment.h
//Mvaluetype={VT_UNDEFINED,VT_TOKEN,VT_INTEGER,VT_BIGINTEGER,VT_DECIMAL,VT_RATIONAL,VT_REAL,VT_TEXT,VT_LIST,VT_MAP}
const char* VALUETYPENAMES[]={"unknown","token","integer","big integer","decimal","rational","real","text","list","map"};
const char* const DEFINEUSERFUNCTION_NAME="function";
const char* const MUTABLEVALUETYPECHARS="utibdqrslm"; // the characters associated with each of the value types
const char* const IMMUTABLEVALUETYPECHARS="UTIBDQRSLM"; // the characters associated with each of the value types
const char* const ERROR_PREFIX="ERROR: "; // used in Mexecution.c as well (defined there as extern!!!)
const long double M_LD_NAN=0.0/0.0; // or strtold("nan",NULL) would work as well
const long double M_LD_Q_EPS=1e-18; // this is the exact boundary to use for approximating 13/11 (which seems to be an notorious long double to approximate with rational (13/11)!!!)

const long long M_DP=20; // the default decimal precision

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

const long double LD_PI=3.1415926535897932384626433832795L; // 31 non-zero decimal digits of PI (before the first 0)

// PI all little more accurate (we could make a PI100 from these numbers)
// source: https://blog.wolfram.com/2011/06/30/all-rational-approximations-of-pi-are-useless/
// wolfram has a Rationalize function to compute rational approximations to a certain accuracy (see https://reference.wolfram.com/language/ref/Rationalize.html)
/////const char* M_QNUM_PI100="394372834342725903069943709807632345074473102456264";
/////const char* M_QDEN_PI100="125532772013612015195543173729505082616186012726141";

const long double LD_E=2.718281828459045235360287471353L; // 30 decimal digits of E

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
long double realsum(Mreal* _real1,Mreal* _real2){
	if(!_real1&&!_real2)return M_LD_NAN; // both undefined
	if(!_real1||ldIsZero(_real1->ld)||ldIsNaN(_real1->ld))return _real2->ld;
	if(!_real2||ldIsZero(_real1->ld)||ldIsNaN(_real1->ld))return _real1->ld;
	return _real1->ld+_real2->ld;
}
bool realIsUndefined(Mreal* _real){return(!_real||ldIsNaN(_real->ld));}
long double realneg(Mreal* _real){return (realIsUndefined(_real)?M_LD_NAN:-_real->ld);}

// operators applied to rationals
Mrational* _qmultiply(Mrational* _rational1,Mrational* _rational2){
	if(!_rational1||!_rational2)return NULL;
	bool delta1undefined=realIsUndefined(_rational1->delta),delta2undefined=realIsUndefined(_rational2->delta);
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
	bool delta1undefined=realIsUndefined(_rational1->delta),delta2undefined=realIsUndefined(_rational2->delta);
	// pure rationals are easy
	if(delta1undefined&&delta2undefined)return _getRational(_num,_den,M_LD_NAN,true,true);
	// if we assume the delta's to be very small (as they will be), the parts containing squares to be too small to care about 
	if(delta1undefined){
		// second denominator term is -(b*d*delta2)**2 which for reasonably small b and d (both denominators) will be negligable

	}
	if(delta2undefined){

	}
	// neither undefined
	return NULL;
}
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
*/
Mrational* _qadd(Mrational* _rational1,Mrational* _rational2){
	Mrational* _rational=NULL;
	if(_rational1&&_rational2){
		// we can speed things up by using a special multiplication method
		Mbiginteger *_num1=_Imul(_rational1->num,_rational2->den),*_num2=_Imul(_rational2->num,_rational1->den);
		if(_num1&&_num2) // we got (and need) both
			_rational=_getRational(_Iadd(_num1,_num2,false),_Imul(_rational1->den,_rational2->den),realsum(_rational1->delta,_rational2->delta),true,true); // free the numerator and denominator
		if(_num1)free_biginteger(_num1);if(_num2)free_biginteger(_num2);
	}
	/* replacing:
	
	// OOPS here we have a problem, we should not use big integers contained in the given rationals itself
	//      but then these new big integer should be freed if we can't bind them

	// ASSERT _rational1 and _rational2 should not be NULL
	Mbiginteger *_den1=_getBigintegerCopy(_rational1->den),*_den2=_getBigintegerCopy(_rational2->den); // get the denominators
	Mbiginteger *_num1=_getBigintegerCopy(_rational1->num),*_num2=_getBigintegerCopy(_rational2->num); // get the numerators
	
	long double deltasum=(_rational1->delta&&_rational2->delta?_rational1->delta->ld+_rational2->delta->ld:(_rational1->delta?_rational1->delta->ld:(_rational2->delta?_rational2->delta->ld:M_LD_NAN))); // MDH@05JUN2019: compute the sum of the deltas
	
	// we can speed it up if certain elements are integer (because the denominator is NULL)
	// DONE problem to solve: the intermediate results should be freed if the rational could not be created!!
	// TODO we can speed up these shortcuts (see how we handled creation errors at the bottom!!!)
	if(!_den1&&!_den2){
		if(amVerbose())output("Both denominators in adding two rationals undefined!");
		Mbiginteger* _num=_Iadd(_num1,_num2,false);free_biginteger(_num1);free_biginteger(_num2); // don't need these anymore
		Mrational* _rational=_getRational(_num,NULL,deltasum,false,true);
		return _rational; // if both denominators are undefined (i.e. 1), return the sum of the numerators
	}
	if(!_den1){
		if(amVerbose())output("First denominator in adding two rationals undefined!");
		Mbiginteger* _mult=(_num1?_Imultiply(_num1,_den2,false):_den2);free_biginteger(_num1); // _num1 no longer needed
		Mbiginteger* _num=(_mult?_Iadd(_num2,_mult,false):NULL);free_biginteger(_num2); // _num2 no longer needed
		if(_num1)free_biginteger(_mult); // _mult no longer needed i.e. when it is not equal to _den2
		Mrational* _rational=NULL;
		if(_num){
			//////outputBiginteger("\nNew numerator: ",_num,".");
			_rational=_getRational(_num,_den2,deltasum,true,true); // free _num and _den2 if not bound to _rational
			//////outputRational("\nSum rational: ",_rational,".");
		}else
			free_biginteger(_den2);
		return _rational;
	}
	if(!_den2){
		if(amVerbose())output("Second denominator in adding two rationals undefined!");
		Mbiginteger* _mult=(_num1?_Imultiply(_num2,_den1,false):_den1);free_biginteger(_num2);
		Mbiginteger* _num=(_mult?_Iadd(_num1,_mult,false):NULL);free_biginteger(_num1);
		if(_num1)free_biginteger(_mult);
		Mrational* _rational=NULL;
		if(_num){
			//////outputBiginteger("\nNew numerator: ",_num,".");
			_rational=_getRational(_num,_den1,deltasum,true,true);
			/////outputRational("\nSum rational: ",_rational,".");
		}else 
			free_biginteger(_den1);
		return _rational;
	}
	// if the denominators are equal it's also easier
	if(mp_cmp(_den1,_den2)==MP_EQ){
		free_biginteger(_den2); // won't need it anymore
		Mbiginteger* _num=_Iadd(_num1,_num2,false);free_biginteger(_num1);free_biginteger(_num2);
		Mrational* _rational=NULL;
		if(_num){
			//////outputBiginteger("\nNew numerator: ",_num,".");
			_rational=_getRational(_num,_den1,deltasum,true,true);
			//////outputRational("\nSum rational: ",_rational,".");
		}else
			free_biginteger(_den1);
		return _rational;
	}

	if(amVerbose()){outputRational("\nAdding true rationals ",_rational1,NULL);outputRational(" and ",_rational2,".");}

	// true rational addition (as _den1 and _den2 are defined, i.e. unequal to 1)
	// TODO we can speed this up as well using ternary operators
	Mbiginteger* _den=_Imultiply(_den1,_den2,false);
	Mbiginteger* _mul1=(_den?_Imultiply(_num1,_den2,false):NULL);
	Mbiginteger* _mul2=(_den&&_mul1?_Imultiply(_num2,_den1,false):NULL);
	// free the original copies
	free_biginteger(_num1);free_biginteger(_num2);
	free_biginteger(_den1);free_biginteger(_den2);
	
	if(amVerbose()){outputBiginteger("\n\tNumerator part 1: ",_mul1,NULL);outputBiginteger(" - part 2: ",_mul2,".");}
	Mbiginteger* _num=(_mul1&&_mul2?_Iadd(_mul1,_mul2,false):NULL);
	free_biginteger(_mul1);free_biginteger(_mul2); // always free the intermediate results (even if we failed to add them)

	//////outputBiginteger("\nSum denominator: ",_den,".");
	if(_num){
		/////outputBiginteger("\nSum numerator: ",_num,".");
		_rational=_getRational(_num,_den,deltasum,true,true);
		/////outputRational("\n\tSum rational: ",_rational,".");
	}else{
		free_biginteger(_num);free_biginteger(_den);
	}
	*/
	return _rational;

}
Mrational* _qneg(Mrational* _rational){
	// normalizes the negated rational if not currently normalized, otherwise it will not normalize it
	Mrational* _rationalNeg=(_rational?_getRational(_getBigintegerNeg(_rational->num),_getBigintegerCopy(_rational->den),realneg(_rational->delta),!_rational->normalized,true):NULL);
	if(_rationalNeg)if(_rational->normalized)_rationalNeg->normalized=true; // if original assumed normalized, so is the negated value
	return _rationalNeg;
}
Mrational* _qsubtract(Mrational* _rational1,Mrational* _rational2){
	if(!_rational1||!_rational2)return NULL;
	Mrational* _rational2Neg=_qneg(_rational2); // get the negated rational2
	Mrational* _rational=_qadd(_rational1,_rational2Neg);
	free_rational(_rational2Neg); // free the negated rational2
	return _rational;
}
// end rational stuff

// decimal stuff
/* MDH@20JUN2019: by not using DP_value anymore, we solved the problem of DP_value holding a reference to the decimal precision value which apparently was released at some point
                  so as soon as the value pointer is released by the value list, a reference is still kept by DP_value but the memory will be reused and _integer might point into uncharted territory at some point in the future
				  if we were to keep using DP_value we should have called assignValue() to assign the value and not DP_value=_getIntegerValue() (see initEnvironment())
Mvalue* DP_value=NULL; 
*/
mpd_context_t* _decimalContext=NULL; // the application-wide decimal context
long long getDP(){
	if(!_decimalContext)_decimalContext=get_mpd_context(M_DP); // _decimalContext won't be created until it's actually needed (so other decimal contexts might be created before!!!!!)
	// better to get it directly out of the _decimalContext (as that holds the actual decimal context being used)
	long long dp=(_decimalContext?_decimalContext->prec:M_LL_INVALID); // replacing: long long dp=(DP_value?DP_value->value._integer->ll:M_LL_INVALID);
	if(dp==M_LL_INVALID)outputLine("BUG: No default decimal context active!");
	return dp;
}
Mvalue* setdp(Mvalue* _value){
	// how about returning the current value, no matter what the argument is????
	long long olddecimalprecision=getDP();
	// ignore if NO value specified...
	if(_value&&_value->type==VT_INTEGER){
		long long decimalprecision=_value->value._integer->ll;
		if(decimalprecision!=M_LL_INVALID){ // if not the default!!!
			if(decimalprecision>=6){
				// if I fail to create the associated decimal context, no go
				mpd_context_t* _newDecimalContext=get_mpd_context(decimalprecision);
				if(_newDecimalContext)
					_decimalContext=_newDecimalContext;
					////DP_value->value._integer->ll=_decimalContext->prec;
				else
					output("%sActive decimal context not replaced: failed to create a decimal context with precision %llu.\n",ERROR_PREFIX,decimalprecision);
			}else
				output("%sRequested decimal precision (%llu) not activated: it should at least be 6.\n",ERROR_PREFIX,decimalprecision);
		}
	}
	return _getIntegerValue(olddecimalprecision);
}
////////mpd_context_t* getDecimalContext(){if(_decimalContext)_decimalContext=get_mpd_context(getDP());return _decimalContext;}

Mdecimal* _dadd(Mdecimal* _decimal1,Mdecimal* _decimal2){
	Mdecimal* _result=__decimal(_decimalContext,0,0);
	mpd_add(_result->mpd,_decimal1->mpd,_decimal2->mpd,_decimalContext);
	return _result;
}

void report_mpd_status(mpd_context_t* mpd_context){
	uint32_t mpd_status=mpd_getstatus(mpd_context);
	if(mpd_status>0){
		outputLine("Decimal computations error report.");
		if(mpd_status&MPD_IEEE_Invalid_operation)outputLine("\tIEEE Invalid operation error.");
		if(mpd_status&MPD_Clamped)outputLine("\tClamped error.");
		if(mpd_status&MPD_Division_by_zero)outputLine("\tDivision by zero error.");
		if(mpd_status&MPD_Fpu_error)outputLine("\tFPU error.");
		if(mpd_status&MPD_Inexact)outputLine("\tInexact error.");
		if(mpd_status&MPD_Not_implemented)outputLine("\tNot implemented error.");
		if(mpd_status&MPD_Overflow)outputLine("\tOverflow error.");
		if(mpd_status&MPD_Rounded)outputLine("\tRounding error.");
		if(mpd_status&MPD_Subnormal)outputLine("\tSubnormal error.");
		if(mpd_status&MPD_Underflow)outputLine("\tUnderflow error.");
	}else
		outputLine("No decimal context errors.");
}
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
Mvalue* pi_ql(Mvalue* _value){
	if(_value&&_value->type==VT_INTEGER){
		long long maxiter=_value->value._integer->ll;
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
							Mrational* _newRational=_qadd(_rational,_addendumRational);
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
Mvalue* pi_q(Mvalue* _value){
	if(_value&&_value->type==VT_INTEGER){
		long long iter=_value->value._integer->ll;
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
						_result=_qadd(_rational,_inverseDenominatorRational);
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

// decimalerrorstatus() filter out the rounding and inexact 'errors'
bool mpd_error(mpd_context_t* mpd_context){return(mpd_getstatus(mpd_context)&0xEFBF)!=0;}

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
Mvalue* pi_d(Mvalue* _value){
	// _value should be a positive integer defining the required precision
	if(amVerbose())output("Computing pi using decimals.\n");
	long long decimalprecision=M_LL_INVALID;if(_value&&_value->type==VT_INTEGER)decimalprecision=_value->value._integer->ll;
	mpd_context_t* mpd_context=(decimalprecision>0?get_mpd_context(decimalprecision):NULL);
	if(!mpd_context){
		if(decimalprecision>0)output("%sFailed to create the requested decimal context. Will use the default instead.\n",ERROR_PREFIX);
		mpd_context=_decimalContext;
	}
	/* replacing:
	if(!mpd_context){if(decimalprecision>0)outputError("Failed to obtain the requested decimal context");else outputError("No (default) decimal context available");return NULL;}
	if(decimalprecision<0)decimalprecision=_decimalContext->prec;
	*/
	if(amVerbose())output("Computing pi to %lld decimals.\n",mpd_context->prec);

	// initialize the variables we need for the iterations
#ifdef __ADEBUG__
	Mdecimal *lasts=__decimal(mpd_context,0,0),*t=__decimal(mpd_context,3,0),*s=__decimal(mpd_context,3,0),*n=__decimal(mpd_context,1,0),*na=__decimal(mpd_context,0,0),*d=__decimal(mpd_context,0,0),*da=__decimal(mpd_context,24,0);
	// some constant decimals we need
	Mdecimal *d8=__decimal(mpd_context,8,0),*d32=__decimal(mpd_context,32,0);
#else
	mpd_t *lasts=__mpd(mpd_context,0),*t=__mpd(mpd_context,3),*s=__mpd(mpd_context,3),*n=__mpd(mpd_context,1),*na=__mpd(mpd_context,0),*d=__mpd(mpd_context,0),*da=__mpd(mpd_context,24);
	// some constant decimals we need
	mpd_t *d8=__mpd(mpd_context,8),*d32=__mpd(mpd_context,32);
#endif
	if(!lasts||!t||!s||!n||!na||!d||!da||!d8||!d32){outputError("Failed to create all helper decimals");return NULL;}
	if(amVerbose())output("Initial decimals created!\n");
	unsigned long long iter=0;
	if(amVerbose()){
		output("Iteration %u:",iter);
#ifdef __ADEBUG__
		char* _lasts=mpd_to_sci(lasts->mpd,0);output(" lasts=%s");free(_lasts);
		char* _t=mpd_to_sci(t->mpd,0);output(" t=%s",_t);free(_t);
		char* _s=mpd_to_sci(s->mpd,0);output(" s=%s",_s);free(_s);
		char* _n=mpd_to_sci(n->mpd,0);output(" n=%s",_n);free(_n);
		char* _na=mpd_to_sci(na->mpd,0);output(" na=%s",_na);free(_na);
		char* _d=mpd_to_sci(d->mpd,0);output(" d=%s",_d);free(_d);
		char* _da=mpd_to_sci(da->mpd,0);output(" da=%s",_da);free(_da);
#else
		char* _lasts=mpd_to_sci(lasts,0);output(" lasts=%s");free(_lasts);
		char* _t=mpd_to_sci(t,0);output(" t=%s",_t);free(_t);
		char* _s=mpd_to_sci(s,0);output(" s=%s",_s);free(_s);
		char* _n=mpd_to_sci(n,0);output(" n=%s",_n);free(_n);
		char* _na=mpd_to_sci(na,0);output(" na=%s",_na);free(_na);
		char* _d=mpd_to_sci(d,0);output(" d=%s",_d);free(_d);
		char* _da=mpd_to_sci(da,0);output(" da=%s",_da);free(_da);
#endif
		outputChar('\n');
	}
	int cmp;
	mpd_qsetprec(mpd_context,mpd_getprec(mpd_context)+2); // increment the precision by 2
	while(!mpd_error(mpd_context)){
		iter++;
#ifdef __ADEBUG__
		//if(amVerbose())output("Iteration: %lld: ",iter);
		cmp=mpd_cmp(lasts->mpd,s->mpd,mpd_context); // lasts == s ?
		//if(amVerbose()){output("a\t");if(mpd_error(mpd_context))break;}
		if(!cmp){if(amVerbose())output("Done!\n");break;}
		//if(amVerbose()){output("b\t");if(mpd_error(mpd_context))break;}
		if(cmp==INT_MAX){output("Something went wrong!\n");break;}
		//if(amVerbose()){output("c\t");if(mpd_error(mpd_context))break;output("lasts = (s) = %s",Mdecimalo_sci(s,0));}
		mpd_copy(lasts->mpd,s->mpd,mpd_context); // lasts = s
		//if(amVerbose()){output("d\t",Mdecimalo_sci(lasts,0));if(mpd_error(mpd_context))break;output("n = (n=%s) + (na=)%s",Mdecimalo_sci(n,0),Mdecimalo_sci(na,0));}
		mpd_add(n->mpd,n->mpd,na->mpd,mpd_context);
		//if(amVerbose()){output(" = %s\ne\t",Mdecimalo_sci(n,0));if(mpd_error(mpd_context))break;output("na = (na=%s) + 8",Mdecimalo_sci(na,0));}
		mpd_add(na->mpd,na->mpd,d8->mpd,mpd_context); // increment n by na and na by 8
		//if(amVerbose()){output(" = %s\nf\t",Mdecimalo_sci(na,0));if(mpd_error(mpd_context))break;output("d = (d=%s) + (da=%s)",Mdecimalo_sci(d,0),Mdecimalo_sci(da,0));}
		mpd_add(d->mpd,d->mpd,da->mpd,mpd_context);
		//if(amVerbose()){output(" = %s\ng\t",Mdecimalo_sci(d,0));if(mpd_error(mpd_context))break;output("da = (da=%s) + 32",Mdecimalo_sci(da,0));}
		mpd_add(da->mpd,da->mpd,d32->mpd,mpd_context); // increment d by da and da by 32
		//if(amVerbose()){output(" = %s\nh\t",Mdecimalo_sci(da,0));if(mpd_error(mpd_context))break;output("t = (t=%s) * (n=%s)",Mdecimalo_sci(t,0),Mdecimalo_sci(n,0));}
		mpd_mul(t->mpd,t->mpd,n->mpd,mpd_context);
		//if(amVerbose()){output(" = %s\ni\t",Mdecimalo_sci(t,0));if(mpd_error(mpd_context))break;output("t = (t=%s) / (n=%s)",Mdecimalo_sci(t,0),Mdecimalo_sci(d,0));}
		mpd_div(t->mpd,t->mpd,d->mpd,mpd_context); // multiply t by n and divide t by d
		//if(amVerbose()){output(" = %s\nj\t",Mdecimalo_sci(t,0));if(mpd_error(mpd_context))break;output("s = (s=%s) + (t=%s)",Mdecimalo_sci(s,0),Mdecimalo_sci(t,0));}
		mpd_add(s->mpd,s->mpd,t->mpd,mpd_context); // add t to s
		//if(amVerbose()){output(" = %s\nk\t",Mdecimalo_sci(s,0));if(mpd_error(mpd_context))break;}
		if(amVerbose()){
			output("Iteration %u:",iter);
			char* _lasts=mpd_to_sci(lasts->mpd,0);output(" lasts=%s");free(_lasts);
			char* _t=mpd_to_sci(t->mpd,0);output(" t=%s",_t);free(_t);
			char* _s=mpd_to_sci(s->mpd,0);output(" s=%s",_s);free(_s);
			char* _n=mpd_to_sci(n->mpd,0);output(" n=%s",_n);free(_n);
			char* _na=mpd_to_sci(na->mpd,0);output(" na=%s",_na);free(_na);
			char* _d=mpd_to_sci(d->mpd,0);output(" d=%s",_d);free(_d);
			char* _da=mpd_to_sci(da->mpd,0);output(" da=%s",_da);free(_da);
			outputChar('\n');
		}
#else
		//if(amVerbose())output("Iteration: %lld: ",iter);
		cmp=mpd_cmp(lasts,s,mpd_context); // lasts == s ?
		//if(amVerbose()){output("a\t");if(mpd_error(mpd_context))break;}
		if(!cmp){if(amVerbose())output("Done!\n");break;}
		//if(amVerbose()){output("b\t");if(mpd_error(mpd_context))break;}
		if(cmp==INT_MAX){output("Something went wrong!\n");break;}
		//if(amVerbose()){output("c\t");if(mpd_error(mpd_context))break;output("lasts = (s) = %s",Mdecimalo_sci(s,0));}
		mpd_copy(lasts,s,mpd_context); // lasts = s
		//if(amVerbose()){output("d\t",Mdecimalo_sci(lasts,0));if(mpd_error(mpd_context))break;output("n = (n=%s) + (na=)%s",Mdecimalo_sci(n,0),Mdecimalo_sci(na,0));}
		mpd_add(n,n,na,mpd_context);
		//if(amVerbose()){output(" = %s\ne\t",Mdecimalo_sci(n,0));if(mpd_error(mpd_context))break;output("na = (na=%s) + 8",Mdecimalo_sci(na,0));}
		mpd_add(na,na,d8,mpd_context); // increment n by na and na by 8
		//if(amVerbose()){output(" = %s\nf\t",Mdecimalo_sci(na,0));if(mpd_error(mpd_context))break;output("d = (d=%s) + (da=%s)",Mdecimalo_sci(d,0),Mdecimalo_sci(da,0));}
		mpd_add(d,d,da,mpd_context);
		//if(amVerbose()){output(" = %s\ng\t",Mdecimalo_sci(d,0));if(mpd_error(mpd_context))break;output("da = (da=%s) + 32",Mdecimalo_sci(da,0));}
		mpd_add(da,da,d32,mpd_context); // increment d by da and da by 32
		//if(amVerbose()){output(" = %s\nh\t",Mdecimalo_sci(da,0));if(mpd_error(mpd_context))break;output("t = (t=%s) * (n=%s)",Mdecimalo_sci(t,0),Mdecimalo_sci(n,0));}
		mpd_mul(t,t,n,mpd_context);
		//if(amVerbose()){output(" = %s\ni\t",Mdecimalo_sci(t,0));if(mpd_error(mpd_context))break;output("t = (t=%s) / (n=%s)",Mdecimalo_sci(t,0),Mdecimalo_sci(d,0));}
		mpd_div(t,t,d,mpd_context); // multiply t by n and divide t by d
		//if(amVerbose()){output(" = %s\nj\t",Mdecimalo_sci(t,0));if(mpd_error(mpd_context))break;output("s = (s=%s) + (t=%s)",Mdecimalo_sci(s,0),Mdecimalo_sci(t,0));}
		mpd_add(s,s,t,mpd_context); // add t to s
		//if(amVerbose()){output(" = %s\nk\t",Mdecimalo_sci(s,0));if(mpd_error(mpd_context))break;}
		if(amVerbose()){
			output("Iteration %u:",iter);
			char* _lasts=mpd_to_sci(lasts,0);output(" lasts=%s");free(_lasts);
			char* _t=mpd_to_sci(t,0);output(" t=%s",_t);free(_t);
			char* _s=mpd_to_sci(s,0);output(" s=%s",_s);free(_s);
			char* _n=mpd_to_sci(n,0);output(" n=%s",_n);free(_n);
			char* _na=mpd_to_sci(na,0);output(" na=%s",_na);free(_na);
			char* _d=mpd_to_sci(d,0);output(" d=%s",_d);free(_d);
			char* _da=mpd_to_sci(da,0);output(" da=%s",_da);free(_da);
			outputChar('\n');
		}
#endif
	}
	if(mpd_context)mpd_qsetprec(mpd_context,mpd_getprec(mpd_context)-2); // decrement the precision by 2
	// get rid of all the decimals we used
#ifdef __ADEBUG__
	free_decimal(lasts);free_decimal(t);free_decimal(n);free_decimal(na);free_decimal(d);free_decimal(da);
	free_decimal(d8);free_decimal(d32);
#else
	mpd_del(lasts);mpd_del(t);mpd_del(n);mpd_del(na);mpd_del(d);mpd_del(da);
	mpd_del(d8);mpd_del(d32);
#endif
	if(mpd_context){
		if(mpd_error(mpd_context)){ // something went wrong
			if(amVerbose())output("%sComputation of pi with precision %lld error status: %u.\n",ERROR_PREFIX,decimalprecision,mpd_getstatus(mpd_context));
			report_mpd_status(mpd_context);
			return NULL;
		}
	}
	// success
#ifdef __ADEBUG__
	mpd_finalize(s->mpd,mpd_context?mpd_context:_decimalContext); // to round to the requested precision
#else
	mpd_finalize(s,mpd_context?mpd_context:_decimalContext); // to round to the requested precision
#endif
	if(amVerbose()){
#ifdef __ADEBUG__
		char* _s=mpd_to_sci(s->mpd,0);
#else
		char* _s=mpd_to_sci(s,0);
#endif
		if(_s){output("Final approximation of pi (rounded to %llu decimals): %s.\n",decimalprecision,_s);free(_s);}
	}
	// wrap the mpd_t in a decimal, and subsequently in an Mvalue!!!
#ifdef __ADEBUG__
	return _getDecimalValue(s,true);
#else
	return _getDecimalValue(_getDecimal(s,0,true),true);
#endif
}

// how about storing all results here?????? instead of in the root environment????
Mvalue* _resultListValue=NULL; // were the results are being kept
// the function that is used to return a specific result value
Mvalue* getResult(Mvalue* _indexValue){
	if(amVerbose())outputLine("Result requested!");
	if(!_indexValue)return _resultListValue;
	long long indexValueInteger=getValueInteger(_indexValue); // NOTE all index values should be positive!!!
	return (indexValueInteger>0?getValueAtIndex(_resultListValue->value._list,indexValueInteger):NULL); // TODO are we calling getResult anywhere????
}

// LIST CONVERSIONS
////////Mvalue* ml(Menvironment* _executionEnvironment){return _getListValue(VT_LIST);} // a list that may only contain list elements is acceptable as map list!!

// list to map
Mvalue* l2m(Mvalue* _value){
	Mvalue* _mapValue=NULL;
	if(_value&&_value->type==VT_LIST){
		_mapValue=_getMapValue(_value->type); // create a map that is of the same type as the list is (typically VT_UNDEFINED)
		if(!listAppendedToMap(_mapValue->value._map,_value->value._list))return NULL; // TODO should we 'release' the map that was created somehow???? I guess the map not getting assigned will be released somehow automatically...
	}
	return _mapValue;
}
// list to map list
Mvalue* l2ml(Mvalue* _value){
	Mvalue* _maplistValue=NULL;
	if(_value&&_value->type==VT_LIST){
		_maplistValue=_getListValue(VT_LIST); // a map list ALWAYS requires element of type VT_LIST
		if(!listAppendedToMaplist(_maplistValue->value._list,_value->value._list))return NULL; // TODO should we 'release' the map that was created somehow???? I guess the map not getting assigned will be released somehow automatically...
	}
	return _maplistValue;
}
// map list to list conversion
Mvalue* ml2l(Mvalue* _value){
	Mvalue* _maplistValue=NULL;
	if(_value&&_value->type==VT_LIST){
		_maplistValue=_getListValue(_value->type); // create a map that is of the same type as the list is (typically VT_UNDEFINED)
		if(!maplistAppendedToList(_maplistValue->value._list,_value->value._list))return NULL; // TODO should we 'release' the map that was created somehow???? I guess the map not getting assigned will be released somehow automatically...
	}
	return _maplistValue;
}
Mvalue* ml2m(Mvalue* _value){
	Mvalue* _mapValue=NULL;
	if(_value&&_value->type==VT_LIST){
		_mapValue=_getMapValue(_value->type); // create a map that is of the same type as the list is (typically VT_UNDEFINED)
		if(!maplistAppendedToMap(_mapValue->value._map,_value->value._list))return NULL; // TODO should we 'release' the map that was created somehow???? I guess the map not getting assigned will be released somehow automatically...
	}
	return _mapValue;
}

// map to map list conversion i.e. each list element is a attribute name - value pair
Mvalue* m2ml(Mvalue* _value){
	Mvalue* _maplistValue=NULL;
	if(_value&&_value->type==VT_MAP){
		_maplistValue=_getListValue(VT_LIST); // a map list should always have element of type VT_LIST (this is the only additional requirement for a list to be accepted as map lists)
		if(!mapAppendedToMaplist(_maplistValue->value._list,_value->value._map))return NULL; // TODO should we release the list that was created somehow????
	}
	return _maplistValue;
}
Mvalue* m2l(Mvalue* _value){
	Mvalue* _listValue=NULL;
	if(_value&&_value->type==VT_MAP){
		_listValue=_getListValue(_value->type);
		if(!mapAppendedToList(_listValue->value._list,_value->value._map))return NULL; // TODO should we release the list that was created somehow????
	}
	return _listValue;
}
// conversion functions
 // the value wrapper for not a real and not an integer...
Mvalue* NAR_value=NULL;
Mvalue* NAI_value=NULL;
Mvalue* NULL_value=NULL; // the value containing the text to show when a value equals NULL

long double getNAR(){return NAR_value->value._real->ld;}
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
	while(--l>=0&&appendedToList(_dlist,_getIntegerValue(llu.octets[l]),(isLittleEndian()&&littleEndianOrder?l+1:0))>0);
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
Mvalue* getRealDecimalListValue(long double ld,bool littleEndianOrder){
	Mlist* _dlist=_getListOfType(VT_INTEGER);
	if(!_dlist)return NULL;
	longdoubleunion lld;
	lld.ld=ld;
	int l=sizeof(long double);if(l>10)l=10; // assume 10-byte extended precision if sizeof(long double) exceeds 10 (like 12 or 16)
	// how about adding a two-element list with the first equal to the field name?????
	while(--l>=0&&appendedToList(_dlist,_getIntegerValue(lld.octets[l]),(isLittleEndian()&&littleEndianOrder?l+1:0))>0);
	return _getValueOfList(_dlist,true);
}

// we need d to compute the decimal from a given value instead of digitizing, so I suppose we'll rename d to b (for getting the bytes)
Mvalue* d(Mvalue* _value){
	if(_value){
		switch(_value->type){
			// TODO all other types
			case VT_RATIONAL:return _getDecimalValue(_getRationalDecimal(_value->value._rational),true);
			default:break;
		}
	}
	return NULL;
}
Mvalue* b(Mvalue* _value){ // little-endian representation list to return
	if(_value){
		switch(_value->type){
			case VT_INTEGER:return getIntegerDecimalListValue(_value->value._integer->ll,true);
			case VT_REAL:return getRealDecimalMapValue(_value->value._real->ld,true);
			default:break;
		}
	}
	return NULL;
} 

Mvalue* B(Mvalue* _value){ // big endian decimal representation list to return
	if(_value){
		switch(_value->type){
			case VT_INTEGER:return getIntegerDecimalListValue(_value->value._integer->ll,false);
			case VT_REAL:return getRealDecimalMapValue(_value->value._real->ld,false);
			default:break;
		}
	}
	return NULL;
}
// TODO to add h/H and b/B functions

Mvalue* i(Mvalue* _value){
	if(amVerbose())outputValue("\nConverting '",_value,"' to an integer.");
	long long ll=getValueInteger(_value);
	return(ll!=M_LL_INVALID?_getIntegerValue(ll):NULL);
}

// convert to a big integer
Mvalue* I(Mvalue* _value){
	if(_value){
		if(_value->type==VT_BIGINTEGER)return _value; // already a big integer
		Mbiginteger* _bigInteger=_getValueBiginteger(_value);
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
			case VT_REAL:
				_rational=_getLongDoubleRational(_value->value._real->ld,250); // TODO how many iterations at most???
				break;
			case VT_LIST:
				if(_value->value._list->numberOfElements>1)
					_rational=_getRational(_getValueBiginteger(_value->value._list->_first->_value),_getValueBiginteger(_value->value._list->_first->_next->_value),
											(_value->value._list->numberOfElements>2?getValueReal(_value->value._list->_first->_next->_next->_value):M_LD_NAN),true,false);
				break;
			default:break;
		}
	}
	return _rational;
}

Mdecimal* _getValueDecimal(Mvalue* _value){
	Mdecimal* _decimal=NULL;
	if(_value){
		if(_value->type!=VT_LIST&&_value->type!=VT_MAP){
			switch(_value->type){
				case VT_DECIMAL:_decimal=_getDecimalCopy(_value->value._decimal);break;
				case VT_INTEGER:_decimal=_getDecimal(__mpd(_decimalContext,_value->value._integer->ll),0,true);break;
				case VT_RATIONAL:
					{ // a rational text representation still contains the numerator/denominator pair, so can't be parsed into a decimal
						// TODO we need to find the decimal approximation with precision equal to the default decimal precision
					}
					break;
				default:
					{ // TODO: use _getTextDecimal instead!!!
						Mstring* _valueText=_getValueText(_value,true);
						if(_valueText){mpd_set_string(_decimal->mpd,string(_valueText),_decimalContext);free_string(_valueText);}
					}
					break;
			}
		}
	}
	return _decimal;
}

// TODO how many iterations would we accept at most?????
Mvalue* Q(Mvalue* _value){
	if(!_value)return NULL;
	if(_value->type==VT_RATIONAL)return _value; // if the value holds a rational itself, return just that
	if(_value->type==VT_REAL)return _getValueOfList(_getLongDoubleRationalList(_value->value._real->ld,250),true); // the intermediate results are stored in a list, and the last element will be the final result!!!
	Mvalue* _rationalValue=_getRationalValue(_getValueRational(_value),true); // make a rational from it and wrap it again
	if(amVerbose())outputValue("Converted to rational '",_rationalValue,"'.");
	return _rationalValue;
}
Mvalue* q(Mvalue* _value){
	if(!_value)return NULL;
	if(_value->type==VT_RATIONAL)return _value; // if the value holds a rational itself, return just that
	if(_value->type==VT_REAL)return _getRationalValue(_getLongDoubleRational(_value->value._real->ld,250),true); // forcefully free the _getLongDoubleRational if we failed to wrap it
	Mvalue* _rationalValue=_getRationalValue(_getValueRational(_value),true); // make a rational from it and wrap it again
	if(amVerbose())outputValue("Converted to rational '",_rationalValue,"'.");
	return _rationalValue;
}

// convert to a real

long double getDecimalLongDouble(Mdecimal* _decimal){
	// easiest way is to transform to text first, and take if from there...
	long double ldDecimal=M_LD_NAN;
	if(_decimal){
		char* _decimalText=mpd_to_sci(_decimal->mpd,0); // NOTE do NOT use _getDecimalText here!!!
		if(_decimalText){ldDecimal=_strtold(_decimalText,ldDecimal);free(_decimalText);} // no need for this anymore
	}
	return ldDecimal;
}
// TODO complete with conversion from big integer and rational
Mvalue* r(Mvalue* _value){
	Mvalue* _realValue=NAR_value;
	if(_value){
		if(amVerbose()){outputValue("Converting '",_value,"'");output(" of type %s to a real.\n",VALUETYPENAMES[_value->type]," to a real.\n");}
		switch(_value->type){
			case VT_INTEGER:_realValue=_getRealValue((long double)_value->value._integer->ll);break;
			case VT_BIGINTEGER:_realValue=_getRealValue(mp_get_long_double(_value->value._biginteger));break;
			case VT_DECIMAL:_realValue=_getRealValue(getDecimalLongDouble(_value->value._decimal));break;
			case VT_RATIONAL:_realValue=_getRealValue(getRationalLongDouble(_value->value._rational));break;
			case VT_REAL:_realValue=_value;break; // TODO should we make a copy here? NO, Mvalue* instances don't need to be duplicated because they are immutable
			case VT_TEXT:_realValue=_getRealValue(_strtold(_value->value._text->_c,getNAR()));break;
			default:break;
		}
	}
	if(amVerbose())outputValue("Converted to '",_realValue,"'.\n");
	return _realValue;
}
// the type of a value
Mvalue* t(Mvalue* _value){
	if(_value)
	switch(_value->type){
		case VT_TOKEN:return _getTextValue("'t",false);
		case VT_INTEGER:return _getTextValue("'i",false);
		case VT_BIGINTEGER:return _getTextValue("'I",false);
		case VT_DECIMAL:return _getTextValue("'d",false);
		case VT_RATIONAL:return _getTextValue("'q",false);
		case VT_REAL:return _getTextValue("'r",false);
		case VT_TEXT:return _getTextValue("'s",false);
		case VT_LIST:return _getTextValue("'l",false);
		case VT_MAP:return _getTextValue("'m",false);
		case VT_UNDEFINED:return _getTextValue("'u",false);
		/////case VT_USERFUNCTION:return _getTextValue("'f",false);
	}
	return NULL;
}

Mvalue* add(Mvalue* _value1,Mvalue* _value2);
Mvalue* Msum(Mvalue* _value){
    if(_value){
		if(amVerbose())outputValue("\nComputing the sum of '",_value,"'.");
        if(_value->type!=VT_LIST)return _value;
				// all the values in the list could be integer
				Mlist* _list=_value->value._list;
				if(_list){
					Mlistelement* _listelement=_list->_first;
					if(_listelement){
						// what if all the elements are integer????
						Mvalue* _sumValue=_getRealValue(0);
						while(_listelement){_sumValue=add(_sumValue,_listelement->_value);_listelement=_listelement->_next;}
						return _sumValue;
					}
				}
    }
    return NULL;
}

Menvironment* _Menvironment; // this is the root (M) environment
///// NOT HERE see Mexecution.c!!!! Menvironment* _executionEnvironment=NULL; // the current execution environment (in which functions are called!!!)

Mtoken* _getToken(Mtoken* prevToken);

bool initEnvironment(){

	long long decimalprecision=getDP();
	if(decimalprecision==M_LL_INVALID)return false; // let's force starting with a default decimal context
	output("Default decimal precision: %llu. Call setdp() to change it.\n",decimalprecision);

	NAR_value=_getRealValue(M_LD_NAN); // NaN is defined in Mexecution.h as 0.0/0.0 (as a constant)
	NAI_value=_getIntegerValue(M_LL_INVALID);
	NULL_value=_getValueOfToken(_getToken(NULL),true);NULL_value->value._token->text=__string("NULL");NULL_value->value._token->type=TT_SQSTRING; // any string type would do!!!

	// either set the DP_value to 0 (failed to get a decimal context somehow)
	/* MDH@20JUN2019: no need for DP_value anymore (as setdp() return _decimalContext->prec now): 
	_decimalContext=get_mpd_context(M_DP); // initialize the application-wide decimal context with precision M_DP
	assignValue(DP_value,_getIntegerValue(_decimalContext?_decimalContext->prec:0L));
	if(!DP_value)output("WARNING: Failed to initialize the decimal precision.");
	*/

	_resultListValue=_getListValue(VT_UNDEFINED); // ascertain to have a list value in which the results can be stored
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
			if(!addVariable(_Menvironment,"NULL",VT_TOKEN,true)||!setValue(_Menvironment,"NULL",NULL_value)){
				outputLine("WARNING: Failed to create, add or initialize constant NULL.");
			}
			if(!NAR_value||!addVariable(_Menvironment,"NAR",VT_REAL,true)||!setValue(_Menvironment,"NAR",NAR_value)){
				outputLine("WARNING: Failed to create, add or initialize Not-a-real constant NAR.");
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
			Mvalue* PI_value=_getRealValue(LD_PI);
			if(!PI_value){
				outputLine("ERROR: Failed to create PI.");
				return false;
			}
			if(!addVariable(_Menvironment,"PI",VT_REAL,true)){
				outputLine("ERROR: Failed to add PI.");
				///////free_value(PI_value);
				return false;
			}
			if(!setValue(_Menvironment,"PI",PI_value)){
				////////free_value(PI_value);
				outputLine("ERROR: Failed to initialize PI.");
				return false;
			}
			Mvalue* E_value=_getRealValue(LD_E);
			if(!E_value){
				///////free_value(E_value);
				outputLine("ERROR: Failed to create E.");
				return false;
			}
			if(!addVariable(_Menvironment,"E",VT_REAL,true)){
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
			if(!completedIntegerFunction(_getFunction(_Menvironment,"setdp"),"setdp",setdp)){
				outputError("Failed to register the setdp function");
				return false;
			}
			// pi() functions (decimal and rational)
			if(!completedIntegerFunction(_getFunction(_Menvironment,"pi$q"),"pi$q",pi_q)||!completedIntegerFunction(_getFunction(_Menvironment,"pi$ql"),"pi$ql",pi_ql)||!completedIntegerFunction(_getFunction(_Menvironment,"pi"),"pi",pi_d)){
				outputError("Failed to register the pi, pi$q and pi$ql functions");
				return false;
			}
			// conversions
			if(!completedValueFunction(_getFunction(_Menvironment,"i"),"i",i)||!completedValueFunction(_getFunction(_Menvironment,"I"),"I",I)
					||!completedValueFunction(_getFunction(_Menvironment,"t"),"t",t)
					||!completedValueFunction(_getFunction(_Menvironment,"r"),"r",r)
					||!completedValueFunction(_getFunction(_Menvironment,"q"),"q",q)||!completedValueFunction(_getFunction(_Menvironment,"Q"),"Q",Q)
					||!completedValueFunction(_getFunction(_Menvironment,"d"),"d",d)
					||!completedValueFunction(_getFunction(_Menvironment,"b"),"b",b)||!completedValueFunction(_getFunction(_Menvironment,"B"),"B",B)){
				outputError("Failed to register value type conversion functions");
				return false;
			}
			if(!completedValueFunction(_getFunction(_Menvironment,"neg"),"neg",Mneg)||!completedValueFunction(_getFunction(_Menvironment,"bnot"),"bnot",Mbnot)||!completedValueFunction(_getFunction(_Menvironment,"not"),"not",Mnot)){
				outputError("Failed to register all unary functions");
				return false;
			}
			if(!completedValueFunction(_getFunction(_Menvironment,"exists"),"exists",Mexists)||!completedValueFunction(_getFunction(_Menvironment,"null"),"null",Mnull)||!completedValueFunction(_getFunction(_Menvironment,"undefined"),"undefined",Mundefined)){
				outputError("Failed to register the null and undefined function");
				return false;
			}
			if(!completedValueFunction(_getFunction(_Menvironment,"sum"),"sum",Msum)||!completedValueFunction(_getFunction(_Menvironment,"len"),"len",Mlen)){
				outputError("Failed to register all list functions");
				return false;
			}
			if(!completedValueFunction(_getFunction(_Menvironment,"fac"),"fac",Mfac)||!completedValueFunction(_getFunction(_Menvironment,"facd"),"facd",Mfacd)){
				outputError("Failed to register the fac and facd function");
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
	if(!pushExecutionEnvironment(_Menvironment)){
		outputError("Failed to register the M environment as execution environment");
		return false;
	}
	return true;
}

// user interaction stuff
#include "Msession.h"

/* sometimes we want to preformat text
char* getFormattedText(char* fmt,uint8_t maxlength,...){
	char str[maxlength+1];
	va_list args;va_start(args,fmt);sprintf(str,fmt,args);va_end(args);
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
	Mstring* _functionsText=_getFunctionMapText(_Menvironment->_functionMap);
	output("\nFunctions: %s.\n",string(_functionsText));
	free_string(_functionsText);
}/* VALIDATED */
void outputVariables(){
	// much easier now that we get the text of any Mvalue (like the variable map of an environment!)
	Mstring* _variablesText=_getMapText(_Menvironment->_variableMap,false,false,false);
	output("\nVariables: %s.\n",string(_variablesText));
	free_string(_variablesText);
}/* VALIDATED */

enum INPUTMODE_ENUM {IM_COMMAND,IM_CONTROL,IM_SHELL}; // the possible input modes: command, control, and shell

enum INPUTMODE_ENUM inputMode=IM_COMMAND; // whether or not in command mode

char* promptinfo[]={"Command mode: cancel the input text with Ctrl-C.","Control mode: Flags: Assist|color scheme (0 or 1)|Debug|Match parentheses|Wrap|Use history command - Options: Reset|eXit|Functions|History|Shell|Variables.","Shell mode: enter a system command to execute."};
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
	output("Edit flags: %c%c%c%c%c - Display flags: %c%c.\n",amAssisting()?'A':'a',amDebugging()?'D':'d',amMatchingparentheses()?'M':'m',amVerbose()?'s':'S',amAcceptinghistorycommand()?'U':'u',amWrapping()?'W':'w',48+getColorscheme());
}

void outputFlags(){
	output("%c%c%c%c%c%c%c",amAssisting()?'A':'a',(48+getColorscheme()),amDebugging()?'D':'d',amMatchingparentheses()?'M':'m',amVerbose()?'s':'S',amWrapping()?'W':'w',amAcceptinghistorycommand()?'U':'u');
}

// MDH@19JUL2019: in order to be able to obtain the body code of functions we're keeping a stack of function names of which the body is requested
typedef struct FunctionBodyRequest{
	char* functionName;
	struct FunctionBodyRequest *_next,*_prev;
}FunctionBodyRequest;
// requests can come out of a single command containing multiple function definitions
FunctionBodyRequest *_firstFunctionBodyRequest=NULL,*_lastFunctionBodyRequest=NULL;
FunctionBodyRequest* requestBodyOfFunction(char* functionName){
	if(functionName&&strlen(functionName)){ // a 'valid' function name
		if(amVerbose())output("The body of function '%s' being requested.\n",functionName);
		FunctionBodyRequest* _functionBodyRequest=(functionName?CALLOC(1,sizeof(FunctionBodyRequest),'B'):NULL);
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

/*
\brief returns the environment for executing the the function called \p functionName
\p functionName the name of the function to execute
obviously when defining the function body there will be no commands to execute
 */
Menvironment* _getFunctionExecutionEnvironment(Mfunction* _function,char* functionName,Mmap* _argumentMap){
	// 1. create an environment in which to execute the expression list of the given function initialized with the argument map provided with the current argument variable values
	Menvironment* _functionExecutionEnvironment=__environment(); // free asap
	if(_functionExecutionEnvironment){
		bool functionExecutionEnvironmentInitialized=true;
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
		Mmapelement* argumentMapelement=_argumentMap->_first;
		Mvariable* argumentMapelementVariable;
		while(functionExecutionEnvironmentInitialized&&argumentMapelement){
			argumentMapelementVariable=argumentMapelement->_variable;
			if(!addVariable(_functionExecutionEnvironment,argumentMapelementVariable->_name,argumentMapelementVariable->valuetype,false)){
				outputError("Failed to add function argument as local variable of a function execution");
				functionExecutionEnvironmentInitialized=false;
			}else
			if(!setValue(_functionExecutionEnvironment,argumentMapelementVariable->_name,argumentMapelementVariable->_value)){
				outputError("Failed to initialize the argument local variable of a function execution");
				functionExecutionEnvironmentInitialized=false;
			}else
				argumentMapelement=argumentMapelement->_next;
		}
		// add the result variable ($ or perhaps later a variable with empty name????) TODO make a predefined constant char* out of it
		if(!addVariable(_functionExecutionEnvironment,"$",VT_UNDEFINED,false)){
			outputError("Failed to add the result variable to the function execution environment");
			functionExecutionEnvironmentInitialized=false;
		}
		if(functionExecutionEnvironmentInitialized)return _functionExecutionEnvironment;
		free_environment(_functionExecutionEnvironment);
	}
	return NULL;
}

typedef struct FunctionBodyInput{
	////////char* functionName;
	Muserfunction* _function;
	struct FunctionBodyInput *_prev;
}FunctionBodyInput;
FunctionBodyInput *_functionBodyInputStack=NULL,*_currentFunctionBodyInput=NULL; // the stack of function bodies being constructed
bool setCurrentFunctionBodyInput(FunctionBodyRequest* _functionBodyRequest){
	if(!_functionBodyRequest)return false;
	_currentFunctionBodyInput=CALLOC(1,sizeof(FunctionBodyInput),'I'); // free if not bound
	if(!_currentFunctionBodyInput){outputError("Failed to create function body input");return false;} // TODO improve feedback
	Mfunction* function=getFunction(getEnvironment(),_functionBodyRequest->functionName);
	if(function&&function->type==FT_USER){		
		_currentFunctionBodyInput->_function=function->functionunion._userfunction;
		if(!_functionBodyInputStack)_functionBodyInputStack=_currentFunctionBodyInput;
		// if we succeed in activating the execution environment of the new function we're good to go
		// we can use the functions parameterMap as argumentMap (providing the defaults to use for executing the newly entered body commands)
		Menvironment* _functionExecutionEnvironment=_getFunctionExecutionEnvironment(function,_functionBodyRequest->functionName,function->_parameterMap);
		if(_functionExecutionEnvironment){
			if(pushExecutionEnvironment(_functionExecutionEnvironment))return true;
			free_environment(_functionExecutionEnvironment);
		}
		outputError("Failed to create function execution environment for accepting its body commands"); // TODO improve feedback
	}else
		output("%sCan't find function '%s' for accepting its body commands.\n",ERROR_PREFIX,_functionBodyRequest->functionName);
	free(_currentFunctionBodyInput);
	return false;
}
bool startFunctionBodyInput(){
	// move out of the queue into the stack
	// push on top of the functionBodyInputStack
	if(!_firstFunctionBodyRequest)return false; // NO function body request to 'execute'
	if(setCurrentFunctionBodyInput(_firstFunctionBodyRequest))return true;
	output("%sFailed to start input of the body of function '%s'.\n",ERROR_PREFIX,_firstFunctionBodyRequest->functionName);
	return false;
}
/*
 \brief will only fail when we fail to start the next one
 */
bool endFunctionBodyInput(){
	// pop the function body request execution environment we just ended
	// MDH@20JUL2019: I need to get a reference to the execution environments function map (before the execution environment get's freed and we loose the reference!!)
	_currentFunctionBodyInput->_function->_functionMap=getEnvironment()->_functionMap;
	popExecutionEnvironment();
	_firstFunctionBodyRequest=_firstFunctionBodyRequest->_next;
	if(!_firstFunctionBodyRequest){_lastFunctionBodyRequest=NULL;return true;} // done with all the requests
	return startFunctionBodyInput(); // start the next one
}
// MDH@19JUL2019 END

// keeping track of the command count, the cursor position and the prompt length (so we can write information messages on the line above where the prompt is)
long long commandCount=0; // the total number of command input
long long commandIndex=0;

Mstring* behindCursorText=NULL; // MDH@27FEB2019: we keep track of the characters behind the cursor
Mstring* shellCommand=NULL;
Mtoken* pCommandToEvaluate=NULL;
Mtoken* pLastCommandToEvaluateToken=NULL; // the last token in the sequence of tokens starting with pCommandToEvaluate

// keeping track of both the cursor position and the total command length
uint16_t cursorPosition(){return(inputMode==IM_COMMAND?(pLastCommandToEvaluateToken?pLastCommandToEvaluateToken->offset+string_length(pLastCommandToEvaluateToken->text):0):(inputMode==IM_SHELL?string_length(shellCommand):0));}
uint16_t behindCursor(){return string_length(behindCursorText);}
uint16_t commandLength(){return cursorPosition()+behindCursor();}

uint8_t promptLength=0;
void prompt(){
	resetOutputColor();
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
				if(_firstFunctionBodyRequest)
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
	/* MDH@26FEB2019: we do not need the following because that's taken care of in writeTokens(pCommandToEvaluate) right after promptForUserInput()
	cursorPosition()=0; // starting at position 0
	*/
}

void promptForUserInput(){
	enableRawmode();
	resetOutputColor();
	output("\n%s\n",promptinfo[inputMode]); // show the appropriate input mode prompt info
	prompt();
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
const uint8_t TOKENTYPE_IDS[NUMBER_OF_TOKEN_TYPES]={0,0b01010000,0b01000000,0b01100000,0b01100101,0b01101010,0b01100110,0b01101000,0b01110000,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,0b1000000,0b11111111};

const char* getTokenColor(enum TOKENTYPE_ENUM tokenType){
	uint8_t tokentype_id=TOKENTYPE_IDS[tokenType];
	///////printf("(%d)",tokentype_id);
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
	///////printf("[%d]",pLastCommandToEvaluateToken->type);
	// ah, the token colors will be a problem with the new type definitions, I suppose we need to distinguish between the operator and non-operator tokens	
}
void outputToken(Mtoken* _token){
	if(!_token)return;
	outputTokenColor(_token);
	// if we allow comments in tokens we're in trouble!!!
	output("%s",string(_token->text));
	/////////if(amAssisting()){resetOutputColor();outputChar('|');}
}
void outputLastTokenChar(Mtoken* _token){
	///////outputTokenColor(pLastCommandToEvaluateToken);
	outputChar(string_last_char(_token->text));
	//////////resetOutputColor();
}
// MDH@30APR2019: when a function returns to a variable and the other way round
void reoutputToken(Mtoken* _token){
	if(!_token)return;
	moveCursorLeft(string_length(_token->text));
	outputToken(_token); // back where we started (hopefully)
}
/**
 * freeToken() frees the memory @pLastCommandToEvaluateToken points to and returns true on successfully removing the entire chain of tokens it points to
 * @returns the previous token (as we need that )  
 */
Mtoken* freeToken(Mtoken* _token){
	// MDH@30APR2019: let's delegate to free_token()
	Mtoken* _prevToken=NULL;if(_token){_prevToken=_token->prev;free_token(_token);}return _prevToken;
}
// output functions that require access to the current token
void toStartOfPreviousLine(){oneLineUp();toStartOfLine();clearLine();toStartOfLine();}
void toStartOfNextLine(){oneLineDown();toStartOfLine();}
void toCursorPosition(){
	moveCursorRight(promptLength+cursorPosition());
	if(pLastCommandToEvaluateToken)outputTokenColor(pLastCommandToEvaluateToken); // return to the current token color
}

// MDH@16MAY2019: not showing the error on the line above the user input line, but now below (in info color)
// MDH@22MAY2019 NOTE: const Mvalue* const is protested against in the call to _getValueText

void inputInfo(const char* const fmt,...){
	if(fmt&&strlen(fmt)){ // we have a format
		toStartOfPreviousLine();resetOutputColor(); // get the default output color!!
		// NOTE we have to call vprintf here NOT printf!!!
		va_list args;va_start(args,fmt);vprintf(fmt,args);va_end(args); // NOTE would be a mistake to call output() here, resulting
		toStartOfNextLine();toCursorPosition();
	}
}
void inputError(const char* const fmt,...){
	toStartOfPreviousLine();setColor(getErrorColor());setBackColor(getBackgroundColor());
	va_list args;va_start(args,fmt);vprintf(fmt,args);va_end(args); // NOTE would be a mistake to call output() here, resulting
	toStartOfNextLine();toCursorPosition();
}
void clearInfo(){toStartOfPreviousLine();resetOutputColor();clearLine();toStartOfNextLine();toCursorPosition();}

void outputStatus(char inputChar,char inputCharType){
	////////printf("[%u,%u]",cursorPosition(),commandLength());
	debugWrite("Status: Cursor position=%u - command length=%u - behind cursor text='%s'.",cursorPosition(),commandLength(),string(behindCursorText));
	if(amDebugging())
		inputInfo("Input character: %c(=0x%x) | Input character type: %c | Token type: %u | Cursor position: %" PRIu16 " | Command length: %" PRIu16 " | Behind cursor text: '%s'.",inputChar,inputChar,inputCharType,(pLastCommandToEvaluateToken!=NULL?pLastCommandToEvaluateToken->type:255),cursorPosition(),commandLength(),string(behindCursorText));
	//////outputInfo("Status: Cursor position=%u - command length=%u - behind cursor text='%s'.",cursorPosition(),commandLength(),string(behindCursorText));
}

Mtoken* _getToken(Mtoken* prevToken){
	Mtoken* pNewToken=__token();
	if(pNewToken){
		// MDH@03MAY2019: if the previous token starts an expression itself, use prevToken itself and not its expr field!!!!
		if(prevToken){
			// finish the previous token
			prevToken->next=pNewToken; // how could I forget about doing this (and checking whether prevToken is not NULL!)!!
			if(!prevToken->significantCharacterCount)prevToken->significantCharacterCount=string_length(prevToken->text); // MDH@22MAR2019: if the token character length is NOT set, set it now...
			// initialize the new token
			pNewToken->prev=prevToken; // set the predecessor
			// MDH@27MAY2019: let's by default copy prevToken-expr over

			// MDH@18MAY2019: if a , starts an expression we won't be pointing to the opening parenthesis!!!
			//                which would mean that on verification we'd have to jump back until we found a non-comma!!!
			//                so we can fix this by NOT including TT_EXPRESSION prev tokens to point to!!!
			//                BUT the first (dummy) expression token should be included though!!!
			// TODO having to test an expression for starting with ( is a bit of a nuisance (so we won't accidently do that on the initial expression token and any comma token!!!)
			// MDH@27MAY2019: set expr NOTE the first token behind the (start of) expression token, should keep pointing to NULL
			pNewToken->expr=prevToken->expr; // DEFAULT: take over the expr of the previous token			
			if(prevToken->type==TT_END_OF_LIST||prevToken->type==TT_END_OF_FUNCTION_CALL||prevToken->type==TT_END_OF_MAP)
				pNewToken->expr=prevToken->expr->expr;
			else
			if(prevToken->type==TT_LIST||prevToken->type==TT_FUNCTION_CALL||prevToken->type==TT_MAP||(prevToken->type==TT_EXPRESSION&&prevToken!=pCommandToEvaluate))
				pNewToken->expr=prevToken;
			
			if(amVerbose()){
				if(pNewToken->expr)inputInfo("Matching: %s",string(pNewToken->expr->text));else inputInfo("%s","-");
			}
			
			///////if(amVerbose()){if(pNewToken->expr)inputInfo("Pointing to %s of type %s.",string(pNewToken->expr->text),TOKENTYPE_STRING[pNewToken->expr->type]);else inputInfo("Nothing to point to.");}
			//////// ending with NULL means all is Ok!! if(!pNewToken->expr)pNewToken->expr=pCommandToEvaluate; // TODO will this help???
			pNewToken->offset=prevToken->offset+string_length(prevToken->text); // set the offset
		}
		// MDH@03MAY2019: TT_EXPRESSION is the default (0) now (always ending at the next non-space character): pNewToken->type=TT_EXPRESSION; // makes more sense to start as expression (same as what we get after a ( or [
		pNewToken->text=__string();
		/* not needed with calloc() allocation
		pNewToken->significantCharacterCount=0; // MDH@22MAR2019: remembers the amount of significant characters (to be set when the token ends)
		pNewToken->next=NULL;
		*/
	}
	return pNewToken;
}

// keep track of all commands so far
#define COMMAND_BLOCKSIZE 8
Mtoken** commands=NULL; // array for storing the pointers to the first token of all commands entered
uint32_t commandBlocks=0;
bool registerCommand(){
	if(!pCommandToEvaluate)return false;
	if(!_currentFunctionBodyInput){ // a top-level (non function body) command
		if(commandCount==commandBlocks*COMMAND_BLOCKSIZE){
			// I have to copy all first token pointers to a new array large enough
			commandBlocks++;
			Mtoken** newCommands=realloc(commands,COMMAND_BLOCKSIZE*commandBlocks*sizeof(Mtoken*));
			if(newCommands==NULL)return false;
			commands=newCommands;
		}
		commands[commandCount++]=pCommandToEvaluate;
		return true;
	}else{ // should be added to the function body
		// NOTE we can create the value and when it is not appended to the list it will not be bound, and be released by the 'garbage collector'
		Mvalue* _commandToEvaluateTokenValue=_getValueOfToken(pCommandToEvaluate,false);
		if(_commandToEvaluateTokenValue){
			if(!_currentFunctionBodyInput->_function->_bodyCommandList)_currentFunctionBodyInput->_function->_bodyCommandList=CALLOC(1,sizeof(Mlist),'L');
			if(appendedToList(_currentFunctionBodyInput->_function->_bodyCommandList,_commandToEvaluateTokenValue,0))return true;
			outputError("Failed to add the command to the body of the function");
		}
	}
	return false;
}
// MDH@21JUN2019: reset() takes care of removing all stored commands
void reset(){
	while(commandCount>0){
		freeToken(commands[--commandCount]);
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
//                                -------------------------------- !"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~-
const char INPUTCHARACTERTYPES[]="iiiciiiibtniiniiiiiiiiiiiixmiiiiW!DCL%&S()*+,-.*NNNNNNNNNN:;>=>?@LLLLLLLLLLLLLLLLLLLLLLLLLL[%]%L`LLLLELLLLLLLLLLLLLLLLLLLLL{&}~b";
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
char* const NO_TRANSITIONS[NUMBER_OF_FINISHABLE_TOKEN_TYPES]={"","","","","","","","","","","","","q","q","`D","`S","","","","","","","","LEN","",""}; // MDH@30APR2019: oops one extra needed...

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
/*
 "EXPR","UNA" ,"A","Baeru","BaErU","BAeRu","BaERu","BAeru" ,"Taeru","VAR" ,"NEWVAR","L_EL","INT","REAL","DQSTRING","SQSTRING","END_DQS","END_SQS","LIST","END_L","MAP","M_V","END_M","FUNCTION","F_CALL","END_FC","CM","ERROR"},*/
const char * const TRANSITIONS[NUMBER_OF_FINISHABLE_TOKEN_TYPES][NUMBER_OF_TOKEN_TYPES]={ \
{"("   ,"!-+~","" ,""     ,""     ,""     ,""      ,""     ,""     ,"LE"  ,""      ,""    ,"N"  ,"."   ,"D"       ,"S"       ,""       ,""       ,"["   ,""     ,"{"  ,""   ,""     ,""        ,""      ,""      ,""  ,"`@; C  % )&*  , >?:    ] }="}, /* EXPRESSION */ \
{"("   ,"!-+~","" ,""     ,""     ,""     ,""      ,""     ,""     ,"LE"  ,""      ,""    ,"N"  ,"."   ,""        ,""        ,""       ,""       ,"["   ,""     ,""   ,""   ,""     ,""        ,""      ,""      ,""  ,"`@; CDS% )&*  , >?:    ]{}="}, /* ONE CHARACTER UNARY !-+ */ \
{"("   ,"!-+~","" ,"="    ,""     ,""     ,""      ,""     ,""     ,"LE"  ,""      ,""    ,"N"  ,"."   ,"D"       ,"S"       ,""       ,""       ,"["   ,""     ,"{"  ,""   ,""     ,""        ,""      ,""      ,""  ,"`@; C  % )&*  , >?:    ] }" }, /* ASSIGNMENT = */ \
{"("   ,"!-+~","" ,""     ,""     ,""     ,""      ,""     ,""     ,"LE"  ,""      ,""    ,"N"  ,"."   ,"D"       ,"S"       ,""       ,""       ,"["   ,""     ,""   ,""   ,""     ,""        ,""      ,""      ,""  ,"`@; C  % )&*  , >?:    ]{}="}, /* Baeru finished bin.op. */ \
{""    ,""    ,"" ,"="    ,""     ,""     ,""      ,""     ,""     ,""    ,""      ,""    ,""   ,""    ,""        ,""        ,""       ,""       ,""    ,""     ,""   ,""   ,""     ,""        ,""      ,""      ,""  ,"`@;!CDS%()&*+-,.>?:LEN[]{}" }, /* BaErU unfinished bin.op. */ \
{"("   ,"!-+~","=",""     ,""     ,""     ,""      ,"R"    ,""     ,"LE"  ,""      ,""    ,"N"  ,"."   ,""        ,""        ,""       ,""       ,"["   ,""     ,""   ,""   ,""     ,""        ,""      ,""      ,""  ,"`@; CDS% )&*  , >?:    ]{}" }, /* BAeRu assignable repeatable */ \
{"("   ,"!-+~","" ,"="    ,""     ,""     ,""      ,"R"    ,""     ,"LE"  ,""      ,""    ,"N"  ,"."   ,"D"       ,"S"       ,""       ,""       ,"["   ,""     ,""   ,""   ,""     ,""        ,""      ,""      ,""  ,"`@; C  % )&*  ,  ?:    ]{}" }, /* BaERu comp. (<>) bin.op. */ \
{"("   ,"!-+~","=",""     ,""     ,""     ,""      ,""     ,""     ,"LE"  ,""      ,""    ,"N"  ,"."   ,"D"       ,"S"       ,""       ,""       ,"["   ,""     ,""   ,""   ,""     ,""        ,""      ,""      ,""  ,"`@; C  % )&*  , >?:    ]{}" }, /* BAeru assignable bin.op. */ \
{"("   ,"!-+~","=",""     ,""     ,""     ,""      ,""     ,""     ,"LE"  ,""      ,""    ,"N"  ,"."   ,"D"       ,"S"       ,""       ,""       ,"["   ,""     ,"{"  ,""   ,""     ,""        ,""      ,""      ,""  ,"`@; C  % )&*  , >?:    ]{}" }, /* Taeru ternary op. (? only now) */ \
{""    ,""    ,"=",""     ,"!"    ,"&*"   ,">"     ,"-+%"  ,"?"    ,"LEN.",""      ,","   ,""   ,""    ,""        ,""        ,""       ,""       ,"["   ,"]"    ,""   ,":"  ,"}"    ,""        ,""      ,")"     ,"C" ,"`@;  DS (               {"  }, /* VARIABLE (identifier that is NOT a function) FUNCTION: some identifier not yet recognized as function name */ \
{""    ,""    ,"=",""     ,"!"    ,"&*"   ,">"     ,"-+%"  ,"?"    ,""    ,"LEN."  ,""    ,""   ,""    ,""        ,""        ,""       ,""       ,"["   ,"]"    ,""   ,":"  ,"}"    ,""        ,""      ,")"     ,"C" ,"`@;  DS (     ,         {"  }, /* NEW_VARIABLE (variable that does not exist yet) */ \
{"("   ,"!-+~","" ,""     ,""     ,""     ,""      ,""     ,""     ,"LE"  ,""      ,","   ,"N"  ,"."   ,"D"       ,"S"       ,""       ,""       ,"["   ,"]"    ,"{"  ,""   ,""     ,""        ,""      ,""      ,""  ,"`@; C  % )&*    >?:      }="}, /* LIST ELEMENT (similar to expression) */ \
{";"   ,""    ,"" ,"?:"   ,"!="   ,"&*"   ,">"     ,"-+%E" ,"?"    ,""    ,""      ,","   ,"N"  ,"."   ,""        ,""        ,""       ,""       ,""    ,"]"    ,""   ,":"  ,"}"    ,""        ,""      ,")"     ,"C" ,"`@   DS (          L  [ {"  }, /* INTEGER: (signless) list of digits */ \
{";"   ,""    ,"" ,"?:"   ,"!="   ,"&*"   ,">"     ,"-+%E" ,"?"    ,""    ,""      ,","   ,""   ,"N"   ,""        ,""        ,""       ,""       ,""    ,"]"    ,""   ,":"  ,"}"    ,""        ,""      ,")"     ,"C" ,"`@   DS (      .   L  [ {"  }, /* REAL: part behind a decimal period */ \
{""    ,""    ,"" ,""     ,""     ,""     ,""      ,""     ,""     ,""    ,""      ,""    ,""   ,""    ,""        ,""        ,"D"      ,""       ,""    ,""     ,""   ,""   ,""     ,""        ,""      ,""      ,""  ,""                           }, /* DQSTRING: double quoted string */ \
{""    ,""    ,"" ,""     ,""     ,""     ,""      ,""     ,""     ,""    ,""      ,""    ,""   ,""    ,""        ,""        ,""       ,"S"      ,""    ,""     ,""   ,""   ,""     ,""        ,""      ,""      ,""  ,""                           }, /* SQSTRING: single quoted string */ \
{";"   ,""    ,"" ,"+"    ,""     ,"&"    ,">"     ,""     ,"?"    ,""    ,""      ,","   ,""   ,""    ,"D"       ,"S"       ,""       ,""       ,""    ,"]"    ,""   ,":"  ,"}"    ,""        ,""      ,")"     ,"C" ,"`@ ! DS%&( * - .   LEN[ {"  }, /* END_DQSTRING: double quoted string at end of double quoted string */ \
{";"   ,""    ,"" ,"+"    ,""     ,"&"    ,">"     ,""     ,"?"    ,""    ,""      ,","   ,""   ,""    ,""        ,""        ,""       ,""       ,""    ,"]"    ,""   ,":"  ,"}"    ,""        ,""      ,")"     ,"C" ,"`@ ! DS%&( * - .   LEN[ {"  }, /* END_SQSTRING single quoted string at end of single quoted string */ \
{"("   ,"!-+~","" ,""     ,""     ,""     ,""      ,""     ,""     ,"LE"  ,""      ,","   ,"N"  ,""    ,"D"       ,"S"       ,""       ,""       ,"["   ,"]"    ,"{"  ,""   ,""     ,""        ,""      ,")"     ,""  ,"`@; C  %& )*   .>?:      }="}, /* LIST: [ starts a list */ \
{";"   ,""    ,"=","?"    ,"!"    ,"&*"   ,">"     ,"-+%"  ,"?"    ,""    ,""      ,","   ,""   ,""    ,""        ,""        ,""       ,""       ,""    ,"]"    ,""   ,":"  ,"}"    ,""        ,""      ,")"     ,"C" ,"`@   DS  (     .  :LEN  {"  }, /* END_OF_LIST: behind ] that ends a list */ \
{"("   ,"!-+~","" ,""     ,""     ,""     ,""      ,""     ,""     ,"LE"  ,""      ,""    ,"N"  ,""    ,"D"       ,"S"       ,""       ,""       ,"["   ,""     ,""   ,""   ,"}"    ,""        ,""      ,")"     ,""  ,"`@; C  %& )*  ,.>?:    ]{ ="}, /* MAP: { starts a map */ \
{"("   ,"!-+~","" ,""     ,""     ,""     ,""      ,""     ,""     ,"LE"  ,""      ,""    ,"N"  ,"."   ,"D"       ,"S"       ,""       ,""       ,"["   ,""     ,"{"  ,""   ,""     ,""        ,""      ,")"     ,""  ,"`@; C  %& )*  , >?:    ] }="}, /* MAP_VALUE: : starts a map value */ \
{";"   ,""    ,"" ,"?"    ,"!="   ,"&*"   ,">"     ,"+"    ,"?"    ,""    ,""      ,","   ,""   ,""    ,""        ,""        ,""       ,""       ,""    ,"]"    ,""   ,""   ,"}"    ,""        ,""      ,")"     ,"C" ,"`@   DS% (   - .  :LEN  {"  }, /* END_OF_MAP: behind } that ends a map */ \
{""    ,""    ,"" ,""     ,""     ,""     ,""      ,""     ,""     ,""    ,""      ,""    ,""   ,""    ,""        ,""        ,""       ,""       ,""    ,""     ,""   ,""   ,""     ,""        ,"("     ,""      ,""  ,"`@;!CDS%& )*+-,.>?:   []{}="}, /* FUNCTION: some identifier recognized as function name */ \
{"("   ,"!-+~","" ,""     ,""     ,""     ,""      ,""     ,""     ,"LE"  ,""      ,","   ,"N"  ,""    ,"D"       ,"S"       ,""       ,""       ,"["   ,""     ,"{"  ,""   ,""     ,""        ,""      ,")"     ,""  ,"`@; C  %&  *   .>?:    ] }="}, /* FUNCTION_CALL ( following the name of a function */ \
{";"   ,""    ,"" ,"?:"   ,"!="   ,"&*"   ,">"     ,"-+%E" ,"?"    ,""    ,""      ,","   ,""   ,""    ,""        ,""        ,""       ,""       ,""    ,"]"    ,""   ,":"  ,"}"    ,""        ,""      ,")"     ,"C" ,"`@   DS  (     .   L N  {"  }, /* END_OF_FUNCTION_CALL ) at end of last function call argument, ending a function call */ \
};

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
bool continuesOperator(Mtoken* pLastCommandToEvaluateToken,char inputChar){
	unsigned int l=string_get_length(pLastCommandToEvaluateToken->text);
	if(inputChar==ASSIGNMENT_CHARACTER){ // appending the 'assignment' operator
		return(l==1||string_get_last_char(pLastCommandToEvaluateToken->text)!=ASSIGNMENT_CHARACTER);
	}else{
		if(l>1)return false; // cannot continue a two-character operator
		// if subtype is assumed to represent the index into the first operator character set
		unsigned int operatorType=pLastCommandToEvaluateToken->type.subtype;
		return(operatorType<=6&&strchr(SECOND_OPERATOR_CHARACTERS[operatorType],inputChar)!=NULL);
	}
}
*/
// keep track of the state of entering a command
void removeToken(){		
	// ASSERT pLastCommandToEvaluateToken should NOT be NULL and empty (i.e. empty tokens should be removed!!!)
	// NOTE if we call freeToken() to free this token all forwardly connected tokens are also freed, so pPrevToken->next should become NULL
	pLastCommandToEvaluateToken=freeToken(pLastCommandToEvaluateToken); // pLastCommandToEvaluateToken now equals its own previous token!!
	if(pLastCommandToEvaluateToken)pLastCommandToEvaluateToken->next=NULL;else pCommandToEvaluate=NULL;
}
void unfinishToken(){
	// for all non-unary token that we are in now that is finished, unfinish it!!
	if(pLastCommandToEvaluateToken)
		if(pLastCommandToEvaluateToken->type!=TT_UNARY) // not a unary operator (of length 1) we ended up in
			if(string_length(pLastCommandToEvaluateToken->text)==pLastCommandToEvaluateToken->significantCharacterCount) // the current length equals the number of significant characters (i.e. we remove the first whitespace in the token)
				pLastCommandToEvaluateToken->significantCharacterCount=0;
}
char removedTokenCharacter(uint16_t behindCursor){
#ifdef __DEBUG__
		printf("%d",behindCursor);
#endif
	uint16_t tokenCharacterPosition;
	// find the token that we should remove a character from (either the current token or the one in front of it (if all tokens are non-empty!))
	while(true){
		if(pLastCommandToEvaluateToken==NULL)return '\0';
		tokenCharacterPosition=string_length(pLastCommandToEvaluateToken->text); // MDH@24APR2019 replacing (what is essentially the same): cursorPosition()-pLastCommandToEvaluateToken->offset;
#ifdef __DEBUG__
		printf("%d",tokenCharacterPosition);
#endif
		if(tokenCharacterPosition>=behindCursor)break;
#ifdef __DEBUG__
		outputChar('.');
#endif		
		pLastCommandToEvaluateToken=pLastCommandToEvaluateToken->prev;
	}
#ifdef __DEBUG__
		printf("%d",tokenCharacterPosition-behindCursor);
#endif	
	// if failing to remove the character serious error
	char c=string_removed_char(pLastCommandToEvaluateToken->text,tokenCharacterPosition-behindCursor);
#ifdef __DEBUG__
		outputChar(c);
#endif		
	if(c){
		if(string_empty(pLastCommandToEvaluateToken->text))removeToken(); // text now empty, remove the token entirely...
		unfinishToken();
	}
	return c;
}

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

// MDH@06MAY2019: Mvaluereference stands for a variable in combination with an item id, this will allow assignments as we know the variable involved!!!!
typedef struct Mvaluereference{
	char* _name; // the name of the host variable or NULL if we're in a substructure
	Mvalue* _value; // either the host value (if no variable name is defined), or the value of the host variable
	Mvalue* _itemid; // the item referenced!!!
}Mvaluereference;
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
			case VT_REAL:_value->value._real=(Mreal*)calloc(1,sizeof(Mreal));break; // initialized to 0.0 I presume
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
			output("Copying token of type '%s' with text '%s'.\n",TOKENTYPE_STRING[_token->type],string(_token->text));
			if(_token->expr)output("\tpointing to token of type '%s' with text '%s'.\n",TOKENTYPE_STRING[_token->expr->type],string(_token->expr->text));
		}
		_tokenCopy->type=_token->type;
		_tokenCopy->significantCharacterCount=_token->significantCharacterCount;
		if(_token->text)_tokenCopy->text=_stringCopy(_token->text,0);
		_tokenCopy->expr=_token->expr; // TODO do I need to do this??? this is also an issue because if we start comparing expr (on evaluation)
		// we're NOT copying _next, _prev, _offset
		//////_tokenCopy->prev=NULL;_tokenCopy->next=NULL;_tokenCopy->offset=0;
	}
	return _tokenCopy;
}
// NOTE by adding endTokenType and maximumNumberOfElements to getListExpressionValue we can use it as well for getting an arguments list...
Mvalue* getValueOfList(TokenType endTokenType,uint32_t maximumNumberOfElements,uint32_t numberOfElementsToNotEvaluate){
	Mtoken* expressionToken=getEnvironmentExpressionToken(); // does NOT need to be freed, so no _ in front of it!
	if(amVerbose())output("Composing a list of %u elements with %u unevaluatable elements starting with '%s'.\n",maximumNumberOfElements,numberOfElementsToNotEvaluate,string(expressionToken->text));
	// MDH@21MAY2019: _getListValue() as opposed to getValueOfExpressionOfType() creates a Mvalue on the value list which will be removed when the reference count of the Mvalue list ends up being 0
	//                then, the list element values will be dereferenced and if their reference count becomes zero freed as well successfully!!!!
	Mvalue* _listValue=_getListValue(VT_UNDEFINED); // replacing: getValueOfExpressionOfType(VT_LIST);
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
			Mtoken* unevaluatedToken=_firstUnevaluatedToken;
			while(unevaluatedToken){
				expressionToken=nextEnvironmentExpressionToken();
				if(!expressionToken)break; // NOTE shouldn't happen though
				if(!expressionToken->expr||expressionToken->expr==expr)if(expressionToken->type==endTokenType||expressionToken->type==TT_LISTELEMENT)break;
				unevaluatedToken->next=_getEvaluatableTokenCopy(expressionToken); // set next to the copy of the expression token
				unevaluatedToken=unevaluatedToken->next;
			}
			_listElementValue=_getValueOfToken(_firstUnevaluatedToken,true);
		}else{ // evaluate
		// theoretically it is possible that this list element is empty in which case we should append NULL to the list
			_listElementValue=(expressionToken->type!=TT_LISTELEMENT?getValueOfExpression("list element",'l',(TokenType[]){endTokenType,TT_LISTELEMENT},2):NULL);
			expressionToken=getEnvironmentExpressionToken(); // essential after calling any function that might advance the current token pointer
		}
		if(!_listElementValue){if(amVerbose())output("List element missing!\n");continue;} // undefined list elements should NEVER be added to the list
		if(amVerbose())output("List element ending token: %s.\n",TOKENTYPE_STRING[expressionToken->type]);
		// get the next list element value, here's a problem as we're supposed to return the offset not the first token
		// if we already have the maximum number of elements, we do not append this list element!!!
		// we're NOT using the number of elements in the list to check agains anymore but the list element index
		if(!maximumNumberOfElements||listElementIndex<=maximumNumberOfElements){
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
		if(expressionToken->type==endTokenType)break; // the list element could have ended with the end token type, in which case we're done!!!
	}
	if(amVerbose())output("List extracted!\n");
	return _listValue;
}

Mvalue* getValueOfMap(){
	Mtoken* expressionToken=getEnvironmentExpressionToken(); // MDH@17JUL2019: one of five functions that use and advance the current expression token
	Mvalue* _mapValue=_getMapValue(VT_UNDEFINED); // MDH@21MAY2019 for the same reason as above: replacing: getValueOfExpressionOfType(VT_MAP);
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
		Mstring* _attributeName=_getValueText(_attributeNameValue,false); // parse the attribute name value (could be undefined though)
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
		if(string_length(_attributeName)>0)
		if(!appendedToMap(_map,string(_attributeName),_attributeValueValue)){
			output("%s",ERROR_PREFIX);outputValue("Failed to append the value of attribute '",_attributeNameValue,"'.\n");
		} // NOTE can't break until we actually bump into the TT_END_OF_MAP!!!
		free_string(_attributeName); // ALWAYS free the name text
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
						Mvalue* _functionEvaluationValue=NULL;
						if(functionBodyCommandList){
							Mlistelement* functionBodyCommandListelement=functionBodyCommandList->_first;
							while(functionBodyCommandListelement){
								_functionExecutionEnvironment->expressionToken=functionBodyCommandListelement->_value->value._token;
								// evaluate that expression
								_functionEvaluationValue=getValueOfExpression("function body command evaluation",'f',(TokenType[]){},0);
								functionBodyCommandListelement=functionBodyCommandListelement->_next;
							}
						}else
							output("No commands in body of user function '%s' to execute!\n",functionName);
						// before popping the function execution environment, see if the result was set
						Mvalue* _functionResultValue=getValue(_functionExecutionEnvironment,"$");
						popExecutionEnvironment();
						// the function result value (if set) takes precedence over the function evaluation value
						return (_functionResultValue?_functionResultValue:_functionEvaluationValue);
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
					output("Applying two-argument function '%s'.\n",functionName);
					if(_firstArgumentmapelement)outputValue(" to '",_firstArgumentmapelement->_variable->_value,"'");
					if(_secondArgumentmapelement)outputValue(" and '",_secondArgumentmapelement->_variable->_value,"'");
					outputChar('\n');
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
					output("Applying three-argument function '%s'.\n",functionName);
					if(_firstArgumentmapelement)outputValue(" to '",_firstArgumentmapelement->_variable->_value,"'");
					if(_secondArgumentmapelement)outputValue(" and '",_secondArgumentmapelement->_variable->_value,"'");
					if(_thirdArgumentmapelement)outputValue(" and '",_thirdArgumentmapelement->_variable->_value,"'");
				}
				return (*_function->functionunion.threeArgumentFunction)((_firstArgumentmapelement?_firstArgumentmapelement->_variable->_value:NULL)
																		,(_secondArgumentmapelement?_secondArgumentmapelement->_variable->_value:NULL)
																		,(_thirdArgumentmapelement?_thirdArgumentmapelement->_variable->_value:NULL));
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
	if(amVerbose())outputValue("\nWrapping value '",_value,"'.");
	Mvaluereference* _valuereference=(Mvaluereference*)calloc(1,sizeof(Mvaluereference));
	assignValue(&_valuereference->_value,_value);
	if(amVerbose())outputValue("Value '",_value,"' wrapped in value reference.\n");
	return _valuereference;
}
void free_valuereference(Mvaluereference* _valuereference){
	if(_valuereference){
		if(_valuereference->_name)free(_valuereference->_name);
		// values themselves are never freed!!!
		if(_valuereference->_value)decrementReferenceCount(_valuereference->_value);
		if(_valuereference->_itemid)decrementReferenceCount(_valuereference->_itemid);
		free(_valuereference);
	}
}
// two essential methods for getting and setting referenced values
Mvalue* getReferencedValue(Mvaluereference* _valuereference){
	// _itemid now represents the entire list of index/attribute name combinations
	if(!_valuereference)return NULL;
	if(_valuereference->_value)return _valuereference->_value; // if we have a value return that!!!
	// if we do NOT have a name it's a literal
	if(!_valuereference->_name)return NULL;
	// if there is no itemid we simply return the 'entire' value of the given variable
	Mvalue* _value=getValue(getEnvironment(),_valuereference->_name); // the value at the top level
	// if we have index/attribute names we have to get the final subvalue
	if(_valuereference->_itemid){
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
					// if we are accessing a map we have to ascertain that the attribute name in a string
					if(_value->type==VT_MAP){
						Mstring* attributenameText=_getValueText(indexorattributenameListelementValue,true); // TODO should we dequote??
						if(attributenameText){
							_value=getValueOfAttribute(_value->value._map,string(attributenameText));		
							free_string(attributenameText);
							continue;	
						}
						output("%s",ERROR_PREFIX);
						outputValue("Failed to convert assumed attribute name '",indexorattributenameListelementValue,"' to text.\n");		
					}
					if(_value->type==VT_LIST){
						// try to convert the index value into a positive integer
						long long index=getValueInteger(indexorattributenameListelementValue);
						if(index!=0&&index!=M_LL_INVALID){
							_value=getValueAtIndex(_value->value._list,index);
							continue;
						}
						if(index){
							output("%s",ERROR_PREFIX);
							outputValue("Assumed index '",indexorattributenameListelementValue,"' does not represent an integer.\n");
						}else
							outputError("A zero index is not allowed");
					}
					// neither a list nor a map, so nothing to return!!!
					return NULL;
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
		}
	}
	return _value;
}
// when assigning, we're supposed to assign to something with a variable name (and optional index/attribute name list) associated with it
bool setReferencedValue(Mvaluereference* _valuereference,Mvalue* _newValue){
	bool result=false;
	if(_valuereference&&_valuereference->_name){
		if(_valuereference->_itemid){ // the hard part: index/attribute name list assignment!!
			result=true;
			Mlist* _itemidlist=_valuereference->_itemid->value._list; // let's assume that is it always a list
			// let's get the first index/attribute name
			Mlistelement* indexorattributenameListelement=_itemidlist->_first;
			if(indexorattributenameListelement){ // we've got one, so not an empty index/attribute name list!!
				Mvalue* _value=getValue(_Menvironment,_valuereference->_name); // we'll be needing the value at the top level
				// we need to find the last index or attribute name
				Mvalue* indexorattributenameListelementValue;
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
					long long index=getValueInteger(indexorattributenameListelement->_value);
					// replace the index to the actual index with the index of the element in the list (so getReferencedValue() will not complain!!!)
					if(index!=LLONG_MIN){
						index=appendedToList(_value->value._list,_newValue,index);
						if(index>0){
							assignValue(&indexorattributenameListelement->_value,_getIntegerValue(index));
						}else{
							result=false;
						}				
					}else{
						result=false;
					}
				}
			}
		}else
			setValue(getEnvironment(),_valuereference->_name,_newValue);
	}
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
			if(expressionToken->next&&expressionToken->next->type==VT_REAL){
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
					Mfunction* function=getFunction(_Menvironment,_significantTokenText); // get the function associated with the name of the function
					if(function){		
						// MDH@17JUL2019: we know the function and when the name is one of the special functions
						//                like 'function' to define a function we know not to evaluate the third argument!!
						//                it's easiest to define first element not to evaluate (i.e. to store the tokens in the list)
						uint32_t numberOfElementsToNotEvaluate=0;
						if(!strcmp(_significantTokenText,DEFINEUSERFUNCTION_NAME)){
							if(amVerbose())outputLine("Definition of a user function encountered!");
							numberOfElementsToNotEvaluate=1;		
						}
						// 1. get the list of function arguments, which depends on the function!!
						expressionToken=nextEnvironmentExpressionToken();
						Mvalue* _functionArgumentsValue=getValueOfList(TT_END_OF_FUNCTION_CALL,function->_parameterMap->numberOfElements,numberOfElementsToNotEvaluate);
						expressionToken=getEnvironmentExpressionToken(); // OOPS always update expressionToken after calling a function that might advance it
						if(_functionArgumentsValue){
							if(amVerbose())outputValue("Function argument list: '",_functionArgumentsValue,"'.\n");
							// 2. get the arguments map
							Mmap* _functionArgumentMap=_getFunctionArgumentMap(function,_functionArgumentsValue->value._list); // assuming to have a list returned by getListExpressionValue()
							/// we do not need to release the function arguments list value because it it never assigned by itself, it is simply a container for the argument list elements (which do have a reference count incremented when added to the list)
							/*
							if(amVerbose())output("Decrementing the reference count of the function arguments value!");
							decrementReferenceCount(_functionArgumentsValue); // TODO is this correct?????
							if(amVerbose())output("Reference count of the function arguments value decremented!");
							*/
							// 3. the result of applying the function to the arguments is the end result
							// MDH@19JUL2019: we need to know when a function is being created, so we can ask for the body commands in command mode
							Mvalue* functionCallValue=getValueOfFunctionCall(function,_significantTokenText,_functionArgumentMap);
							// if this was a call to the 'define user function' function
							if(!strcmp(_significantTokenText,DEFINEUSERFUNCTION_NAME)){ // a function being defined
								// is the result 1???
								if(functionCallValue&&functionCallValue->type==VT_INTEGER&&functionCallValue->value._integer->ll){ // function successfully created
									// let's push the function name on the stack of functions to create
									// we know the first argument contains the function name
									char* definedFunctionName=_functionArgumentMap->_first->_variable->_value->value._text->_c;
									Mfunction* definedFunction=getFunction(getEnvironment(),definedFunctionName);
									// if the function now exists but does not yet have a body, queue the function name on the list of bodies to be set
									if(definedFunction&&definedFunction->type==FT_USER&&!definedFunction->functionunion._userfunction->_bodyCommandList)
										requestBodyOfFunction(definedFunctionName);
									else
									if(amVerbose())output("Function '%s' completely specified with single body command!\n",definedFunctionName);
								}
							}
							assignValue(&_valueReference->_value,functionCallValue);
							// except getValueOfFunctionCall() doesn't CORRECTION can't harm can it????
							expressionToken=getEnvironmentExpressionToken(); // essential to update after calling a function that updates the expression token
							// we have to free the map ourselves (this is what the _ in front of getFunctionArgumentMap means)
							if(amVerbose()){outputValue("Function call result value: '",_valueReference->_value,"'.\n");outputLine("Freeing the function argument map!");}
							free_map(_functionArgumentMap); // MDH@21MAY2019: no need for the function argument map anymore!!!
							if(amVerbose())outputLine("Function argument map freed!");
						}else
							outputError("No function arguments");
					}else
						output("%sFunction '%s' unknown!\n",ERROR_PREFIX,_significantTokenText);
				}
				break;
			case TT_NEW_VARIABLE: // a non-existing value reference
				// we have to create the variable first (TODO should we wait until actually assigning???)
				if(!addVariable(_Menvironment,_significantTokenText,VT_UNDEFINED,false))break; // NO retrieves the undefined value subsequently!!
			case TT_VARIABLE: // a value reference
				_valueReference->_name=_significantTokenText;_significantTokenText=NULL; // store a copy of the name of the variable being referenced
				if(amVerbose())output("Variable name: '%s'.\n",_valueReference->_name);
				// NOTE do NOT assign the value of an indexed expression because it we did (as we done) the value would be returned as result and not the value at the given index
				///////////////////assignValue(&_valueReference->_value,getValue(_Menvironment,_valueReference->_name)); // store a reference to the value
				/////////////////incrementReferenceCount(_valueReference->_value); // TODO combine this with getValue to something called storeValue
				// a variable can be followed by an index that we should store in the value reference's itemid field
				if(expressionToken->next&&expressionToken->next->type==TT_LIST){
					///////expressionToken=nextEnvironmentExpressionToken();
					Mvalue* indexListValue=getValueOfList(TT_END_OF_LIST,0,0); // typically allow for any number of indices (although perhaps we should check!!)
					expressionToken=getEnvironmentExpressionToken(); // essential after calling a function that might advance the current expression token
					// using the indexValue we should now update the value represented up until the last index (in case we have an assignment)
					// which means that only the last index value has to be stored and the container of that last index (map or list)
					if(indexListValue&&indexListValue->type==VT_LIST&&indexListValue->value._list->_first){ // a non-empty list
						assignValue(&_valueReference->_itemid,indexListValue); // now storing the entire index/attribute name list
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
					}
				}
				// MDH@29MAY2019: if we do NOT have an indexed value, retrieve the value...
				// TODO as a side-effect getReferencedValue() will bind the added value to the value reference (as result) BUT I don't think that is how it should be!!! no the assignment takes care of that
				if(!_valueReference->_itemid){
					if(amVerbose())output("Retrieving the value of '%s'.\n",_valueReference->_name);
					assignValue(&_valueReference->_value,getValue(getEnvironment(),_valueReference->_name));
				}
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
						Mdecimal* _decimal=__decimal(_decimalContext,0,0);
						if(amDebugging())outputLine("Decimal created!");
						if(_decimal){
							mpd_set_string(_decimal->mpd,string(pRealText),_decimalContext);
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
						// replacing: assignValue(&_valueReference->_value,_getRealValue(_strtold(string(pRealText),getNAR())));
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
              				assignValue(&_valueReference->_value,_getIntegerValue(mp_get_i64(_biginteger)));
							free_biginteger(_biginteger);
						}else
							assignValue(&_valueReference->_value,_getBigintegerValue(_biginteger,true));
					}else{
						free_biginteger(_biginteger);
						outputErrorAndText("Failed to create the big integer to store integer ",_significantTokenText);
					}
				}
				break;
			case TT_REAL: // unlikely without integer part in front of it though
				assignValue(&_valueReference->_value,_getRealValue(_strtold(_significantTokenText,getNAR())));
				///////////////incrementReferenceCount(_valueReference->_value); // TODO combine this with getValue to something called storeValue
				break;
			case TT_DQSTRING:
			case TT_SQSTRING: // a string literal
				assignValue(&_valueReference->_value,_getTextValue(_significantTokenText,false));
				////////////////incrementReferenceCount(_valueReference->_value); // TODO combine this with getValue to something called storeValue
				break;
			case TT_LIST: // a list literal
				_valueReference=_getValuereference(getValueOfList(TT_END_OF_LIST,0,0));
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
				Mvalue* _expressionListValue=getValueOfList(TT_END_OF_FUNCTION_CALL,1,0);
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
				assignValue(&_valueReference->_value,applyUnaryOperator(string_char(unaryOperators,--l),_valueReference->_value));
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
				if(appendedToList(_result,_listelement1->_value,_listelement1->index))consumed1=true;
			}else{
				if(appendedToList(_result,_listelement2->_value,_listelement2->index))consumed2=true;
			}
		}else
		if(_listelement1){
			if(appendedToList(_result,_listelement1->_value,_listelement1->index))consumed1=true;
		}else
			if(appendedToList(_result,_listelement2->_value,_listelement2->index))consumed2=true;
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
Mvalue* add(Mvalue* _value1,Mvalue* _value2){
	// if either is NULL return the other
	if(!_value1||isValueZero(_value1))return _value2;
	if(!_value2||isValueZero(_value2))return _value1;
	// ASSERT neither are NULL
	// if either is a list apply 'add' to the list (NOTE scalar addition is NOT the same as list addition)
	if(_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,add);
	if(_value2->type==VT_LIST)return _appliedToList(_value2->value._list,_value1,add);
	// if either is a rational, compute the sum rational
	if(_value1->type==VT_RATIONAL||_value2->type==VT_RATIONAL){
		Mrational *_rational1=_getValueRational(_value1),*_rational2=_getValueRational(_value2); // OOPS careful here, _getValueRational might construct a new rational or what????
		Mrational* _sumRational=_qadd(_rational1,_rational2);
		if(_value1->type!=VT_RATIONAL)free_rational(_rational1);else if(_value2->type!=VT_RATIONAL)free_rational(_rational2); // after adding the two rationals we do not need the newly created rationals anymore
		if(!_sumRational)return NULL; // failed to create the sum for whatever reason
		return _getRationalValue(_sumRational,true);
	}
	// if either is a decimal, compute the sum decimal
	if(_value1->type==VT_DECIMAL||_value2->type==VT_DECIMAL){
		Mdecimal *_decimal1=_getValueDecimal(_value1),*_decimal2=_getValueDecimal(_value2); // OOPS careful here, _getValueRational might construct a new rational or what????
		Mdecimal* _sumDecimal=_dadd(_decimal1,_decimal2);
		if(_value1->type!=VT_DECIMAL)free_decimal(_decimal1);else if(_value2->type!=VT_DECIMAL)free_decimal(_decimal2); // after adding the two rationals we do not need the newly created rationals anymore
		if(!_sumDecimal)return NULL; // failed to create the sum for whatever reason
		return _getDecimalValue(_sumDecimal,true);
	}
	if(_value1->type==VT_TEXT){ // force string concatenation using the quote character in the Mvalue in the resulting text
		Mstring* _valueText=__string();
		Mstring* p=_valueText;
		p=string_append_char(p,_value1->value._text->presuffix);
		p=string_append(p,_value1->value._text->_c);
		Mstring* _value2Text=_getValueText(_value2,true); // get the text representation of the second argument without quotes
		if(_value2Text){p=string_append(p,string(_value2Text));free_string(_value2Text);}
		Mvalue* _value=(p?_getTextValue(string(_valueText),false):NULL);
		free_string(_valueText);
		return _value;
	}
	if((_value1->type==VT_INTEGER||_value1->type==VT_REAL)&&(_value2->type==VT_INTEGER||_value2->type==VT_REAL||_value2->type==VT_TEXT)){
		// if the second argument is text, convert it to a real or integer number
		if(_value2->type==VT_TEXT){
			// are we going to convert it to an integer or a real????
			// NOTE a real has a period in the text, so use that
			char* valueText=_value2->value._text->_c;
			if(strchr(valueText,'.')!=NULL){ // a period 
				////////if(strlen(_valueText)==1)return _value1; // if a single period no need to actually add it unless someone want to change an integer in a real????
				////// we can use _strtold!!! long double ld=0;if(strlen(valueText)>1){char *endPtr=NULL;ld=strtold(valueText,&endPtr);if(endPtr==valueText){output("ERROR: Can't add '%s'.",valueText);return NULL;}} // failure
				_value2=_getRealValue(_strtold(valueText,getNAR()));
			}else{ // no period
				_value2=_getIntegerValue(_strtoll(valueText,getNAI()));
				/* replacing:
				int l=strlen(valueText)-1; // the last character
				if(l<0||(l==((valueText[0]=='-'||valueText[0]=='+'))&&valueText[l]=='0'))return _value1; // if adding zero just return _value1 (and therefore something of the same type)
				long long ll=atoll(valueText);
				if(!ll){output("ERROR: Can't add '%s'!",valueText);return NULL;}; // if zero the text does not represent a valid integer!!!
				_value2=_getIntegerValue(ll); //re-use the _value2 pointer so we can perform the requested addition
				*/
			}
		}
		if(_value1->type==VT_INTEGER&&_value2->type==VT_INTEGER)return _getIntegerValue(_value1->value._integer->ll+_value2->value._integer->ll);
		return _getRealValue((_value1->type==VT_INTEGER?_value1->value._integer->ll:_value1->value._real->ld)+(_value2->type==VT_INTEGER?_value2->value._integer->ll:_value2->value._real->ld));
	}
	return NULL;
}
Mvalue* subtract(Mvalue* _value1,Mvalue* _value2){
	if(!_value1||isValueZero(_value1))return Mneg(_value2);if(!_value2||isValueZero(_value1))return _value1;
	if(_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,subtract);if(_value2->type==VT_LIST)return _appliedToList2(_value1,_value2->value._list,subtract);
	if(_value1->type==VT_RATIONAL||_value2->type==VT_RATIONAL){
		Mrational *_rational1=_getValueRational(_value1),*_rational2=_getValueRational(_value2); // OOPS careful here, _getValueRational might construct a new rational or what????
		Mrational* _differenceRational=_qsubtract(_rational1,_rational2);
		if(_value1->type!=VT_RATIONAL)free_rational(_rational1);else if(_value2->type!=VT_RATIONAL)free_rational(_rational2); // after adding the two rationals we do not need the newly created rationals anymore
		if(!_differenceRational)return NULL; // failed to create the sum for whatever reason
		return _getRationalValue(_differenceRational,true);
	}
	if((_value1->type==VT_INTEGER||_value1->type==VT_REAL)&&(_value2->type==VT_INTEGER||_value2->type==VT_REAL)){
		if(_value1->type==VT_INTEGER&&_value2->type==VT_INTEGER)return _getIntegerValue(_value1->value._integer->ll-_value2->value._integer->ll);
		return _getRealValue((_value1->type==VT_INTEGER?_value1->value._integer->ll:_value1->value._real->ld)-(_value2->type==VT_INTEGER?_value2->value._integer->ll:_value2->value._real->ld));
	}
	return NULL;
}

Mvalue* multiply(Mvalue* _value1,Mvalue* _value2){
	if(!_value1||!_value2)return NULL;
	if(isValueZero(_value1)||isValueOne(_value2))return _value1;if(isValueZero(_value2)||isValueOne(_value1))return _value2;
	if(_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,multiply);if(_value2->type==VT_LIST)return _appliedToList(_value2->value._list,_value1,multiply);
	// if either is rational do a rational multiplication
	if(_value1->type==VT_RATIONAL||_value2->type==VT_RATIONAL){
		Mrational *_rational1=_getValueRational(_value1),*_rational2=_getValueRational(_value2);
		Mrational* _multiplicationRational=_qmultiply(_rational1,_rational2);
		if(_value1->type!=VT_RATIONAL)free_rational(_rational1);else if(_value2->type!=VT_RATIONAL)free_rational(_rational2); // after dividing the two rationals we do not need the newly created rationals anymore
		if(!_multiplicationRational)return NULL; // failed to create the sum for whatever reason
		return _getRationalValue(_multiplicationRational,true);
	}
	if((_value1->type==VT_INTEGER||_value1->type==VT_REAL)&&(_value2->type==VT_INTEGER||_value2->type==VT_REAL)){
		if(_value1->type==VT_INTEGER&&_value2->type==VT_INTEGER)return _getIntegerValue(_value1->value._integer->ll*_value2->value._integer->ll);
		return _getRealValue((_value1->type==VT_INTEGER?_value1->value._integer->ll:_value1->value._real->ld)*(_value2->type==VT_INTEGER?_value2->value._integer->ll:_value2->value._real->ld));
	}
	return NULL;
}

Mvalue* _getValueOneOfType(Mvaluetype valuetype){
	switch(valuetype){
		case VT_INTEGER: return _getIntegerValue(1);
		case VT_BIGINTEGER: return _getBigintegerValue(_getBiginteger(1),true);
		case VT_REAL: return _getRealValue(1.0);
		case VT_RATIONAL: return _getRationalValue(_getRational(_getBiginteger(1),NULL,M_LD_NAN,false,true),true);
		case VT_DECIMAL: return _getDecimalValue(_getDecimal(__mpd(_decimalContext,1),0,true),true);
		default:break;
	}
	return NULL;
}

long double getRealPowerValue(long double base,Mvalue* _powerValue){
	// ASSERT assuming power does not equal 0
	if(!ldIsNaN(base)&&!ldIsInf(base)){
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
			case VT_REAL:return powl(base,_powerValue->value._real->ld);
			default:break;
		}
	}
	return M_LD_NAN; // uncomputable
}
long double getRealValuePower(Mvalue* _baseValue,long double power){
	// ASSERT assuming power does not equal 0
	if(!ldIsNaN(power)&&!ldIsInf(power)){
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
			case VT_REAL:return powl(_baseValue->value._real->ld,power);
			default:break;
		}
	}
	return M_LD_NAN; // uncomputable
}

Mdecimal* getValueDecimal(Mvalue* _value){
	// ASSERT typically _value should contain a numeric (non-real) value
	if(_value&&_value->type==VT_DECIMAL)return _value->value._decimal;
	return _getValueDecimal(_value); // will always create a new one...
}
Mvalue* power(Mvalue* _value1,Mvalue* _value2){
	if(!_value1||!_value2)return NULL;
	if(isValueZero(_value1))return _value1;
	if(_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,power);if(_value2->type==VT_LIST)return _appliedToList2(_value1,_value2->value._list,power);
	if(isValueZero(_value2))return _getValueOneOfType(_value1->type); // if the power is zero, we return the value 1 with the same type as 
	if((_value1->type==VT_INTEGER||_value1->type==VT_REAL||_value1->type==VT_BIGINTEGER||_value1->type==VT_DECIMAL||_value1->type==VT_RATIONAL)&&
		(_value2->type==VT_INTEGER||_value2->type==VT_REAL||_value2->type==VT_BIGINTEGER||_value2->type==VT_DECIMAL||_value2->type==VT_RATIONAL)){
		// computing the power is not so easy for certain value type combinations
		// I suppose if the base or exponent is real, the result should also be real (because it will be approximate)
		if(_value2->type==VT_REAL)return _getRealValue(getRealValuePower(_value1,_value2->value._real->ld));
		// ASSERT exponent is NOT a real
		if(_value1->type==VT_REAL)return _getRealValue(getRealPowerValue(_value1->value._real->ld,_value2));
		// ASSERT base and exponent are not reals
		// therefore exact computations should be possible
		// there's a mpd_pow() methods that we technically use on anything that convertable to a decimal
		// converting a rational to a decimal is difficult unless the rational represents a decimal (i.e. the denominator is a power of 10 or we can make it a power of 10 somehow)
		Mdecimal* _baseDecimal=getValueDecimal(_value1);
		Mdecimal* _exponentDecimal=getValueDecimal(_value2);
		Mdecimal* _powerDecimal=__decimal(_decimalContext,0,0);
		if(_powerDecimal){
			mpd_pow(_powerDecimal->mpd,_baseDecimal->mpd,_exponentDecimal->mpd,_decimalContext);
			// TODO are we converting back?
		}
		if(_value1->type!=VT_DECIMAL)free_decimal(_baseDecimal);if(_value2->type!=VT_DECIMAL)free_decimal(_exponentDecimal);
		return _getDecimalValue(_powerDecimal,true);
	}
	return NULL;
}

long double getReal(Mreal* _real){return(_real?_real->ld:M_LD_NAN);}
Mvalue* epower(Mvalue* _value1,Mvalue* _value2){
	if(!_value1||!_value2)return NULL;
	if(isValueZero(_value1)||isValueZero(_value2))return _value1; // NOTE if the power is zero, the multiplication factor will be 1
	if(_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,epower);if(_value2->type==VT_LIST)return _appliedToList2(_value1,_value2->value._list,epower);
	// if both values are numeric (somehow) we can do the computation
	if((_value1->type==VT_INTEGER||_value1->type==VT_REAL||_value1->type==VT_BIGINTEGER||_value1->type==VT_DECIMAL||_value1->type==VT_RATIONAL)&&
		(_value2->type==VT_INTEGER||_value2->type==VT_REAL||_value2->type==VT_BIGINTEGER||_value2->type==VT_DECIMAL||_value2->type==VT_RATIONAL)){
		// if the epower exponent is zero _value1 is the result
		if(isValueZero(_value2))return _value1;
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
		// replacing: return _getRealValue((_value1->type==VT_INTEGER?_value1->value._integer->ll:_value1->value._real->ld*pow(10.,(_value2->type==VT_INTEGER?_value2->value._integer->ll:_value2->value._real->ld))));
	}
	return NULL;
}

// MDH@07JUN2019: when two integers are presented to divide instead of actually computing the division we can store the division as a rational (so we kind of have a slow evaluation of the division, and we maintain accuracy as long as possible)
Mvalue* divide(Mvalue* _value1,Mvalue* _value2){
	if(!_value1||!_value2)return NULL;
	if(isValueZero(_value1)||isValueOne(_value2))return _value1;
	if(_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,divide);if(_value2->type==VT_LIST)return _appliedToList2(_value1,_value2->value._list,divide);
	// integer divisions are not computed but stored in rational format (without a delta to not suggest that the division is decimal)
	if((_value1->type==VT_INTEGER||_value1->type==VT_BIGINTEGER)&&(_value2->type==VT_INTEGER||_value2->type==VT_BIGINTEGER)){ // both are integer
		Mbiginteger* _numerator=(_value1->type==VT_INTEGER?_getBiginteger(_value1->value._integer->ll):_getBigintegerCopy(_value1->value._biginteger));
		Mbiginteger* _denominator=(_value2->type==VT_INTEGER?_getBiginteger(_value2->value._integer->ll):_getBigintegerCopy(_value2->value._biginteger));
		Mrational* _rational=_getRational(_numerator,_denominator,M_LD_NAN,true,true); // free num/den when failing to bind
		return _getRationalValue(_rational,true); // when failing to bind _rational to a value, free it as well
	}
	// if one of them is a rational do a rational division
	if(_value1->type==VT_RATIONAL||_value2->type==VT_RATIONAL){
		Mrational *_rational1=_getValueRational(_value1),*_rational2=_getValueRational(_value2);
		Mrational* _divisionRational=_qdivide(_rational1,_rational2);
		if(_value1->type!=VT_RATIONAL)free_rational(_rational1);else if(_value2->type!=VT_RATIONAL)free_rational(_rational2); // after dividing the two rationals we do not need the newly created rationals anymore
		return _getRationalValue(_divisionRational,true);
	}
	// always real divide
	if((_value1->type==VT_INTEGER||_value1->type==VT_REAL)&&(_value2->type==VT_INTEGER||_value2->type==VT_REAL)){
		long double ld1=(_value1->type==VT_INTEGER?_value1->value._integer->ll:_value1->value._real->ld); // TODO casting to a long double is perhaps not the best way?
		long double ld2=(_value2->type==VT_INTEGER?_value2->value._integer->ll:_value2->value._real->ld); // TODO casting to a long double is perhaps not the best way?
		return _getRealValue(ld1/ld2);
	}
	return NULL;
}
Mvalue* integerdivide(Mvalue* _value1,Mvalue* _value2){
	if(!_value1||!_value2)return NULL;
	if(isValueZero(_value1)||isValueOne(_value2))return _value1;
	if(_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,integerdivide);if(_value2->type==VT_LIST)return _appliedToList2(_value1,_value2->value._list,integerdivide);
	if((_value1->type==VT_INTEGER||_value1->type==VT_REAL)&&(_value2->type==VT_INTEGER||_value2->type==VT_REAL)){
		// if both integer, use lldiv to perform the integer division
		if(_value1->type==VT_INTEGER&&_value2->type==VT_INTEGER)return _getIntegerValue(lldiv(_value1->value._integer->ll,_value2->value._integer->ll).quot);
		// at least one is real, perform floating point division, then trunc!!!
		long double ld1=(_value1->type==VT_INTEGER?_value1->value._integer->ll:_value1->value._real->ld);
		long double ld2=(_value2->type==VT_INTEGER?_value2->value._integer->ll:_value2->value._real->ld);
		return _getIntegerValue(truncl(ld1/ld2));
	}
	return NULL;
}
Mvalue* divideremainder(Mvalue* _value1,Mvalue* _value2){
	if(!_value1||!_value2)return NULL;
	if(isValueZero(_value1))return _value1;if(isValueOne(_value2))return(_value2->type==VT_INTEGER?_getIntegerValue(0):_getRealValue(0));
	if(_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,divideremainder);if(_value2->type==VT_LIST)return _appliedToList2(_value1,_value2->value._list,divideremainder);
	if((_value1->type==VT_INTEGER||_value1->type==VT_REAL)&&(_value2->type==VT_INTEGER||_value2->type==VT_REAL)){
		// if both integer, use lldiv to perform the integer division
		if(_value1->type==VT_INTEGER&&_value2->type==VT_INTEGER)return _getIntegerValue(lldiv(_value1->value._integer->ll,_value2->value._integer->ll).rem);
		// at least one is real, perform floating point division, then trunc!!!
		long double ld1=(_value1->type==VT_INTEGER?_value1->value._integer->ll:_value1->value._real->ld);
		long double ld2=(_value2->type==VT_INTEGER?_value2->value._integer->ll:_value2->value._real->ld);
		return _getRealValue(ld1-ld2*truncl(ld1/ld2)); // what's left after subtracting the truncated value
	}
	return NULL;
}
// integer arithmetic 
Mvalue* xor(Mvalue* _value1,Mvalue* _value2){
	if(!_value1||!_value2)return NULL;
	if(_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,xor);if(_value2->type==VT_LIST)return _appliedToList2(_value1,_value2->value._list,xor);
	if(_value1->type==VT_INTEGER&&_value2->type==VT_INTEGER)return _getIntegerValue(_value1->value._integer->ll^_value2->value._integer->ll);
	return NULL;
}
Mvalue* bitwiseand(Mvalue* _value1,Mvalue* _value2){
	if(!_value1||!_value2)return NULL;
	if(_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,bitwiseand);if(_value2->type==VT_LIST)return _appliedToList2(_value1,_value2->value._list,bitwiseand);
	if(_value1->type==VT_INTEGER&&_value2->type==VT_INTEGER)return _getIntegerValue(_value1->value._integer->ll&_value2->value._integer->ll);
	return NULL;
}
Mvalue* logicaland(Mvalue* _value1,Mvalue* _value2){
	if(!_value1||!_value2)return NULL;
	if(_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,logicaland);if(_value2->type==VT_LIST)return _appliedToList2(_value1,_value2->value._list,logicaland);
	if(_value1->type==VT_INTEGER&&_value2->type==VT_INTEGER)return _getIntegerValue(_value1->value._integer->ll&&_value2->value._integer->ll);
	return NULL;
}
Mvalue* bitwiseor(Mvalue* _value1,Mvalue* _value2){
	if(!_value1||!_value2)return NULL;
	if(_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,bitwiseor);if(_value2->type==VT_LIST)return _appliedToList2(_value1,_value2->value._list,bitwiseor);
	if(_value1->type==VT_INTEGER&&_value2->type==VT_INTEGER)return _getIntegerValue(_value1->value._integer->ll|_value2->value._integer->ll);
	return NULL;
}
Mvalue* logicalor(Mvalue* _value1,Mvalue* _value2){
	if(!_value1||!_value2)return NULL;
	if(_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,logicalor);if(_value2->type==VT_LIST)return _appliedToList2(_value1,_value2->value._list,logicalor);
	if(_value1->type==VT_INTEGER&&_value2->type==VT_INTEGER)return _getIntegerValue(_value1->value._integer->ll||_value2->value._integer->ll);
	return NULL;
}
Mvalue* shiftleft(Mvalue* _value1,Mvalue* _value2){
	if(!_value1||!_value2)return NULL;
	if(_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,shiftleft);if(_value2->type==VT_LIST)return _appliedToList2(_value1,_value2->value._list,shiftleft);
	if(_value1->type==VT_INTEGER&&_value2->type==VT_INTEGER)return _getIntegerValue(_value1->value._integer->ll<<_value2->value._integer->ll);
	return NULL;
}
Mvalue* shiftright(Mvalue* _value1,Mvalue* _value2){
	if(!_value1||!_value2)return NULL;
	if(_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,shiftright);if(_value2->type==VT_LIST)return _appliedToList2(_value1,_value2->value._list,shiftright);
	if(_value1->type==VT_INTEGER&&_value2->type==VT_INTEGER)return _getIntegerValue(_value1->value._integer->ll>>_value2->value._integer->ll);
	return NULL;
}
// comparison operators
Mvalue* smallerthan(Mvalue* _value1,Mvalue* _value2){
	if(!_value1||!_value2)return NULL;
	if(_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,smallerthan);if(_value2->type==VT_LIST)return _appliedToList2(_value1,_value2->value._list,smallerthan);
	if((_value1->type==VT_INTEGER||_value1->type==VT_REAL)&&(_value2->type==VT_INTEGER||_value2->type==VT_REAL))
		return _getIntegerValue((_value1->type==VT_INTEGER?_value1->value._integer->ll:_value1->value._real->ld)<(_value2->type==VT_INTEGER?_value2->value._integer->ll:_value2->value._real->ld)?1:0);
	return NULL;
}
Mvalue* smallerthanorequalto(Mvalue* _value1,Mvalue* _value2){
	if(!_value1||!_value2)return NULL;
	if(_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,smallerthanorequalto);if(_value2->type==VT_LIST)return _appliedToList2(_value1,_value2->value._list,smallerthanorequalto);
	if((_value1->type==VT_INTEGER||_value1->type==VT_REAL)&&(_value2->type==VT_INTEGER||_value2->type==VT_REAL))
		return _getIntegerValue((_value1->type==VT_INTEGER?_value1->value._integer->ll:_value1->value._real->ld)<=(_value2->type==VT_INTEGER?_value2->value._integer->ll:_value2->value._real->ld)?1:0);
	return NULL;
}
Mvalue* largerthan(Mvalue* _value1,Mvalue* _value2){
	if(!_value1||!_value2)return NULL;
	if(_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,largerthan);if(_value2->type==VT_LIST)return _appliedToList2(_value1,_value2->value._list,largerthan);
	if((_value1->type==VT_INTEGER||_value1->type==VT_REAL)&&(_value2->type==VT_INTEGER||_value2->type==VT_REAL))
		return _getIntegerValue((_value1->type==VT_INTEGER?_value1->value._integer->ll:_value1->value._real->ld)>(_value2->type==VT_INTEGER?_value2->value._integer->ll:_value2->value._real->ld)?1:0);
	return NULL;
}
Mvalue* largerthanorequalto(Mvalue* _value1,Mvalue* _value2){
	if(!_value1||!_value2)return NULL;
	if(_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,largerthanorequalto);if(_value2->type==VT_LIST)return _appliedToList2(_value1,_value2->value._list,largerthanorequalto);
	if((_value1->type==VT_INTEGER||_value1->type==VT_REAL)&&(_value2->type==VT_INTEGER||_value2->type==VT_REAL))
		return _getIntegerValue((_value1->type==VT_INTEGER?_value1->value._integer->ll:_value1->value._real->ld)>=(_value2->type==VT_INTEGER?_value2->value._integer->ll:_value2->value._real->ld)?1:0);
	return NULL;
}
Mvalue* unequalto(Mvalue* _value1,Mvalue* _value2){
	if(!_value1||!_value2)return NULL;
	if(_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,unequalto);if(_value2->type==VT_LIST)return _appliedToList2(_value1,_value2->value._list,unequalto);
	if((_value1->type==VT_INTEGER||_value1->type==VT_REAL)&&(_value2->type==VT_INTEGER||_value2->type==VT_REAL))
		return _getIntegerValue((_value1->type==VT_INTEGER?_value1->value._integer->ll:_value1->value._real->ld)!=(_value2->type==VT_INTEGER?_value2->value._integer->ll:_value2->value._real->ld)?1:0);
	return NULL;
}
Mvalue* equalto(Mvalue* _value1,Mvalue* _value2){
	if(!_value1||!_value2)return NULL;
	if(_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,equalto);if(_value2->type==VT_LIST)return _appliedToList2(_value1,_value2->value._list,equalto);
	if((_value1->type==VT_INTEGER||_value1->type==VT_REAL)&&(_value2->type==VT_INTEGER||_value2->type==VT_REAL))
		return _getIntegerValue((_value1->type==VT_INTEGER?_value1->value._integer->ll:_value1->value._real->ld)==(_value2->type==VT_INTEGER?_value2->value._integer->ll:_value2->value._real->ld)?1:0);
	return NULL;
}

Mvalue* applyBinaryOperator(char* operator,Mvalue* _value1,Mvalue* _value2){
	if(_value1&&_value2){
		if(amVerbose()){outputValue("Computing '",_value1,NULL);output("' %s '",operator);outputValue(NULL,_value2,"'.\n");}
		switch(operator[0]){
			// real arithmetic
			case '+' :return add(_value1,_value2);
			case '-' :return subtract(_value1,_value2);
			case '*' :return (strlen(operator)-1?power(_value1,_value2):multiply(_value1,_value2));
			case 'e' :return epower(_value1,_value2);
			case '/' :return (strlen(operator)-1?integerdivide(_value1,_value2):divide(_value1,_value2));
			case '\\':return integerdivide(_value1,_value2);
			case '%' :return divideremainder(_value1,_value2);
			// integer arithmetic
			case '^' :return xor(_value1,_value2);
			case '&' :return (strlen(operator)-1?logicaland(_value1,_value2):bitwiseand(_value1,_value2));
			case '|' :return (strlen(operator)-1?logicalor(_value1,_value2):bitwiseor(_value1,_value2));
			// comparison operators
			case '<' :return (strlen(operator)-1?(operator[1]=='<'?shiftleft(_value1,_value2):smallerthanorequalto(_value1,_value2)):smallerthan(_value1,_value2));
			case '>' :return (strlen(operator)-1?(operator[1]=='>'?shiftright(_value1,_value2):largerthanorequalto(_value1,_value2)):largerthan(_value1,_value2));
			case '!' :return unequalto(_value1,_value2);
			case '=' :return equalto(_value1,_value2);
		}
	}
	return NULL;
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
	Mtoken* expressionToken=getEnvironmentExpressionToken();
	// typically the offset token determines what the expression ends with!!
	// e.g. ( ends with , or )    [ ends with ]     { ends with }    etc.   
	Mvalue* _expressionValue=NULL;
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
			if(amVerbose())if(expressionToken)output("Does '%s' of type '%s' end the expression?",string(expressionToken->text),TOKENTYPE_STRING[expressionToken->type]);
			endTokenTypeIndex=endTokenTypeCount;
			while(endTokenTypeIndex&&expressionToken->type!=endTokenTypes[endTokenTypeIndex-1]/*&&expressionToken->type>=8*/)endTokenTypeIndex--;
			if(endTokenTypeIndex){if(amVerbose())output("Token '%s' of type %s ends the %s expression.\n",string(expressionToken->text),TOKENTYPE_STRING[expressionToken->type],info);break;}
			if(amVerbose())if(expressionToken)outputLine(" NO");

			if(expressionToken){
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
			uint16_t numberOfAssignments=0;
			Mformulaelement* _lastAssignmentFormulaelement=NULL;
			_formulaelement=formula;
			while(_formulaelement){
				if(string_last_char(_formulaelement->_operator)!='=')break; // not ending with assignment operator character to start with
				if(string_char(_formulaelement->_operator,0)=='<'||string_char(_formulaelement->_operator,0)=='>'||string_char(_formulaelement->_operator,0)=='!')break; // break on <=, >= and !=
				if(string_length(_formulaelement->_operator)>1&&string_char(_formulaelement->_operator,0)=='=')break; // break on ==
				if(_lastAssignmentFormulaelement)_formulaelement->_prev=_lastAssignmentFormulaelement; // MDH@21MAY2019: in order to be able to traverse back!!!
				_lastAssignmentFormulaelement=_formulaelement;
				numberOfAssignments++;
				_formulaelement=_formulaelement->_next;
			}
			if(amVerbose())output("Number of assignments: %u.\n",numberOfAssignments);

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
						assignValue(&_result,applyBinaryOperator(string(_formulaelement->_operator),getReferencedValue(_valuereference),_result));
						// replacing:	assignValue(&_result,applyBinaryOperator(string(_formulaelement->_operator),getValue(_Menvironment,_valuereference->_name),_result));
					}
					setReferencedValue(_valuereference,_result);
					assignValue(&_result,getReferencedValue(_valuereference)); // should we do this???? well, in case the assignment failed!!!
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
			assignValue(&_expressionValue,_result); // MDH@21MAY2019: this will increment the reference count of _result so it makes sense to actually decrement its reference count after being used

			// free the formula
			Mformulaelement* _nextformulaelement;
			_formulaelement=formula;
			while(_formulaelement){
				free_string(_formulaelement->_operator);
				free_valuereference(_formulaelement->_operand);
				_nextformulaelement=_formulaelement->_next;
				free(_formulaelement);
				_formulaelement=_nextformulaelement;
			}
		}else
		if(amVerbose())outputLine("No result to store.");
	}
	if(amVerbose())outputValue("Expression value: '",_expressionValue,"'.\n");
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
	Mtoken* pCommandToken=pCommandToEvaluate; // TODO can we get rid of using commandcount-1 here????
	while(pCommandToken){
		// if we bump into a comment we're done!!!
		if(pCommandToken->type==TT_COMMENT)break;
		// TODO there must be a better way to do the coloring!!!
		if(color){string_append(commandText,ES"38;5;");string_append(commandText,getTokenColor(pCommandToken->type));string_append_char(commandText,'m');} // assuming the same back color is used on ALL tokens, so we won't have to pass that along
		string_append(commandText,string(pCommandToken->text));
		// MDH@03MAY2019: place an asterisk in front of the type to indicate that expr is NOT null!!
		if(amAssisting()){
			if(color){string_append(commandText,ES"38;5;");string_append(commandText,getInfoColor());string_append_char(commandText,'m');}
			string_append_char(commandText,'(');if(pCommandToken->expr)string_append_char(commandText,'*');string_append(commandText,TOKENTYPE_STRING[pCommandToken->type]);string_append(commandText,") ");
		}
		pCommandToken=pCommandToken->next;
	}
	if(color)if(!amAssisting()){string_append(commandText,ES"38;5;");string_append(commandText,getInfoColor());string_append_char(commandText,'m');} // reset to info color
	return commandText;
}

void clearCommand(){
	pLastCommandToEvaluateToken=NULL;
	// a small precaution here!!!
	if(pCommandToEvaluate){freeToken(pCommandToEvaluate);pCommandToEvaluate=NULL;}
}
void outputValueColored(Mvalue* _value){
	if(_value)
	switch(_value->type){
		case VT_TOKEN:outputTokenTypeColor(_value->value._token->type);output(string(_value->value._token->text));break; // easy the token type determines the color to use!!!
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
		case VT_REAL:outputTokenTypeColor(TT_REAL);outputValue(NULL,_value,NULL);break;
		case VT_TEXT:outputTokenTypeColor(_value->value._text->presuffix=='"'?TT_DQSTRING:TT_SQSTRING);outputValue(NULL,_value,NULL);break;
		case VT_LIST:
			// TODO not using _getListText() as defined in Mexecution
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
		default:
			break;
	}
	resetOutputColor();
}

// anything the user types is a sequence of tokens which we can store in a linked list
bool evaluateCommand(){
	
	/// NOT HERE!! outputChar('\n'); // indicating that the command is being evaluated!!!

	// 1. if no command nothing evaluated TODO don't call when this is the case though
	if(!pCommandToEvaluate){outputError("Nothing to evaluate!");return false;}
	
	// 2. if the last token is a comment, remove it before further evaluation TODO should we unfinish the token??????
	//    as a result pLastCommandToEvaluateToken and pCommandToEvaluate could now both be NULL, that's why we test this first
	if(pLastCommandToEvaluateToken->type==TT_COMMENT)removeToken();

	// 3. any command always has two significant tokens TODO could compare pCommandToEvaluate with pLastCommandToEvaluateToken which should be different!!!
	//    in this case we clear the command, so that the command won't be repeated, and the user can switch to control mode immediately with the Enter key!!
	if(pCommandToEvaluate==pLastCommandToEvaluateToken->expr){outputError("Empty command.");clearCommand();return false;}

	// 2. if the last token is an error, can't evaluate (well, better not)
	// TODO it makes sense to remove the error token
	if(pLastCommandToEvaluateToken->type==TT_ERROR){outputError("Can't evaluate erroneous command.");removeToken();unfinishToken();return false;}

	// 3. if the last token is an operator of sorts the command is incomplete
	if(pLastCommandToEvaluateToken->type<=8){outputError("Value behind operator at end of command missing.");return false;}

	// MDH@03MAY2019: this is new, if expr is not NULL apparently we have missing parentheses!!!!
	//                BUT given that the first token always is of type TT_EXPRESSION and the last token will be pointing to it when complete we'd have to check for that too
	//                    this actually means that if expr is NULL there's one parentheses too many!!!
	/*
	if(!pLastCommandToEvaluateToken->expr){outputError("Too many parentheses!");return false;}
	if(pLastCommandToEvaluateToken->expr!=pCommandToEvaluate){outputError("Not enough parentheses!");return false;}
	*/
	// MDH@22MAY2019: the following is complex because we might be right behind the closing of a list, map or function call, in which case the command is still complete!!!
	// MDH@27MAY2019: the last token should now either point to the first token in the command, or to something that does point to the first token in the command
	//////////// already noticed while entering the expression!!!!: if(!pLastCommandToEvaluateToken->expr){outputError("Too many parentheses!");return false;}
	if(pLastCommandToEvaluateToken->expr){
		// this is allowed if this token ends something that points to NULL
		if((pLastCommandToEvaluateToken->type!=TT_END_OF_LIST&&pLastCommandToEvaluateToken->type!=TT_END_OF_FUNCTION_CALL&&pLastCommandToEvaluateToken->type!=TT_END_OF_MAP)||pLastCommandToEvaluateToken->expr->expr){
			switch(pLastCommandToEvaluateToken->expr->expr->type){
				case TT_LIST:outputError("Missing end of list.");break;
				case TT_FUNCTION_CALL:outputError("Missing end of function call!");break;
				case TT_MAP:outputError("Missing end of map!");break;
				default:outputError("Not enough parentheses.");break;
			}
			return false;
		}
	}

	// 4. can't end with function of function call
	// MDH@20JUL2019: BUT we can treat the function as (new) variable, although new variables should not occur at the end of a command???
	if(pLastCommandToEvaluateToken->type==TT_FUNCTION){outputError("Function call missing at end of command.");return false;}
	if(pLastCommandToEvaluateToken->type==TT_FUNCTION_CALL){outputError("Unfinished function call.");return false;}
	if(pLastCommandToEvaluateToken->type==TT_LIST||pLastCommandToEvaluateToken->type==TT_LISTELEMENT){outputError("Unfinished list.");return false;}
	if(pLastCommandToEvaluateToken->type==TT_DQSTRING||pLastCommandToEvaluateToken->type==TT_SQSTRING){outputError("Unfinished string literal.");return false;}
	if(pLastCommandToEvaluateToken->type==TT_EXPRESSION){outputError("Unfinished expression.");return false;}
	if(pLastCommandToEvaluateToken->type==TT_MAP||pLastCommandToEvaluateToken->type==TT_MAP_VALUE){outputError("Unfinished map.");return false;}

	// evaluating means getting the value of the expression that pCommandToEvaluate points to
	// NOTE that the first token is always a dummy token (which will at most contain the whitespace at the start of the command)
	Mstring* commandText=_getCommandText(true);
	// plug the token following the dummy starting token of the command into the current execution environment (typically _Menvironment I suppose)
	getEnvironment()->expressionToken=pCommandToEvaluate->next; // initialize the (current) expression token
	clock_t then=clock();
	Mvalue* _commandExpressionValue=getValueOfExpression("command",'e',(TokenType[]){},0);
	long long elapsed=(clock()-then)/1000;if(elapsed>0)output("The evaluation took %d ms.\n",elapsed);/////////else output("less than 1 ms.");
	// output the commandText
	output("%s = ",string(commandText));
	// if the result is a null value, show the NULL_value
	outputValueColored(isNull(_commandExpressionValue)?NULL_value:_commandExpressionValue);
	
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

// MDH@24APR2019: writeCommand() writes the command to evaluate, and sets pLastCommandToEvaluateToken in the process
void writeCommand(){Mtoken* token=pCommandToEvaluate;while(token){outputToken(pLastCommandToEvaluateToken=token);token=token->next;}}

uint32_t commandPage=0; // the command page to show (when 0 not paging through the commands)
uint32_t commandPages=0; // the total number of command pages
void setCommandPage(uint32_t newCommandPage){
	commandPage=newCommandPage;
	int32_t commandToShowIndex=10,lastCommandToShowIndex=commandCount-(commandPage*10);
	while(--commandToShowIndex>=0&&lastCommandToShowIndex+commandToShowIndex>=0){
		resetOutputColor();
		output("%d. ",lastCommandToShowIndex+commandToShowIndex+1);
		Mtoken* token=commands[lastCommandToShowIndex+commandToShowIndex];
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
	if(cursorPosition()<commandLength()){
		Mstring* restOfCommand=__string();
		if(restOfCommand!=NULL){
			uint16_t tokenPosition=cursorPosition()-pLastCommandToEvaluateToken->offset;
			if(tokenPosition)string_append(restOfCommand,string_remainder(pLastCommandToEvaluateToken->text,tokenPosition));
			string_setlength(pLastCommandToEvaluateToken->text,tokenPosition); // the new length of the token (cutting off what's behind it)
			// now to append the text in the rest of the tokens
			Mtoken* token=pLastCommandToEvaluateToken->next;
			if(token!=NULL){
				while(token!=NULL){string_append(restOfCommand,string(pLastCommandToEvaluateToken->text));token=token->next;}
				freeToken(token); // we'll free all the token starting at the successor of pLastCommandToEvaluateToken
				pLastCommandToEvaluateToken->next=NULL;
			}
			outputInfo("Rest of command: '%s'.",string(restOfCommand));
			return string(restOfCommand);
		}
	}
	return NULL;
} 
*/
/*
void writeRestOfCommand(){ // writes rest of command assuming pLastCommandToEvaluateToken is not NULL and we are to return to the current cursor position adterwards!!
	uint16_t leftToWrite=commandLength()-cursorPosition();
	if(leftToWrite>0){ // something left to write
		// something of the current token to write?
		if(cursorPosition()>pLastCommandToEvaluateToken->offset){ // part of current token to write
			outputTokenColor(pLastCommandToEvaluateToken);printf("%s",string_remainder(pLastCommandToEvaluateToken->text,cursorPosition()-pLastCommandToEvaluateToken->offset));
		}
		// write the rest of the tokens
		writeTokens(pLastCommandToEvaluateToken->next);
		moveCursorLeft(leftToWrite);
	}
}
*/

char switchToControlMode(char* message){
	if(inputMode!=IM_CONTROL){
		if(inputMode==IM_COMMAND)clearCommand();
		resetOutputColor();
		if(message!=NULL)outputLine(message);
		inputMode=IM_CONTROL;
	}
	///////////outputFlags(); // show the user the current flags!!
	return 'o'; // to make the loop know to quit
	//output("%s\n >> ","Control mode: Flags: Assist Debug - Options: eXit History Shell");
}

void writeBehindCursorText(bool clearAfterBehindCursorText){
	uint16_t l=string_length(behindCursorText);
	if(l||clearAfterBehindCursorText){
		debugWrite("Behind cursor text to write: '%s'.",string(behindCursorText));
		resetOutputColor();
		setColor(getBehindCursorTextColor());
		if(l)output("%s",string(behindCursorText));
		if(clearAfterBehindCursorText){
			outputChar(' ');
			moveCursorLeft(l+1);
		}else
		if(l)
			moveCursorLeft(l); // back to where we started to write the behind cursor text
		if(pLastCommandToEvaluateToken)outputTokenColor(pLastCommandToEvaluateToken); // return to the color of the current token
	}
}

void backToPrompt(){
	// this will be more complicated if the command occupies multiple lines
	// therefore we need to move the cursor left, write a single blank and move the cursor one left again and so on
	// replacing: restoreCursor();clearScreenFromCursor();
	uint16_t cp=cursorPosition();
	while(cp--)backspace(); // MDH@24APR2019 replacing: while(cursorPosition()>0){cursorPosition()--;backspace();}
	/*
	if(cursorPosition()>0){moveCursorLeft(cursorPosition());cursorPosition()=0;}
	clearScreenFromCursor();
	*/
	/* replacing:
	while(characterCount>0){
		characterCount--;
		moveCursorLeft(1);resetOutputColor();outputChar(' ');moveCursorLeft(1);
	}
	*/
}

void setCommandToEvaluate(Mtoken* pCommand){
	pLastCommandToEvaluateToken=pCommandToEvaluate=pCommand;
	writeCommand();
	writeBehindCursorText(false);
}
/**
 * setCommandIndex() accepts @newCommandIndex between 0 and commandCount at most
 * but 0 is now also accepted, returning to show pCommandToEvaluate (if any)
 */
void setCommandIndex(uint32_t newCommandIndex){
	commandIndex=newCommandIndex;
	// it's easier to go to the beginning of the line although we could be on the line below!!!!
	// replacing: 
	backToPrompt();
	clearScreenFromCursor();
	// MDH@24APR2019 obsolete: commandLength()=cursorPosition()=0; // do we need this????
	string_setlength(behindCursorText,0); // clear the behind cursor text (in any situation)
	if(commandIndex){
		Mtoken* token=commands[commandCount-commandIndex];
		// MDH@19APR2019: if not to accept the history command we use the previous command as behind cursor text
		if(amAcceptinghistorycommand()){ // use the history command as autocompletion text instead of accepting it immediately as command!!!
			setCommandToEvaluate(token);
			inputInfo("Showing registered command #%u.",(commandCount-commandIndex+1));
			return;
		}
		// the previous command will be used as behind cursor text!!
		while(token){
			///////outputText("(%s)",tokenText);
			string_append(behindCursorText,string(token->text));
			token=token->next;
		}
	}
	setCommandToEvaluate(NULL);
	clearInfo();
	/////////////printf("(%d)",commandLength());
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

void newCommand(){
	// MDH@24APR2019 obsolete: commandLength()=string_length(behindCursorText); // MDH@21APR2019: oops was 0 before...
	resetOutputColor(); // TODO do we need this here?????
	pLastCommandToEvaluateToken=pCommandToEvaluate=_getToken(NULL);
	// MDH@27MAY2019: NO let's just keep expr NULL!!!
	pCommandToEvaluate->expr=NULL; // TODO do I need this???? YES, because we used _getToken()! PERHAPS NOT as prevToken is NULL???????
}

// TODO copyCommand() should set ->expr correctly
void copyCommand(){
	// if fails to copy pCommandToEvaluate pLastCommandToEvaluateToken should end up as NULL
	pLastCommandToEvaluateToken=NULL;
	Mtoken* _tokenToCopy=pCommandToEvaluate;
	pCommandToEvaluate=NULL;
	// the essence is that pLastCommandToEvaluateToken points to the last token in pCommandToEvaluate
	// NOTE theoretically pLastCommandToEvaluateToken could be NULL due to _getToken() failing to create a new token
	while(_tokenToCopy){
		pLastCommandToEvaluateToken=_getToken(pLastCommandToEvaluateToken);
		pLastCommandToEvaluateToken->type=_tokenToCopy->type;
		/* TODO check whether the following is correct!!! guess not!!
		if(pLastCommandToEvaluateToken->type==TT_END_OF_FUNCTION_CALL||pLastCommandToEvaluateToken->type==TT_END_OF_LIST||pLastCommandToEvaluateToken->type==TT_END_OF_MAP){
			if(_tokenToCopy->expr)
				pLastCommandToEvaluateToken->expr=pLastCommandToEvaluateToken->expr->expr;
			else
				outputLine("BUG: End of argument list or map encountered, but not started.");
		}
		*/
		pLastCommandToEvaluateToken->expr=_tokenToCopy->expr; // MDH@20MAY2019: just copy the expr over!!!!
		pLastCommandToEvaluateToken->significantCharacterCount=_tokenToCopy->significantCharacterCount;
		// if failing to copy the text over get rid of the command constructed so far, and break
		pLastCommandToEvaluateToken->text=_stringCopy(_tokenToCopy->text,0);
		if(!pLastCommandToEvaluateToken->text){pLastCommandToEvaluateToken=NULL;break;}
		// MDH@24APR2019 obsolete: commandLength()+=string_length(pLastCommandToEvaluateToken->text);
		// some additional fields to copy over (NOT the offset is that is set automatically)
#ifdef __DEBUG__
        printf("%d:%s",pLastCommandToEvaluateToken->type,string(pLastCommandToEvaluateToken->text));
#endif
		if(!pCommandToEvaluate)pCommandToEvaluate=pLastCommandToEvaluateToken;
		// get the next token to copy...
		_tokenToCopy=_tokenToCopy->next;
	}
}

// NEWYEAR'S DAY 2019: It's a nuisance to show a command without copying it into an actual newCommand
/**
 * setCommand() creates a new (empty) command (in pCommandToEvaluate) and initializes it to the token in pNewCommand (the command pointed to by commandIndex)
 *              which is supposedly showing behind the cursor!!!
 * ASSUMPTION should only be called when at the prompt (cursorPosition()=0) ready for starting or changing a command
 * setCommand() won't show the command anymore as we assume that any registered command passed in is already showing!!!
 */
/*
Mtoken* getCommand(){
	return(commandIndex&&cursorPosition()?commands[commandCount-commandIndex]:pCommandToEvaluate);
}
void echoCommand(){
	Mtoken* token=pCommandToEvaluate;
	resetOutputColor();
	while(token){printf("%s",string(token->text));token=token->next;}
}
void setCommand(Mtoken* pNewCommand){
	// ASSERT let's assume we're at the prompt (i.e. cursorPosition()==0 and pCommandToEvaluate==NULL)
	// NO we cannot assume that because there might be a command currently showing at the prompt
	if(pCommandToEvaluate){clearCommand();backToPrompt();} // if we have a command get rid of it and ascertain to be at the prompt!!
	// the problem is that we do NOT want to actually change the new command, so we have to copy it somehow
	newCommandToEvaluate(); // NOTE might fail, in which case pLastCommandToEvaluateToken will be NULL!!
	if(pNewCommand){ // something to copy
		// at least once we need to set pLastCommandToEvaluateToken!!!
		Mtoken* pNewToken=pNewCommand; // first token to copy!!
		// NOTE theoretically pLastCommandToEvaluateToken could be NULL due to _getToken() failing to create a new token
		while(pLastCommandToEvaluateToken){
			// if failing to copy the text over get rid of the command constructed so far, and break
			if(!_stringCopy(pNewToken->text,pLastCommandToEvaluateToken->text)){clearCommand();break;}
			// MDH@24APR2019 obsolete: commandLength()+=string_length(pLastCommandToEvaluateToken->text);
			// some additional fields to copy over (NOT the offset is that is set automatically)
			pLastCommandToEvaluateToken->type=pNewToken->type;
#ifdef __DEBUG__
            printf("%d:%s",pLastCommandToEvaluateToken->type,string(pLastCommandToEvaluateToken->text));
#endif
			pNewToken=pNewToken->next;
			if(!pNewToken)break;
			// we're going to need another token!!!
			pLastCommandToEvaluateToken=_getToken(pLastCommandToEvaluateToken);
		}
		// if the user decides to start typing ascertain to show it in the right color!!
		if(pLastCommandToEvaluateToken)outputTokenColor(pLastCommandToEvaluateToken);
#ifdef __DEBUG__
		echoCommand();
#endif
	}
}
*/

// MDH@30APR2019: if the current token is a variable/function check whether it still is
//                call whenever the current token changes (in removePreviousTokenCharacter() and commandCharacterAccepted())
bool tokenCheckedForBeingAFunction(bool endOfInput){
	bool result=false;
	if(pLastCommandToEvaluateToken->type==TT_VARIABLE||pLastCommandToEvaluateToken->type==TT_NEW_VARIABLE){
		result=true;
		// is it a function (now)?
		if(getFunction(_Menvironment,string(pLastCommandToEvaluateToken->text))){ // yes, it is
			// if a new variable before (now a function), remove the (assignment) character in the behind cursor text
			if(amMatchingparentheses())if(pLastCommandToEvaluateToken->type==TT_NEW_VARIABLE)if(string_char(behindCursorText,0)=='=')string_removed_char(behindCursorText,0);
			// the minimum we can do is put an opening parenthesis in the behind cursor text
			pLastCommandToEvaluateToken->type=TT_FUNCTION;
			reoutputToken(pLastCommandToEvaluateToken);
			// insert an opening parenthesis for the function call
			if(endOfInput)if(amMatchingparentheses())if(string_char(behindCursorText,0)!='(')string_insert_char(behindCursorText,0,'(');
		}
	}else
	if(pLastCommandToEvaluateToken->type==TT_FUNCTION){
		result=true;
		// is it (still) a function?
		if(!getFunction(_Menvironment,string(pLastCommandToEvaluateToken->text))){ // no, it ain't
			// the minimum we can do is remove the opening parenthesis behind it (if it is still there!!!!!)
			pLastCommandToEvaluateToken->type=TT_VARIABLE;
			reoutputToken(pLastCommandToEvaluateToken);
			//////////outputInfo("Variable redrawn!");
			// remove any opening parenthesis from the behind cursor text
			if(endOfInput)if(amMatchingparentheses())if(behindCursor())if(string_char(behindCursorText,0)=='(')string_removed_char(behindCursorText,0);
		}
	}
	// non-existing variables should be assigned to so it's a good idea to put the assignment operator behind it, although it might be hard to remove it though
	if(result){ // a variable or function
		char* _identifierName=_stringstart(pLastCommandToEvaluateToken->text,pLastCommandToEvaluateToken->significantCharacterCount); // free asap
		// check whether the variable exists or not
		if(pLastCommandToEvaluateToken->type==TT_VARIABLE){ // a (new) variable
			if(!containsVariable(_Menvironment,_identifierName)){ // apparently does NOT exist
				pLastCommandToEvaluateToken->type=TT_NEW_VARIABLE;
				reoutputToken(pLastCommandToEvaluateToken);
				if(endOfInput)if(amMatchingparentheses())if(string_char(behindCursorText,0)!='=')string_insert_char(behindCursorText,0,'=');
			}
		}else
		if(pLastCommandToEvaluateToken->type==TT_NEW_VARIABLE){ // a new variable
			if(containsVariable(_Menvironment,_identifierName)){ // now an existing variable
				pLastCommandToEvaluateToken->type=TT_VARIABLE;
				reoutputToken(pLastCommandToEvaluateToken);
				if(endOfInput)if(amMatchingparentheses())if(string_char(behindCursorText,0)=='=')string_removed_char(behindCursorText,0);
			}
		}
		free(_identifierName); // freed
	}
	return result;
}

// in response to backspace the previous token character is to be removed
void removePreviousTokenCharacter(){ // NOTE always due to a backspace!
	char removedCharacter=removedTokenCharacter(1);
	if(removedCharacter){
		// adapt screen
		moveCursorLeft(1); // will decrement cursorPosition() // MDH@24APR2019: NOT anymore...
		clearScreenFromCursor(); // will clear what's behind the cursor
		// MDH@24APR2019 obsolete: commandLength()--; // decrement the total command length
		if(cursorPosition()){ // still something left of the command (that we might check for being a function or not)
			// on screen as well please
			// before writing the behind cursor text we're going to check whether the current token still is a function or variable
			tokenCheckedForBeingAFunction(true);
			// MDH@27FEB2019: if what's behind the cursor is NOT in the command but in behindCursorText that's what we should now write
			writeBehindCursorText(false);
			// replacing: if(pCommandToEvaluate)writeRestOfCommand(); // write all characters at and after the cursor (will reset the cursor!!)
		}/* removedTokenCharacter() calls removeToken which will NULL the pCommandToEvaluate and pLastCommandToEvaluateToken when the first command character is removed, in which case we do not need:
			else clearCommand();*/
	}else // MDH@03MAY2019: can't switch to control mode here (so we just report the error!!!)
		inputError("%s","Failed to remove the last entered character.");
}

void outputTokenInfo(){
	Mtoken* token=pCommandToEvaluate;
	uint16_t tokenIndex=0;
	output("%s:\n","Tokens");
	output("%s\t%s\t%s\t%s\t%s\t\t\t%s\n","#","OFFSET","USED","LENGTH","TYPE","TEXT","EXPR");
	while(token!=NULL){
		tokenIndex++;
		output("%u\t%u\t%u\t%u\t%-24s`%s`\n",tokenIndex,token->offset,token->significantCharacterCount,string_length(token->text),TOKENTYPE_STRING[token->type],string(token->text));
		if(token->expr){
			output("%s\t%u\t%s\t%s\t%-24s`%s`\n","part of",token->expr->offset,"","",TOKENTYPE_STRING[token->expr->type],string(token->expr->text));
		}
		token=token->next;
	}
}

bool isBinaryOperatorTokenType(uint8_t tokenType){return(TOKENTYPE_IDS[tokenType]>>4)==0b0110;}
bool isOneCharacterTokenType(uint8_t tokenType){
	// TODO how about TT_EXPRESSION -> NO because a TT_EXPRESSION token is always considered ended, i.e. significantCharacterCount is not an issue in determining whether a new token starts there
	return(tokenType==TT_ASSIGNMENT||tokenType==TT_UNARY||tokenType==TT_TERNARY_aeru||tokenType==TT_LIST||tokenType==TT_LISTELEMENT||tokenType==TT_END_OF_LIST||tokenType==TT_MAP||tokenType==TT_END_OF_MAP||tokenType==TT_FUNCTION_CALL||tokenType==TT_END_OF_FUNCTION_CALL);
}

// MDH@09JUL2019: count the number of list elements in front of the current token
uint32_t getListElementCount(){
	uint32_t listElementCount=0;
	Mtoken* token=pLastCommandToEvaluateToken;
	Mtoken* startToken=pLastCommandToEvaluateToken->expr;
	while(token!=startToken){if(token->expr==startToken&&token->type==TT_LISTELEMENT)listElementCount++;token=token->prev;}
	return listElementCount;
}
void changeFunctionTokenToAVariable(bool endOfInput){
	char* _identifierName=_stringstart(pLastCommandToEvaluateToken->text,pLastCommandToEvaluateToken->significantCharacterCount); // free asap
	pLastCommandToEvaluateToken->type=(containsVariable(getEnvironment(),_identifierName)?TT_VARIABLE:TT_NEW_VARIABLE);
	free(_identifierName);
	reoutputToken(pLastCommandToEvaluateToken);
	// I think we should remove ( from the behind cursor text if it was inserted
	if(endOfInput)if(amMatchingparentheses())
	if(string_length(behindCursorText)&&string_char(behindCursorText,0)=='(')
	if(!string_removed_char(behindCursorText,0))inputError("Failed to remove the function argument list opening parenthesis from the feed forward text."); // TODO is there a better way???
	// ready to redetermine the new token type!!!!
}
// MDH@12APR2019: in order to implement the Tab character we have to delegate entering a character (typed) to a separate function
//       		  ASSERTION pCommandToEvaluate and pLastCommandToEvaluateToken are  NOT  NULL
//                the endofinput flag is used to indicate whether this is the end of the input
bool commandCharacterAccepted(char inputChar,char inputCharacterType,bool endOfInput){
	// MDH@21APR2019: there are two situation where we need to get a command
	//                1. we haven't got one 2. we have got a registered command which hasn't changed yet (in which case commandIndex will still be positive)
	if(!pCommandToEvaluate) // no current command
		newCommand(); // we need to make a new token (to start the command to evaluate)
	else // we have a current command BUT 
	if(commandIndex)
		copyCommand();
	// if pLastCommandToEvaluateToken is now NULL something went wrong (in copyCommand or newCommand most likely)
	if(pLastCommandToEvaluateToken==NULL)return false;
	commandIndex=0; // to indicate we are now working with a NEW command (even if we fail to accept the character!!!)
	clearInfo(); // TODO make a separate function to do this???

	/* MDH@28MAR2019: if the user enters the comment character we should toggle the token type's highest bit (bit 7)
	if(inputCharType=='C'){
		pLastCommandToEvaluateToken->type^=0x70; // toggling bit 7
		// a comment character will NEVER change the (actual) token type but it should change the color to use
		if(pLastCommandToEvaluateToken->type&0x70){commenting=true;outputTokenColor(pLastCommandToEvaluateToken);}else notCommenting=true; // if a comment was started, switch to the comment token color
	}else // not a comment character
	if((pLastCommandToEvaluateToken->type&0x70)==0){ // not in a comment
		if(notCommenting){notCommenting=false;outputTokenColor(pLastCommandToEvaluateToken);} // if behind coming out of a comment, we have to reset the output token color
	*/
	/*
	// MDH@26FEB2019: when a user starts inserting characters instead of appending them we can cut off the rest of the characters in the command
	//                and put it in a single Mstring instance and append these one at a time 
	char* removed=removedRestOfCommand();
	*/
	// determine the token type associated with the newly inputted character
	// MDH@28MAR2019: if we're in a binary token type with the repeatable flag set AND the user has repeated the previous first token character the inputCharacterType should become R to get the right transition
	if((TOKENTYPE_IDS[pLastCommandToEvaluateToken->type]&0x62)==0x62)if(inputChar==string_char(pLastCommandToEvaluateToken->text,0))inputCharacterType='R';
	// MDH@16APR2019: W indicates a whitespace character BUT it is NOT a functional whitespace character in a comment, an error, or a string literal
	if(inputCharacterType=='W')if(pLastCommandToEvaluateToken->type==TT_ERROR||pLastCommandToEvaluateToken->type==TT_COMMENT||pLastCommandToEvaluateToken->type==TT_DQSTRING||pLastCommandToEvaluateToken->type==TT_SQSTRING)inputCharacterType='w';
	int16_t newTokenType=0; // MDH@05JUN2019: we need newTokenType AFTER appending the last character allowed in a token (like q behind a integer or real)
	if(inputCharacterType!='W'){ // only characters that are not whitespace can start a new token
		newTokenType=nextTokenType(pLastCommandToEvaluateToken->type,inputCharacterType); // MDH@22MAR2019: this is a bit of a quick fix, so whitespace never ends up in nextTokenType() as whitespace never ends the current token, or changes its type
#ifdef __DEBUG__
	resetOutputColor();
	printf("[%d+%c->%d]",pLastCommandToEvaluateToken->type,inputCharacterType,newTokenType);
	outputTokenColor(pLastCommandToEvaluateToken);
#endif
		//MDH@17JUL2019: typically we'd get an error immediately when NOT entering a function call character ( behind a function identifier
		if(newTokenType==TT_ERROR&&pLastCommandToEvaluateToken->type==TT_FUNCTION){
			// we should assume that the identifier represents a (new) variable (identifier)
			changeFunctionTokenToAVariable(endOfInput);
			newTokenType=nextTokenType(pLastCommandToEvaluateToken->type,inputCharacterType);
		}

		// TODO just like unary operators expressions, maps and list end immediately
		// some combinations are (still) not allowed...
		if(newTokenType<0||newTokenType==pLastCommandToEvaluateToken->type){
			/* MDH@27MAY2019: most of the time we do allow the same one-character token behind another!!!
			MDH@12JUL2019: BUT NOT ALWAYS (values and binary operator e.g.) I have to think this through again */
			if(newTokenType==pLastCommandToEvaluateToken->type&&pLastCommandToEvaluateToken->significantCharacterCount>0)
			// MDH@16APR2019: most tokens cannot follow each other directly except for unary and TODO ternary operators and list element tokens (although undefined list element cells do not need to be inserted!!)
			if(pLastCommandToEvaluateToken->type!=TT_UNARY&&pLastCommandToEvaluateToken->type!=TT_TERNARY_aeru&&pLastCommandToEvaluateToken->type!=TT_LISTELEMENT){
				newTokenType=TT_ERROR;
				if(amVerbose())inputError("Token already finished!");
			}
		}else{ // different token types
			// a shortcut assignment can NOT be turned into a equality comparison
			if(inputCharacterType=='='&&pLastCommandToEvaluateToken->type==TT_ASSIGNMENT&&(pLastCommandToEvaluateToken->prev->type==TT_BINARY_AeRu||pLastCommandToEvaluateToken->prev->type==TT_BINARY_Aeru)){
				newTokenType=TT_ERROR;
				if(amVerbose())inputError("A shortcut operator assignment cannot change into an equality.");
			}
		}
		// MDH@03MAY2019: no matter what the new token type is, any token of type TT_EXPRESSION always ends immediately...
		//                this is because the first (offset) token in a command is always of type TT_EXPRESSION which should end immediately on any next token although significantCharacterCount will still be zero
		//                this way it will always be there!!
		if(newTokenType<0){

		}else
		if(newTokenType!=pLastCommandToEvaluateToken->type||pLastCommandToEvaluateToken->type==TT_EXPRESSION||pLastCommandToEvaluateToken->significantCharacterCount>0){

			// MDH@10APR2019: NOT every new token type starts a new token:
			//                if we're in a binary operator and move to another binary operator type it's an extension
			//                NO we decide NOT to do this when the command is evaluated we should compose the values and apply the operators
			///////if(!isBinaryOperatorTokenType(pLastCommandToEvaluateToken->type)||!isBinaryOperatorTokenType(newTokenType))

			// MDH@16APR2019: a character that is assumed to indicate the assignment operator has to be checked because it could well be the = that starts the binary equality operator
			//                which means we have to switch from assignment token to BearU token (which is unfinished)
			if(newTokenType==TT_ASSIGNMENT){
				// checking for validity of accepting as assignment is not that easy
				// we can allow a binary operator in front of the assignment of course in that case it definitely is an assignment if it is not the = is an error!!
				bool behindBinaryOperator=(pLastCommandToEvaluateToken->type==TT_BINARY_AeRu||pLastCommandToEvaluateToken->type==TT_BINARY_Aeru);
				// NOTE if behind binary operator there must always be a token in front of it, so pLastCommandToEvaluateTokenToCheck cannot be NULL!!
				// MDH@21MAY2019: possibly we have multiple tokens representing a binary operator (like ** << and >> which are allowed!!!) so we need to skip all binary operators in front of the assignment character
				Mtoken* pLastCommandToEvaluateTokenToCheck=pLastCommandToEvaluateToken;
				if(behindBinaryOperator)while(pLastCommandToEvaluateTokenToCheck->type>=3&&pLastCommandToEvaluateTokenToCheck->type<=7)pLastCommandToEvaluateTokenToCheck=pLastCommandToEvaluateTokenToCheck->prev;
				if(amVerbose())inputInfo("Type of token to check: %s.",TOKENTYPE_STRING[pLastCommandToEvaluateTokenToCheck->type]);
				// ASSERT pLastCommandToEvaluateTokenToCheck should either represent a variable or the end of a list element to allow for operator
				if(pLastCommandToEvaluateTokenToCheck->type==TT_END_OF_LIST){ // end of a list
					// we have to find the associated start of the list, and the token in front of that (which should be a variable!!!)
					// which is easy because the expr tells us the start of the list BUT 
					pLastCommandToEvaluateTokenToCheck=pLastCommandToEvaluateTokenToCheck->expr;
					///////////if(amVerbose())inputInfo("Presumed list start token");
					if(pLastCommandToEvaluateTokenToCheck)pLastCommandToEvaluateTokenToCheck=pLastCommandToEvaluateTokenToCheck->prev;else inputError("%s","Start of index list not found!");
				}
				// two options: = behind a binary operator without variable (or list) in front of it is not allowed, i.e. an error, otherwise we assume that = represents the first = of == the equality operator...
				if(pLastCommandToEvaluateTokenToCheck==NULL||(pLastCommandToEvaluateTokenToCheck->type!=TT_VARIABLE&&pLastCommandToEvaluateTokenToCheck->type!=TT_NEW_VARIABLE)){
					if(behindBinaryOperator){
						newTokenType=TT_ERROR;
						//if(amVerbose())inputError("No variable to assign to.");
					}else
						newTokenType=TT_BINARY_aErU;
				}
			}

			pLastCommandToEvaluateToken=_getToken(pLastCommandToEvaluateToken);
/*
#ifdef __DEBUG__
			printf("@%p=%p?:%s",pCommandToEvaluate,pLastCommandToEvaluateToken,string(pCommandToEvaluate->text));
#endif
*/
			// ending a function call, list or map is only allowed with expr defined
			if(newTokenType==TT_END_OF_FUNCTION_CALL||newTokenType==TT_LISTELEMENT||newTokenType==TT_END_OF_LIST||newTokenType==TT_END_OF_MAP){
				if(pLastCommandToEvaluateToken->expr){ // i.e. pointing to some token that should be of the right type!!!
					// check whether the match is correct
					switch(newTokenType){
						case TT_END_OF_FUNCTION_CALL:
							if(pLastCommandToEvaluateToken->expr->type!=TT_FUNCTION_CALL&&pLastCommandToEvaluateToken->expr->type!=TT_EXPRESSION){
								/////inputError("%s","No function call or expression to end here!");
								inputError("End of function call/expression character does not match '%s' of type '%s'!",string(pLastCommandToEvaluateToken->expr->text),TOKENTYPE_STRING[pLastCommandToEvaluateToken->expr->type]);
								newTokenType=TT_ERROR;
							}
							break;
						case TT_LISTELEMENT:
							// MDH@09JUL2019: a comma (starting a list element) is allowed in a function call accepting multiple parameters)
							if(pLastCommandToEvaluateToken->expr->type!=TT_LIST&&pLastCommandToEvaluateToken->expr->type!=TT_MAP){
								// so if it's a function call it might be allowed
								if(pLastCommandToEvaluateToken->expr->type!=TT_FUNCTION_CALL){
									inputError("First expression token '%s' of type '%s' does not start a list or map!",string(pLastCommandToEvaluateToken->expr->text),TOKENTYPE_STRING[pLastCommandToEvaluateToken->expr->type]);
									newTokenType=TT_ERROR;
								}else{
									// the token in front of the function call token should denote a function
									char* functionName=string(pLastCommandToEvaluateToken->expr->prev->text);
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
										newTokenType=TT_ERROR;
									}
								}
							}
							break;
						case TT_END_OF_LIST:
							if(pLastCommandToEvaluateToken->expr->type!=TT_LIST){
								inputError("First token '%s' of type '%s' does not start a list!",string(pLastCommandToEvaluateToken->expr->text),TOKENTYPE_STRING[pLastCommandToEvaluateToken->expr->type]);
								newTokenType=TT_ERROR;
							}
							break;
						case TT_END_OF_MAP:
							if(pLastCommandToEvaluateToken->expr->type!=TT_MAP){
								inputError("First expression token '%s' of type '%s' does not start a map!",string(pLastCommandToEvaluateToken->expr->text),TOKENTYPE_STRING[pLastCommandToEvaluateToken->expr->type]);
								//inputError("No map to end here!");
								newTokenType=TT_ERROR;
							}
							break;
					}
					// if we do the following we need to move 
					// TODO we can do the following even on error but what if we didn't???
					///////////////////pLastCommandToEvaluateToken->expr=pLastCommandToEvaluateToken->expr->expr;
				}else{
					inputError("%s","Can't end a (function argument) list or map here!");
					newTokenType=TT_ERROR;
				}
			}

			pLastCommandToEvaluateToken->type=newTokenType;
			
			// MDH@27MAY2019: a lot of tokens are one-character tokens
	
			// MDH@15APR2019: there are some other characters as well, that immediately end the token like parentheses, comma's and semicolons and ? and : TODO are there more??????
			if(pLastCommandToEvaluateToken->significantCharacterCount==0){
				if(isOneCharacterTokenType(newTokenType))pLastCommandToEvaluateToken->significantCharacterCount=1;
				/* replacing:
				if(pLastCommandToEvaluateToken->type!=TT_ERROR&&pLastCommandToEvaluateToken->type!=TT_COMMENT&&pLastCommandToEvaluateToken->type!=TT_DQSTRING&&pLastCommandToEvaluateToken->type!=TT_SQSTRING)
					if(inputCharacterType=='('||inputCharacterType=='['||inputCharacterType=='{'||inputCharacterType==','||inputCharacterType==';'||inputCharacterType==':'||inputCharacterType=='?')
						pLastCommandToEvaluateToken->significantCharacterCount=1;
				*/
			}
			// TODO should we write the associated colors here?????
			outputTokenColor(pLastCommandToEvaluateToken);
		}
	}else // a functional whitespace character, ends a current token!!
	if(pLastCommandToEvaluateToken->significantCharacterCount==0&&pLastCommandToEvaluateToken->type!=TT_EXPRESSION) // MDH@22MAR2019: first whitespace character in a non-whitespace token ends the current token (but should never change its type (see NO_TRANSITIONS))
		pLastCommandToEvaluateToken->significantCharacterCount=string_length(pLastCommandToEvaluateToken->text);

	// append the typed character at cursorPosition() minus current token offset in pLastCommandToEvaluateToken->text
	string_append_char(pLastCommandToEvaluateToken->text,inputChar);

	if(newTokenType<0)pLastCommandToEvaluateToken->significantCharacterCount=string_length(pLastCommandToEvaluateToken->text);

#ifdef __DEBUG__
	printf("[%s]",string(pLastCommandToEvaluateToken->text));
#endif
	// MDH@24APR2019 obsolete: commandLength()++; // increment total command length
	outputChar(inputChar); ///////// replacing: outputLastTokenChar(pLastCommandToEvaluateToken); // echo the last token character
	
	if(endOfInput){
		//////if(amAssisting())output(":%c",inputCharacterType);
		debugWrite("Command length after inserting %c: %" PRIu16 ".",inputChar,commandLength());
	}

	// MDH@24APR2019 obsolete: cursorPosition()++; // increment the current cursor position
	bool notCheckedForBeingAFunction=!tokenCheckedForBeingAFunction(endOfInput); // MDH@28MAY2019: ALWAYS check for being a function!!!!

	if(endOfInput){
		// MDH@29APR2019: I'd like to detect when a variable becomes a function or vice versa
		if(notCheckedForBeingAFunction){
			// MDH@16APR2019: we can check for an unfinished binary operator in which case we should show = behind 
			// MDH@15APR2019: it seems like a good idea to adapt the behind cursor text if we entered the start character of a list (element), map or expression opening parenthesis
			if(pLastCommandToEvaluateToken->type!=TT_ERROR){ // MDH@29APR2019: don't add closing bracket to autocompletion text when in error!!!
				if(pLastCommandToEvaluateToken->type!=TT_BINARY_aErU){
					if(amMatchingparentheses()){
						if(!string_length(behindCursorText)) // MDH@28MAY2019: if there's nothing behind the cursor yes we do append closing stuff
						switch(inputCharacterType){
							case '[':string_insert_char(behindCursorText,0,']');break;
							case '{':string_insert_char(behindCursorText,0,'}');break;
							case '(':string_insert_char(behindCursorText,0,')');break;
							case '"':string_insert_char(behindCursorText,0,'"');break;
							case '\'':string_insert_char(behindCursorText,0,'\'');break;
						}
					}
				}else
					string_insert_char(behindCursorText,0,'=');
			}
		}
		writeBehindCursorText(true); // just in case we removed some character (see TT_FUNCTION->TT_VARIABLE)
		debugWrite("Command length after writing behind cursor text: %" PRIu16 ".",commandLength());
		outputStatus(inputChar,inputCharacterType);
	}

	return true;
}

bool COMMAND_PROCESSOR_AVAILABLE=0;
void clearShellCommand(){
	string_setlength(behindCursorText,0);
	string_setlength(shellCommand,0);
	// MDH@24APR2019 obsolete: cursorPosition()=0;
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
	inputMode=IM_SHELL;
	clearShellCommand();
	return 's';
	//output("%s\n $ ","Enter your shell command, and press the Return button to execute.");
}
void switchToCommandMode(){
	if(inputMode==IM_COMMAND)return;
	inputMode=IM_COMMAND;
	string_setlength(behindCursorText,0); // ascertain to not have autocompletion text
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
					if(argv[arg][i]=='s')setVerbose(true);else
					if(argv[arg][i]=='S')setVerbose(false);else
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
	outputLine("Welcome to M, the fancy interpreter.");
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

	Mstring* predefinedVariableNames=_getVariableNames(_Menvironment,", ");
	if(predefinedVariableNames){
		output("Predefined variables: %s.\n",string(predefinedVariableNames));
		free_string(predefinedVariableNames); // no get rid of it!!!
	}else
		outputLine("No predefined variables!");
	//////////output("Number of predefined variables: %d.",getNumberOfVariables(mEnvironment));
	
	// initialize commands and input mode
	shellCommand=__string(); // MDH@12APR2019: allow executing shell commands (calling system())
	behindCursorText=__string(); // MDH@27FEB2019: create the behind cursor text (to be cleared whenever we start a new command)
	pCommandToEvaluate=NULL; // the current command (token)

	///// writeCommand() will take care of this!!!! commandLength()=0; // keep track of the total command length...
	inputMode=IM_COMMAND; // TODO should this go into promptForUserInput()?

	char inputChar,inputCharType;

	outputLine("");
	outputLine("Use Ctrl-Z to exit M immediately at any time.");
	outputLine("In any mode press the Enter key on an empty line to switch modes.");

	// let's mark the allocations BEFORE we start looping
	addallocationtype('!');

	while(1){

		// if we're supposed to start a new command (i.e. it's not a command continuation)
		promptForUserInput();

		/* MDH@16MAR2019: we're behind the prompt now and should start out without a current command (in pCommnad)
		//                if pCommandToEvaluate is NOT null, we have to make it NULL
		commandIndex=0; // MDH@16MAR2019: pretty essential otherwise it would keep evaluating previous commands
		if(pCommandToEvaluate) // if we still have a command to free, free it entirely
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
			// MDH@24APR2019: pCommandToEvaluate could be non-null if we failed to evaluate it (e.g. when being imcomplete), and we allow a retry
			//                NOTE registered commands should always be successfully evaluated, so do NOT get rid of any pending command!!!!
			if(pCommandToEvaluate){writeCommand();writeBehindCursorText(false);}else string_setlength(behindCursorText,0);
			/////////////// if(pCommandToEvaluate)clearCommand(); // TODO do we need this????
			/* replacing:
			if(pCommandToEvaluate==NULL)if(!string_setlength(behindCursorText,0))output("??"); // TODO should we be loosing behindCursorText here????
			commandLength()=cursorPosition()=writeTokens(pCommandToEvaluate);
			*/
			/////////outputStatus();
		}
		// which used to be: writeCommand(); // write the current command (if any)

		/* replacing:
		pLastCommandToEvaluateToken=pCommandToEvaluate;
		// an existing command to show
		while(pLastCommandToEvaluateToken!=NULL){
			outputToken(pLastCommandToEvaluateToken);
			// the cursor will move along with every printf()
			cursorPosition()+=string_length(pLastCommandToEvaluateToken->text);
			pLastCommandToEvaluateToken=pLastCommandToEvaluateToken->next;
		}
		*/
		// we do NOT need a command until after the first character which makes sense because we allow ` and arrow up and down to switch to option mode or select another command
		// now we need to read characters one at a time and echo them from the command line
		// Ctrl-D to exit M
		while(inputCharRead(&inputChar)){
			
			////////inputChar=getInputChar();

			if(inputChar>127)continue; // undefined input character

			inputCharType=INPUTCHARACTERTYPES[inputChar];
			
			////////printf("(%d)",inputCharType);

			// if not in control mode, and the switch to control mode character is entered, switch to control mode if first character (NOTE cursorPosition() is only defined in the other two modes)
			// MDH@16APR2019: I want to use the Enter key (ASCII 13) to switch to the next mode, because the associated input character type is n which will ALWAYS break
			//                in that case we do NOT need the o input character type!!!
			if(inputCharType=='o'){
				if(inputMode==IM_CONTROL){
					switchToCommandMode();
					break;
				}
				// not in control mode, go to control mode if first character on line
				if(!cursorPosition()){
					inputCharType=switchToControlMode(NULL);
					break;
				}
				// accept (might be an acceptable character in string literal in commands or in shell commands)
			}

			/////if(inputChar!=ESCAPE_CHARACTER)printf("(%d)",inputChar);

			// special (control) input character types
			// first the ones that will break in any input mode!!!!
			if(inputCharType=='i')continue; // insignificant input character without specific purpose

			if(inputCharType=='n')break; // end-of-line (CR of LF) character
			if(inputCharType=='x')break; // eXit (Ctrl-C or Ctrl-Z) character

			// from now on no continue's anymore, because at the end of the loop we want to check for inputCharType equaling o
			if(inputMode==IM_COMMAND){
#ifdef __DEBUG__
				outputChar(inputCharType);
#endif
				//////////outputStatus(inputChar,inputCharType);
				if(inputCharType=='d'){ // MDH@18APR2019: delete now always deletes the first character in the behind cursor text
					/////debugWrite("DELETE");
					if(string_length(behindCursorText)){
						if(string_removed_char(behindCursorText,0)){ // success!!!
							writeBehindCursorText(true);
						}else
							inputCharType=switchToControlMode("Failed to remove the first character in the auto-complete text.");	
					}else
						beep();
				}else
				if(inputCharType=='b'){ // backspace
					///////debugWrite("BACKSPACE");
					// something to remove?
					if(cursorPosition()) // TODO pCommandToEvaluate should be NULL at the same time commandLength() becomes 0!!!
						removePreviousTokenCharacter();
					else // nothing to remove
						beep();
				}else
				if(inputCharType=='c'){ // cancel command (Ctrl-D)
					// replacing: if(pCommandToEvaluate!=NULL){clearCommand();break;}beep(); 
					if(pCommandToEvaluate!=NULL){
						backToPrompt();
						clearCommand();
						/////////if(amWrapping()())break; // if in amWrapping()() can't guarantee backspace() to move into the previous line which means just prompt again...
					}else
						beep();
				}else
				if(inputCharType=='t'){ // Tab character
					// if there's a preview (well, code completion by way of a behindCursorText)
					uint16_t bc=behindCursor();
					if(bc){
						// normally all will be Ok, and we can (post)decrement bc until it's zero
						while(bc--){
							char newInputChar=string_removed_char(behindCursorText,0);
							if(!newInputChar){
								writeBehindCursorText(false); // there will be characters behind the cursor left to show
								inputCharType=switchToControlMode("Failed to remove the suggested character.");
								break;
							}
							// MDH@24APR2019 obsolete: commandLength()--; // until we manage to insert the character removed, we have one less character in the total command length
							if(!commandCharacterAccepted(newInputChar,INPUTCHARACTERTYPES[newInputChar],bc==0)){
								inputCharType=switchToControlMode("Failed to accept the suggested character.");
								break;
							}
						}
					}else
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
											if(string_length(behindCursorText)){
												if(string_removed_char(behindCursorText,0))
													writeBehindCursorText(true);
												else
													inputCharType=switchToControlMode("Failed to remove the first character of the auto-completion text.");	
											}else // nothing under the cursor to delete
												beep();
										}
									}
								}else
								if(inputChar==65){ // up arrow 
									if(inputMode==IM_COMMAND){ // i.e. show previous command if any
										if(!commandIndex&&pCommandToEvaluate)
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
										if(!commandIndex&&pCommandToEvaluate)
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
									if(behindCursor()){
										// MDH@22MAR2019: instead of doing everything here (duplicating all code that is down below), we can find a way to use the 'normal' code
										////////////bool success=false;
										char newInputChar=string_removed_char(behindCursorText,0);
										if(newInputChar){
											// MDH@24APR2019: commandLength()--;
											if(!commandCharacterAccepted(newInputChar,INPUTCHARACTERTYPES[newInputChar],true))
												inputCharType=switchToControlMode("Suggested character extracted, but not accepted.");
										}else
											inputCharType=switchToControlMode("Suggested character not extracted!");
									}else
										beep();
								}else
								if(inputChar==68){ // left arrow
									if(cursorPosition()){
										// TODO apparently pCommandToEvaluate will still be NULL when we're scrolling through the list of previous commands...
										if(commandIndex)copyCommand(); // will also set commandLength()!!!
										// MDH@27FEB2019: we should remove the last character of the current token (and command) and move it into behindCursorText
										bool success=false;
										char c=removedTokenCharacter(1);
										if(c){ // removing the character behind the cursor succeeded
											debugWrite("Character '%c' removed.",c);
											// prefix it to behindCursorText
											string_insert_char(behindCursorText,0,c);
											debugWrite("Behind cursor text: '%s'.",string(behindCursorText));
											success=true;
										}
										if(success){
											moveCursorLeft(1);
											/* NO going back and forth with the cursor does NOT change commandLength()!!!!
											commandLength()--; // MDH@28FEB2019: essential bro' otherwise when we go back to the end, cursorPosition() will stay below commandLength()
											*/
											// if the cursor position now matches the offset of the current token
											// i.e. the current token is now empty!!!!
											if(!string_length(pLastCommandToEvaluateToken->text)){
												// we can check the offset to see if this is the first token, but pLastCommandToEvaluateToken->prev is a little more secure
												pLastCommandToEvaluateToken=freeToken(pLastCommandToEvaluateToken);
												if(!pLastCommandToEvaluateToken)pCommandToEvaluate=NULL; // if no last command token anymore, we apparently released the first command token
											}
											writeBehindCursorText(false);
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
					// MDH@21APR2019: creating a command if need be is delegated to commandCharacterAccepted() which we know
					//                we always need a command (being edited)
					if(!commandCharacterAccepted(inputChar,inputCharType,true))
						inputCharType=switchToControlMode(pCommandToEvaluate?"Failed to accept the character.":"Failed to create a new command");
					/*
					// we need to have a token (to append the input character to) which initializes to pCommandToEvaluate
					if(pCommandToEvaluate==NULL) // no first command token
						// if commandIndex we should one of the registered commands
						setCommand(commandIndex?commands[commandCount-commandIndex]:NULL); // will also set commandLength()!!!
					// if still NULL (also when we fail to actually create a new first command token)
					if(pLastCommandToEvaluateToken!=NULL){
						if(!commandCharacterAccepted(inputChar,inputCharType,true))
							switchToControlMode("Failed to accept the character.");
					}else
						switchToControlMode("Failed to create a new command!");
					*/
				}
				outputStatus(inputChar,inputCharType);
			}else
			if(inputMode==IM_CONTROL){ // inputChar received in control mode
				outputChar(inputChar); // nice to see the character we typed...
				// might be paging through the commands
				if(!commandPage){ // not currently paging through the commands
					// single character responses (and out again)
					if(inputChar=='a'||inputChar=='A'){setAssisting(inputChar=='A');inputCharType='n';break;}
					if(inputChar=='d'||inputChar=='D'){setDebugging(inputChar=='D');inputCharType='n';break;}
					if(inputChar=='m'||inputChar=='M'){setMatchingparentheses(inputChar='M');inputCharType='n';break;}
					if(inputChar=='s'||inputChar=='S'){setVerbose(inputChar=='s');inputCharType='n';break;} // 'amVerbose()' is implemented using 'silent' flag!!!
					if(inputChar=='u'||inputChar=='U'){setAcceptinghistorycommand(inputChar='U');inputCharType='n';break;}
					if(inputChar>='0'&&inputChar<='9'){setColorscheme(inputChar-'0');inputCharType='n';break;}
					if(inputChar=='w'||inputChar=='W'){setWrapping(inputChar=='W');inputCharType='n';break;}
					if(inputChar=='v'||inputChar=='V'){outputVariables();inputCharType='n';break;}
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
					uint16_t cp=cursorPosition();
					if(cp){
						if(string_removed_char(shellCommand,cp-1)){
							moveCursorLeft(1);
							writeBehindCursorText(false);
						}else
							inputCharType=switchToControlMode("Failed to remove the shell command character!");
					}else // nothing to remove
						beep();
				}else
				if(inputCharType=='d'){
					if(string_length(behindCursorText)){ // something behind the cursor that we can remove
						if(string_removed_char(behindCursorText,0))
							writeBehindCursorText(true);
						else
							inputCharType=switchToControlMode("Failed to remove the first autocompletion character!");
					}else // nothing to remove
						beep();
				}else
				if(inputCharType=='c'){ // cancel command (Ctrl-C)
					if(pCommandToEvaluate!=NULL){
						backToPrompt();
						clearShellCommand();
						//////////if(amWrapping()())break;
					}else
						beep();
				}else
				if(inputCharType=='t'){ // Tab character
					// if there's a preview (well, code completion by way of a behindCursorText)
					uint16_t bc=behindCursor();
					if(bc){
						while(bc--){
							char newInputChar=string_removed_char(behindCursorText,0);
							if(!newInputChar){
								writeBehindCursorText(false);
								inputCharType=switchToControlMode("Failed to accept all suggested characters.");
								break;
							}
							string_append_char(shellCommand,newInputChar);
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
											if(behindCursor()){
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
									if(behindCursor()){
										char newInputChar=string_removed_char(behindCursorText,0);
										if(!newInputChar){
											writeBehindCursorText(false);
											inputCharType=switchToControlMode("Failed to accept the suggested characters.");
										}else
											string_insert_char(shellCommand,cursorPosition(),newInputChar);
									}else
										beep();
								}else
								if(inputChar==68){ // left arrow
									uint16_t cp=cursorPosition();
									if(cp){
										bool success=false;
										char c=string_removed_char(shellCommand,cp-1);
										if(c){ // removing the character behind the cursor succeeded
											// prefix it to behindCursorText
											string_insert_char(behindCursorText,0,c);
											moveCursorLeft(1);
											writeBehindCursorText(false);
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
					// MDH@24APR2019: cursorPosition()++;
				}
			}
			// if switched to control mode, inputCharType will be equal to 'o' and we break out of this input loop!!!
			if(inputCharType=='o'||inputCharType=='s')break;
		} // end of character input 

		// if eXit input character(s) received...
		if(inputCharType=='x'){
			// we should only exit M when not entering a function body
			if(!_currentFunctionBodyInput)break;
			// switch back to command mode
			switchToCommandMode();
			endFunctionBodyInput();
		}else
		// MDH@16APR2019: now if we use n to switch modes as well, we can do that if there's no command
		if(inputCharType=='n'){
			if(inputMode==IM_COMMAND){
				// MDH@21JUL2019: if the last token appears to be a function identifier change it to a variable
				//                so we won't end up with refusal of evaluation
				if(pLastCommandToEvaluateToken)if(pLastCommandToEvaluateToken->type==TT_FUNCTION)changeFunctionTokenToAVariable(false);
				inputInfo("%s",""); // so that line will be empty
				resetOutputColor(); // prevent showing subsequent output in the wrong colors
				clearScreenFromCursor(); // so we won't see the behind cursor text anymore
			}
			outputChar('\n');
			if(inputMode==IM_COMMAND){ // the newline character ends the command to be evaluated!!
				// if pCommandToEvaluate is set, we have a command to evaluate
				/*
				Mtoken* pCommandToEvaluateToEvaluate=NULL; // this would be the command to register if we succeed in evaluating it!!!
				if(pCommandToEvaluate){ // a current command being edited
					// MDH@22MAR2019: currently the first token is an EXPRESSION token
					if(cursorPosition()>string_length(pCommandToEvaluate->text)){
						// finish the last token???
						if(pLastCommandToEvaluateToken->significantCharacterCount==0)pLastCommandToEvaluateToken->significantCharacterCount=string_length(pLastCommandToEvaluateToken->text);
						pCommandToEvaluateToEvaluate=pCommandToEvaluate; // but only when not at start of command!!!
						if(amDebugging())outputTokenInfo();
					}
				}else // no command yet, although we might be looking at a previous command
				if(commandIndex&&cursorPosition()) // NOTE using cursorPosition() is better than using amAcceptinghistorycommand() (causing it!!)
					pCommandToEvaluateToEvaluate=commands[commandCount-commandIndex];
				*/

				// if we succeeded in evaluating a command we should register it
				if(pCommandToEvaluate){ // technically something to evaluate
					if(amVerbose())outputTokenInfo();
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
					if(amVerbose())outputLine("Command evaluated!");
					string_setlength(behindCursorText,0); // clear the autocompletion text NOTE if we fail to evaluate the command it will not be cleared!!!!
					// if we succeed in registering the command the command tokens should NOT be freed, BUT if we fail to register the command we should free ALL command tokens
					if(!registerCommand()){
						// if commandIndex (>0) we have evaluated a previous command which should also NEVER be freed
						if(!commandIndex){ // a new command being registered!!!
							freeToken(pCommandToEvaluate);
							outputError("Failed to register the command! Probable cause: out of memory");
						}else
							outputError("Failed to register the command again! Probable cause: out of memory");
					}else{
						if(amVerbose())outputLine("Command registered!");

					}
					// start anew (without a current command to evaluate!!!!)
					pLastCommandToEvaluateToken=pCommandToEvaluate=NULL; // remove reference to current command

					// garbage collection: remove any values not used anymore...
					size_t removedValueCount=getNumberOfRemovedValues();
					if(amDebugging())output("Number of removed values: %lu.",removedValueCount);

					// switch to function body input mode when this command contained at least one user function definition
					// (even when dealing with currently inputting function body commands)
					if(_firstFunctionBodyRequest)startFunctionBodyInput();

				}else
				if(behindCursor()==0)
					switchToControlMode(NULL);
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