
Mdecimal _getValueDecimal(Mvalue* _value){
    
}
Mdecimal* getValueDecimal(Mvalue* _value){
	// ASSERT typically _value should contain a numeric (non-real) value
	return(_value?(_value->type==VT_DECIMAL?_value->value._decimal:_getValueDecimal(_value):NULL); // will always create a new one...
}