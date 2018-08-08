#include "events.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef __linux__

#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <time.h>

int events_create_manager() { return epoll_create1(0); }

void events_free(struct event *event) {
  if (event->type == EVENT_CUSTOM || event->type == EVENT_TIMER) {
    close(event->id);
  };
  free(event);
}

int events_send_raw_custom(int manager, struct event *event) {
  int fd;
  uint64_t buffer;
  struct epoll_event native_event;

  fd = eventfd(0, EFD_NONBLOCK);
  if (fd == -1) {
    return -1;
  }

  event->id = fd;
  native_event.data.ptr = event;
  native_event.events = EPOLLIN | EPOLLONESHOT;
  if (epoll_ctl(manager, EPOLL_CTL_ADD, fd, &native_event) == -1) {
    close(fd);
    free(event);
    return -1;
  }

  buffer = 1;
  buffer = write(fd, &buffer, sizeof(uint64_t));

  return fd;
}

int events_send_custom(int manager, void *data) {
  int result;
  struct event *event;

  event = malloc(sizeof(struct event));
  event->type = EVENT_CUSTOM;
  event->data = data;

  result = events_send_raw_custom(manager, event);
  if (result < 0) {
    free(event);
  }
  return result;
}

int events_send_timeout(int manager, int timeout, void *data) {
  int fd, result;
  struct itimerspec ts;
  struct epoll_event native_event;
  struct event *event;

  event = malloc(sizeof(struct event));
  event->type = EVENT_TIMER;
  event->data = data;

  if (timeout <= 0) {
    result = events_send_raw_custom(manager, event);
    if (result < 0) {
      free(event);
    }
    return result;
  }

  ts.it_interval.tv_sec = 0;
  ts.it_interval.tv_nsec = 0;
  ts.it_value.tv_sec = timeout / 1000;
  ts.it_value.tv_nsec = (timeout % 1000) * 1000000;

  fd = timerfd_create(_POSIX_MONOTONIC_CLOCK, 0);
  if (fd == -1) {
    return -1;
  }

  if (timerfd_settime(fd, 0, &ts, NULL) < 0) {
    close(fd);
    return -1;
  }

  event->id = fd;

  native_event.data.ptr = event;
  native_event.events = EPOLLIN | EPOLLONESHOT;
  if (epoll_ctl(manager, EPOLL_CTL_ADD, fd, &native_event) == -1) {
    close(fd);
    free(event);
    return -1;
  }

  return fd;
}

int events_subscribe_read(int manager, int fd, void *data) {
  struct epoll_event native_event;
  struct event *event;

  event = malloc(sizeof(struct event));
  event->type = EVENT_READ;
  event->id = fd;
  event->data = data;

  native_event.data.ptr = event;
  native_event.events = EPOLLIN;

  if (epoll_ctl(manager, EPOLL_CTL_ADD, fd, &native_event) == -1) {
    free(event);
    return -1;
  }

  return fd;
}

int events_subscribe_write(int manager, int fd, void *data) {
  struct epoll_event native_event;
  struct event *event;

  event = malloc(sizeof(struct event));
  event->type = EVENT_WRITE;
  event->id = fd;
  event->data = data;

  native_event.data.ptr = event;
  native_event.events = EPOLLOUT;

  if (epoll_ctl(manager, EPOLL_CTL_ADD, fd, &native_event) == -1) {
    free(event);
    return -1;
  }

  return fd;
}

void events_unsubscribe(int manager, struct event *event) {
  epoll_ctl(manager, EPOLL_CTL_DEL, event->id, NULL);
}

int events_wait(int manager, struct event **events, int max_events) {
  int event_count;
  struct epoll_event *native_events;

  native_events = malloc(max_events * sizeof(struct epoll_event));

  event_count = epoll_wait(manager, native_events, max_events, -1);
  if (event_count < 0) {
    free(native_events);
    return event_count;
  }

  for (int i = 0; i < event_count; i++) {
    events[i] = native_events[i].data.ptr;
  }

  free(native_events);
  return event_count;
}

#endif

#if defined(__FreeBSD__) || defined(__APPLE__)

#include <sys/event.h>

static int events_next_id = 0;

int events_create_manager() { return kqueue(); }

void events_free(struct event *event) {
  if (event->type == EVENT_READ) {
    close(event->id);
  }
  free(event);
}

int events_send_custom(int manager, void *data) {
  struct event *event;
  struct kevent native_event;

  events_next_id += 1;
  event = malloc(sizeof(struct event));
  event->type = EVENT_CUSTOM;
  event->id = events_next_id;
  event->data = data;

  EV_SET(&native_event, events_next_id, EVFILT_USER, EV_ADD | EV_ONESHOT,
         NOTE_FFCOPY, 0, event);
  kevent(manager, &native_event, 1, NULL, 0, NULL);
  EV_SET(&native_event, events_next_id, EVFILT_USER, EV_ENABLE,
         NOTE_FFCOPY | NOTE_TRIGGER, 0, event);
  kevent(manager, &native_event, 1, NULL, 0, NULL);
  return events_next_id;
}

int events_send_timeout(int manager, int timeout, void *data) {
  struct event *event;
  struct kevent native_event;

  events_next_id += 1;
  event = malloc(sizeof(struct event));
  event->type = EVENT_TIMER;
  event->id = events_next_id;
  event->data = data;

  EV_SET(&native_event, events_next_id, EVFILT_TIMER, EV_ADD | EV_ONESHOT, 0,
         timeout, event);
  kevent(manager, &native_event, 1, NULL, 0, NULL);
  return events_next_id;
}

int events_subscribe_read(int manager, int fd, void *data) {
  struct kevent native_event;
  struct event *event;

  event = malloc(sizeof(struct event));
  event->type = EVENT_READ;
  event->id = fd;
  event->data = data;

  EV_SET(&native_event, fd, EVFILT_READ, EV_ADD, 0, 0, event);
  kevent(manager, &native_event, 1, NULL, 0, NULL);

  return fd;
}

int events_subscribe_write(int manager, int fd, void *data) {
  struct kevent native_event;
  struct event *event;

  event = malloc(sizeof(struct event));
  event->type = EVENT_WRITE;
  event->id = fd;
  event->data = data;

  EV_SET(&native_event, fd, EVFILT_WRITE, EV_ADD, 0, 0, event);
  kevent(manager, &native_event, 1, NULL, 0, NULL);

  return fd;
}

void events_unsubscribe(int manager, struct event *event) {
  struct kevent native_event;
  if (event->type == EVENT_READ) {
    EV_SET(&native_event, event->id, EVFILT_READ, EV_DELETE, 0, 0, event);
    kevent(manager, &native_event, 1, NULL, 0, NULL);
  } else if (event->type == EVENT_WRITE) {
    EV_SET(&native_event, event->id, EVFILT_WRITE, EV_DELETE, 0, 0, event);
    kevent(manager, &native_event, 1, NULL, 0, NULL);
  }
}

int events_wait(int manager, struct event **events, int max_events) {
  int event_count;
  struct kevent *native_events;

  native_events = malloc(max_events * sizeof(struct kevent));

  event_count = kevent(manager, NULL, 0, native_events, max_events, NULL);

  for (int i = 0; i < event_count; i++) {
    events[i] = native_events[i].udata;
  }

  free(native_events);
  return event_count;
}

#endif
