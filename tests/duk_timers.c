#include "acutest.h"
#include "duk_timers.h"

void test_register() {
  duk_context *ctx;
  int events_manager;

  events_manager = events_create_manager();
  ctx = duk_create_heap_default();
  duk_timers_register(ctx, events_manager);
  duk_get_global_string(ctx, "setTimeout");
  TEST_CHECK(duk_is_function(ctx, -1));
  duk_get_global_string(ctx, "clearTimeout");
  TEST_CHECK(duk_is_function(ctx, -1));

  duk_destroy_heap(ctx);
  close(events_manager);
}

void test_set_timeout() {
  duk_context *ctx;
  int events_manager, ret;
  struct event *event;

  events_manager = events_create_manager();
  ctx = duk_create_heap_default();
  duk_timers_register(ctx, events_manager);

  ret = duk_peval_string(ctx,
                         "setTimeout(function () { this.foo = 'bar';}, 200)");
  TEST_CHECK(ret == 0);

  events_wait(events_manager, &event, 1);
  duk_timers_handle_event(ctx, event);
  duk_get_global_string(ctx, "foo");
  TEST_CHECK(strcmp(duk_get_string(ctx, -1), "bar") == 0);

  duk_destroy_heap(ctx);
  close(events_manager);
}

void test_clear_timeout() {
  duk_context *ctx;
  int events_manager, ret;
  struct event *event;

  events_manager = events_create_manager();
  ctx = duk_create_heap_default();
  duk_timers_register(ctx, events_manager);

  ret = duk_peval_string(
      ctx,
      "t = setTimeout(function () { this.foo = 'bar';}, 200); clearTimeout(t)");
  TEST_CHECK(ret == 0);

  events_wait(events_manager, &event, 1);
  duk_timers_handle_event(ctx, event);
  duk_get_global_string(ctx, "foo");
  TEST_CHECK(duk_is_undefined(ctx, -1));

  duk_destroy_heap(ctx);
  close(events_manager);
}

TEST_LIST = {{"setTimeout/clearTimeout registration", test_register},
             {"setTimeout", test_set_timeout},
             {"clearTimeout", test_clear_timeout},
             {NULL, NULL}};
