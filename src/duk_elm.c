#include "duk_elm.h"
#include "duk_util.h"

duk_context *duk_elm_create_context(int qid) {
  duk_context *ctx;
  ctx = duk_create_heap_default();

  duk_push_global_stash(ctx);
  duk_push_int(ctx, qid);
  duk_put_prop_string(ctx, -2, "qid");
  duk_pop(ctx);

  return ctx;
}

int duk_elm_get_input_ports(duk_context *ctx, const char *ports[],
                            int port_count) {
  int nport = 0;

  if (!duk_check_type(ctx, -1, DUK_TYPE_OBJECT)) {
    return -1;
  }
  duk_get_prop_string(ctx, -1, "ports");
  if (!duk_check_type(ctx, -1, DUK_TYPE_OBJECT)) {
    return -1;
  }
  duk_enum(ctx, -1, DUK_ENUM_OWN_PROPERTIES_ONLY);
  while (duk_next(ctx, -1, 1)) {
    if (nport >= port_count) {
      duk_pop_2(ctx);
      break;
    }
    if (!duk_elm_is_input_port(ctx)) {
      duk_pop_2(ctx);
      continue;
    }
    ports[nport] = duk_to_string(ctx, -2);
    nport += 1;
    duk_pop_2(ctx);
  }
  duk_pop_2(ctx);
  return nport;
}

int duk_elm_get_output_ports(duk_context *ctx, const char *ports[],
                             int port_count) {
  int nport = 0;

  if (!duk_check_type(ctx, -1, DUK_TYPE_OBJECT)) {
    return -1;
  }
  duk_get_prop_string(ctx, -1, "ports");
  if (!duk_check_type(ctx, -1, DUK_TYPE_OBJECT)) {
    return -1;
  }
  duk_enum(ctx, -1, DUK_ENUM_OWN_PROPERTIES_ONLY);
  while (duk_next(ctx, -1, 1)) {
    if (nport >= port_count) {
      break;
    }
    if (!duk_elm_is_output_port(ctx)) {
      duk_pop_2(ctx);
      continue;
    }
    ports[nport] = duk_to_string(ctx, -2);
    nport += 1;
    duk_pop_2(ctx);
  }
  duk_pop_2(ctx);
  return nport;
}

int duk_elm_is_input_port(duk_context *ctx) {
  if (!duk_check_type(ctx, -1, DUK_TYPE_OBJECT)) {
    return 0;
  }
  return duk_has_prop_string(ctx, -1, "send");
}

int duk_elm_is_output_port(duk_context *ctx) {
  if (!duk_check_type(ctx, -1, DUK_TYPE_OBJECT)) {
    return 0;
  }
  return duk_has_prop_string(ctx, -1, "subscribe") &&
         duk_has_prop_string(ctx, -1, "unsubscribe");
}

int duk_elm_peval_file(duk_context *ctx, const char *filepath) {
  int result;

  result = duk_util_peval_commonjs_file(ctx, filepath);
  if (result != 0) {
    return result;
  }

  duk_util_put_first_prop_name(ctx);
  duk_get_prop(ctx, -2);
  duk_push_string(ctx, "worker");
  result = duk_pcall_prop(ctx, -2, 0);
  duk_insert(ctx, -3);
  duk_pop(ctx);
  duk_pop(ctx);
  return result;
}
