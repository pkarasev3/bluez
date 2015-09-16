#ifndef BTGATT_TCPIP_H
#define BTGATT_TCPIP_H

#include <sys/socket.h>       /*  socket definitions        */
#include <sys/types.h>        /*  socket types              */
#include <sys/time.h>

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

int timeval_subtract(struct timeval *result, struct timeval *t2, struct timeval *t1);

struct tcpip_server*   setup_tcpip_server(struct tcpip_server*  serv);


#endif
