#ifndef BTGATT_TCPIP_H
#define BTGATT_TCPIP_H

#include <sys/socket.h>       /*  socket definitions        */
#include <sys/types.h>        /*  socket types              */
#include <sys/time.h>

#define LISTENQ        (1024)
#include <sys/socket.h>
#include <errno.h>
#include "src/shared/util.h"

struct tcpip_server
{
    int       list_s;                /*  listening socket          */
    int       conn_s;                /*  connection socket         */
    short int port;                  /*  port number               */
    struct    sockaddr_in servaddr;  /*  socket address structure  */
    char      buffer[1024];          /*  character buffer          */
    char     *endptr;                /*  for strtol()              */
    int       loop_idx;
    int       line_len;              /*  # of chars in line, before hitting \n */
};

struct readwrite_config;

int timeval_subtract(struct timeval *result, struct timeval *t2, struct timeval *t1);

struct tcpip_server*   setup_tcpip_server(struct tcpip_server*  serv, struct readwrite_config* rwcfg);

struct tcpip_server* tcpip_server_create();

void tcpip_server_write_line(struct tcpip_server* serv);

void tcpip_server_destroy(struct tcpip_server* serv);

ssize_t Readline(int fd, void *vptr, size_t maxlen);
ssize_t Writeline(int fc, const void *vptr, size_t maxlen);

int make_socket_non_blocking (int sfd);

#endif
