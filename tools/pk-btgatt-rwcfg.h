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
#include <string.h>

#define  MaxCommandPacketSize  512
struct readwrite_config
{
  uint16_t  handle_Read;            // what index we listen to notifications on
  uint16_t  handle_Write;           // where we are writing values to
  int       tcpip_Port;             // port where data is published
  int       tcpip_PacketSize;       // (expected) size of packets to send
  int       tcpip_Enabled;          // is sending on tcpip enabled
  int       print_notify_verbosity; // should we printf bunch of times during notify callback

  //! next commands to send (e.g. after connected or from later command being invoked)
  char      next_WriteValues[MaxCommandPacketSize];
};

void  initialize_rwcfg(struct readwrite_config*  arg);

void  clone_rwcfg(const struct readwrite_config*  src,struct readwrite_config*  dst);


/**
 * given argument \param{cmd} like "invc", send the characters as hex values followed
 *   by line termination with backslash-r backslash-n
 *   (i.e.  hex 0x0d 0x0a or integers 13 10
      @return{number of bytes in generated command}
      @param{arg} will have its "next command" field populated by this call.*/
size_t  cmd_from_arg(struct readwrite_config*  arg, const uint8_t* cmd);

/** \note{alias to cmd_from_arg with more obvious name...}*/
size_t  put_string_as_hex_values_command(struct readwrite_config* arg, const uint8_t* cmd);


#define COLOR_OFF	    "\x1B[0m"
#define COLOR_RED	    "\x1B[0;91m"
#define COLOR_BOLDRED	"\x1B[1;5;91m"
#define COLOR_GREEN	"\x1B[0;92m"
#define COLOR_YELLOW	"\x1B[0;93m"
#define COLOR_BLUE	"\x1B[0;94m"
#define COLOR_MAGENTA	"\x1B[0;95m"
#define COLOR_BOLDGRAY	"\x1B[1;30m"
#define COLOR_BOLDWHITE	"\x1B[1;37m"


//! \note{print an error with color then return}
#define RETURN_PRINTF_ERROR(ErrorName,StringInfo)\
{fprintf(stderr, COLOR_BOLDWHITE " [" ErrorName "]    "\
 COLOR_RED " %s \n" COLOR_OFF,StringInfo);\
 fprintf(stderr,COLOR_BLUE \
          " (at %s : %s @ %d)\n" COLOR_OFF,\
           __FILE__,__FUNCTION__,__LINE__);\
 fflush(stderr); return;}



void usage(void);

//! @return{false if too many args, true if correct.}
//! @note{unkown behavior if not enough args versus expected count}
bool parse_args(char *str, int expected_argc,  char **argv, int *argc);

//! @return{string description of error code from BT backend}
const char *ecode_to_string(uint8_t ecode);

//! @brief{printf only if object's verbosity is at least N}
#define printf_VN(arg,n,...) \
     {if(arg->print_notify_verbosity >= n){\
         printf(__VA_ARGS__); printf(COLOR_BLUE "#" COLOR_OFF);}\
     };
#define printf_V1(arg,...) printf_VN(arg,1,__VA_ARGS__)

#define printf_V2(arg,...) printf_VN(arg,2,__VA_ARGS__)

#endif
