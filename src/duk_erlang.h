#pragma once

#include "duktape.h"
#include "ei.h"
#include "erl_interface.h"

ETERM *duk_erlang_get_term(duk_context *ctx, duk_idx_t idx);
void duk_erlang_push_term(duk_context *ctx, ETERM *term);
