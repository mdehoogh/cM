#include "Mdecimal.h"

#include "Msettings.h"
#include "Moutput.h"
#include "Msession.h"

extern mpd_context_t* const _decimalContext;
extern long double const M_LD_NAN;
extern const char* const ERROR_PREFIX;

// the work horse of converting any value (if possible) to a decimal
Mdecimal* _getValueDecimal(Mvalue* _value){
	Mdecimal* _decimal=NULL;
	if(_value){
		if(_value->type!=VT_LIST&&_value->type!=VT_MAP){
			switch(_value->type){
				case VT_DECIMAL:_decimal=_getDecimalCopy(_value->value._decimal);break;
				case VT_INTEGER:_decimal=_getDecimal(__mpd(_decimalContext,_value->value._integer->ll),0,true);break;
                case VT_BIGINTEGER:
                    {
                        // NOTE we need to make a copy of the big integer because otherwise free_rational() below would free the big integer wrapped inside the value, which would be a terrible mistake
                        Mrational* _rational=_getRational(_getBigintegerCopy(_value->value._biginteger),NULL,M_LD_NAN,false,true);
                        if(_rational){
                            _decimal=_getRationalDecimal(_rational);
                            free_rational(_rational);
                        }
                    }
                    break;
				case VT_RATIONAL:
					_decimal=_getRationalDecimal(_value->value._rational);
					break;
				default:
					{ // TODO: use _getTextDecimal instead!!!
                        _decimal=__decimal(_decimalContext,0,0);
                        if(_decimal){
						    Mstring* _valueText=_getValueText(_value,true);
						    if(_valueText){mpd_set_string(_decimal->mpd,string(_valueText),_decimalContext);free_string(_valueText);}
                        }
					}
					break;
			}
		}
	}
	return _decimal;
}

Mdecimal* getValueDecimal(Mvalue* _value){
	return(_value?(_value->type==VT_DECIMAL?_value->value._decimal:_getValueDecimal(_value)):NULL);
}

// decimalerrorstatus() filter out the rounding and inexact 'errors'
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

bool mpd_error(mpd_context_t* mpd_context){return(mpd_getstatus(mpd_context)&0xEFBF)!=0;}

Mdecimal* pi_decimal(mpd_context_t* mpd_context){

	// if decimalContext equals NULL use the global decimal context, in _decimalContext
	if(!mpd_context)mpd_context=_decimalContext;

	mpd_ssize_t decimalprecision=(mpd_context?mpd_context->prec:0);
	if(decimalprecision<=0){outputError("Cannot approximate pi: no decimal context available");return NULL;}

	/* replacing:
	if(!mpd_context){if(decimalprecision>0)outputError("Failed to obtain the requested decimal context");else outputError("No (default) decimal context available");return NULL;}
	if(decimalprecision<0)decimalprecision=_decimalContext->prec;
	*/
	if(amVerbose())output("Computing pi to %lld decimals.\n",decimalprecision);

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
			//char* _lasts=mpd_to_sci(lasts,0);if(_lasts){output(" lasts=%s");free(_lasts);}else output(" ?");
			char* _t=mpd_to_sci(t,0);if(_t){output(" t=%s",_t);free(_t);}else output(" ?");
			char* _s=mpd_to_sci(s,0);if(_s){output(" s=%s",_s);free(_s);}else output(" ?");
			char* _n=mpd_to_sci(n,0);if(_n){output(" n=%s",_n);free(_n);}else output(" ?");
			char* _na=mpd_to_sci(na,0);if(_na){output(" na=%s",_na);free(_na);}else output(" ?");
			char* _d=mpd_to_sci(d,0);if(_d){output(" d=%s",_d);free(_d);}else output(" ?");
			char* _da=mpd_to_sci(da,0);if(_da){output(" da=%s",_da);free(_da);}else output(" ?");
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
	// wrap the mpd_t in a decimal
#ifdef __ADEBUG__
	return _getDecimalValue(s,true);
#else
	return _getDecimal(s,0,true);
#endif

}