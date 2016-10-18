/*****************************************************************************
 * Copyright (C) 2014-2015
 * file:    select.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-04-27 00:59
 * updated: 2015-07-12 00:41
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/select.h>
#include <libmacro.h>
#include <liblog.h>
#include "libgevent.h"
#include <string>
#include <list>
#include <map>

#define SELECT_MAX_FD	1024

extern gevent_map_t g_gevent_map;
extern gevent_fd_list_t g_gevent_fd_list;

struct select_ctx {
    int nfds;		/* Highest fd in fd set */
    fd_set *rfds;
    fd_set *wfds;
    fd_set *efds;
};

static void *select_init(void)
{
    struct select_ctx *sc = CALLOC(1, struct select_ctx);
    if (!sc) {
        loge("malloc select_ctx failed!\n");
        return NULL;
    }
    fd_set *rfds = CALLOC(1, fd_set);
    fd_set *wfds = CALLOC(1, fd_set);
    fd_set *efds = CALLOC(1, fd_set);
    if (!rfds || !wfds || !efds) {
        loge("malloc fd_set failed!\n");
        if(rfds != NULL)
            free(rfds);
        if(wfds != NULL)
            free(wfds);
        if(efds != NULL)
            free(efds);
        return NULL;
    }
    sc->rfds = rfds;
    sc->wfds = wfds;
    sc->efds = efds;
    return sc;
}

static void select_deinit(void *ctx)
{
    struct select_ctx *sc = (struct select_ctx *)ctx;
    if (!ctx) {
        return;
    }
    free(sc->rfds);
    free(sc->wfds);
    free(sc->efds);
    free(sc);
}

static int select_add(struct gevent_base *eb, struct gevent *e)
{
    return 0;
#if 0
    struct select_ctx *sc = (struct select_ctx *)eb->ctx;

    FD_ZERO(sc->rfds);
    FD_ZERO(sc->wfds);
    FD_ZERO(sc->efds);

    if (sc->nfds < e->evfd) {
        sc->nfds = e->evfd;
    }

    if (e->flags & EVENT_READ)
        FD_SET(e->evfd, sc->rfds);
    if (e->flags & EVENT_WRITE)
        FD_SET(e->evfd, sc->wfds);
    if (e->flags & EVENT_EXCEPT)
        FD_SET(e->evfd, sc->efds);
    return 0;
#endif
}

static int select_del(struct gevent_base *eb, struct gevent *e)
{
    return 0;
#if 0
    struct select_ctx *sc = (struct select_ctx *)eb->ctx;
    if (sc->rfds)
        FD_CLR(e->evfd, sc->rfds);
    if (sc->wfds)
        FD_CLR(e->evfd, sc->wfds);
    if (sc->efds)
        FD_CLR(e->evfd, sc->efds);
    return 0;
#endif
}

static int select_dispatch(struct gevent_base *eb, struct timeval *tv)
{
    int n;
    struct select_ctx *sc = (struct select_ctx *)eb->ctx;
    int nfds = 0;
    struct timeval select_tv = {0, 100*1000};

    FD_ZERO(sc->rfds);
    FD_ZERO(sc->wfds);
    FD_ZERO(sc->efds);
    g_gevent_fd_list.clear();
    for(gevent_map_t::iterator iter=g_gevent_map.begin(); iter!=g_gevent_map.end(); iter++)
    {
        struct gevent *e = iter->second;
        if(e == NULL || e->evfd < 0 || (e->evcb->ev_in == NULL && e->evcb->ev_out == NULL && e->evcb->ev_err == NULL))
            continue;

        g_gevent_fd_list.push_back(e->evfd);
        if (nfds < e->evfd) {
            nfds = e->evfd;
        }

        if (e->flags & EVENT_READ)
            FD_SET(e->evfd, sc->rfds);
        if (e->flags & EVENT_WRITE)
            FD_SET(e->evfd, sc->wfds);
        if (e->flags & EVENT_EXCEPT)
            FD_SET(e->evfd, sc->efds);
    }

    n = select(nfds+1, sc->rfds, sc->wfds, sc->efds, &select_tv);
    if (-1 == n) {
        loge("errno=%d %s\n", errno, strerror(errno));
        return -1;
    }
    if (0 == n) {
        //loge("select timeout\n");
        return 0;
    }
    while(g_gevent_fd_list.size() > 0)
    {
        int fd = g_gevent_fd_list.front();
        if(g_gevent_map.find(fd) != g_gevent_map.end())
        {
            struct gevent *e = g_gevent_map[fd];
            if(e != NULL && e->evfd == fd)
            {
                if (FD_ISSET(e->evfd, sc->rfds) && e->evcb->ev_in != NULL)
                    e->evcb->ev_in(e->evfd, (void *)e->evcb->args);
                if (FD_ISSET(e->evfd, sc->wfds) && e->evcb->ev_out != NULL)
                    e->evcb->ev_out(e->evfd, (void *)e->evcb->args);
                if (FD_ISSET(e->evfd, sc->efds) && e->evcb->ev_err != NULL)
                    e->evcb->ev_err(e->evfd, (void *)e->evcb->args);
            }
        }

        if(g_gevent_fd_list.size() > 0 && g_gevent_fd_list.front() == fd)
        {
            g_gevent_fd_list.pop_front();
        }
    }

    return 0;
}

struct gevent_ops selectops = {
    .init     = select_init,
    .deinit   = select_deinit,
    .add      = select_add,
    .del      = select_del,
    .dispatch = select_dispatch,
};
