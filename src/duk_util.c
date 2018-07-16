
#include "duk_util.h"

int duk_util_unroll_array(duk_context *ctx) {
  int enum_index, count = 0;
  if (!duk_is_array(ctx, -1)) {
    return 0;
  }
  duk_enum(ctx, -1, DUK_ENUM_ARRAY_INDICES_ONLY);
  enum_index = duk_normalize_index(ctx, -1);
  while (duk_next(ctx, enum_index, 1)) {
    duk_remove(ctx, -2);
    count += 1;
  }
  duk_remove(ctx, enum_index);
  duk_remove(ctx, enum_index - 1);
  return count;
}
