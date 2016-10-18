/*****************************************************************************
 * Copyright (C) 2014-2015
 * file:    libgevent.h
 * author:  gozfree <gozfree@163.com>
 * created: 2015-04-27 00:59
 * updated: 2015-07-12 00:42
 *****************************************************************************/
#ifndef LIBGEVENT_H
#define LIBGEVENT_H

#include <stdint.h>
#include <stdlib.h>
#include <map>
#include <list>

enum gevent_flags {
    EVENT_TIMEOUT  = 1<<0,
    EVENT_READ     = 1<<1,
    EVENT_WRITE    = 1<<2,
    EVENT_SIGNAL   = 1<<3,
    EVENT_PERSIST  = 1<<4,
    EVENT_ET       = 1<<5,
    EVENT_FINALIZE = 1<<6,
    EVENT_CLOSED   = 1<<7,
    EVENT_ERROR    = 1<<8,
    EVENT_EXCEPT   = 1<<9,
};

struct gevent_cbs {
    void (*ev_in)(int fd, void *arg);
    void (*ev_out)(int fd, void *arg);
    void (*ev_err)(int fd, void *arg);
    void *args;
};

struct gevent {
    int evfd;
    int flags;
    struct gevent_cbs *evcb;
};

typedef std::map<int, struct gevent*> gevent_map_t;
typedef std::list<int> gevent_fd_list_t;

struct gevent_base;
struct gevent_ops {
    void *(*init)();
    void (*deinit)(void *ctx);
    int (*add)(struct gevent_base *eb, struct gevent *e);
    int (*del)(struct gevent_base *eb, struct gevent *e);
    int (*dispatch)(struct gevent_base *eb, struct timeval *tv);
};

struct gevent_base {
    /** Pointer to backend-specific data. */
    void *ctx;
    int loop;
    int rfd;
    int wfd;
    const struct gevent_ops *evop;
};

struct gevent_base *gevent_base_create();
void gevent_base_destroy(struct gevent_base *);
int gevent_base_loop(struct gevent_base *);
void gevent_base_loop_break(struct gevent_base *);
int gevent_base_wait(struct gevent_base *eb);
void gevent_base_signal(struct gevent_base *eb);

struct gevent *gevent_create(int fd,
        void (ev_in)(int, void *),
        void (ev_out)(int, void *),
        void (ev_err)(int, void *),
        void *args);

void gevent_destroy(struct gevent *e);
int gevent_add(struct gevent_base *eb, struct gevent *e);
int gevent_del(struct gevent_base *eb, struct gevent *e);
int gevent_mod(struct gevent_base *eb, struct gevent *e,
        void (ev_in)(int, void *),
        void (ev_out)(int, void *),
        void (ev_err)(int, void *));

#endif
