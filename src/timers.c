
#include "timers.h"

static duk_ret_t timers_set_timeout(duk_context *ctx) {
  int arg_count, qid, last_id, timeout;
  struct kevent event;

  duk_push_global_stash(ctx);
  duk_get_prop_string(ctx, -1, "qid");
  qid = duk_get_int(ctx, -1);
  duk_pop(ctx);
  duk_get_prop_string(ctx, -1, "lastId");
  last_id = duk_get_int(ctx, -1);
  duk_pop(ctx);
  duk_pop(ctx);

  arg_count = duk_get_top(ctx);
  if (arg_count > 2 || arg_count <= 0) {
    return DUK_RET_ERROR;
  }
  if (arg_count == 2) {
    timeout = duk_get_int(ctx, -1);
    duk_pop(ctx);
  } else {
    timeout = 0;
  }

  duk_push_global_stash(ctx);
  duk_get_prop_string(ctx, -1, "timeoutCallbacks");
  duk_insert(ctx, -2);
  duk_pop(ctx);
  duk_insert(ctx, -2);
  duk_put_prop_index(ctx, -2, last_id + 1);
  duk_pop(ctx);

  duk_push_global_stash(ctx);
  duk_push_int(ctx, last_id + 1);
  duk_put_prop_string(ctx, -2, "lastId");
  duk_pop(ctx);

  EV_SET(&event, last_id + 1, EVFILT_TIMER, EV_ADD | EV_ENABLE, 0, timeout, 0);
  kevent(qid, &event, 1, NULL, 0, NULL);

  duk_push_int(ctx, last_id + 1);

  return 1;
}

static duk_ret_t timers_clear_timeout(duk_context *ctx) {
  int qid;

  duk_push_global_stash(ctx);
  duk_get_prop_string(ctx, -1, "qid");
  qid = duk_get_int(ctx, -1);

  printf("TODO: clearTimeout/1 %d\n", qid);
  return 0;
}

void timers_handle_event(duk_context *ctx, struct kevent *event) {
  if (event->filter != EVFILT_TIMER) {
    return;
  }

  duk_push_global_stash(ctx);
  duk_get_prop_string(ctx, -1, "timeoutCallbacks");
  duk_get_prop_index(ctx, -1, event->ident);
  duk_call(ctx, 0);

  duk_pop_2(ctx);
  duk_get_prop_string(ctx, -1, "timeoutCallbacks");
  duk_del_prop_index(ctx, -1, event->ident);
  duk_pop_2(ctx);
}

void timers_register(duk_context *ctx) {
  duk_push_global_stash(ctx);
  duk_push_int(ctx, 0);
  duk_put_prop_string(ctx, -2, "lastId");
  duk_push_object(ctx);
  duk_put_prop_string(ctx, -2, "timeoutCallbacks");
  duk_pop(ctx);

  duk_push_c_function(ctx, timers_set_timeout, DUK_VARARGS);
  duk_put_global_string(ctx, "setTimeout");
  duk_push_c_function(ctx, timers_clear_timeout, DUK_VARARGS);
  duk_put_global_string(ctx, "clearTimeout");
}
