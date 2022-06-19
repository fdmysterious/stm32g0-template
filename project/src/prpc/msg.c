#include <stdarg.h>
#include <printf/printf.h>

#include "lex.h"
#include "msg.h"

static size_t _prpc_build_msg_va( char *buf, const size_t max_len, const size_t id, const char *cmd, const size_t nvals, va_list args )
{
    static const char *bool_vals[] = { "no", "yes" };

    char          *ptr  = buf;
    size_t         len  = max_len;
    size_t      written = 0;

    int ntoken = 0; // EOF for waiting type, then data

    len -= (written = snprintf( ptr, len, "%lu:%s", (unsigned long)id, cmd ));
    ptr += written;

    size_t i;
    for( i = 0 ; i < nvals ; i++ ) {
        ntoken = va_arg(args, int);
        switch( ntoken ) {
            case PRPC_IDENTIFIER:
             len -= (written = snprintf( ptr, len, " %s", va_arg(args, const char*) ));
            break;

            case PRPC_STRING:
             len -= (written = snprintf( ptr, len, " \"%s\"", va_arg(args, const char*) ));
            break;

            case PRPC_INT:
             len -= (written = snprintf( ptr, len, " %llu", va_arg(args, uint64_t) ));
            break;

            case PRPC_FLOAT:
             len -= (written = snprintf( ptr, len, " %f", va_arg(args, double) ));
            break;

            case PRPC_BOOLEAN:
             len -= (written = snprintf( ptr, len, " %s", bool_vals[va_arg(args, int) == 1] ));
            break;
            default:break; // TODO // ERROR ?
        }
        //log_verbose("written = %ld", written);

        if( len > max_len ) len = 0; // Intg overflow protection
        ptr += written;
    }

    if( len <= 1 ) {
        //log_error("Not enough place !");
        len -= 1 + (len == 0);
    }

    // Closing resp string
    //*(ptr  ) = '\n';
    //*(ptr+1) =    0;
    *(ptr)=0;
    return max_len - len;
}

size_t prpc_build_msg( char *buf, const size_t max_len, const size_t id, const char *cmd, const size_t nvals, ... )
{
    va_list args;
    va_start( args, nvals );
    size_t written = _prpc_build_msg_va( buf, max_len, id, cmd, nvals, args );
    va_end( args );

    return written;
}

//////////////////////////////////////////

size_t prpc_build_ok( char *buf, const size_t max_len, const size_t id )
{
    //snprintf( buf, max_len, "%lu:ok", id );
    return prpc_build_msg( buf, max_len, id, "ok", 0 );
}

size_t prpc_build_error( char *buf, const size_t max_len, const size_t id, const char *err )
{
    //snprintf( buf, max_len, "%lu:error \"%s\"", id, err );
    return prpc_build_msg( buf, max_len, id, "error", 1, PRPC_STRING, err );
}

size_t prpc_build_error_status( char *buf, const size_t max_len, const size_t id, const PRPC_Status_t err )
{
    switch( err.status ) {
        case PRPC_ERROR_UNEXCEPTED_TOKEN:
        return snprintf(buf, max_len,
            "%lu:error \"Unexcepted token for arg %lu : Excepted %s, got %s\"",
            (unsigned long)id, (unsigned long)err.token.idx,
            token_type_str( err.token.excepted ),
            token_type_str( err.token.got      )
        );
        break;
        default : return snprintf( buf, max_len, "%lu:error \"Unexcepted error\"", (unsigned long)id ); break;
    }
}

size_t prpc_build_result( char *buf, const size_t max_len, const size_t id, const size_t nvals, ... )
{
    va_list args;
    va_start( args, nvals );
    size_t written = _prpc_build_msg_va( buf, max_len, id, "result", nvals, args );
    va_end( args );

    return written;
}

size_t prpc_build_result_boolean( char *buf, const size_t max_len, const size_t id, const uint8_t val )
{
    return prpc_build_result( buf, max_len, id, 1, PRPC_BOOLEAN, val );
}

size_t prpc_build_result_int( char *buf, const size_t max_len, const size_t id, const int intg )
{
    return prpc_build_result( buf, max_len, id, 1, PRPC_INT, intg );
}
