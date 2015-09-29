#include "pk-btgatt-rwcfg.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void initialize_rwcfg(struct readwrite_config* arg)
{
  arg->handle_Read      = 0x0018;
  arg->handle_Write     = 0x0015;
  arg->tcpip_Port       = 1337;
  arg->tcpip_PacketSize = 23;
  arg->tcpip_Enabled    = 1;

  memset(&arg->init_WriteValues[0],0,sizeof(arg->init_WriteValues));
}


size_t cmd_from_arg(struct readwrite_config* arg, const char* cmd)
{
  size_t i;
  if(!cmd)
    return 0;

  for(i=0;i<(sizeof(arg->init_WriteValues)-2);i++)
  {
    if( 0 == cmd[i] )
      break;
    arg->init_WriteValues[i] = cmd[i];
  }
  arg->init_WriteValues[i++] = 0x0d;
  arg->init_WriteValues[i++] = 0x0a;
  printf("init cmd = %s ... \n",arg->init_WriteValues);
  return i;
}

void clone_rwcfg(const struct readwrite_config* src,struct readwrite_config* dst)
{
  dst->handle_Read=src->handle_Read;
  dst->handle_Write=src->handle_Write;
  dst->tcpip_Enabled=src->tcpip_Enabled;
  dst->tcpip_PacketSize=src->tcpip_PacketSize;
  dst->tcpip_Port=src->tcpip_Port;

  memcpy(&dst->init_WriteValues[0],&src->init_WriteValues[0],sizeof(src->init_WriteValues));
}
