/*************************
 * Dumb easy lexer thing *
 *************************/
// Florian Dupeyron
// July 2019

#pragma once

#include <stdlib.h>
#include <stdint.h>

#include "types.h"

//////////////////////////////////////////

const char *token_type_str( Token_Type_t type );
void        token_next( const char **ptr, Token_t *dst );

PRPC_Status_t token_next_except( const char **ptr, Token_t *dst, const Token_Type_t type );
PRPC_Status_t token_next_arg   ( const char **ptr, Token_t *dst, const Token_Type_t type );
