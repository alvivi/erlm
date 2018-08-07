#pragma once

#include "duk_config.h"
#include "duktape.h"
#include "events.h"

void duk_timers_handle_event(duk_context *ctx, struct event *event);
void duk_timers_register(duk_context *ctx, int events_manager);
