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

#include <stdio.h>
#include "../ascii_num.h"

static void strreverse(char *begin, char *end)
{
	char aux;
	while (end > begin)
		aux = *end, *end-- = *begin, *begin++ = aux;
}

void modp_itoa10(int value, char *str)
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

void modp_dtoa2(double value, char *str, int prec)
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

// read unsigned int from string
const char *jRead_atoi( const char *p, unsigned int *result )
{
	unsigned int x = 0;
	while (*p >= '0' && *p <= '9') {
		x = (x*10) + (*p - '0');
		++p;
	}
	*result= x;
	return p;
}

// read long int from string
//
const char *jRead_atol( const char *p, long *result )
{
    long x = 0;
    int neg = 0;
    if (*p == '-') {
        neg = 1;
        ++p;
    }
    while (*p >= '0' && *p <= '9') {
        x = (x*10) + (*p - '0');
        ++p;
    }
    if (neg) {
        x = -x;
    }
	*result= x;
	return p;
}



// read double from string
// *CAUTION* does not handle exponents
//
//
const char * jRead_atof( const char *p, double *result)
{
    double sign, value;

    // Get sign, if any.
    sign = 1.0;
    if (*p == '-') {
        sign = -1.0;
        p += 1;

    } else if (*p == '+') {
        p += 1;
    }

    // Get digits before decimal point or exponent, if any.
    for (value = 0.0; valid_digit(*p); p += 1) {
        value = value * 10.0 + (*p - '0');
    }

    // Get digits after decimal point, if any.
    if (*p == '.') {
        double pow10 = 10.0;
        p += 1;
        while (valid_digit(*p)) {
            value += (*p - '0') / pow10;
            pow10 *= 10.0;
            p += 1;
        }
    }
	*result= sign * value;
	return p;
}


