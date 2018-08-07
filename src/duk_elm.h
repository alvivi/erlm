#pragma once

#include "duktape.h"

duk_context *duk_elm_create_context(int events_manager);
int duk_elm_get_input_ports(duk_context *ctx, const char *ports[],
                            int port_count);
int duk_elm_get_output_ports(duk_context *ctx, const char *ports[],
                             int port_count);
int duk_elm_is_input_port(duk_context *ctx);
int duk_elm_is_output_port(duk_context *ctx);
int duk_elm_peval_file(duk_context *ctx, const char *filepath);
