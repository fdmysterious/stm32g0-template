#include "cmds.h"

#include <prpc/lex.h>
#include <prpc/msg.h>

/* ┌────────────────────────────────────────┐
   │ Generic commands                       │
   └────────────────────────────────────────┘ */

PRPC_Parse_Function_t prpc_cmd_parser_get( const char **ptr, const char *end );
size_t prpc_cmd_has(const char **ptr, char *resp_buf, const size_t max_resp_len, PRPC_ID_t id)
{
    const char *name_begin, *name_end;
    PRPC_Status_t stat = prpc_cmd_parse_args( ptr, id, 1, TOKEN_IDENTIFIER, &name_begin, &name_end );
    if( stat.status == PRPC_OK ) {
        PRPC_Parse_Function_t cmd = prpc_cmd_parser_get(&name_begin, name_end);
        return prpc_build_result_boolean( resp_buf, max_resp_len, id, cmd != NULL );
    }
    else return prpc_build_error_status( resp_buf, max_resp_len, id, stat );
}

size_t prpc_cmd_hello(const char **ptr, char *resp_buf, const size_t max_resp_len, PRPC_ID_t id)
{
    return prpc_build_ok( resp_buf, max_resp_len, id );
}

/* ┌────────────────────────────────────────┐
   │ Function name parser                   │
   └────────────────────────────────────────┘ */

PRPC_Parse_Function_t prpc_cmd_parser_get( const char **ptr, const char *end )
{
    const char *YYMARKER;

    /*!re2c
        re2c:define:YYCTYPE  = char;
        re2c:define:YYCURSOR = (*ptr);
        re2c:define:YYLIMIT  = end;
        re2c:yyfill:enable   = 0;

        end = [ \t\r\n] | '\x00';

        *                          { return NULL;                         }
		'has'                  end { return prpc_cmd_has;                 }
        'hello'                end { return prpc_cmd_hello;               }
     */
}


/* ┌────────────────────────────────────────┐
   │ Process command                        │
   └────────────────────────────────────────┘ */

size_t process_cmd( char *resp, const size_t max_len, const PRPC_ID_t id, const char *name_start, const char *name_end, const char **ptr )
{
    PRPC_Parse_Function_t cmd = prpc_cmd_parser_get(&name_start, name_end);
    if( cmd != NULL ) {
        return cmd( ptr, resp, max_len, id );
    }

    else {
        return prpc_build_error( resp, max_len, id, "Uknown method" );
    }
}


/* ┌────────────────────────────────────────┐
   │ Init                                   │
   └────────────────────────────────────────┘ */

void cmds_init()
{
    prpc_process_callback_register( process_cmd, NULL );
}
