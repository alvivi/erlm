#include "duk_erlang.h"

ETERM *duk_erlang_get_term(duk_context *ctx, duk_idx_t idx) {
  switch (duk_get_type(ctx, idx)) {
  case DUK_TYPE_UNDEFINED:
    return erl_mk_atom("undefined");
  case DUK_TYPE_NULL:
    return erl_mk_atom("null");
  case DUK_TYPE_BOOLEAN:
    if (duk_get_boolean(ctx, idx)) {
      return erl_mk_atom("true");
    }
    return erl_mk_atom("false");
  case DUK_TYPE_NUMBER:
    return erl_mk_float(duk_get_number(ctx, idx));
  case DUK_TYPE_STRING:
    return erl_mk_string(duk_get_string(ctx, idx));
  case DUK_TYPE_OBJECT:
    if (duk_is_array(ctx, idx)) {
      int array_index = 0;
      duk_size_t array_length = duk_get_length(ctx, idx);
      ETERM **buffer = malloc(sizeof(ETERM *) * array_length);
      ETERM *list = erl_mk_empty_list();
      duk_enum(ctx, idx, DUK_ENUM_ARRAY_INDICES_ONLY);
      while (duk_next(ctx, -1, 1)) {
        buffer[array_index] = duk_erlang_get_term(ctx, -1);
        duk_pop_2(ctx);
        array_index += 1;
      }
      duk_pop(ctx);
      for (array_index = array_length - 1; array_index >= 0; array_index--) {
        list = erl_cons(buffer[array_index], list);
      }
      free(buffer);
      return list;
    } else {
      ETERM *keyword_list = erl_mk_empty_list();
      ETERM *tuple_values[2];
      duk_enum(ctx, idx, DUK_ENUM_OWN_PROPERTIES_ONLY);
      while (duk_next(ctx, -1, 1)) {
        tuple_values[0] = erl_mk_string(duk_get_string(ctx, -2));
        tuple_values[1] = duk_erlang_get_term(ctx, -1);
        keyword_list = erl_cons(erl_mk_tuple(tuple_values, 2), keyword_list);
        duk_pop_2(ctx);
      }
      duk_pop(ctx);
      return keyword_list;
    }
  default:
    return erl_mk_atom("undefined");
  }
}

void duk_erlang_push_term(duk_context *ctx, ETERM *term) {
  duk_idx_t array_index;
  int list_length;
  if (ERL_IS_INTEGER(term)) {
    duk_push_int(ctx, ERL_INT_VALUE(term));
    return;
  }
  if (ERL_IS_UNSIGNED_INTEGER(term)) {
    duk_push_int(ctx, ERL_INT_VALUE(term));
    return;
  }
  if (ERL_IS_FLOAT(term)) {
    duk_push_number(ctx, ERL_FLOAT_VALUE(term));
    return;
  }
  if (ERL_IS_LIST(term)) {
    list_length = erl_length(term);
    array_index = duk_push_array(ctx);
    for (int i = 0; i < list_length; i++) {
      duk_erlang_push_term(ctx, erl_hd(term));
      duk_put_prop_index(ctx, array_index, i);
      term = erl_tl(term);
    }
    return;
  }

  fprintf(stderr, "error: unsupported term type\n");
  abort();
}
