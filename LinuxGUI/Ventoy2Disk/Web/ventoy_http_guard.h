/******************************************************************************
 * ventoy_http_guard.h
 *
 * Hardening helpers shared by the local Ventoy web servers
 * (Plugson and the Linux WebUI).
 *
 *  1. Host / Origin validation
 *     Both servers bind to a local address and expose powerful JSON APIs
 *     (editing ventoy.json, or even installing Ventoy onto a disk as root).
 *     Without validating the Host header, a malicious web page can use
 *     DNS rebinding to make the victim's browser talk to the local server as
 *     if it were same-origin, read the API responses (including the session
 *     token returned by "sysinfo") and issue arbitrary commands.
 *     Without validating Origin, any page can fire blind cross-site POSTs
 *     (Content-Type: text/plain needs no CORS preflight).
 *
 *  2. Robust request-body reading
 *     Caps the body size, checks malloc() and mg_read() results so a bogus
 *     Content-Length or a dropped connection cannot crash the server or write
 *     before the start of the buffer.
 *
 * Header-only on purpose, so the existing build scripts need no changes.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 ******************************************************************************/
#ifndef __VENTOY_HTTP_GUARD_H__
#define __VENTOY_HTTP_GUARD_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#if defined(_MSC_VER)
#ifndef inline
#define inline __inline
#endif
#define strncasecmp _strnicmp
#else
#include <strings.h>
#endif

#ifndef VTOY_HTTP_BODY_MAX
#define VTOY_HTTP_BODY_MAX  (16 * 1024 * 1024)
#endif

static char g_vtoy_guard_ip[64];
static char g_vtoy_guard_port[16];

static inline void ventoy_http_guard_init(const char *ip, const char *port)
{
    snprintf(g_vtoy_guard_ip, sizeof(g_vtoy_guard_ip), "%s", ip ? ip : "127.0.0.1");
    snprintf(g_vtoy_guard_port, sizeof(g_vtoy_guard_port), "%s", port ? port : "");
}

/* case-insensitive compare of a host[:port] value against name:port */
static inline int ventoy_http_guard_match(const char *value, size_t len, const char *name)
{
    size_t nlen = strlen(name);
    size_t plen = strlen(g_vtoy_guard_port);

    if (nlen == 0 || len < nlen)
    {
        return 0;
    }

    if (strncasecmp(value, name, nlen) != 0)
    {
        return 0;
    }

    if (len == nlen)
    {
        /* no explicit port: only acceptable when we listen on 80 */
        return strcmp(g_vtoy_guard_port, "80") == 0;
    }

    if (value[nlen] != ':' || len != nlen + 1 + plen)
    {
        return 0;
    }

    return strncmp(value + nlen + 1, g_vtoy_guard_port, plen) == 0;
}

static inline int ventoy_http_guard_host_ok(const char *value, size_t len)
{
    char bracket[80];

    if (!value || len == 0)
    {
        return 0;
    }

    if (ventoy_http_guard_match(value, len, "127.0.0.1") ||
        ventoy_http_guard_match(value, len, "localhost") ||
        ventoy_http_guard_match(value, len, "[::1]"))
    {
        return 1;
    }

    /* the address the user explicitly asked the server to listen on */
    if (g_vtoy_guard_ip[0] && strcmp(g_vtoy_guard_ip, "0.0.0.0") != 0)
    {
        if (strchr(g_vtoy_guard_ip, ':') && g_vtoy_guard_ip[0] != '[')
        {
            snprintf(bracket, sizeof(bracket), "[%s]", g_vtoy_guard_ip);
            return ventoy_http_guard_match(value, len, bracket);
        }
        return ventoy_http_guard_match(value, len, g_vtoy_guard_ip);
    }

    return 0;
}

/*
 * Returns 1 if the request may proceed.
 * Returns 0 if it was rejected (a 403 response has already been sent).
 */
static inline int ventoy_http_guard_check(struct mg_connection *conn)
{
    const char *host = mg_get_header(conn, "Host");
    const char *origin = mg_get_header(conn, "Origin");
    const char *reason = NULL;

    if (!ventoy_http_guard_host_ok(host, host ? strlen(host) : 0))
    {
        reason = "bad host";
    }
    else if (origin && strcmp(origin, "null") != 0)
    {
        if (strncasecmp(origin, "http://", 7) != 0 ||
            !ventoy_http_guard_host_ok(origin + 7, strlen(origin + 7)))
        {
            reason = "bad origin";
        }
    }
    else if (origin)
    {
        reason = "null origin";
    }

    if (reason)
    {
        mg_printf(conn,
                  "HTTP/1.1 403 Forbidden\r\n"
                  "Content-Type: text/plain\r\n"
                  "Content-Length: %d\r\n"
                  "Connection: close\r\n"
                  "\r\n%s",
                  (int)strlen(reason), reason);
        return 0;
    }

    return 1;
}

/*
 * Read the whole POST body into a NUL terminated buffer.
 * Uses stack_buf when it is big enough, otherwise malloc().
 * Returns the buffer (caller frees it if != stack_buf) or NULL on error.
 */
static inline char *ventoy_http_guard_read_body(struct mg_connection *conn, long long content_length,
                                                char *stack_buf, int stack_len)
{
    int total = 0;
    int ret = 0;
    int buf_len = 0;
    char *buf = NULL;

    if (content_length > VTOY_HTTP_BODY_MAX)
    {
        return NULL;
    }

    if (content_length >= 0 && content_length < stack_len)
    {
        buf = stack_buf;
        buf_len = stack_len;
    }
    else if (content_length >= 0)
    {
        buf_len = (int)content_length + 1;
        buf = (char *)malloc(buf_len);
        if (!buf)
        {
            return NULL;
        }
    }
    else
    {
        /* unknown length (chunked): read at most the stack buffer */
        buf = stack_buf;
        buf_len = stack_len;
    }

    while (total < buf_len - 1)
    {
        ret = mg_read(conn, buf + total, (size_t)(buf_len - 1 - total));
        if (ret <= 0)
        {
            break;
        }
        total += ret;
    }

    if (ret < 0 && total == 0)
    {
        if (buf != stack_buf)
        {
            free(buf);
        }
        return NULL;
    }

    buf[total] = 0;
    return buf;
}

#endif /* __VENTOY_HTTP_GUARD_H__ */
