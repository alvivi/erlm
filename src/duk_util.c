
#include "duk_util.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

int duk_util_peval_commonjs_file(duk_context *ctx, const char *filepath) {
  int result;

  duk_push_object(ctx);
  duk_put_global_string(ctx, "module");

  result = duk_util_peval_file(ctx, filepath);
  if (result != 0) {
    return -1;
  }

  duk_pop(ctx);
  duk_get_global_string(ctx, "module");
  duk_get_prop_string(ctx, -1, "exports");
  duk_insert(ctx, -2);
  duk_pop(ctx);
  return 0;
}

int duk_util_peval_file(duk_context *ctx, const char *filepath) {
  int fid, file_size, result;
  const char *file_contents;

  fid = open(filepath, O_RDONLY);
  if (fid < 0) {
    return -1;
  }

  file_size = lseek(fid, 0, SEEK_END);
  if (file_size < 0) {
    close(fid);
    return -1;
  }

  file_contents = mmap(0, file_size, PROT_READ, MAP_PRIVATE, fid, 0);
  if (file_contents == MAP_FAILED) {
    close(fid);
    return -1;
  }

  result = duk_peval_string(ctx, file_contents);

  munmap((void *)file_contents, file_size);
  close(fid);
  return result;
}

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

void duk_util_put_first_prop_name(duk_context *ctx) {
  duk_enum(ctx, -1, DUK_ENUM_OWN_PROPERTIES_ONLY);
  duk_next(ctx, -1, 0);
  duk_insert(ctx, -2);
  duk_pop(ctx);
}
