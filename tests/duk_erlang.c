#include "acutest.h"
#include "duk_erlang.h"
#include <string.h>

void test_get_undefined(void) {
  duk_context *ctx;
  ETERM *term;

  erl_init(NULL, 0);
  ctx = duk_create_heap_default();

  duk_push_undefined(ctx);
  term = duk_erlang_get_term(ctx, -1);
  TEST_CHECK(ERL_IS_ATOM(term));
  TEST_CHECK(strcmp(ERL_ATOM_PTR(term), "undefined") == 0);

  duk_destroy_heap(ctx);
  erl_free_term(term);
}

void test_get_null(void) {
  duk_context *ctx;
  ETERM *term;

  erl_init(NULL, 0);
  ctx = duk_create_heap_default();

  duk_push_null(ctx);
  term = duk_erlang_get_term(ctx, -1);
  TEST_CHECK(ERL_IS_ATOM(term));
  TEST_CHECK(strcmp(ERL_ATOM_PTR(term), "null") == 0);

  duk_destroy_heap(ctx);
  erl_free_term(term);
}

void test_get_boolean(void) {
  duk_context *ctx;
  ETERM *term;

  erl_init(NULL, 0);
  ctx = duk_create_heap_default();

  duk_push_true(ctx);
  term = duk_erlang_get_term(ctx, -1);
  TEST_CHECK(ERL_IS_ATOM(term));
  TEST_CHECK(strcmp(ERL_ATOM_PTR(term), "true") == 0);

  duk_push_false(ctx);
  term = duk_erlang_get_term(ctx, -1);
  TEST_CHECK(ERL_IS_ATOM(term));
  TEST_CHECK(strcmp(ERL_ATOM_PTR(term), "false") == 0);

  duk_destroy_heap(ctx);
  erl_free_term(term);
}

void test_get_integer(void) {
  duk_context *ctx;
  ETERM *term;

  erl_init(NULL, 0);
  ctx = duk_create_heap_default();

  duk_push_int(ctx, 9001);
  term = duk_erlang_get_term(ctx, -1);
  TEST_CHECK(ERL_IS_FLOAT(term));
  TEST_CHECK(ERL_FLOAT_VALUE(term) == 9001);

  duk_destroy_heap(ctx);
  erl_free_term(term);
}

void test_get_float(void) {
  duk_context *ctx;
  ETERM *term;

  erl_init(NULL, 0);
  ctx = duk_create_heap_default();

  duk_push_number(ctx, 1.618);
  term = duk_erlang_get_term(ctx, -1);
  TEST_CHECK(ERL_IS_FLOAT(term));
  TEST_CHECK(ERL_FLOAT_VALUE(term) == 1.618);

  duk_destroy_heap(ctx);
  erl_free_term(term);
}

void test_get_string(void) {
  duk_context *ctx;
  ETERM *term;

  erl_init(NULL, 0);
  ctx = duk_create_heap_default();

  duk_push_string(ctx, "foo");
  term = duk_erlang_get_term(ctx, -1);

  TEST_CHECK(ERL_IS_CONS(term));
  TEST_CHECK(ERL_IS_INTEGER(erl_hd(term)));

  TEST_CHECK(strcmp(erl_iolist_to_string(term), "foo") == 0);

  duk_destroy_heap(ctx);
  erl_free_term(term);
}

void test_get_empty_array(void) {
  duk_context *ctx;
  ETERM *term;

  erl_init(NULL, 0);
  ctx = duk_create_heap_default();

  duk_push_array(ctx);
  term = duk_erlang_get_term(ctx, -1);

  TEST_CHECK(ERL_IS_EMPTY_LIST(term));

  duk_destroy_heap(ctx);
  erl_free_term(term);
}

void test_get_array(void) {
  ETERM *term;
  duk_context *ctx;
  duk_idx_t arr_idx;

  erl_init(NULL, 0);
  ctx = duk_create_heap_default();

  arr_idx = duk_push_array(ctx);
  duk_push_string(ctx, "zero");
  duk_put_prop_index(ctx, arr_idx, 0);
  duk_push_string(ctx, "one");
  duk_put_prop_index(ctx, arr_idx, 1);
  duk_push_string(ctx, "two");
  duk_put_prop_index(ctx, arr_idx, 2);

  term = duk_erlang_get_term(ctx, -1);

  TEST_CHECK(ERL_IS_LIST(term));
  TEST_CHECK(erl_length(term) == 3);
  TEST_CHECK(strcmp(erl_iolist_to_string(erl_hd(term)), "zero") == 0);
  TEST_CHECK(strcmp(erl_iolist_to_string(erl_hd(erl_tl(term))), "one") == 0);
  TEST_CHECK(
      strcmp(erl_iolist_to_string(erl_hd(erl_tl(erl_tl(term)))), "two") == 0);

  duk_destroy_heap(ctx);
  erl_free_term(term);
}

void test_get_empty_object(void) {
  duk_context *ctx;
  ETERM *term;

  erl_init(NULL, 0);
  ctx = duk_create_heap_default();

  duk_push_object(ctx);
  term = duk_erlang_get_term(ctx, -1);

  TEST_CHECK(ERL_IS_EMPTY_LIST(term));

  duk_destroy_heap(ctx);
  erl_free_term(term);
}

void test_get_object(void) {
  duk_context *ctx;
  ETERM *term;
  char buffer[256];
  FILE *file;

  erl_init(NULL, 0);
  ctx = duk_create_heap_default();

  duk_push_object(ctx);
  duk_push_string(ctx, "bar");
  duk_put_prop_string(ctx, -2, "foo");
  term = duk_erlang_get_term(ctx, -1);
  duk_push_string(ctx, "to the moon");
  duk_put_prop_string(ctx, -2, "wow");
  term = duk_erlang_get_term(ctx, -1);

  TEST_CHECK(erl_length(term) == 2);
  TEST_CHECK(ERL_IS_TUPLE(erl_hd(term)));

  TEST_CHECK(
      strcmp(erl_iolist_to_string(erl_element(1, (erl_hd(term)))), "wow") == 0);
  TEST_CHECK(strcmp(erl_iolist_to_string(erl_element(2, (erl_hd(term)))),
                    "to the moon") == 0);
  TEST_CHECK(
      strcmp(erl_iolist_to_string(erl_element(1, (erl_hd(erl_tl(term))))),
             "foo") == 0);
  TEST_CHECK(
      strcmp(erl_iolist_to_string(erl_element(2, (erl_hd(erl_tl(term))))),
             "bar") == 0);

  duk_destroy_heap(ctx);
  erl_free_term(term);
}

TEST_LIST = {{"get an undefined", test_get_undefined},
             {"get a null", test_get_null},
             {"get a boolean", test_get_boolean},
             {"get an integer", test_get_integer},
             {"get a float", test_get_float},
             {"get a string", test_get_string},
             {"get an empty array", test_get_empty_array},
             {"get an array", test_get_array},
             {"get an empty object", test_get_empty_object},
             {"get an object", test_get_object},
             {NULL, NULL}};
