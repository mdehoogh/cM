#include "Mdecimal.h"

extern mpd_context_t* const _decimalContext;
extern long double const M_LD_NAN;

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