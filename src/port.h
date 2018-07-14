
#pragma once

typedef unsigned char byte;

int erlm_packet2_write(int fd, byte *buffer, int length);
int erlm_unbuffered_read(int fd, byte *buffer, int length);
int erlm_unbuffered_write(int fd, byte *buffer, int length);
