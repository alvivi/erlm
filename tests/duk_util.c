#include "acutest.h"
#include "duk_util.h"
#include "duktape.h"
#include <string.h>

void test_unroll_array_not_an_array(void) {
  duk_context *ctx;
  int result;

  ctx = duk_create_heap_default();

  duk_push_undefined(ctx);
  result = duk_util_unroll_array(ctx);

  TEST_CHECK(result == 0);
  TEST_CHECK(duk_is_undefined(ctx, -1));

  duk_destroy_heap(ctx);
}

void test_unroll_array_empty(void) {
  duk_context *ctx;
  int result;

  ctx = duk_create_heap_default();

  duk_push_undefined(ctx);
  duk_push_array(ctx);
  result = duk_util_unroll_array(ctx);

  TEST_CHECK(result == 0);
  TEST_CHECK(duk_is_undefined(ctx, -1));

  duk_destroy_heap(ctx);
}

void test_unroll_array(void) {
  duk_context *ctx;
  int result;
  duk_idx_t arr_idx;

  ctx = duk_create_heap_default();

  arr_idx = duk_push_array(ctx);
  duk_push_string(ctx, "foo");
  duk_put_prop_index(ctx, arr_idx, 0);
  duk_push_string(ctx, "bar");
  duk_put_prop_index(ctx, arr_idx, 1);
  duk_push_string(ctx, "qux");
  duk_put_prop_index(ctx, arr_idx, 2);
  result = duk_util_unroll_array(ctx);

  TEST_CHECK(result == 3);
  TEST_CHECK(strcmp(duk_get_string(ctx, -1), "qux") == 0);
  TEST_CHECK(strcmp(duk_get_string(ctx, -2), "bar") == 0);
  TEST_CHECK(strcmp(duk_get_string(ctx, -3), "foo") == 0);

  duk_destroy_heap(ctx);
}

void test_put_first_prop_name(void) {
  duk_context *ctx;
  int result;

  ctx = duk_create_heap_default();

  duk_push_object(ctx);
  duk_push_string(ctx, "foo");
  duk_put_prop_string(ctx, -2, "bar");
  duk_util_put_first_prop_name(ctx);

  TEST_CHECK(duk_is_string(ctx, -1));
  TEST_CHECK(strcmp(duk_get_string(ctx, -1), "bar") == 0);

  duk_destroy_heap(ctx);
}

void test_peval_file(void) {
  char *content = "'foobar'";
  char filename[] = "/tmp/erlmXXXXXX";
  duk_context *ctx;
  int fd;

  fd = mkstemp(filename);
  write(fd, content, strlen(content) + 1);
  close(fd);
  ctx = duk_create_heap_default();
  duk_util_peval_file(ctx, filename);

  TEST_CHECK(duk_is_string(ctx, -1));
  TEST_CHECK(strcmp(duk_get_string(ctx, -1), "foobar") == 0);

  duk_destroy_heap(ctx);
}

TEST_LIST = {{"unroll not an array", test_unroll_array_not_an_array},
             {"unroll an empty arra", test_unroll_array_empty},
             {"unroll", test_unroll_array},
             {"put first prop name", test_put_first_prop_name},
             {"eval a file", test_peval_file},
             {NULL, NULL}};
