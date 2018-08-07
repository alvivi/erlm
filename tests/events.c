#include "acutest.h"
#include "events.h"
#include "time.h"

void test_custom_event(void) {
  int manager;
  struct event *event;

  manager = events_create_manager();
  events_send_custom(manager, (void *)9001);
  events_wait(manager, &event, 1);
  TEST_CHECK(event->type == EVENT_CUSTOM);
  TEST_CHECK(event->data == (void *)9001);
  events_free(event);
}

void test_timeout_event(void) {
  int manager;
  int timestamp;
  struct event *event;

  manager = events_create_manager();
  events_send_timeout(manager, 1000, (void *)9001);
  timestamp = (int)time(NULL);
  events_wait(manager, &event, 1);
  TEST_CHECK(event->type == EVENT_TIMER);
  TEST_CHECK(event->data == (void *)9001);
  TEST_CHECK((int)time(NULL) >= timestamp + 1);
  events_free(event);
}

void test_read_event(void) {
  int fd[2], manager;
  const char *src = "To The Moon\n", buffer[256];
  struct event *event;

  pipe(fd);
  write(fd[1], src, strlen(src) + 1);
  close(fd[1]);
  manager = events_create_manager();
  events_subscribe_read(manager, fd[0], (void *)9001);
  events_wait(manager, &event, 1);
  read(fd[0], (void *)buffer, 256);
  TEST_CHECK(event->type == EVENT_READ);
  TEST_CHECK(event->data == (void *)9001);
  TEST_CHECK(strcmp(buffer, src) == 0);
  close(fd[0]);
  events_free(event);
}

void test_write_event(void) {
  int fd[2], manager;
  struct event *event;

  pipe(fd);
  manager = events_create_manager();
  events_subscribe_write(manager, fd[1], (void *)9001);
  events_wait(manager, &event, 1);
  TEST_CHECK(event->type == EVENT_WRITE);
  TEST_CHECK(event->data == (void *)9001);
  close(fd[0]);
  close(fd[1]);
  events_free(event);
}

TEST_LIST = {{"custom event", test_custom_event},
             {"timeout event", test_timeout_event},
             {"read event", test_read_event},
             {"write event", test_write_event},
             {NULL, NULL}};
