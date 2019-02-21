#ifndef BSD_SNPRINTF_H
#define BSD_SNPRINTF_H

#include "cstdmf/stdmf.hpp"

#ifdef HAVE_LONG_DOUBLE
# define LDOUBLE long double
#else
# define LDOUBLE double
#endif

#define LLONG int64
#define ULLONG uint64

/* Conversion Flags */
#define DP_C_SHORT   1
#define DP_C_LONG    2
#define DP_C_LDOUBLE 3
#define DP_C_LLONG   4

// BigWorld variable width field flags
#define VARIABLE_MIN_WIDTH 1
#define VARIABLE_MAX_WIDTH 2
typedef short WidthType;

#include <string>

void handleFormatString( const char *str, FormatStringHandler &fsh );

void bsdFormatString( const char *value, int flags, int min, int max,
	std::string &out );

void bsdFormatInt( LLONG value, uint8 base, int min, int max, int flags,
	std::string &out );

void bsdFormatFloat( LDOUBLE fvalue, int min, int max, int flags,
	std::string &out );

#endif
