#include "io.h"
#include <unistd.h>

int io_read(int fd, void *buffer, int length) {
  int got_or_error, got = 0;

  do {
    got_or_error = read(fd, buffer + got, length - got);
    if (got_or_error <= 0) {
      return got_or_error;
    }
    got += got_or_error;
  } while (got < length);

  return got;
}

int io_read_packed2(int fd, void *buffer) {
  size_t length;

  if (io_read(fd, buffer, 2) != 2) {
    return (-1);
  }
  length = (((char *)buffer)[0] << 8) | ((char *)buffer)[1];
  return io_read(fd, buffer, length);
}

int io_write(int fd, void *buffer, int length) {
  int wrote_or_error, wrote = 0;

  do {
    wrote_or_error = write(fd, buffer + wrote, length - wrote);
    if (wrote_or_error <= 0) {
      return wrote_or_error;
    }
    wrote += wrote_or_error;

  } while (wrote < length);

  return wrote;
}

int io_write_packed2(int fd, void *buffer, int length) {
  int tmp;

  tmp = (length >> 8) & 0xff;
  io_write(fd, &tmp, 1);

  tmp = length & 0xff;
  io_write(fd, &tmp, 1);

  return io_write(fd, buffer, length);
}
