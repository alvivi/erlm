#pragma once

#include "duktape.h"

int duk_util_unroll_array(duk_context *ctx);
void duk_util_put_first_prop_name(duk_context *ctx);
