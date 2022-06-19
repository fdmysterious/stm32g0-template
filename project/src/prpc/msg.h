/***************************
 * Build messages for PRPC *
 ***************************/
// Florian Dupeyron
// July 2019

#include "types.h"

#pragma once

size_t prpc_build_ok            ( char *buf, const size_t max_len, const size_t id);
size_t prpc_build_error         ( char *buf, const size_t max_len, const size_t id, const char *msg    );
size_t prpc_build_error_status  ( char *buf, const size_t max_len, const size_t id, const PRPC_Status_t stat );
size_t prpc_build_msg           ( char *buf, const size_t max_len, const size_t id, const char *cmd, const size_t nvals, ... );
size_t prpc_build_result        ( char *buf, const size_t max_len, const size_t id, const size_t nvals,  ... );
size_t prpc_build_result_boolean( char *buf, const size_t max_len, const size_t id, const uint8_t val  );
size_t prpc_build_result_int    ( char *buf, const size_t max_len, const size_t id, const int     intg );
