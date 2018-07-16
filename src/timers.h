#pragma once

#include "duk_config.h"
#include "duktape.h"
#include <sys/event.h>

void timers_register(duk_context *ctx);
void timers_handle_event(duk_context *ctx, struct kevent *event);
