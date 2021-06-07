/*
 * %CopyrightBegin%
 * 
 * Copyright Ericsson AB 2006-2016. All Rights Reserved.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * 
 * %CopyrightEnd%
 */



/*
 * Darwin needs conversion!
 * http://developer.apple.com/library/mac/#qa/qa2001/qa1235.html
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "sys.h"
#include "global.h"

#if defined(__APPLE__) && defined(__MACH__) && !defined(__DARWIN__)
#define __DARWIN__ 1
#endif

#if !defined(__WIN32__)
#include <locale.h>
#if !defined(HAVE_SETLOCALE) || !defined(HAVE_NL_LANGINFO) || !defined(HAVE_LANGINFO_H)
#define PRIMITIVE_UTF8_CHECK 1
#else
#include <langinfo.h>
#endif
#endif

/*
 * erts_check_io_time is used by the erl_check_io implementation. The
 * global erts_check_io_time variable is declared here since there
 * (often) exist two versions of erl_check_io (kernel-poll and
 * non-kernel-poll), and we dont want two versions of this variable.
 */
erts_smp_atomic_t erts_check_io_time;

/* Written once and only once */

static int filename_encoding = ERL_FILENAME_UNKNOWN;
static int filename_warning = ERL_FILENAME_WARNING_WARNING;
#if defined(__WIN32__) || defined(__DARWIN__)
/* Default unicode on windows and MacOS X */
static int user_filename_encoding = ERL_FILENAME_UTF8; 
#else
static int user_filename_encoding = ERL_FILENAME_UNKNOWN;
#endif
/* This controls the heuristic in printing characters in shell and w/ 
   io:format("~tp", ...) etc. */
static int printable_character_set = ERL_PRINTABLE_CHARACTERS_LATIN1;

void erts_set_user_requested_filename_encoding(int encoding, int warning)
{
    user_filename_encoding = encoding;
    filename_warning = warning;
}

int erts_get_user_requested_filename_encoding(void)
{
    return user_filename_encoding;
}

int erts_get_filename_warning_type(void)
{
    return filename_warning;
}

void erts_set_printable_characters(int range) {
    /* Not an atomic */
    printable_character_set = range;
}

int erts_get_printable_characters(void) {
    return  printable_character_set;
}

void erts_init_sys_common_misc(void)
{
#if defined(__WIN32__)
    /* win_efile will totally fail if this is not set. */
    filename_encoding = ERL_FILENAME_WIN_WCHAR;
#else
    if (user_filename_encoding != ERL_FILENAME_UNKNOWN) {
	filename_encoding = user_filename_encoding;
    } else {
	char *l;
	filename_encoding = ERL_FILENAME_LATIN1;
#  ifdef PRIMITIVE_UTF8_CHECK
	setlocale(LC_CTYPE, "");  /* Set international environment, 
				     ignore result */
	if (((l = getenv("LC_ALL"))   && *l) ||
	    ((l = getenv("LC_CTYPE")) && *l) ||
	    ((l = getenv("LANG"))     && *l)) {
	    if (strstr(l, "UTF-8")) {
		filename_encoding = ERL_FILENAME_UTF8;
	    } 
	}
	
#  else
	l = setlocale(LC_CTYPE, "");  /* Set international environment */
	if (l != NULL) {
	    if (strcmp(nl_langinfo(CODESET), "UTF-8") == 0) {
		filename_encoding = ERL_FILENAME_UTF8;
	    }
	}
#  endif
    }
#  if defined(__DARWIN__)
    if (filename_encoding == ERL_FILENAME_UTF8) {
	filename_encoding = ERL_FILENAME_UTF8_MAC;
    }
#  endif
#endif
}

int erts_get_native_filename_encoding(void)
{
    return filename_encoding;
}

/* For internal use by sys_double_to_chars_fast() */
static char* find_first_trailing_zero(char* p)
{
    for (; *(p-1) == '0'; --p);
    if (*(p-1) == '.') ++p;
    return p;
}

int
sys_double_to_chars(double fp, char *buffer, size_t buffer_size)
{
    return sys_double_to_chars_ext(fp, buffer, buffer_size, SYS_DEFAULT_FLOAT_DECIMALS);
}

/* Convert float to string using fixed point notation.
 *   decimals must be >= 0
 *   if compact != 0, the trailing 0's will be truncated
 */
int
sys_double_to_chars_fast(double f, char *buffer, int buffer_size, int decimals,
			 int compact)
{
    #define SYS_DOUBLE_RND_CONST 0.5
    #define FRAC_SIZE            52
    #define EXP_SIZE             11
    #define EXP_MASK             (((Uint64)1 << EXP_SIZE) - 1)
    #define MAX_DECIMALS         (sizeof(cs_sys_double_pow10) \
				   / sizeof(cs_sys_double_pow10[0]))
    #define FRAC_MASK            (((Uint64)1 << FRAC_SIZE) - 1)
    #define FRAC_MASK2           (((Uint64)1 << (FRAC_SIZE + 1)) - 1)
    #define MAX_FLOAT            ((Uint64)1 << (FRAC_SIZE+1))

    static const double cs_sys_double_pow10[] = {
        SYS_DOUBLE_RND_CONST / 1e0,
        SYS_DOUBLE_RND_CONST / 1e1,
        SYS_DOUBLE_RND_CONST / 1e2,
        SYS_DOUBLE_RND_CONST / 1e3,
        SYS_DOUBLE_RND_CONST / 1e4,
        SYS_DOUBLE_RND_CONST / 1e5,
        SYS_DOUBLE_RND_CONST / 1e6,
        SYS_DOUBLE_RND_CONST / 1e7,
        SYS_DOUBLE_RND_CONST / 1e8,
        SYS_DOUBLE_RND_CONST / 1e9,
        SYS_DOUBLE_RND_CONST / 1e10,
        SYS_DOUBLE_RND_CONST / 1e11,
        SYS_DOUBLE_RND_CONST / 1e12,
        SYS_DOUBLE_RND_CONST / 1e13,
        SYS_DOUBLE_RND_CONST / 1e14,
        SYS_DOUBLE_RND_CONST / 1e15,
        SYS_DOUBLE_RND_CONST / 1e16,
        SYS_DOUBLE_RND_CONST / 1e17,
        SYS_DOUBLE_RND_CONST / 1e18
    };

    Uint64 mantissa, int_part, frac_part;
    int exp;
    int fbits;
    int max;
    int neg;
    double fr;
    union { Uint64 L; double F; } x;
    char *p = buffer;

    if (decimals < 0)
        return -1;

    if (f >= 0) {
        neg = 0;
        fr  = decimals < MAX_DECIMALS ? (f + cs_sys_double_pow10[decimals]) : f;
        x.F = fr;
    } else {
        neg = 1;
        fr  = decimals < MAX_DECIMALS ? (f - cs_sys_double_pow10[decimals]) : f;
        x.F = -fr;
    }

    exp      = (x.L >> FRAC_SIZE) & EXP_MASK;
    mantissa = x.L & FRAC_MASK;

    if (exp == EXP_MASK) {
        if (mantissa == 0) {
            if (neg)
                *p++ = '-';
            *p++ = 'i';
            *p++ = 'n';
            *p++ = 'f';
        } else {
            *p++ = 'n';
            *p++ = 'a';
            *p++ = 'n';
        }
        *p = '\0';
        return p - buffer;
    }

    exp      -= EXP_MASK >> 1;
    mantissa |= ((Uint64)1 << FRAC_SIZE);

    /* Don't bother with optimizing too large numbers or too large precision */
    if (x.F > MAX_FLOAT || decimals >= MAX_DECIMALS) {
        int len = erts_snprintf(buffer, buffer_size, "%.*f", decimals, f);
        char* p = buffer + len;
        if (len >= buffer_size)
            return -1;
        /* Delete trailing zeroes */
        if (compact)
            p = find_first_trailing_zero(p);
        *p = '\0';
        return p - buffer;
    } else if (exp >= FRAC_SIZE) {
        int_part  = mantissa << (exp - FRAC_SIZE);
        frac_part = 0;
        fbits = FRAC_SIZE;  /* not important as frac_part==0 */
    } else if (exp >= 0) {
        fbits = FRAC_SIZE - exp;
        int_part  = mantissa >> fbits;
        frac_part = mantissa & (((Uint64)1 << fbits) -1);
    } else /* if (exp < 0) */ {
        int_part = 0;
        frac_part = mantissa;
        fbits = FRAC_SIZE - exp;
    }

    if (!int_part) {
        if (neg)
            *p++ = '-';
        *p++ = '0';
    } else {
        int ret, i, n;
        while (int_part != 0) {
            *p++ = (char)((int_part % 10) + '0');
            int_part /= 10;
        }
        if (neg)
            *p++ = '-';
        /* Reverse string */
        ret = p - buffer;
        for (i = 0, n = ret/2; i < n; i++) {
            int  j = ret - i - 1;
            char c = buffer[i];
            buffer[i] = buffer[j];
            buffer[j] = c;
        }
    }

    if (decimals > 0) {
        int i;
        *p++ = '.';

        max = buffer_size - (p - buffer) - 1 /* leave room for trailing '\0' */;

        if (decimals > max)
            return -1;  /* the number is not large enough to fit in the buffer */

        max = decimals;

        for (i = 0; i < max; i++) {
            if (frac_part > (ERTS_UINT64_MAX/5)) {
                frac_part >>= 3;
                fbits -= 3;
            }

            /* Multiply by 10 (5*2) to extract decimal digit as integer part */
            frac_part *= 5;
            fbits--;

            if (fbits >= 64) {
                *p++ = '0';
            }
            else {
                *p++ = (char)((frac_part >> fbits) + '0');
                frac_part &= ((Uint64)1 << fbits) - 1;
            }
        }

        /* Delete trailing zeroes */
        if (compact)
            p = find_first_trailing_zero(p);
    }

    *p = '\0';
    return p - buffer;
}