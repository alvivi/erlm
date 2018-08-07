#pragma once

enum event_type { EVENT_CUSTOM, EVENT_READ, EVENT_TIMER, EVENT_WRITE };

struct event {
  enum event_type type;
  int id;
  void *data;
};

int events_create_manager();
void events_free(struct event *event);
int events_send_custom(int manager, void *data);
int events_send_timeout(int manager, int timeout, void *data);
int events_subscribe_read(int manager, int fd, void *data);
int events_subscribe_write(int manager, int fd, void *data);
int events_wait(int manager, struct event **events, int max_events);
void events_unsubscribe(int manager, struct event *event);
