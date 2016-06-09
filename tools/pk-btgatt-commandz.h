#ifndef BTGATT_CMNDZ
#define BTGATT_CMNDZ

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
 *  Copyup (C) 2016 Garbage Inc.
 */

#ifdef HAVE_CONFIG_H
  #include <config.h>
#endif

#include "tools/pk-btgatt-rwcfg.h"

//! @brief{Main Context for BTLE comms}
struct client
{
  int fd;
  struct bt_att *att;
  struct gatt_db *db;
  struct bt_gatt_client *gatt;

  unsigned int reliable_session_id;

  //////////////////////////////////
  struct readwrite_config  rwcfg;
};

void write_cb(bool success, uint8_t att_ecode, void *user_data);

void cmd_write_string(struct client *cli, char *cmd_str);

void write_string_usage();

//! @brief convert something like hex 0x001B into string version "0x001B"
//!    note: caller is responsible for safety of array bounds!
int  uint16_as_hex_string(uint16_t Xhandle,const char* cmd0);

void send_stringAsHexValueSequence(struct client* cli);

int  write_string_to_handle(
        struct client* cli,
        const char* cmd,
        size_t sz,
        uint16_t Xhandle    );


#define str2hex_doc "\tSend a string as hex command (with injected \\r\\n suffix)"
#define str2hex_cmd "write-string"

#endif


