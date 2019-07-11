/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Nick Galbreath
 * Copyright (c) 2019 Jonas Andersson
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 */

extern "C" {

#define valid_digit(c) ((c) >= '0' && (c) <= '9')

//=================================================================
//
// modp value-to-string functions
// - modified for C89
//
// We use these functions as they are a lot faster than sprintf()
//
// Origin of these routines: https://github.com/client9/stringencoders

/** \brief convert an signed integer to char buffer
 *
 * \param[in] value
 * \param[out] buf the output buffer.  Should be 16 chars or more.
 */
void modp_itoa10(int value, char *str);

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
void modp_dtoa2(double value, char *str, int prec);
//------------------------------------------------------
// Other jRead utilities which may be useful...
//
const char * jRead_atoi( const char *p, unsigned int *result );	// string to unsigned int
const char * jRead_atol( const char *p, long *result );			// string to signed long
const char * jRead_atof( const char *p, double *result);		// string to double (does not do exponents)

}
