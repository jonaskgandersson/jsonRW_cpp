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

#include "../jsonRW.hpp"
#include <stdio.h>
#include <string.h>

using namespace jonaskgandersson;

Json::Json(char *pbuffer, int buf_len) : buffer(pbuffer), buflen(buf_len), bufp(buffer), error(JWRITE_OK),  callNo(0), stackpos(0),  isPretty(false)
{
}

void Json::open(NodeType rootType, int is_Pretty)
{
	memset(buffer, 0, buflen); // zap the whole destination buffer (all string terminators)
	bufp = buffer;
	nodeStack[0].nodeType = rootType;
	nodeStack[0].elementNo = 0;
	stackpos = 0;
	error = JWRITE_OK;
	callNo = 1;
	isPretty = is_Pretty;
	putch((rootType == NodeType::JS_OBJECT) ? '{' : '[');
}

int Json::close()
{
	if (error == JWRITE_OK)
	{
		if (stackpos == 0)
		{
			NodeType node = nodeStack[0].nodeType;
			if (isPretty)
				putch('\n');
			putch((node == NodeType::JS_OBJECT) ? '}' : ']');
		}
		else
		{
			error = JWRITE_NEST_ERROR; // nesting error, not all objects closed when jwClose() called
		}
	}
	return error;
}

int Json::end()
{
	if (error == JWRITE_OK)
	{
		NodeType node;
		int lastElemNo = nodeStack[stackpos].elementNo;
		node = pop();
		if (lastElemNo > 0)
			pretty();
		putch((node == NodeType::JS_OBJECT) ? '}' : ']');
	}
	return error;
}

int Json::errorPos()
{
	return callNo;
}

int Json::add(const char *key, NodeType nodeType)
{
	switch (nodeType)
	{
	case NodeType::JS_OBJECT:
	{
		if (_jwObj(key) == JWRITE_OK)
		{
			putch('{');
			push(NodeType::JS_OBJECT);
		}
	}
	break;

	case NodeType::JS_ARRAY:
	{
		if (_jwObj(key) == JWRITE_OK)
		{
			putch('[');
			push(NodeType::JS_ARRAY);
		}
	}
	break;
	case NodeType::JS_NULL:
	{
		addRaw(key, "null");
	}
	break;
	default:
		error = JWRITE_BAD_TYPE;
		break;
	}
	return error;
}

int Json::add(NodeType nodeType)
{
	switch (nodeType)
	{
	case NodeType::JS_OBJECT:
	{
		if (_jwArr() == JWRITE_OK)
		{
			putch('{');
				push(NodeType::JS_OBJECT);
			}
		}
		break;
		case NodeType::JS_ARRAY:
		{
			if (_jwArr() == JWRITE_OK)
			{
				putch('[');
				push(NodeType::JS_ARRAY);
			}
		}
		break;
		case NodeType::JS_NULL:
			addRaw("null");
			break;
		default:
			error = JWRITE_BAD_TYPE;
			break;
		}

		return error;
	}
	void Json::addRaw(const char *key, const char *rawtext)
	{
		if (_jwObj(key) == JWRITE_OK)
			putraw(rawtext);
	}

	void Json::add(const char *key, const char *value)
	{
		if (_jwObj(key) == JWRITE_OK)
			putstr(value);
	}

	void Json::add(const char *key, int value)
	{
		modp_itoa10(value, tmpbuf);
		addRaw(key, tmpbuf);
	}

	void Json::add(const char *key, double value)
	{
		modp_dtoa2(value, tmpbuf, 6);
		addRaw(key, tmpbuf);
	}

	void Json::add(const char *key, bool oneOrZero)
	{
		addRaw(key, (oneOrZero) ? "true" : "false");
	}

	void Json::addRaw(const char *rawtext)
	{
		if (_jwArr() == JWRITE_OK)
			putraw(rawtext);
	}

	void Json::add(const char *value)
	{
		if (_jwArr() == JWRITE_OK)
			putstr(value);
	}

	void Json::add(int value)
	{
		modp_itoa10(value, tmpbuf);
		addRaw(tmpbuf);
	}

	void Json::add(double value)
	{
		modp_dtoa2(value, tmpbuf, 6);
		addRaw(tmpbuf);
	}

	void Json::add(bool oneOrZero)
	{
		addRaw((oneOrZero) ? "true" : "false");
	}

	const char *Json::errorToString(int err)
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
		switch (err)
		{
		case JWRITE_OK:
			return "OK";
		case JWRITE_BUF_FULL:
			return "BUF_FULL";
		case JWRITE_NOT_ARRAY:
			return "NOT_ARRAY";
		case JWRITE_NOT_OBJECT:
			return "NOT_OBJECT";
		case JWRITE_STACK_FULL:
			return "STACK_FULL";
		case JWRITE_STACK_EMPTY:
			return "STACK_EMPTY";
		case JWRITE_NEST_ERROR:
			return "NEST_ERROR";
		}
		return "ERROR?";
	}

	/********************************************/ /**
	 *  Internal functions
	 ***********************************************/

	void Json::pretty()
	{
		int i;
		if (isPretty)
		{
			putch('\n');
			for (i = 0; i < stackpos + 1; i++)
				putraw("  ");
		}
	}

	void Json::push(NodeType nodeType)
	{
		if ((stackpos + 1) >= JWRITE_STACK_DEPTH)
			error = JWRITE_STACK_FULL; // array/object nesting > JWRITE_STACK_DEPTH
		else
		{
			nodeStack[++stackpos].nodeType = nodeType;
			nodeStack[stackpos].elementNo = 0;
		}
	}

	NodeType Json::pop()
	{
		NodeType retval = nodeStack[stackpos].nodeType;
		if (stackpos == 0)
			error = JWRITE_STACK_EMPTY; // stack underflow error (too many 'end's)
		else
			stackpos--;
		return retval;
	}

	void Json::putch(const char c)
	{
		if ((unsigned int)(bufp - buffer + 1) >= buflen)
		{
			error = JWRITE_BUF_FULL;
		}
		else
		{
			*bufp++ = c;
		}
	}

	void Json::putstr(const char *str)
	{
		putch('\"');
		while (*str != '\0')
			putch(*str++);
		putch('\"');
	}

	void Json::putraw(const char *str)
	{
		while (*str != '\0')
			putch(*str++);
	}

	int Json::_jwObj(const char *key)
	{
		if (error == JWRITE_OK)
		{
			callNo++;
			if (nodeStack[stackpos].nodeType != NodeType::JS_OBJECT)
				error = JWRITE_NOT_OBJECT; // tried to write Object key/value into Array
			else if (nodeStack[stackpos].elementNo++ > 0)
				putch(',');
			pretty();
			putstr(key);
			putch(':');
			if (isPretty)
				putch(' ');
		}
		return error;
	}

	int Json::_jwArr()
	{
		if (error == JWRITE_OK)
		{
			callNo++;
			if (nodeStack[stackpos].nodeType != NodeType::JS_ARRAY)
				error = JWRITE_NOT_ARRAY; // tried to write array value into Object
			else if (nodeStack[stackpos].elementNo++ > 0)
				putch(',');
			pretty();
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

	static void strreverse(char *begin, char *end)
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
	void Json::modp_itoa10(int value, char *str)
	{
		char *wstr = str;
		// Take care of sign
		unsigned int uvalue = (value < 0) ? -value : value;
		// Conversion. Number is reversed.
		do
			*wstr++ = (char)(48 + (uvalue % 10));
		while (uvalue /= 10);
		if (value < 0)
			*wstr++ = '-';
		*wstr = '\0';

		// Reverse string
		strreverse(str, wstr - 1);
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
	void Json::modp_dtoa2(double value, char *str, int prec)
	{
		/* if input is larger than thres_max, revert to exponential */
		const double thres_max = (double)(0x7FFFFFFF);
		int count;
		double diff = 0.0;
		char *wstr = str;
		int neg = 0;
		int whole;
		double tmp;
		unsigned int frac;

		/* Hacky test for NaN
		 * under -fast-math this won't work, but then you also won't
		 * have correct nan values anyways.  The alternative is
		 * to link with libmath (bad) or hack IEEE double bits (bad)
		 */
		if (!(value == value))
		{
			str[0] = 'n';
			str[1] = 'a';
			str[2] = 'n';
			str[3] = '\0';
			return;
		}

		if (prec < 0)
		{
			prec = 0;
		}
		else if (prec > 9)
		{
			/* precision of >= 10 can lead to overflow errors */
			prec = 9;
		}

		/* we'll work in positive values and deal with the
		   negative sign issue later */
		if (value < 0)
		{
			neg = 1;
			value = -value;
		}

		whole = (int)value;
		tmp = (value - whole) * pow10[prec];
		frac = (unsigned int)(tmp);
		diff = tmp - frac;

		if (diff > 0.5)
		{
			++frac;
			/* handle rollover, e.g.  case 0.99 with prec 1 is 1.0  */
			if (frac >= pow10[prec])
			{
				frac = 0;
				++whole;
			}
		}
		else if (diff == 0.5 && ((frac == 0) || (frac & 1)))
		{
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
		if (value > thres_max)
		{
			sprintf(str, "%e", neg ? -value : value);
			return;
		}

		if (prec == 0)
		{
			diff = value - whole;
			if (diff > 0.5)
			{
				/* greater than 0.5, round up, e.g. 1.6 -> 2 */
				++whole;
			}
			else if (diff == 0.5 && (whole & 1))
			{
				/* exactly 0.5 and ODD, then round up */
				/* 1.5 -> 2, but 2.5 -> 2 */
				++whole;
			}

			//vvvvvvvvvvvvvvvvvvv  Diff from modp_dto2
		}
		else if (frac)
		{
			count = prec;
			// now do fractional part, as an unsigned number
			// we know it is not 0 but we can have leading zeros, these
			// should be removed
			while (!(frac % 10))
			{
				--count;
				frac /= 10;
			}
			//^^^^^^^^^^^^^^^^^^^  Diff from modp_dto2

			// now do fractional part, as an unsigned number
			do
			{
				--count;
				*wstr++ = (char)(48 + (frac % 10));
			} while (frac /= 10);
			// add extra 0s
			while (count-- > 0)
				*wstr++ = '0';
			// add decimal
			*wstr++ = '.';
		}

		// do whole part
		// Take care of sign
		// Conversion. Number is reversed.
		do
			*wstr++ = (char)(48 + (whole % 10));
		while (whole /= 10);
		if (neg)
		{
			*wstr++ = '-';
		}
		*wstr = '\0';
		strreverse(str, wstr - 1);
	}

	// jRead
	// read element into destination buffer and add '\0' terminator
	// - always copies element irrespective of dataType (unless it's an error)
	// - destBuffer is always '\0'-terminated (even on zero lenght returns)
	// - returns pointer to destBuffer
	//
	char *Json::copyElementValue( char *destBuffer, int destLength, struct ReadElement *pElement )
	{
		int i;
		int len= pElement->bytelen;
		char *pdest= destBuffer;
		char *psrc= (char *)pElement->pValue;
		if( pElement->error == 0 )
		{
			if( len >= destLength )
				len= destLength;
			for( i=0; i<destLength; i++ )
				*pdest++= *psrc++;
		}
		*pdest= '\0';
		return destBuffer;
	}

	// getObjectLength
	// - used when query ends at an object, we want to return the object length
	// - on entry pJson -> "{... "
	// - used to skip unwanted values which are objects
	// - keyIndex normally passed as -1 unless we're looking for the nth "key" value
	//   in which case keyIndex is the index of the key we want
	//
	const char * Json::getObjectLength( const char *pJson, struct ReadElement *pResult, int keyIndex )
	{
		struct ReadElement jElement;
		int jTok;
		const char *sp;
		pResult->dataType= JREAD_OBJECT;
		pResult->error= 0;
		pResult->elements= 0;
		pResult->pValue= pJson;
		sp= findTok( pJson+1, &jTok ); // check for empty object
		if( jTok == JREAD_EOBJECT )		
		{
			pJson= sp+1;
		}else
		{
			while( 1 )
			{
				pJson= getElementString( ++pJson, &jElement, '\"' );
				if( jElement.dataType != JREAD_STRING )
				{
					pResult->error= 3;		// Expected "key"
					break;
				}
				if( pResult->elements == keyIndex )		// if passed keyIndex
				{
					*pResult= jElement;		// we return "key" at this index
					pResult->dataType= JREAD_KEY;
					return pJson;
				}
				pJson= findTok( pJson, &jTok );
				if( jTok != JREAD_COLON )
				{
					pResult->error= 4;		// Expected ":"
					break;
				}
				pJson= getElement( ++pJson, "", &jElement );
				if( pResult->error )
					break;
				pJson= findTok( pJson, &jTok );
				pResult->elements++;
				if( jTok == JREAD_EOBJECT )
				{
					pJson++;
					break;
				}
				if( jTok != JREAD_COMMA )
				{
					pResult->error= 6;		// Expected "," in object
					break;
				}
			}
		}
		if( keyIndex >= 0 )
		{
			// we wanted a "key" value - that we didn't find
			pResult->dataType= JREAD_ERROR;
			pResult->error= 11;			// Object key not found (bad index)
		}else{
			pResult->bytelen= pJson - (char *)pResult->pValue;
		}
		return pJson;
	}



	// getArrayLength
	// - used when query ends at an array, we want to return the array length
	// - on entry pJson -> "[... "
	// - used to skip unwanted values which are arrays
	//
	const char * Json::getArrayLength( const char *pJson, struct ReadElement *pResult )
	{
		struct ReadElement jElement;
		int jTok;
		const char *sp;
		pResult->dataType= JREAD_ARRAY;
		pResult->error= 0;
		pResult->elements= 0;
		pResult->pValue= pJson;
		sp= findTok( pJson+1, &jTok ); // check for empty array
		if( jTok == JREAD_EARRAY )		
		{
			pJson= sp+1;
		}else
		{
			while( 1 )
			{
				pJson= getElement( ++pJson, "", &jElement );	// array value
				if( pResult->error )
					break;
				pJson= findTok( pJson, &jTok );	// , or ]
				pResult->elements++;
				if( jTok == JREAD_EARRAY )
				{
					pJson++;
					break;
				}
				if( jTok != JREAD_COMMA )
				{
					pResult->error= 9;		// Expected "," in array
					break;
				}
			}
		}
		pResult->bytelen= pJson - (char *)pResult->pValue;
		return pJson;
	}

	// getArrayElement()
	// - reads one value from an array
	// - assumes pJsonArray points at the start of an array or array element
	//
	const char *Json::getArrayElement( const char *pJsonArray, struct ReadElement *pResult )
	{
		int jTok;

		pJsonArray= findTok( pJsonArray, &jTok );
		switch( jTok )
		{
		case JREAD_ARRAY:	// start of array
		case JREAD_COMMA:	// element separator
			return getElement( ++pJsonArray, "", pResult );

		case JREAD_EARRAY:	// end of array
			pResult->error= 13;		// End of array found
			break;
		default:			// some other error
			pResult->error= 9;		// expected comma in array
			break;
		}
		pResult->dataType= JREAD_ERROR;
		return pJsonArray;
	}


	// jRead
	// - reads a complete JSON <value>
	// - matches pQuery against pJson, results in pResult
	// returns: pointer into pJson
	//
	// Note: is recursive

	const char *Json::getElement( const char *pJson, const char *pQuery, struct ReadElement *pResult, int *queryParams )
	{
		int qTok, jTok, bytelen;
		unsigned int index, count;
		struct ReadElement qElement, jElement;

		pJson= findTok( pJson, &jTok );
		pQuery= findTok( pQuery, &qTok );

		pResult->dataType= jTok;
		pResult->bytelen= pResult->elements= pResult->error= 0;
		pResult->pValue= pJson;

		if( (qTok != JREAD_EOL) && (qTok != jTok) )
		{
			pResult->error= 1;	// JSON does not match Query
			return pJson;
		}

		switch( jTok )
		{
		case JREAD_ERROR:		// general error, eof etc.
			pResult->error= 2;	// Error reading JSON value
			break;

		case JREAD_OBJECT:		// "{"
			if( qTok == JREAD_EOL )
				return getObjectLength( pJson, pResult, -1 );	// return length of object 

			pQuery= findTok( ++pQuery, &qTok );			// "('key'...", "{NUMBER", "{*" or EOL
			if( qTok != JREAD_STRING )
			{
				index= 0;
				switch( qTok )
				{
				case JREAD_NUMBER:
					pQuery= read_atoi( (char *)pQuery, &index );	// index value
					break;
				case JREAD_QPARAM:
					pQuery++;
					index= (queryParams != NULL) ? *queryParams++ : 0;	// substitute parameter
					break;
				default:
					pResult->error= 12;								// Bad Object key
					return pJson;
				}
				return getObjectLength( pJson, pResult, index );
			}

			pQuery= getElementString( pQuery, &qElement, QUERY_QUOTE );	// qElement = query 'key'
			//
			// read <key> : <value> , ... }
			// loop 'til key matched
			//
			while( 1 )
			{
				pJson= getElementString( ++pJson, &jElement, '\"' );
				if( jElement.dataType != JREAD_STRING )
				{
					pResult->error= 3;		// Expected "key"
					break;
				}
				pJson= findTok( pJson, &jTok );
				if( jTok != JREAD_COLON )
				{
					pResult->error= 4;		// Expected ":"
					break;
				}
				// compare object keys
				if( equalElement( &qElement, &jElement ) == 0 )
				{
					// found object key
					return getElement( ++pJson, pQuery, pResult, queryParams );
				}
				// no key match... skip this value
				pJson= getElement( ++pJson, "", pResult, NULL );
				pJson= findTok( pJson, &jTok );
				if( jTok == JREAD_EOBJECT )
				{
					pResult->error= 5;		// Object key not found
					break;
				}
				if( jTok != JREAD_COMMA )
				{
					pResult->error= 6;		// Expected "," in object
					break;
				}
			}
			break;
		case JREAD_ARRAY:		// "[NUMBER" or "[*"
			//
			// read index, skip values 'til index
			//
			if( qTok == JREAD_EOL )
				return getArrayLength( pJson, pResult );	// return length of object 

			index= 0;
			pQuery= findTok( ++pQuery, &qTok );		// "[NUMBER" or "[*"
			if( qTok == JREAD_NUMBER )		
			{
				pQuery= read_atoi( pQuery, &index );		// get array index	
			}else if( qTok == JREAD_QPARAM )
			{
				pQuery++;
				index= (queryParams != NULL) ? *queryParams++ : 0;	// substitute parameter
			}

			count=0;
			while( 1 )
			{
				if( count == index )
					return getElement( ++pJson, pQuery, pResult, queryParams );	// return value at index
				// not this index... skip this value
				pJson= getElement( ++pJson, "", &jElement, NULL );
				if( pResult->error )
					break;
				count++;				
				pJson= findTok( pJson, &jTok );			// , or ]
				if( jTok == JREAD_EARRAY )
				{
					pResult->error= 10;		// Array element not found (bad index)
					break;
				}
				if( jTok != JREAD_COMMA )
				{
					pResult->error= 9;		// Expected "," in array
					break;
				}
			}
			break;
		case JREAD_STRING:		// "string" 
			pJson= getElementString( pJson, pResult, '\"' );
			break;
		case JREAD_NUMBER:		// number (may be -ve) int or float
		case JREAD_BOOL:		// true or false
		case JREAD_NULL:		// null
			bytelen= getElementStringLenght( pJson );
			pResult->dataType= jTok;
			pResult->bytelen= bytelen;
			pResult->pValue= pJson;
			pResult->elements= 1;
			pJson += bytelen;
			break;
		default:
			pResult->error= 8;	// unexpected character (in pResult->dataType)
		}
		// We get here on a 'terminal value'
		// - make sure the query string is empty also
		pQuery= findTok( pQuery, &qTok );
		if( !pResult->error && (qTok != JREAD_EOL) )
			pResult->error= 7;	// terminal value found before end of query
		if( pResult->error )
		{
			pResult->dataType= JREAD_ERROR;
			pResult->elements= pResult->bytelen= 0;
			pResult->pValue= pJson;		// return pointer into JSON at error point
		}
		return pJson;
	}


	// Internal for reading 
	const char *Json::skipWhitespace( const char *sp )
	{
		while( (*sp != '\0') && (*sp <= ' ') )
			sp++;
		return sp;
	}


	// Find start of a token
	// - returns pointer to start of next token or element
	//   returns type via tokType
	//
	const char *Json::findTok( const char *sp, int *tokType )
	{
		char c;
		sp= skipWhitespace(sp);
		c= *sp;
		if( c == '\0' )	*tokType= JREAD_EOL;
		else if((c == '"') || (c == QUERY_QUOTE))*tokType= JREAD_STRING;
		else if((c >= '0') && (c <= '9')) *tokType= JREAD_NUMBER;
		else if( c == '-') *tokType= JREAD_NUMBER;
		else if( c == '{') *tokType= JREAD_OBJECT;
		else if( c == '[') *tokType= JREAD_ARRAY;
		else if( c == '}') *tokType= JREAD_EOBJECT; 
		else if( c == ']') *tokType= JREAD_EARRAY;
		else if((c == 't') || (c == 'f')) *tokType= JREAD_BOOL;
		else if( c == 'n') *tokType= JREAD_NULL;
		else if( c == ':') *tokType= JREAD_COLON;
		else if( c == ',') *tokType= JREAD_COMMA;
		else if( c == '*') *tokType= JREAD_QPARAM;
		else *tokType= JREAD_ERROR;
		return sp;
	}

	// getElementString
	// - assumes next element is "string" which may include "\" sequences
	// - returns pointer to -------------^
	// - pElem contains result ( JREAD_STRING, length, pointer to string)
	// - pass quote = '"' for Json, quote = '\'' for query scanning
	//
	// returns: pointer into pJson after the string (char after the " terminator)
	//			pElem contains pointer and length of string (or dataType=JREAD_ERROR)
	//
	const char * Json::getElementString( const char *pJson, struct ReadElement *pElem, char quote )
	{
		short skipch;
		pElem->dataType= JREAD_ERROR;
		pElem->elements= 1;
		pElem->bytelen= 0;
		pJson= skipWhitespace( pJson );
		if( *pJson == quote )
		{
			pJson++;
			pElem->pValue= pJson;				// -> start of actual string
			pElem->bytelen=0;
			skipch= 0;
			while( *pJson != '\0' )
			{
				if( skipch )
					skipch= 0;
				else if( *pJson == '\\' )		//  "\" sequence
					skipch= 1;
				else if( *pJson == quote )
				{
					pElem->dataType= JREAD_STRING;
					pJson++;
					break;
				}
				pElem->bytelen++;
				pJson++;
			};
		};
		return pJson;
	}

	// getElementStringLenght
	// - used to identify length of element text
	// - returns no. of chars from pJson upto a terminator
	// - terminators: ' ' , } ]
	//
	int Json::getElementStringLenght( const char *pJson )
	{
		int len= 0;
		while(	(*pJson >  ' ' ) &&		// any ctrl char incl '\0'
				(*pJson != ',' ) &&
				(*pJson != '}' ) &&
				(*pJson != ']' ) )
		{
			len++;
			pJson++;
		}
		return len;
	}

	// compare two json elements
	// returns: 0 if they are identical strings, else 1
	//
	int Json::equalElement( struct ReadElement *j1, struct ReadElement *j2 )
	{
		int i;
		if( (j1->dataType != JREAD_STRING) || 
			(j2->dataType != JREAD_STRING) ||
			(j1->bytelen != j2->bytelen ) )
			return 1;

		for( i=0; i< j1->bytelen; i++ )
			if( ((char *)(j1->pValue))[i] != ((char *)(j2->pValue))[i] )
				return 1;
		return 0; 
	}

	// read unsigned int from string
	const char *Json::read_atoi( const char *p, unsigned int *result )
	{
		unsigned int x = 0;
		while (*p >= '0' && *p <= '9') {
			x = (x*10) + (*p - '0');
			++p;
		}
		*result= x;
		return p;
	}

