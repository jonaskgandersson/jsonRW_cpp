/**
 * @brief jsonRW.cpp
 * 
 * *really* simple JSON writer in C++
 * 
 * Original C version: TonyWilk, Mar 2015
 * C++ ("Arduino version"): TonyWilk, Mar 2018
 * This version C++ ("Mbed version"): JonasAndersson, Nov 2018
 * 
 */

#include "jsonRW.hpp"

/**
 * @brief open writing of JSON
 * 
 * initialise with user string buffer of length buflen
 * 
 * @param rootType is the base JSON type: JW_OBJECT or JW_ARRAY
 * @param is_Pretty controls 'prettifying' the output: JW_PRETTY or JW_COMPACT)
 */
void jWrite::open( enum jwNodeType rootType, int is_Pretty )
{
	memset( buffer, 0, buflen );	// zap the whole destination buffer (all string terminators)
	bufp= buffer;
	nodeStack[0].nodeType= rootType;
	nodeStack[0].elementNo= 0;
	stackpos=0;
	error= JWRITE_OK;
	callNo= 1;
	isPretty= is_Pretty;
	putch( (rootType==JW_OBJECT) ? '{' : '[' );
}

/**
 * @brief Closes the element opened by open()
 * 
 * After an error, all following jWrite calls are skipped internally
 * so the error code is for the first error detected
 * 
 * @return int error code (0 = JWRITE_OK)
 */
int jWrite::close( )
{
	if( error == JWRITE_OK )
	{
		if( stackpos == 0 )
		{
			enum jwNodeType node= nodeStack[0].nodeType;
			if( isPretty )
				putch( '\n' );
			putch( (node == JW_OBJECT) ? '}' : ']');
		}else{
			error= JWRITE_NEST_ERROR;	// nesting error, not all objects closed when jwClose() called
		}
	}
	return error;
}

/**
 * @brief End the current array/object
 * 
 * @return int error code
 */
int jWrite::end( )
{
	if( error == JWRITE_OK )
	{
		enum jwNodeType node;
		int lastElemNo= nodeStack[stackpos].elementNo;
		node= pop( );
		if( lastElemNo > 0 )
			pretty( );
		putch( (node == JW_OBJECT) ? '}' : ']');
	}
	return error;
}

/**
 * @brief Error Position
 * 
 * If jwClose returned an error, this function returns the number of the jWrite function call
 * which caused that error.
 * 
 * @return int position of error: the nth call to a jWrite function
 */
int jWrite::errorPos( )
{
	return callNo;
}

/**
 * @brief Object raw insert functions
 * 
 * Put raw string to object (i.e. contents of rawtext without quotes
 * 
 * @param key Object key name
 * @param rawtext Object value as raw text
 */
void jWrite::obj_raw( const char *key, const char *rawtext )
{
	if(_jwObj( key ) == JWRITE_OK)
		putraw( rawtext);
}

/**
 * @brief Object string insert functions
 * 
 * Put "quoted" string to object
 * 
 * @param key Object key name
 * @param value Object value
 */
void jWrite::obj_string( const char *key, const char *value )
{
	if(_jwObj( key ) == JWRITE_OK)
		putstr( value );
}

/**
 * @brief Object integer insert functions
 * 
 * @param key Object key name
 * @param value Object value as integer
 */
void jWrite::obj_int( const char *key, int value )
{
	modp_itoa10( value, tmpbuf );
	obj_raw( key, tmpbuf );
}

/**
 * @brief Object double insert functions
 * 
 * @param key Object key name
 * @param value Object value as double
 */
void jWrite::obj_double( const char *key, double value )
{
	modp_dtoa2( value, tmpbuf, 6 );
	obj_raw( key, tmpbuf );
}

/**
 * @brief Object bool insert functions
 * 
 * Insert bool object, 0 or 1 is written as "true" or "false"
 * 
 * @param key Object key name
 * @param oneOrZero Object value as bool 0 or 1
 */
void jWrite::obj_bool( const char *key, int oneOrZero )
{
	obj_raw( key, (oneOrZero) ? "true" : "false" );
}

/**
 * @brief Object null insert functions
 * 
 * Insert empty object
 * 
 * @param key Object key name
 */
void jWrite::obj_null( const char *key )
{
	obj_raw( key, "null" );
}

/**
 * @brief Object in Object
 * 
 * Open new object inside current object
 * 
 * @param key Object key name
 */
void jWrite::obj_object( const char *key )
{
	if(_jwObj( key ) == JWRITE_OK)
	{
		putch( '{' );
		push( JW_OBJECT );
	}
}

/**
 * @brief Array in Object
 * 
 * Open new Array inside current object
 * 
 * @param key Object key name
 */
void jWrite::obj_array( const char *key )
{
	if(_jwObj( key ) == JWRITE_OK)
	{
		putch( '[' );
		push( JW_ARRAY );
	}
}

/**
 * @brief Array raw insert functions
 * 
 * Put raw string to array (i.e. contents of rawtext without quotes)
 * 
 * @param rawtext Array value as raw text
 */
void jWrite::arr_raw( const char *rawtext )
{
	if(_jwArr( ) == JWRITE_OK)
		putraw( rawtext);
}

/**
 * @brief Array string insert functions
 * 
 * Put "quoted" string to array
 * 
 * @param value Array value as string
 */
void jWrite::arr_string( const char *value )
{
	if(_jwArr( ) == JWRITE_OK)
		putstr( value );
}

/**
 * @brief Array integer insert functions
 * 
 * @param value Array value as integer
 */
void jWrite::arr_int( int value )
{
	modp_itoa10( value, tmpbuf );
	arr_raw( tmpbuf );
}

/**
 * @brief Array double insert functions
 * 
 * @param value Array value as double
 */
void jWrite::arr_double( double value )
{
	modp_dtoa2( value, tmpbuf, 6 );
	arr_raw( tmpbuf );
}

/**
 * @brief Array bool insert functions
 * 
 * Insert bool object, 0 or 1 is written as "true" or "false"
 * 
 * @param oneOrZero Array value as 0 or 1
 */
void jWrite::arr_bool( int oneOrZero )
{
	arr_raw( (oneOrZero) ? "true" : "false" );
}

/**
 * @brief Array null insert functions
 * 
 * Insert empty array value
 * 
 */
void jWrite::arr_null( )
{
	arr_raw( "null" );
}

/**
 * @brief Array object insert functions
 * 
 * Create new object inside current array
 * 
 */
void jWrite::arr_object( )
{
	if(_jwArr( ) == JWRITE_OK)
	{
		putch( '{' );
		push( JW_OBJECT );
	}
}

/**
 * @brief Array array insert functions
 * 
 * Create new array inside current array
 * 
 */
void jWrite::arr_array( )
{
	if(_jwArr( ) == JWRITE_OK)
	{
		putch( '[' );
		push( JW_ARRAY );
	}
}

/**
 * @brief ErrorToString
 * 
 * @param err Error code
 * @return const char* string describing error code
 */
const char * jWrite::errorToString( int err )
{
	/* Not using verbose error messages
	switch( err )
	{
	case JWRITE_OK:         return "OK"; 
	case JWRITE_BUF_FULL:   return "output buffer full";
	case JWRITE_NOT_ARRAY:	return "tried to write Array value into Object";
	case JWRITE_NOT_OBJECT:	return "tried to write Object key/value into Array";
	case JWRITE_STACK_FULL:	return "array/object nesting > JWRITE_STACK_DEPTH";
	case JWRITE_STACK_EMPTY:return "stack underflow error (too many 'end's)";
	case JWRITE_NEST_ERROR:	return "nesting error, not all objects closed when jwClose() called";
	}
	return "Unknown error";
	*/
	switch( err )
	{
	case JWRITE_OK:         return "OK"; 
	case JWRITE_BUF_FULL:   return "BUF_FULL";
	case JWRITE_NOT_ARRAY:	return "NOT_ARRAY";
	case JWRITE_NOT_OBJECT:	return "NOT_OBJECT";
	case JWRITE_STACK_FULL:	return "STACK_FULL";
	case JWRITE_STACK_EMPTY:return "STACK_EMPTY";
	case JWRITE_NEST_ERROR:	return "NEST_ERROR";
	}
	return "ERROR?";
}

 /********************************************//**
 *  Internal functions
 ***********************************************/

/**
 * @brief Pretty printing
 * 
 * Add newline and whitspace to create pretty json print format
 * 
 */
void jWrite::pretty( )
{
	int i;
	if( isPretty )
	{
		putch( '\n' );
		for( i=0; i<stackpos+1; i++ )
			putraw( "  " );
	}
}

/**
 * @brief Puch node stack
 * 
 * Add node to top of stack
 * 
 * @param jwNodeType Node type to push to stack
 */
void jWrite::push( enum jwNodeType nodeType )
{
	if( (stackpos+1) >= JWRITE_STACK_DEPTH )
		error= JWRITE_STACK_FULL;		// array/object nesting > JWRITE_STACK_DEPTH
	else
	{
		nodeStack[++stackpos].nodeType= nodeType;
		nodeStack[stackpos].elementNo= 0;
	}
}

/**
 * @brief Pop node stack
 * 
 * Get nodetype on top of stack
 * 
 * @return enum jWrite::pop Node type on top of stack
 */
enum jwNodeType jWrite::pop( )
{
	enum jwNodeType retval= nodeStack[stackpos].nodeType;
	if( stackpos == 0 )
		error= JWRITE_STACK_EMPTY;		// stack underflow error (too many 'end's)
	else
		stackpos--;
	return retval;
}

/**
 * @brief Write character to buffer
 * 
 * Put one char to JSON buffer, overflow check.
 * 
 * @param c Character to write to buffer
 */
void jWrite::putch( const char c )
{
	if( (unsigned int)(bufp - buffer + 1) >= buflen )
	{
		error= JWRITE_BUF_FULL;
	}else{
		*bufp++ = c;
	}
}

/**
 * @brief Write quoted string to buffer
 * 
 * Quote string with \"
 * 
 * @param str NULL terminated string to write to buffer
 */
void jWrite::putstr( const char *str )
{
	putch( '\"' );
	while( *str != '\0' )
		putch( *str++ );
	putch( '\"' );
}

/**
 * @brief Write raw string to buffer
 * 
 * @param str NULL terminated string to write to buffer
 */
void jWrite::putraw( const char *str )
{
	while( *str != '\0' )
		putch( *str++ );
}

/**
 * @brief Common Object function
 * 
 * - checks error
 * - checks current node is OBJECT
 * - adds comma if reqd
 * - adds "key" :
 * 
 * @param key Object key name
 * @return int Error code
 */
int jWrite::_jwObj( const char *key )
{
	if( error == JWRITE_OK )
	{
		callNo++;
		if( nodeStack[stackpos].nodeType != JW_OBJECT )
			error= JWRITE_NOT_OBJECT;			// tried to write Object key/value into Array
		else if( nodeStack[stackpos].elementNo++ > 0 )
			putch( ',' );
		pretty( );
		putstr( key );
		putch( ':' );
		if( isPretty )
			putch( ' ' );
	}
	return error;
}

/**
 * @brief Common Array function*
 * 
 * - checks error
 * - checks current node is ARRAY
 * - adds comma if reqd
 * 
 * @return int Error code
 */
int jWrite::_jwArr( )
{
	if( error == JWRITE_OK )
	{
		callNo++;
		if( nodeStack[stackpos].nodeType != JW_ARRAY )
			error= JWRITE_NOT_ARRAY;			// tried to write array value into Object
		else if( nodeStack[stackpos].elementNo++ > 0 )
			putch( ',' );
		pretty( );
	}
	return error;
}

//=================================================================
//
// modp value-to-string functions
// - modified for C89
//
// We use these functions as they are a lot faster than sprintf()
//
// Origin of these routines:
/*
 * <pre>
 * Copyright &copy; 2007, Nick Galbreath -- nickg [at] modp [dot] com
 * All rights reserved.
 * http://code.google.com/p/stringencoders/
 * Released under the bsd license.
 * </pre>
 */

static void strreverse(char* begin, char* end)
{
    char aux;
    while (end > begin)
        aux = *end, *end-- = *begin, *begin++ = aux;
}

/** \brief convert an signed integer to char buffer
 *
 * \param[in] value
 * \param[out] buf the output buffer.  Should be 16 chars or more.
 */
void jWrite::modp_itoa10(int32_t value, char* str)
{
    char* wstr=str;
    // Take care of sign
    unsigned int uvalue = (value < 0) ? -value : value;
    // Conversion. Number is reversed.
    do *wstr++ = (char)(48 + (uvalue % 10)); while(uvalue /= 10);
    if (value < 0) *wstr++ = '-';
    *wstr='\0';

    // Reverse string
    strreverse(str,wstr-1);
}

/**
 * @brief Powers of 10
 * 
 * 10^0 to 10^9
 */
static const double pow10[] = {1, 10, 100, 1000, 10000, 100000, 1000000,
                               10000000, 100000000, 1000000000};

/** \brief convert a floating point number to char buffer with a
 *         variable-precision format, and no trailing zeros
 *
 * This is similar to "%.[0-9]f" in the printf style, except it will
 * NOT include trailing zeros after the decimal point.  This type
 * of format oddly does not exists with printf.
 *
 * If the input value is greater than 1<<31, then the output format
 * will be switched exponential format.
 *
 * \param[in] value
 * \param[out] buf  The allocated output buffer.  Should be 32 chars or more.
 * \param[in] precision  Number of digits to the right of the decimal point.
 *    Can only be 0-9.
 */
void jWrite::modp_dtoa2(double value, char* str, int prec)
{
    /* if input is larger than thres_max, revert to exponential */
    const double thres_max = (double)(0x7FFFFFFF);
    int count;
    double diff = 0.0;
    char* wstr = str;
	int neg= 0;
	int whole;
	double tmp;
	uint32_t frac;

    /* Hacky test for NaN
     * under -fast-math this won't work, but then you also won't
     * have correct nan values anyways.  The alternative is
     * to link with libmath (bad) or hack IEEE double bits (bad)
     */
    if (! (value == value)) {
        str[0] = 'n'; str[1] = 'a'; str[2] = 'n'; str[3] = '\0';
        return;
    }

    if (prec < 0) {
        prec = 0;
    } else if (prec > 9) {
        /* precision of >= 10 can lead to overflow errors */
        prec = 9;
    }

    /* we'll work in positive values and deal with the
       negative sign issue later */
    if (value < 0) {
        neg = 1;
        value = -value;
    }


    whole = (int) value;
    tmp = (value - whole) * pow10[prec];
    frac = (uint32_t)(tmp);
    diff = tmp - frac;

    if (diff > 0.5) {
        ++frac;
        /* handle rollover, e.g.  case 0.99 with prec 1 is 1.0  */
        if (frac >= pow10[prec]) {
            frac = 0;
            ++whole;
        }
    } else if (diff == 0.5 && ((frac == 0) || (frac & 1))) {
        /* if halfway, round up if odd, OR
           if last digit is 0.  That last part is strange */
        ++frac;
    }

    /* for very large numbers switch back to native sprintf for exponentials.
       anyone want to write code to replace this? */
    /*
      normal printf behavior is to print EVERY whole number digit
      which can be 100s of characters overflowing your buffers == bad
    */
    if (value > thres_max) {
        sprintf(str, "%e", neg ? -value : value);
        return;
    }

    if (prec == 0) {
        diff = value - whole;
        if (diff > 0.5) {
            /* greater than 0.5, round up, e.g. 1.6 -> 2 */
            ++whole;
        } else if (diff == 0.5 && (whole & 1)) {
            /* exactly 0.5 and ODD, then round up */
            /* 1.5 -> 2, but 2.5 -> 2 */
            ++whole;
        }

        //vvvvvvvvvvvvvvvvvvv  Diff from modp_dto2
    } else if (frac) {
        count = prec;
        // now do fractional part, as an unsigned number
        // we know it is not 0 but we can have leading zeros, these
        // should be removed
        while (!(frac % 10)) {
            --count;
            frac /= 10;
        }
        //^^^^^^^^^^^^^^^^^^^  Diff from modp_dto2

        // now do fractional part, as an unsigned number
        do {
            --count;
            *wstr++ = (char)(48 + (frac % 10));
        } while (frac /= 10);
        // add extra 0s
        while (count-- > 0) *wstr++ = '0';
        // add decimal
        *wstr++ = '.';
    }

    // do whole part
    // Take care of sign
    // Conversion. Number is reversed.
    do *wstr++ = (char)(48 + (whole % 10)); while (whole /= 10);
    if (neg) {
        *wstr++ = '-';
    }
    *wstr='\0';
    strreverse(str, wstr-1);
}

/* end of jWrite.cpp */
