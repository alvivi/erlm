#include "acutest.h"
#include "io.h"
#include <stdlib.h>
#include <string.h>

void test_io_read(void) {
  char filename[] = "/tmp/erlmXXXXXX";
  const char *src = "To The Moon";
  char trg[0xFF];
  int fd, count;

  fd = mkstemp(filename);
  write(fd, src, strlen(src) + 1);
  lseek(fd, 0, SEEK_SET);
  count = io_read(fd, (void *)trg, strlen(src) + 1);
  close(fd);

  TEST_CHECK(strcmp(src, trg) == 0);
  TEST_CHECK(count == strlen(src) + 1);
}

void test_io_read_packed2(void) {
  char filename[] = "/tmp/erlmXXXXXX";
  const char *src = "To The Moon";
  char trg[0xFF];
  int fd, count;

  fd = mkstemp(filename);
  trg[0] = 0;
  write(fd, trg, 1);
  trg[0] = strlen(src) + 1;
  write(fd, trg, 1);
  write(fd, src, strlen(src) + 1);
  lseek(fd, 0, SEEK_SET);
  count = io_read_packed2(fd, (void *)trg);
  close(fd);

  TEST_CHECK(strcmp(src, trg) == 0);
  TEST_CHECK(count == strlen(src) + 1);
}

void test_io_read_sliced(void) {
  char filename[] = "/tmp/erlmXXXXXX";
  const char *src = "To The Moon";
  char trg[0xFF];
  int fd, count;

  fd = mkstemp(filename);
  write(fd, src, strlen(src) + 1);
  lseek(fd, 0, SEEK_SET);
  count = io_read(fd, (void *)trg, 6);
  close(fd);

  trg[6] = '\0';
  TEST_CHECK(strcmp("To The", trg) == 0);
  TEST_CHECK(count == 6);
}

void test_io_write(void) {
  char filename[] = "/tmp/erlmXXXXXX";
  const char *src = "To The Moon";
  char trg[0xFF];
  int fd, count;

  fd = mkstemp(filename);
  count = io_write(fd, (void *)src, strlen(src) + 1);
  lseek(fd, 0, SEEK_SET);
  read(fd, trg, 0xFF);
  close(fd);

  TEST_CHECK(strcmp(src, trg) == 0);
  TEST_CHECK(count == strlen(src) + 1);
}

void test_io_write_sliced(void) {
  char filename[] = "/tmp/erlmXXXXXX";
  const char *src = "To The Moon";
  char trg[0xFF];
  int fd, count;

  fd = mkstemp(filename);
  count = io_write(fd, (void *)src, 6);
  lseek(fd, 0, SEEK_SET);
  read(fd, trg, 0xFF);
  close(fd);

  trg[6] = '\0';
  TEST_CHECK(strcmp("To The", trg) == 0);
  TEST_CHECK(count == 6);
}

void test_io_write_packed2(void) {
  char filename[] = "/tmp/erlmXXXXXX";
  const char *src = "To The Moon";
  char trg[0xFF];
  int fd, count;

  fd = mkstemp(filename);
  count = io_write_packed2(fd, (void *)src, strlen(src) + 1);
  lseek(fd, 0, SEEK_SET);
  read(fd, trg, 0xFF);
  close(fd);

  TEST_CHECK(trg[0] == 0);
  TEST_CHECK(trg[1] == strlen(src) + 1);
  TEST_CHECK(strcmp(src, &trg[2]) == 0);
  TEST_CHECK(count == strlen(src) + 1);
}

TEST_LIST = {{"io_read", test_io_read},
             {"io_read sliced", test_io_read_sliced},
             {"io_read_packed2", test_io_read_packed2},
             {"io_write", test_io_write},
             {"io_write sliced", test_io_write_sliced},
             {"io_write_packed2", test_io_write_packed2},
             {NULL, NULL}};
