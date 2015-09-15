#ifndef BTGATT_TCPIP_H
#define BTGATT_TCPIP_H

#include <sys/socket.h>       /*  socket definitions        */
#include <sys/types.h>        /*  socket types              */

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

struct tcpip_server*   setup_tcpip_server(struct tcpip_server*  serv);


#endif
