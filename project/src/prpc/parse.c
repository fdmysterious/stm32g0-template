#include <stdint.h>
#include <string.h>

#include "parse.h"

int parse_int( const char *start, const char *end )
{
    int sig          = 1;
    unsigned int mul = 1;
    unsigned int p   = 0;

    if( *start == '-' ) {
        sig = -1;
        start++;
    }

    const char *ptr;
    for( ptr = end ; ptr != start ; ptr-- ) {
        char c = *(ptr-1);
        p   += (c - '0') * mul;
        mul *= 10;
    }

    return p * sig;
}

float parse_float( const char *start, const char *end )
{
    int sig            = 1;
    uint32_t mul       = 1;
    uint32_t div       = 1;

    uint32_t p         = 0;

    const char *dot = NULL; // Pointer to dot char

    if( (*start) == '-' ) {
        sig = -1;
        start++;
    }

    const char *ptr;
    for( ptr = end ; ptr != start ; ptr-- ) {
        char c = *(ptr-1);
        if( c == '.' ) dot = (ptr-1);
        else {
            p += (c-'0')*mul;
            mul *= 10;
            if( dot == NULL ) div *= 10;
        }
    }

    return (float)sig * ((float)p / (float)div);
}
