#pragma once

int io_read(int fd, void *buffer, int length);
int io_read_packed2(int fd, void *buffer);
int io_write(int fd, void *buffer, int length);
int io_write_packed2(int fd, void *buffer, int length);
