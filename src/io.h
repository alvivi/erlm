#pragma once

int io_read(int fd, void *buffer, int length);
int io_read_packet2(int fd, void *buffer);
int io_write(int fd, void *buffer, int length);
int io_write_packet2(int fd, void *buffer, int length);
