#ifndef BTGATT_RWCFG
#define BTGATT_RWCFG

/*
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2014  Google Inc.
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

struct readwrite_config
{
  uint16_t  handle_Read;            // what index we listen to notifications on
  uint16_t  handle_Write;           // where we are writing values to
  int       tcpip_Port;             // port where data is published
  int       tcpip_PacketSize;       // (expected) size of packets to send
  int       tcpip_Enabled;          // is sending on tcpip enabled
  bool      PRINT_NOTIFY_CB;        // should we printf bunch of times during notify callback
  char      init_WriteValues [16];  // initial commands to send after connected
};

void  initialize_rwcfg(struct readwrite_config*  arg);

void  clone_rwcfg(const struct readwrite_config*  src,struct readwrite_config*  dst);

size_t  cmd_from_arg(struct readwrite_config*  arg, const char* cmd);

#define COLOR_OFF	"\x1B[0m"
#define COLOR_RED	"\x1B[0;91m"
#define COLOR_GREEN	"\x1B[0;92m"
#define COLOR_YELLOW	"\x1B[0;93m"
#define COLOR_BLUE	"\x1B[0;94m"
#define COLOR_MAGENTA	"\x1B[0;95m"
#define COLOR_BOLDGRAY	"\x1B[1;30m"
#define COLOR_BOLDWHITE	"\x1B[1;37m"

static void usage(void)
{
  printf("btgatt-client\n");

  printf("\nUsage:\n\tbtgatt-client [options]\n");

  printf("Options:\n"
    "\t-i, --index <id>\t\tSpecify adapter index, e.g. hci0\n"
    "\t-d, --dest <addr>\t\tSpecify the destination address\n"
    "\t-t, --type [random|public] \tSpecify the LE address type\n"
    "\t-m, --mtu <mtu> \t\tThe ATT MTU to use\n"
    "\t-n, --notify-via-printf \tEnable printf during notify callback\n"
    "\t-s, --security-level <sec> \tSet security level (low|"
                "medium|high)\n"
    "\t-v, --verbose\t\t\tEnable extra logging\n"
    "\t-h, --help\t\t\tDisplay help\n"
    "\t-P  --tcpip-port <N> \t\tSet tcpip port to send packets on (N<1 to disable)\n"
    "\t-Z  --tcpip-packet-sz <N> \tSet tcpip packet size to N bytes \n"
    "\t-W  --handle-write <hex>  \tDefine handle to write values to after"
                                  " device is ready. Example: -W 0x002A  \n"
    "\t-N  --handle-notify <hex> \tDefine handle for registering notifications "
                                  "during startup. Example: -N 0x002A  \n"
    "\t-C  --command-init <cmd1> <cmd2> ... <cmd5>\n"
                                "\t\t\t\t\tDefine commands to write as values to "
                                "write handle.\n"
         );
}

#endif
