#include <stdarg.h>
#include <memory.h>

#include "cmds.h"

#include "types.h"
#include "msg.h"
#include "lex.h"

static PRPC_Process_CMD_Callback_t            process_cmd = NULL;
static PRPC_Process_NOTIFICATION_Callback_t process_notif = NULL;

size_t prpc_process_line( const char *line, char *resp_buf, const size_t max_resp_len )
{
    const char *ptr  = line;
    Token_t tk;
	size_t written = 0;

    memset( resp_buf, 0, max_resp_len );

    do {
        token_next( &ptr, &tk );
        switch( tk.type ) {
            default:break; // TODO // Throw error
            case TOKEN_EOL:case TOKEN_EOF:break; // End of stream

            case TOKEN_COMMAND:
            if( tk.data.cmd.id == PRPC_ID_NOTIFY ) written += prpc_process_notification(
                &resp_buf[written], max_resp_len-written,
                tk.data.cmd.name_begin, tk.data.cmd.name_end,
                &ptr
            );

            else written += prpc_process_cmd(
                &resp_buf[written], max_resp_len-written,
                tk.data.cmd.id, tk.data.cmd.name_begin, tk.data.cmd.name_end,
                &ptr
            );
            break;
        }
    } while( (tk.type != TOKEN_ERROR) && (tk.type != TOKEN_EOL) && (tk.type != TOKEN_EOF) );

	return written;
}

size_t prpc_process_cmd( char *resp_buf, const size_t max_resp_len, const PRPC_ID_t id, const char *cmd_name_begin, const char *cmd_name_end, const char **args_ptr )
{
    if( process_cmd ) return process_cmd( resp_buf, max_resp_len, id, cmd_name_begin, cmd_name_end, args_ptr );
    else              return prpc_build_error( resp_buf, max_resp_len, id, "Not supported" );
}

size_t prpc_process_notification( char *resp_buf, const size_t max_resp_len, const char *notify_name_begin, const char *notify_name_end, const char **args_ptr )
{
    if( process_notif ) return process_notif( resp_buf, max_resp_len, notify_name_begin, notify_name_end, args_ptr );
	else                return 0;
}

void prpc_process_callback_register( PRPC_Process_CMD_Callback_t for_cmd, PRPC_Process_NOTIFICATION_Callback_t for_notifs )
{
    process_cmd   = for_cmd;
    process_notif = for_notifs;
}

//////////////////////////////////////////
// Parse arguments
//////////////////////////////////////////
PRPC_Status_t prpc_cmd_parse_args( const char **ptr, const size_t id, const size_t n_args, ... )
{
    PRPC_Status_t ret = { .status = PRPC_OK };
    int tt; // Excepted token type
    Token_t tk;      // Parsed token

    va_list args;

    va_start( args, n_args );
    size_t i;
    for( i = 0 ; (i < n_args) && (ret.status == PRPC_OK) ; i++ ) {
        tt = va_arg( args, int );
        
        ret = token_next_arg( ptr, &tk, tt );
        if( ret.status == PRPC_OK ) {
            if( (tt == TOKEN_STRING) || (tt == TOKEN_IDENTIFIER) ) {
                const char **begin, **end;
                begin = va_arg( args, const char** );
                end   = va_arg( args, const char** );

                (*begin) = tk.begin;
                (*end)   = tk.end;
            }
            
            else if( tt == TOKEN_INT ) {
                int *intg = va_arg( args, int* );
                (*intg) = tk.data.intg;
            }

            else if( tt == TOKEN_FLOAT ) {
                float *ff = va_arg( args, float* );
                (*ff) = tk.data.num;
            }

            else if( tt == TOKEN_BOOLEAN ) {
                uint8_t *bb = va_arg( args, uint8_t* );
                (*bb) = tk.data.boolean;
            }

            else { // Should not go here
                ret.status = PRPC_ERROR_UNKNOWN;
            }
        }
    }
    va_end( args );

    return ret;
}
