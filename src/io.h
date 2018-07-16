#pragma once

typedef unsigned char byte;

int io_read(int fd, byte *buffer, int length);
int io_read_packet2(int fd, byte *buffer);
int io_write(int fd, byte *buffer, int length);
int io_write_packet2(int fd, byte *buffer, int length);
