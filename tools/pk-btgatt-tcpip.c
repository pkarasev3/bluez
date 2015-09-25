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
#include <fcntl.h>

#include "pk-btgatt-tcpip.h"
#include "pk-btgatt-rwcfg.h"

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

int timeval_subtract(struct timeval *result, struct timeval *t2, struct timeval *t1)
{
    long int diff;
    diff = (t2->tv_usec + 1000000 * t2->tv_sec) - (t1->tv_usec + 1000000 * t1->tv_sec);
    result->tv_sec = diff / 1000000;
    result->tv_usec = diff % 1000000;
    return  ((int)(diff<0));
}

int make_socket_non_blocking (int sfd)
{
  int flags, s;

  flags = fcntl (sfd, F_GETFL, 0);
  if (flags == -1)
    {
      perror ("fcntl");
      return -1;
    }

  flags |= O_NONBLOCK;
  s = fcntl (sfd, F_SETFL, flags);
  if (s == -1)
    {
      perror ("fcntl");
      return -1;
    }

  return 0;
}

ssize_t Readline(int sockd, void *vptr, size_t maxlen)
{
    ssize_t rc;
    size_t  n;
    char    c, *buffer;

    buffer = vptr;

    for ( n = 1; n < maxlen; n++ )
    {
            if ( (rc = read(sockd, &c, 1)) == 1 )
        {
                *buffer++ = c;
                if ( c == '\n' )
                    break;
            }
            else if ( rc == 0 )
        {
                if ( n == 1 )
                        return 0;
                else
                        break;
            }
            else
        {
                if ( errno == EINTR )
                        continue;
                return -1;
            }
    }

    *buffer = 0;
    return n;
}

ssize_t Writeline(int sockd, const void *vptr, size_t n)
{
    size_t      nleft;
    ssize_t     nwritten;
    const char *buffer;

    buffer = vptr;
    nleft  = n;

    while ( nleft > 0 )
    {
        nwritten = write(sockd, buffer, nleft);
        //printf("nwritten = %04ld\n",nwritten);
        if ( nwritten <= 0 )
        {
            if ( errno == EINTR )
                return 0;
            else
                return -1;
        }

        nleft  -= nwritten;
        buffer += nwritten;
    }

    return nwritten;
}


struct tcpip_server* tcpip_server_create()
{
  struct tcpip_server* serv;

  serv = new0(struct tcpip_server, 1);

  return serv;
}

void tcpip_server_write_line(struct tcpip_server* serv)
{
  int wroteCount;

  if( serv->conn_s < 0 )
    serv->conn_s = accept(serv->list_s, NULL, NULL);

  if( serv->conn_s < 0 )
  {
    printf(" failed to accept() connection! \n ");
    serv->line_len = 0;
    return;
  }

  if( serv->line_len <= 0 )
    return;

  wroteCount = Writeline(serv->conn_s,serv->buffer,serv->line_len);
  if( wroteCount != serv->line_len )
    printf(COLOR_RED "Error, only wrote %d, expected %d\n" COLOR_OFF,wroteCount,serv->line_len);
  else
  {/*usleep(500);*/}          /*printf(" success!\n ") ;*/

  serv->line_len = 0;
}

void tcpip_server_destroy(struct tcpip_server* serv)
{
  close(serv->conn_s);
  free(serv);
}
