/**
 * @brief jsonRW.hpp
 * A *really* simple JSON reader/writer in C++
 * a collection of functions to generate JSON semi-automatically
 * 
 * The idea is to simplify writing native C values into a JSON string and
 * to provide some error trapping to ensure that the result is valid JSON.
 * 
 * Original C version: TonyWilk, Mar 2015
 * C++ ("Arduino version"): TonyWilk, Mar 2018
 * This version C++ ("Mbed version"): JonasAndersson, Nov 2018
 * 
 */

#include <stdio.h>

#define JWRITE_STACK_DEPTH 32 // max nesting depth of objects/arrays

#define JW_COMPACT 0 // output string control for jwOpen()
#define JW_PRETTY 1  // pretty adds \n and indentation

// Error Codes
// -----------
#define JWRITE_OK 0
#define JWRITE_BUF_FULL 1	// output buffer full
#define JWRITE_NOT_ARRAY 2   // tried to write Array value into Object
#define JWRITE_NOT_OBJECT 3  // tried to write Object key/value into Array
#define JWRITE_STACK_FULL 4  // array/object nesting > JWRITE_STACK_DEPTH
#define JWRITE_STACK_EMPTY 5 // stack underflow error (too many 'end's)
#define JWRITE_NEST_ERROR 6  // nesting error, not all objects closed when jwClose() called
#define JWRITE_BAD_TYPE 7	// bad object type

// uncomment this if you really want to use double quotes in query strings instead of '
//#define JREAD_DOUBLE_QUOTE_IN_QUERY

// By default we use single quote in query strings so it's a lot easier
// to type in code i.e.  "{'key'" instead of "{\"key\""
//
#ifdef JREAD_DOUBLE_QUOTE_IN_QUERY
#define QUERY_QUOTE '\"'
#else
#define QUERY_QUOTE '\''
#endif

//
// return dataTypes:
#define JREAD_ERROR 0  // general error, eof etc.
#define JREAD_OBJECT 1 // "{"
#define JREAD_ARRAY 2  // "["
#define JREAD_STRING 3 // "string"
#define JREAD_NUMBER 4 // number (may be -ve) int or float
#define JREAD_BOOL 5   // true or false
#define JREAD_NULL 6   // null
#define JREAD_KEY 7	// object "key"
// internal values:
#define JREAD_COLON 8	// ":"
#define JREAD_EOL 9		 // end of input string (ptr at '\0')
#define JREAD_COMMA 10   // ","
#define JREAD_EOBJECT 11 // "}"
#define JREAD_EARRAY 12  // "]"
#define JREAD_QPARAM 13  // "*" query string parameter

namespace jonaskgandersson
{
//------------------------------------------------------
// ReadElement
// - structure to return JSON elements
// - error=0 for valid returns
//
// *NOTES*
//    the returned pValue pointer points into the passed JSON
//    string returns are not '\0' terminated.
//    bytelen specifies the length of the returned data pointed to by pValue
//
struct ReadElement
{
	int dataType;		// one of JREAD_...
	int elements;		// number of elements (e.g. elements in array or object)
	int bytelen;		// byte length of element (e.g. length of string, array text "[ ... ]" etc.)
	const void *pValue; // pointer to value string in JSON text
	int error;			// error value if dataType == JREAD_ERROR
};

enum class NodeType
{
	JS_OBJECT = 1,
	JS_ARRAY,
	JS_NULL
};

enum class ReadError
{
	JS_OK = 1,
	JS_ERROR
};

/**
	 * @brief JSON object class
	 * 
	 */
class Json
{
private:
	// Variables:
	char *buffer;		 // pointer to application's buffer
	unsigned int buflen; // length of buffer
	char *bufp;			 // current write position in buffer
	char tmpbuf[32];	 // local buffer for int/double convertions
	int error;			 // error code
	int callNo;			 // API call on which error occurred
	struct jwNodeStack
	{
		NodeType nodeType;
		int elementNo;
	} nodeStack[JWRITE_STACK_DEPTH]; // stack of array/object nodes
	int stackpos;
	int isPretty; // 1= pretty output (inserts \n and spaces)
	// private methods:

	//-------------------------------------------------
	// Optional String output Functions
	//
	const char *jReadTypeStrings[14] = {
		"Error",	  // 0
		"Object",	 // 1
		"Array",	  // 2
		"String",	 // 3
		"Number",	 // 4
		"Bool",		  // 5
		"null",		  // 6
		"Object key", // 7
		"colon",	  // 8
		"eol",		  // 9
		"comma",	  // 10
		"}",		  // 11
		"]",		  // 12
		"* parameter" // 13
	};

	const char *jReadErrorStrings[15] = {
		"Ok",										// 0
		"JSON does not match Query",				// 1
		"Error reading JSON value",					// 2
		"Expected \"key\"",							// 3
		"Expected ':'",								// 4
		"Object key not found",						// 5
		"Expected ',' in object",					// 6
		"Terminal value found before end of query", // 7
		"Unexpected character",						// 8
		"Expected ',' in array",					// 9
		"Array element not found (bad index)",		// 10
		"Object key not found (bad index)",			// 11
		"Bad object key",							// 12
		"End of array found",						// 13
		"End of object found"						// 14
	};

	/**
		 * @brief Write character to buffer
		 * 
		 * Put one char to JSON buffer, overflow check.
		 * 
		 * @param c Character to write to buffer
		 */
	void putch(const char c);

	/**
		 * @brief Write quoted string to buffer
		 * 
		 * Quote string with \"
		 * 
		 * @param str NULL terminated string to write to buffer
		 */
	void putstr(const char *str);

	/**
		 * @brief Write raw string to buffer
		 * 
		 * @param str NULL terminated string to write to buffer
		 */
	void putraw(const char *str);

	/**
		 * @brief Pretty printing
		 * 
		 * Add newline and whitspace to create pretty json print format
		 * 
		 */
	void pretty();

	/**
		 * @brief Pop node stack
		 * 
		 * Get nodetype on top of stack
		 * 
		 * @return jWrite::pop Node type on top of stack
		 */
	NodeType pop();

	/**
		 * @brief Puch node stack
		 * 
		 * Add node to top of stack
		 * 
		 * @param NodeType Node type to push to stack
		 */
	void push(NodeType nodeType);

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
	int _jwObj(const char *key);

	/**
		 * @brief Common Array function*
		 * 
		 * - checks error
		 * - checks current node is ARRAY
		 * - adds comma if reqd
		 * 
		 * @return int Error code
		 */
	int _jwArr();

	//------------------------------------------------------
	// Internal Functions

	static const char *skipWhitespace(const char *sp);
	static const char *findTok(const char *sp, int *tokType);
	static const char *getElementString(const char *pJson, struct ReadElement *pElem, char quote);
	static int getElementStringLenght(const char *pJson);
	static int equalElement(struct ReadElement *j1, struct ReadElement *j2);
	static const char *getObjectLength(const char *pJson, struct ReadElement *pResult, int keyIndex);
	static const char *getArrayLength(const char *pJson, struct ReadElement *pResult);
	static char *copyElementValue(char *destBuffer, int destLength, struct ReadElement *pElement);
	//=======================================================

public:
	Json(char *pbuffer, int buf_len);

	/**
		 * @brief open writing of JSON
		 * 
		 * initialise with user string buffer of length buflen
		 * 
		 * @param rootType is the base JSON type: JS_OBJECT or JW_ARRAY
		 * @param is_Pretty controls 'prettifying' the output: JW_PRETTY or JW_COMPACT)
		 */
	void open(NodeType rootType, int is_Pretty);

	/**
		 * @brief Closes the element opened by open()
		 * 
		 * After an error, all following jWrite calls are skipped internally
		 * so the error code is for the first error detected
		 * 
		 * @return int error code (0 = JWRITE_OK)
		 */
	int close();

	/**
		 * @brief Error Position
		 * 
		 * If jwClose returned an error, this function returns the number of the jWrite function call
		 * which caused that error.
		 * 
		 * @return int position of error: the nth call to a jWrite function
		 */
	int errorPos();

	/*******************************************/

	/**
		 * @brief  Open new object in object
		 * 
		 * @param key Object key
		 * @param nodeType Object type, as object, array or null
		 * @return int error code
		 */
	int add(const char *key, NodeType nodeType);

	/**
		 * @brief Add object to array
		 * 
		 * @param nodeType Object type, as object, array or null
		 * @return int error code
		 */
	int add(NodeType nodeType);

	/**
		 * @brief Object string insert functions
		 * 
		 * Used to insert "key":"value" pairs into an object
		 * Put "quoted" string to object
		 * 
		 * @param key Object key name
		 * @param value Object value
		 */
	void add(const char *key, const char *value);

	/**
		 * @brief Object integer insert functions
		 * 
		 * Used to insert "key":"value" pairs into an object
		 * 
		 * @param key Object key name
		 * @param value Object value as integer
		 */
	void add(const char *key, int value);

	/**
		 * @brief Object double insert functions
		 * 
		 * Used to insert "key":"value" pairs into an object
		 * 
		 * @param key Object key name
		 * @param value Object value as double
		 */
	void add(const char *key, double value);

	/**
		 * @brief Object bool insert functions
		 * 
		 * Used to insert "key":"value" pairs into an object
		 * Insert bool object, 0 or 1 is written as "true" or "false"
		 * 
		 * @param key Object key name
		 * @param oneOrZero Object value as bool 0 or 1
		 */
	void add(const char *key, bool oneOrZero);

	/**
		 * @brief Array string insert functions
		 * 
		 * Used to insert "value" elements into an array
		 * Put "quoted" string to array
		 * 
		 * @param value Array value as string
		 */
	void add(const char *value);

	/**
		 * @brief Array integer insert functions
		 * 
		 * Used to insert "value" elements into an array
		 * 
		 * @param value Array value as integer
		 */
	void add(int value);

	/**
		 * @brief Array double insert functions
		 * 
		 * Used to insert "value" elements into an array
		 * 
		 * @param value Array value as double
		 */
	void add(double value);

	/**
		 * @brief Array bool insert functions
		 * 
		 * Used to insert "value" elements into an array
		 * Insert bool object, 0 or 1 is written as "true" or "false"
		 * 
		 * @param oneOrZero Array value as 0 or 1
		 */
	void add(bool oneOrZero);

	/**
		 * @brief End the current array/object
		 * 
		 * @return int error code
		 */
	int end();

	/**
		 * @brief Object raw insert functions
		 * 
		 * Put raw string to object (i.e. contents of rawtext without quotes)
		 * Use if your app. supplies its own value->string functions
		 * 
		 * @param key Object key name
		 * @param rawtext Object value as raw text
		 */
	void addRaw(const char *key, const char *rawtext);

	/**
		 * @brief Array raw insert functions
		 * 
		 * Put raw string to array (i.e. contents of rawtext without quotes)
		 * Use if your app. supplies its own value->string functions
		 * 
		 * @param rawtext Array value as raw text
		 */
	void addRaw(const char *rawtext);

	/**
		 * @brief ErrorToString
		 * 
		 * @param err Error code
		 * @return const char* '\0'-termianted string describing the error (as returned by jwClose())
		 */
	const char *errorToString(int err);

	//------------------------------------------------------
	// The JSON reader functions
	//------------------------------------------------------

	/**
		 * @brief Get the Element object
		 * 
		 * Traverses the JSON according to the pQuery string
		 * matches pQuery against JSON and return match as an element struct
		 * 
		 * Note: is recursive
		 * 
		 * @param pQuery Query string to match object, array and key
		 * @param pResult Return of element matching the query string
		 * @return const char* Pointer into JSON after the queried value
		 */
	const char *getElement(const char *pQuery, struct ReadElement &pResult);

	/**
		 * @brief Get the Element object
		 * 
		 * Traverses the JSON according to the pQuery string
		 * matches pQuery against JSON and return match as an element struct
		 * 
		 * Allows one or more queryParam integers to be substituted
		 * for array or object indexes marked by a '*' in the query
		 * 
		 * ! CAUTION !
		 * You can supply an array of integers which are indexed for each '*' in pQuery
		 * however, horrid things will happen if you don't supply enough parameters
		 * 
		 * Note: is recursive
		 * 
		 * @param pQuery Query string to match object, array and key
		 * @param queryParams Array of integers as index for each '*' in pQuery
		 * @param pResult Return of element matching the query string
		 * @return const char* Pointer into JSON after the queried value
		 */
	const char *getElement(const char *pQuery, int *queryParams, struct ReadElement &pResult);

	/**
		 * @brief Get the Element object
		 * 
		 * Static implementation to quickly query JSON string without creating an JSON object
		 * 
		 * Traverses the JSON according to the pQuery string
		 * matches pQuery against JSON and return match as an element struct
		 * 
		 * Note: is recursive
		 * 
		 * @param pJson '\0'-terminated JSON text string to query
		 * @param pQuery Query string to match object, array and key
		 * @param pResult Return of element matching the query string
		 * @return const char* Pointer into JSON after the queried value
		 */
	static const char *getElement(const char *pJson, const char *pQuery, struct ReadElement *pResult);

	/**
		 * @brief Get the Element object
		 * 
		 * Static implementation to quickly query JSON string without creating an JSON object
		 * 
		 * Traverses the JSON according to the pQuery string
		 * matches pQuery against JSON and return match as an element struct
		 * 
		 * Allows one or more queryParam integers to be substituted
		 * for array or object indexes marked by a '*' in the query
		 * 
		 * ! CAUTION !
		 * You can supply an array of integers which are indexed for each '*' in pQuery
		 * however, horrid things will happen if you don't supply enough parameters
		 * 
		 * Note: is recursive
		 * 
		 * @param pJson '\0'-terminated JSON text string to query
		 * @param pQuery Query string to match object, array and key
		 * @param queryParams Array of integers as index for each '*' in pQuery
		 * @param pResult Return of element matching the query string
		 * @return const char* Pointer into JSON after the queried value
		 */
	static const char *getElement(const char *pJson, const char *pQuery, int *queryParams, struct ReadElement *pResult);

	/**
		 * @brief Get the Array Element object
		 * 
		 * Assumes pJsonArray is JSON source of an array "[ ... ]"
		 * 
		 * If end of array is encountered, pResult->error = 13 "End of array found"
		 * 
		 * @param pJsonArray JSON source of an array "[ ... ]"
		 * @param pResult Returns element of the array
		 * @return const char* Pointer to end of element, to be passed to next call of getArrayElement
		 */
	static const char *getArrayElement(const char *pJsonArray, struct ReadElement *pResult);

	//------------------------------------------------------
	// "Helper" Functions
	//------------------------------------------------------

	/**
		 * @brief Get the Value object as signed long
		 * 
		 * Simple routine to extract values from JSON, does coercion of types where possible
		 * Use param for returning JSON value and return error code for parsing status
		 * 
		 * Returns number from NUMBER or STRING elements (if possible)
		 * Returns 1 or 0 from BOOL elements
		 * Otherwise returns 0
		 *
		 * @param pQuery Query string to match object, array and key
		 * @param value Ref for return of JSON value
		 * @return ReadError Error code, JS_OK or JS_ERROR
		 */
	ReadError getValue(const char *pQuery, long &value);

	/**
		 * @brief Get the Value object as signed long
		 * 
		 * Simple routine to extract values from JSON, does coercion of types where possible
		 * Use param for returning JSON value and return error code for parsing status
		 * 
		 * Returns number from NUMBER or STRING elements (if possible)
		 * Returns 1 or 0 from BOOL elements
		 * Otherwise returns 0
		 * 
		 * Allows one or more queryParam integers to be substituted
		 * for array or object indexes marked by a '*' in the query
		 * 
		 * ! CAUTION !
		 * You can supply an array of integers which are indexed for each '*' in pQuery
		 * however, horrid things will happen if you don't supply enough parameters 
		 *
		 * @param pQuery Query string to match object, array and key
		 * @param queryParams Array of integers as index for each '*' in pQuery
		 * @param value Ref for return of JSON value
		 * @return ReadError Error code, JS_OK or JS_ERROR
		 */
	ReadError getValue(const char *pQuery, int *queryParams, long &value);

	/**
		 * @brief Get the Value object as signed long
		 * 
		 * Static implementation to quickly query JSON string without creating an JSON object
		 * 
		 * Simple routine to extract values from JSON, does coercion of types where possible
		 * Use param for returning JSON value and return error code for parsing status
		 * 
		 * Returns number from NUMBER or STRING elements (if possible)
		 * Returns 1 or 0 from BOOL elements
		 * Otherwise returns 0
		 * 
		 * Allows one or more queryParam integers to be substituted
		 * for array or object indexes marked by a '*' in the query
		 * 
		 * ! CAUTION !
		 * You can supply an array of integers which are indexed for each '*' in pQuery
		 * however, horrid things will happen if you don't supply enough parameters 
		 *
		 * @param pJson '\0'-terminated JSON text string to query
		 * @param pQuery Query string to match object, array and key
		 * @param queryParams Array of integers as index for each '*' in pQuery
		 * @param value Pointer for return of JSON value
		 * @return ReadError Error code, JS_OK or JS_ERROR
		 */
	static ReadError getValue(const char *pJson, const char *pQuery, int *queryParams, long *value);

	/**
		 * @brief Get the Value object as integer
		 * 
		 * Simple routine to extract values from JSON, does coercion of types where possible
		 * Use param for returning JSON value and return error code for parsing status
		 * 
		 * Returns number from NUMBER or STRING elements (if possible)
		 * Returns 1 or 0 from BOOL elements
		 * Otherwise returns 0
		 *
		 * @param pQuery Query string to match object, array and key
		 * @param value Ref for return of JSON value
		 * @return ReadError Error code, JS_OK or JS_ERROR
		 */
	ReadError getValue(const char *pQuery, int &value);

	/**
		 * @brief Get the Value object as integer
		 * 
		 * Simple routine to extract values from JSON, does coercion of types where possible
		 * Use param for returning JSON value and return error code for parsing status
		 * 
		 * Returns number from NUMBER or STRING elements (if possible)
		 * Returns 1 or 0 from BOOL elements
		 * Otherwise returns 0
		 * 
		 * Allows one or more queryParam integers to be substituted
		 * for array or object indexes marked by a '*' in the query
		 * 
		 * ! CAUTION !
		 * You can supply an array of integers which are indexed for each '*' in pQuery
		 * however, horrid things will happen if you don't supply enough parameters 
		 *
		 * @param pQuery Query string to match object, array and key
		 * @param queryParams Array of integers as index for each '*' in pQuery
		 * @param value Ref for return of JSON value
		 * @return ReadError Error code, JS_OK or JS_ERROR
		 */
	ReadError getValue(const char *pQuery, int *queryParams, int &value);

	/**
		 * @brief Get the Value object as integer
		 * 
		 * Static implementation to quickly query JSON string without creating an JSON object
		 * 
		 * Simple routine to extract values from JSON, does coercion of types where possible
		 * Use param for returning JSON value and return error code for parsing status
		 * 
		 * Returns number from NUMBER or STRING elements (if possible)
		 * Returns 1 or 0 from BOOL elements
		 * Otherwise returns 0
		 * 
		 * Allows one or more queryParam integers to be substituted
		 * for array or object indexes marked by a '*' in the query
		 * 
		 * ! CAUTION !
		 * You can supply an array of integers which are indexed for each '*' in pQuery
		 * however, horrid things will happen if you don't supply enough parameters 
		 *
		 * @param pJson '\0'-terminated JSON text string to query
		 * @param pQuery Query string to match object, array and key
		 * @param queryParams Array of integers as index for each '*' in pQuery
		 * @param value Pointer for return of JSON value
		 * @return ReadError Error code, JS_OK or JS_ERROR
		 */
	static ReadError getValue(const char *pJson, const char *pQuery, int *queryParams, int *value);

	/**
		 * @brief Get the Value object as double
		 * 
		 * Simple routine to extract values from JSON, does coercion of types where possible
		 * Use param for returning JSON value and return error code for parsing status
		 * 
		 * Returns number from NUMBER or STRING elements (if possible)
		 * Otherwise returns 0.0
		 *
		 * @param pQuery Query string to match object, array and key
		 * @param value Ref for return of JSON value
		 * @return ReadError Error code, JS_OK or JS_ERROR
		 */
	ReadError getValue(const char *pQuery, double &value);

	/**
		 * @brief Get the Value object as double
		 * 
		 * Simple routine to extract values from JSON, does coercion of types where possible
		 * Use param for returning JSON value and return error code for parsing status
		 * 
		 * Returns number from NUMBER or STRING elements (if possible)
		 * Otherwise returns 0.0
		 * 
		 * Allows one or more queryParam integers to be substituted
		 * for array or object indexes marked by a '*' in the query
		 * 
		 * ! CAUTION !
		 * You can supply an array of integers which are indexed for each '*' in pQuery
		 * however, horrid things will happen if you don't supply enough parameters 
		 *
		 * @param pQuery Query string to match object, array and key
		 * @param queryParams Array of integers as index for each '*' in pQuery
		 * @param value Ref for return of JSON value
		 * @return ReadError Error code, JS_OK or JS_ERROR
		 */
	ReadError getValue(const char *pQuery, int *queryParams, double &value);

	/**
		 * @brief Get the Value object as double
		 * 
		 * Static implementation to quickly query JSON string without creating an JSON object
		 * 
		 * Simple routine to extract values from JSON, does coercion of types where possible
		 * Use param for returning JSON value and return error code for parsing status
		 * 
		 * Returns number from NUMBER or STRING elements (if possible)
		 * Otherwise returns 0.0
		 * 
		 * Allows one or more queryParam integers to be substituted
		 * for array or object indexes marked by a '*' in the query
		 * 
		 * ! CAUTION !
		 * You can supply an array of integers which are indexed for each '*' in pQuery
		 * however, horrid things will happen if you don't supply enough parameters 
		 *
		 * @param pJson '\0'-terminated JSON text string to query
		 * @param pQuery Query string to match object, array and key
		 * @param queryParams Array of integers as index for each '*' in pQuery
		 * @param value Pointer for return of JSON value
		 * @return ReadError Error code, JS_OK or JS_ERROR
		 */
	static ReadError getValue(const char *pJson, const char *pQuery, int *queryParams, double *value);

	/**
		 * @brief Get the Value object as bool
		 * 
		 * Simple routine to extract values from JSON, does coercion of types where possible
		 * Use param for returning JSON value and return error code for parsing status
		 * 
		 * Returns true for true and false for false and null
		 * Otherwise returns false
		 *
		 * @param pQuery Query string to match object, array and key
		 * @param value Ref for return of JSON value
		 * @return ReadError Error code, JS_OK or JS_ERROR
		 */
	ReadError getValue(const char *pQuery, bool &value);

	/**
		 * @brief Get the Value object as bool
		 * 
		 * Simple routine to extract values from JSON, does coercion of types where possible
		 * Use param for returning JSON value and return error code for parsing status
		 * 
		 * Returns true for true and false for false and null
		 * Otherwise returns false
		 * 
		 * Allows one or more queryParam integers to be substituted
		 * for array or object indexes marked by a '*' in the query
		 * 
		 * ! CAUTION !
		 * You can supply an array of integers which are indexed for each '*' in pQuery
		 * however, horrid things will happen if you don't supply enough parameters 
		 *
		 * @param pQuery Query string to match object, array and key
		 * @param queryParams Array of integers as index for each '*' in pQuery
		 * @param value Ref for return of JSON value
		 * @return ReadError Error code, JS_OK or JS_ERROR
		 */
	ReadError getValue(const char *pQuery, int *queryParams, bool &value);

	/**
		 * @brief Get the Value object as bool
		 * 
		 * Static implementation to quickly query JSON string without creating an JSON object
		 * 
		 * Simple routine to extract values from JSON, does coercion of types where possible
		 * Use param for returning JSON value and return error code for parsing status
		 * 
		 * Returns true for true and false for false and null
		 * Otherwise returns false0.0
		 * 
		 * Allows one or more queryParam integers to be substituted
		 * for array or object indexes marked by a '*' in the query
		 * 
		 * ! CAUTION !
		 * You can supply an array of integers which are indexed for each '*' in pQuery
		 * however, horrid things will happen if you don't supply enough parameters 
		 *
		 * @param pJson '\0'-terminated JSON text string to query
		 * @param pQuery Query string to match object, array and key
		 * @param queryParams Array of integers as index for each '*' in pQuery
		 * @param value Pointer for return of JSON value
		 * @return ReadError Error code, JS_OK or JS_ERROR
		 */
	static ReadError getValue(const char *pJson, const char *pQuery, int *queryParams, bool *value);

	/**
		 * @brief Get the Value object as string(char *)
		 * 
		 * Simple routine to extract values from JSON, does coercion of types where possible
		 * Use param for returning JSON value and return error code for parsing status
		 * 
		 * Copy string to pDest and '\0'-terminate it (upto destlen total bytes)
		 *
		 * Note: any element can be returned as a string
		 * 
		 * @param pQuery Query string to match object, array and key
		 * @param pDest Pointer for return of JSON value
		 * @param destlen Max length of returning JSON string value
		 * @return ReadError Error code, JS_OK or JS_ERROR
		 */
	ReadError getValue(const char *pQuery, char *pDest, int destlen);

	/**
		 * @brief Get the Value object as string(char *)
		 * 
		 * Simple routine to extract values from JSON, does coercion of types where possible
		 * Use param for returning JSON value and return error code for parsing status
		 * 
		 * Copy string to pDest and '\0'-terminate it (upto destlen total bytes)
		 *
		 * Allows one or more queryParam integers to be substituted
		 * for array or object indexes marked by a '*' in the query
		 * 
		 * ! CAUTION !
		 * You can supply an array of integers which are indexed for each '*' in pQuery
		 * however, horrid things will happen if you don't supply enough parameters 
		 * 
		 * Note: any element can be returned as a string
		 * 
		 * @param pQuery Query string to match object, array and key
		 * @param queryParams Array of integers as index for each '*' in pQuery
		 * @param pDest Pointer for return of JSON value
		 * @param destlen Max length of returning JSON string value
		 * @return ReadError Error code, JS_OK or JS_ERROR
		 */
	ReadError getValue(const char *pQuery, int *queryParams, char *pDest, int destlen);

	/**
		 * @brief Get the Value object as string(char *)
		 * 
		 * Static implementation to quickly query JSON string without creating an JSON object
		 * 
		 * Simple routine to extract values from JSON, does coercion of types where possible
		 * Use param for returning JSON value and return error code for parsing status
		 * 
		 * Copy string to pDest and '\0'-terminate it (upto destlen total bytes)
		 *
		 * Allows one or more queryParam integers to be substituted
		 * for array or object indexes marked by a '*' in the query
		 * 
		 * ! CAUTION !
		 * You can supply an array of integers which are indexed for each '*' in pQuery
		 * however, horrid things will happen if you don't supply enough parameters 
		 *
		 * Note: any element can be returned as a string
		 * 
		 * @param pJson '\0'-terminated JSON text string to query
		 * @param pQuery Query string to match object, array and key
		 * @param queryParams Array of integers as index for each '*' in pQuery
		 * @param pDest Pointer for return of JSON value
		 * @param destlen Max length of returning JSON string value
		 * @return ReadError Error code, JS_OK or JS_ERROR
		 */
	static ReadError getValue(const char *pJson, const char *pQuery, int *queryParams, char *pDest, int destlen);

	//------------------------------------------------------
	// Optional String output Functions
	//
	const char *jReadTypeToString(int dataType); // string describes dataType
	const char *jReadErrorToString(int error);   // string descibes error code
};
} // namespace jonaskgandersson
