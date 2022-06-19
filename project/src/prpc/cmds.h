/*****************************
 * Test command parser thing *
 *****************************/
// Florian Dupeyron
// July 2019

#pragma once

#include "types.h"

size_t prpc_process_line( const char *line, char *resp_buf, const size_t max_resp_len );

typedef size_t (*PRPC_Process_CMD_Callback_t)(
    char *resp_buf, const size_t max_resp_len,
    const PRPC_ID_t id, const char *cmd_name_begin, const char *cmd_name_end,
    const char **args_ptr
);

typedef size_t (*PRPC_Process_NOTIFICATION_Callback_t)(
    char *resp_buf, const size_t max_resp_len,
    const char *notify_name_begin, const char *notify_name_end,
    const char **args_ptr
);

size_t prpc_process_cmd(
    char *resp_buf, const size_t max_resp_len,
    const PRPC_ID_t id, const char *cmd_name_begin, const char *cmd_name_end,
    const char **args_ptr
);

size_t prpc_process_notification(
    char *resp_buf, const size_t max_resp_len,
    const char *notify_name_begin, const char *notify_name_end,
    const char **args_ptr
);

void prpc_process_callback_register( PRPC_Process_CMD_Callback_t for_cmd, PRPC_Process_NOTIFICATION_Callback_t for_notifs );

PRPC_Status_t prpc_cmd_parse_args(
    const char **ptr,
    const size_t id,
    const size_t n_args,
    ...
);
