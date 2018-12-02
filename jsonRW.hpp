/**
 * @brief jsonRW.hpp
 * A *really* simple JSON writer in C++
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

#define JWRITE_STACK_DEPTH 32			// max nesting depth of objects/arrays

#define JW_COMPACT	0					// output string control for jwOpen()
#define JW_PRETTY	1					// pretty adds \n and indentation

// Error Codes
// -----------
#define JWRITE_OK           0
#define JWRITE_BUF_FULL     1		// output buffer full
#define JWRITE_NOT_ARRAY	2		// tried to write Array value into Object
#define JWRITE_NOT_OBJECT	3		// tried to write Object key/value into Array
#define JWRITE_STACK_FULL	4		// array/object nesting > JWRITE_STACK_DEPTH
#define JWRITE_STACK_EMPTY	5		// stack underflow error (too many 'end's)
#define JWRITE_NEST_ERROR	6		// nesting error, not all objects closed when jwClose() called

enum jwNodeType{
	JW_OBJECT= 1,
	JW_ARRAY
};

/**
 * @brief JSON object class
 * 
 */
class jWrite{
  private:
	// Variables:
	char *buffer;						// pointer to application's buffer
	unsigned int buflen;		// length of buffer
	char *bufp;							// current write position in buffer
	char tmpbuf[32];				// local buffer for int/double convertions
	int error;							// error code
	int callNo;							// API call on which error occurred
	struct jwNodeStack{
		enum jwNodeType nodeType;
		int elementNo;
	} nodeStack[JWRITE_STACK_DEPTH];	// stack of array/object nodes
	int stackpos;
	int isPretty;						// 1= pretty output (inserts \n and spaces)
	// private methods:

	/**
	 * @brief Write character to buffer
	 * 
	 * Put one char to JSON buffer, overflow check.
	 * 
	 * @param c Character to write to buffer
	 */
	void putch( const char c );

	/**
	 * @brief Write quoted string to buffer
	 * 
	 * Quote string with \"
	 * 
	 * @param str NULL terminated string to write to buffer
	 */
	void putstr( const char *str );

	/**
	 * @brief Write raw string to buffer
	 * 
	 * @param str NULL terminated string to write to buffer
	 */
	void putraw( const char *str );
	void modp_itoa10(int value, char* str);
	void modp_dtoa2(double value, char* str, int prec);

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
	 * @return enum jWrite::pop Node type on top of stack
	 */
	enum jwNodeType pop();

	/**
	 * @brief Puch node stack
	 * 
	 * Add node to top of stack
	 * 
	 * @param jwNodeType Node type to push to stack
	 */
	void push( enum jwNodeType nodeType );
	
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
	int _jwObj( const char *key );

	/**
	 * @brief Common Array function*
	 * 
	 * - checks error
	 * - checks current node is ARRAY
	 * - adds comma if reqd
	 * 
	 * @return int Error code
	 */
	int _jwArr( );
	
  public:
	jWrite( char *pbuffer, int buf_len ) : buffer(pbuffer), buflen(buf_len){
		open( JW_OBJECT, JW_COMPACT );
	};
	
	/**
	 * @brief open writing of JSON
	 * 
	 * initialise with user string buffer of length buflen
	 * 
	 * @param rootType is the base JSON type: JW_OBJECT or JW_ARRAY
	 * @param is_Pretty controls 'prettifying' the output: JW_PRETTY or JW_COMPACT)
	 */
	void open( enum jwNodeType rootType, int is_Pretty );

	/**
	 * @brief Closes the element opened by open()
	 * 
	 * After an error, all following jWrite calls are skipped internally
	 * so the error code is for the first error detected
	 * 
	 * @return int error code (0 = JWRITE_OK)
	 */
	int close( );

	/**
	 * @brief Error Position
	 * 
	 * If jwClose returned an error, this function returns the number of the jWrite function call
	 * which caused that error.
	 * 
	 * @return int position of error: the nth call to a jWrite function
	 */
	int errorPos( );

	/**
	 * @brief Object string insert functions
	 * 
	 * Used to insert "key":"value" pairs into an object
	 * Put "quoted" string to object
	 * 
	 * @param key Object key name
	 * @param value Object value
	 */
	void obj_string( const char *key, const char *value );

	/**
	 * @brief Object integer insert functions
	 * 
	 * Used to insert "key":"value" pairs into an object
	 * 
	 * @param key Object key name
	 * @param value Object value as integer
	 */
	void obj_int( const char *key, int value );

	/**
	 * @brief Object double insert functions
	 * 
	 * Used to insert "key":"value" pairs into an object
	 * 
	 * @param key Object key name
	 * @param value Object value as double
	 */
	void obj_double( const char *key, double value );

	/**
	 * @brief Object bool insert functions
	 * 
	 * Used to insert "key":"value" pairs into an object
	 * Insert bool object, 0 or 1 is written as "true" or "false"
	 * 
	 * @param key Object key name
	 * @param oneOrZero Object value as bool 0 or 1
	 */
	void obj_bool( const char *key, int oneOrZero );

	/**
	 * @brief Object null insert functions
	 * 
	 * Used to insert "key":"value" pairs into an object
	 * Insert empty object
	 * 
	 * @param key Object key name
	 */
	void obj_null( const char *key );

	/**
	 * @brief Object in Object
	 * 
	 * Open new object inside current object
	 * 
	 * @param key Object key name
	 */
	void obj_object( const char *key );

	/**
	 * @brief Array in Object
	 * 
	 * Open new Array inside current object
	 * 
	 * @param key Object key name
	 */
	void obj_array( const char *key );

	/**
	 * @brief Array string insert functions
	 * 
	 * Used to insert "value" elements into an array
	 * Put "quoted" string to array
	 * 
	 * @param value Array value as string
	 */
	void arr_string( const char *value );

	/**
	 * @brief Array integer insert functions
	 * 
	 * Used to insert "value" elements into an array
	 * 
	 * @param value Array value as integer
	 */
	void arr_int( int value );

	/**
	 * @brief Array double insert functions
	 * 
	 * Used to insert "value" elements into an array
	 * 
	 * @param value Array value as double
	 */
	void arr_double( double value );

	/**
	 * @brief Array bool insert functions
	 * 
	 * Used to insert "value" elements into an array
	 * Insert bool object, 0 or 1 is written as "true" or "false"
	 * 
	 * @param oneOrZero Array value as 0 or 1
	 */
	void arr_bool( int oneOrZero );

	/**
	 * @brief Array null insert functions
	 * 
	 * Used to insert "value" elements into an array
	 * Insert empty array value
	 * 
	 */
	void arr_null( );


	/**
	 * @brief Array object insert functions
	 * 
	 * Create new object inside current array
	 * 
	 */
	void arr_object( );

	/**
	 * @brief Array array insert functions
	 * 
	 * Create new array inside current array
	 * 
	 */
	void arr_array( );

	/**
	 * @brief End the current array/object
	 * 
	 * @return int error code
	 */
	int end( );

	/**
	 * @brief Object raw insert functions
	 * 
	 * Put raw string to object (i.e. contents of rawtext without quotes)
	 * Use if your app. supplies its own value->string functions
	 * 
	 * @param key Object key name
	 * @param rawtext Object value as raw text
	 */
	void obj_raw( const char *key, const char *rawtext );


	/**
	 * @brief Array raw insert functions
	 * 
	 * Put raw string to array (i.e. contents of rawtext without quotes)
	 * Use if your app. supplies its own value->string functions
	 * 
	 * @param rawtext Array value as raw text
	 */
	void arr_raw( const char *rawtext );
	
	/**
	 * @brief ErrorToString
	 * 
	 * @param err Error code
	 * @return const char* '\0'-termianted string describing the error (as returned by jwClose())
	 */
	const char * errorToString( int err );

};

/* end of jWrite.hpp */
