#include <arpa/inet.h>        /*  inet (3) funtions         */
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <limits.h>
#include <errno.h>
#include <string.h>

#include "pk-btgatt-tcpip.h"

#define LISTENQ        (1024)

void error(const char *msg);

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

struct tcpip_server*   setup_tcpip_server( struct tcpip_server*  serv)
{
    serv->port = 1337;
    fprintf(stdout,"port = %d\n",serv->port);

    if ( (serv->list_s = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
    {
        fprintf(stderr, "ECHOSERV: Error creating listening socket.\n");
        exit(EXIT_FAILURE);
    }

    memset(&serv->servaddr, 0, sizeof(serv->servaddr));
    serv->servaddr.sin_family      = AF_INET;
    serv->servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv->servaddr.sin_port        = htons(serv->port);

    serv->line_len = 0;
    if(setsockopt(serv->list_s, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0)
        error("setsockopt(SO_REUSEADDR) failed");

    if ( bind(serv->list_s, (struct sockaddr *) &serv->servaddr, sizeof(serv->servaddr)) < 0 )
    {
        fprintf(stderr, "ECHOSERV: Error calling bind()\n");
        exit(EXIT_FAILURE);
    }
    if ( listen(serv->list_s, LISTENQ) < 0 )
    {
        fprintf(stderr, "ECHOSERV: Error calling listen()\n");
        exit(EXIT_FAILURE);
    }

    serv->conn_s = -2;

    return serv;
}
