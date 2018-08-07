
#include "duk_timers.h"
#include "events.h"

static duk_ret_t duk_timers_set_timeout(duk_context *ctx) {
  int arg_count, events_manager, timeout, timer_id;

  arg_count = duk_get_top(ctx);
  if (arg_count != 2) {
    return DUK_RET_ERROR;
  }

  duk_push_global_stash(ctx);
  duk_get_prop_string(ctx, -1, "timersEventsManager");
  events_manager = duk_get_int(ctx, -1);
  duk_pop(ctx);
  timeout = duk_get_int(ctx, -2);

  timer_id = events_send_timeout(events_manager, timeout, NULL);
  duk_get_prop_string(ctx, -1, "timeoutCallbacks");
  duk_dup(ctx, -4);
  duk_put_prop_index(ctx, -2, timer_id);
  duk_pop_n(ctx, 4);

  duk_push_int(ctx, timer_id);

  return 1;
}

static duk_ret_t duk_timers_clear_timeout(duk_context *ctx) {
  int arg_count, timer_id;

  arg_count = duk_get_top(ctx);
  if (arg_count != 1) {
    return DUK_RET_ERROR;
  }

  timer_id = duk_get_int(ctx, -1);
  duk_pop(ctx);
  duk_push_global_stash(ctx);
  duk_get_prop_string(ctx, -1, "timeoutCallbacks");
  duk_del_prop_index(ctx, -1, timer_id);
  duk_pop_2(ctx);
  return 0;
}

void duk_timers_handle_event(duk_context *ctx, struct event *event) {
  if (event->type != EVENT_TIMER) {
    return;
  }

  duk_push_global_stash(ctx);
  duk_get_prop_string(ctx, -1, "timeoutCallbacks");
  duk_get_prop_index(ctx, -1, event->id);
  if (duk_is_function(ctx, -1)) {
    duk_call(ctx, 0);
    duk_pop(ctx);
    duk_del_prop_index(ctx, -1, event->id);
    duk_pop_2(ctx);
  } else {
    duk_pop_3(ctx);
  }
}

void duk_timers_register(duk_context *ctx, int events_manager) {
  duk_push_global_stash(ctx);
  duk_push_int(ctx, events_manager);
  duk_put_prop_string(ctx, -2, "timersEventsManager");
  duk_push_object(ctx);
  duk_put_prop_string(ctx, -2, "timeoutCallbacks");
  duk_pop(ctx);

  duk_push_c_function(ctx, duk_timers_set_timeout, DUK_VARARGS);
  duk_put_global_string(ctx, "setTimeout");
  duk_push_c_function(ctx, duk_timers_clear_timeout, DUK_VARARGS);
  duk_put_global_string(ctx, "clearTimeout");
}
