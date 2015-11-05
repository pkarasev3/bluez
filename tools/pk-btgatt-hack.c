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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <limits.h>
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>
#include <termios.h>
#include <stdatomic.h>  //sha-zam!

#include "lib/bluetooth.h"
#include "lib/hci.h"
#include "lib/hci_lib.h"
#include "lib/l2cap.h"
#include "lib/uuid.h"

#include "src/shared/mainloop.h"
//#include "src/shared/util.h"
#include "src/shared/att.h"
#include "src/shared/queue.h"
#include "src/shared/gatt-db.h"
#include "src/shared/gatt-client.h"

#include "pk-btgatt-rwcfg.h"
#include "pk-btgatt-tcpip.h"

#define ATT_CID 4

#define PRLOG(...) \
  printf(__VA_ARGS__); print_prompt();


static bool verbose = false;

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

/////////////////////////////////////////////////////////
struct tcpip_server*  global_server;
/////////////////////////////////////////////////////////
struct client*        global_client;
/////////////////////////////////////////////////////////

////////////////////////////////////////
atomic_int   RegistrationState  = ATOMIC_VAR_INIT(0);

////////////////////////////////////////
atomic_int   PromptPrintState   = ATOMIC_VAR_INIT(0);
static int   PromptPrint_OFF_   = 2;


static struct option main_options[] =
{
  { "tcpip-packet-sz",  1, 0, 'Z' },
  { "command-init",     1, 0, 'C' },
  { "handle-notify",    1, 0, 'N' },
  { "handle-write",     1, 0, 'W' },
  { "tcpip-port",       1, 0, 'P' },
  { "index",            1, 0, 'i' },
  { "dest",             1, 0, 'd' },
  { "type",             1, 0, 't' },
  { "mtu",   	          1, 0, 'm' },
  { "notify-via-printf",0, 0, 'n' },
  { "security-level",	  1, 0, 's' },
  { "verbose",          0, 0, 'v' },
  { "help",             0, 0, 'h' },
  { }
};


static void print_prompt(void)
{
  int prompt_print_state = atomic_load(&PromptPrintState);
  if( prompt_print_state == PromptPrint_OFF_ )
    return;
  printf(COLOR_BLUE "[GATT client]" COLOR_OFF "# ");
  fflush(stdout);
}

static const char *ecode_to_string(uint8_t ecode)
{
  switch (ecode) {
  case BT_ATT_ERROR_INVALID_HANDLE:
    return "Invalid Handle";
  case BT_ATT_ERROR_READ_NOT_PERMITTED:
    return "Read Not Permitted";
  case BT_ATT_ERROR_WRITE_NOT_PERMITTED:
    return "Write Not Permitted";
  case BT_ATT_ERROR_INVALID_PDU:
    return "Invalid PDU";
  case BT_ATT_ERROR_AUTHENTICATION:
    return "Authentication Required";
  case BT_ATT_ERROR_REQUEST_NOT_SUPPORTED:
    return "Request Not Supported";
  case BT_ATT_ERROR_INVALID_OFFSET:
    return "Invalid Offset";
  case BT_ATT_ERROR_AUTHORIZATION:
    return "Authorization Required";
  case BT_ATT_ERROR_PREPARE_QUEUE_FULL:
    return "Prepare Write Queue Full";
  case BT_ATT_ERROR_ATTRIBUTE_NOT_FOUND:
    return "Attribute Not Found";
  case BT_ATT_ERROR_ATTRIBUTE_NOT_LONG:
    return "Attribute Not Long";
  case BT_ATT_ERROR_INSUFFICIENT_ENCRYPTION_KEY_SIZE:
    return "Insuficient Encryption Key Size";
  case BT_ATT_ERROR_INVALID_ATTRIBUTE_VALUE_LEN:
    return "Invalid Attribute value len";
  case BT_ATT_ERROR_UNLIKELY:
    return "Unlikely Error";
  case BT_ATT_ERROR_INSUFFICIENT_ENCRYPTION:
    return "Insufficient Encryption";
  case BT_ATT_ERROR_UNSUPPORTED_GROUP_TYPE:
    return "Group type Not Supported";
  case BT_ATT_ERROR_INSUFFICIENT_RESOURCES:
    return "Insufficient Resources";
  case BT_ERROR_CCC_IMPROPERLY_CONFIGURED:
    return "CCC Improperly Configured";
  case BT_ERROR_ALREADY_IN_PROGRESS:
    return "Procedure Already in Progress";
  case BT_ERROR_OUT_OF_RANGE:
    return "Out of Range";
  default:
    return "Unknown error type";
  }
}

static void att_disconnect_cb(int err, void *user_data)
{
  printf("Device disconnected: %s\n", strerror(err));

  mainloop_quit();
}

static void att_debug_cb(const char *str, void *user_data)
{
  const char *prefix = user_data;

  PRLOG(COLOR_BOLDGRAY "%s" COLOR_BOLDWHITE "%s\n" COLOR_OFF, prefix, str);
}

static void gatt_debug_cb(const char *str, void *user_data)
{
  const char *prefix = user_data;
  fprintf(stdout,"gatt_debug_cb: ");
  PRLOG(COLOR_GREEN "%s%s\n" COLOR_OFF, prefix, str);
}

static void ready_cb(bool success, uint8_t att_ecode, void *user_data);
static void service_changed_cb(uint16_t start_handle, uint16_t end_handle,
              void *user_data);

static void log_service_event(struct gatt_db_attribute *attr, const char *str)
{
  char uuid_str[MAX_LEN_UUID_STR];
  bt_uuid_t uuid;
  uint16_t start, end;

  gatt_db_attribute_get_service_uuid(attr, &uuid);
  bt_uuid_to_string(&uuid, uuid_str, sizeof(uuid_str));

  gatt_db_attribute_get_service_handles(attr, &start, &end);

  PRLOG("%s - UUID: %s start: 0x%04x end: 0x%04x\n", str, uuid_str,
                start, end);
}

static void service_added_cb(struct gatt_db_attribute *attr, void *user_data)
{
  log_service_event(attr, "Service Added");
}

static void service_removed_cb(struct gatt_db_attribute *attr, void *user_data)
{
  log_service_event(attr, "Service Removed");
}

void cmd_register_notify(struct client *cli, char *cmd_str);

static struct client *client_create(int fd, uint16_t mtu)
{
  struct client *cli;

  cli = new0(struct client, 1);
  if (!cli) {
    fprintf(stderr, "Failed to allocate memory for client\n");
    return NULL;
  }

  initialize_rwcfg(&cli->rwcfg);

  cli->att = bt_att_new(fd, false);
  if (!cli->att) {
    fprintf(stderr, "Failed to initialze ATT transport layer\n");
    bt_att_unref(cli->att);
    free(cli);
    return NULL;
  }

  if (!bt_att_set_close_on_unref(cli->att, true)) {
    fprintf(stderr, "Failed to set up ATT transport layer\n");
    bt_att_unref(cli->att);
    free(cli);
    return NULL;
  }

  if (!bt_att_register_disconnect(cli->att, att_disconnect_cb, NULL,
                NULL)) {
    fprintf(stderr, "Failed to set ATT disconnect handler\n");
    bt_att_unref(cli->att);
    free(cli);
    return NULL;
  }

  cli->fd = fd;
  cli->db = gatt_db_new();
  if (!cli->db) {
    fprintf(stderr, "Failed to create GATT database\n");
    bt_att_unref(cli->att);
    free(cli);
    return NULL;
  }

  cli->gatt = bt_gatt_client_new(cli->db, cli->att, mtu);
  if (!cli->gatt) {
    fprintf(stderr, "Failed to create GATT client\n");
    gatt_db_unref(cli->db);
    bt_att_unref(cli->att);
    free(cli);
    return NULL;
  }

  gatt_db_register(cli->db, service_added_cb, service_removed_cb,
                NULL, NULL);

  if (verbose) {
    bt_att_set_debug(cli->att, att_debug_cb, "att: ", NULL);
    bt_gatt_client_set_debug(cli->gatt, gatt_debug_cb, "gatt: ",
                  NULL);
  }

  bt_gatt_client_set_ready_handler(cli->gatt, ready_cb, cli, NULL);
  bt_gatt_client_set_service_changed(cli->gatt, service_changed_cb, cli,
                  NULL);

  /* bt_gatt_client already holds a reference */
  gatt_db_unref(cli->db);

  return cli;
}

static void client_destroy(struct client *cli)
{
  bt_gatt_client_unref(cli->gatt);
  bt_att_unref(cli->att);
  free(cli);
}

static void print_uuid(const bt_uuid_t *uuid)
{
  char uuid_str[MAX_LEN_UUID_STR];
  bt_uuid_t uuid128;

  bt_uuid_to_uuid128(uuid, &uuid128);
  bt_uuid_to_string(&uuid128, uuid_str, sizeof(uuid_str));

  printf("%s\n", uuid_str);
}

static void print_incl(struct gatt_db_attribute *attr, void *user_data)
{
  struct client *cli = user_data;
  uint16_t handle, start, end;
  struct gatt_db_attribute *service;
  bt_uuid_t uuid;

  if (!gatt_db_attribute_get_incl_data(attr, &handle, &start, &end))
    return;

  service = gatt_db_get_attribute(cli->db, start);
  if (!service)
    return;

  gatt_db_attribute_get_service_uuid(service, &uuid);

  printf("\t  " COLOR_GREEN "include" COLOR_OFF " - handle: "
          "0x%04x, - start: 0x%04x, end: 0x%04x,"
          "uuid: ", handle, start, end);
  print_uuid(&uuid);
}

static void print_desc(struct gatt_db_attribute *attr, void *user_data)
{
  printf("\t\t  " COLOR_MAGENTA "descr" COLOR_OFF
          " - handle: 0x%04x, uuid: ",
          gatt_db_attribute_get_handle(attr));
  print_uuid(gatt_db_attribute_get_type(attr));
}

static void print_chrc(struct gatt_db_attribute *attr, void *user_data)
{
  uint16_t handle, value_handle;
  uint8_t properties;
  bt_uuid_t uuid;

  if (!gatt_db_attribute_get_char_data(attr, &handle,
                &value_handle,
                &properties,
                &uuid))
    return;

  printf("\t  " COLOR_YELLOW "charac" COLOR_OFF
          " - start: 0x%04x, value: 0x%04x, "
          "props: 0x%02x, uuid: ",
          handle, value_handle, properties);
  print_uuid(&uuid);

  gatt_db_service_foreach_desc(attr, print_desc, NULL);
}

static void print_service(struct gatt_db_attribute *attr, void *user_data)
{
  struct client *cli = user_data;
  uint16_t start, end;
  bool primary;
  bt_uuid_t uuid;

  if (!gatt_db_attribute_get_service_data(attr, &start, &end, &primary,
                  &uuid))
    return;

  printf(COLOR_RED "service" COLOR_OFF " - start: 0x%04x, "
        "end: 0x%04x, type: %s, uuid: ",
        start, end, primary ? "primary" : "secondary");
  print_uuid(&uuid);

  gatt_db_service_foreach_incl(attr, print_incl, cli);
  gatt_db_service_foreach_char(attr, print_chrc, NULL);

  printf("\n");
}

static void print_services(struct client *cli)
{
  printf("\n");

  gatt_db_foreach_service(cli->db, NULL, print_service, cli);
}

static void print_services_by_uuid(struct client *cli, const bt_uuid_t *uuid)
{
  printf("\n");

  gatt_db_foreach_service(cli->db, uuid, print_service, cli);
}

static void print_services_by_handle(struct client *cli, uint16_t handle)
{
  printf("\n");

  /* TODO: Filter by handle */
  gatt_db_foreach_service(cli->db, NULL, print_service, cli);
}

static void write_cb(bool success, uint8_t att_ecode, void *user_data);

///////////////////////////////////////
struct thread_info
{
    pthread_t thread_id;
    long int timeout_secs;
    struct client* cli;
};


////////////////////////////////////////
static void* loop_register_later( void* arg )
{
    struct timeval time0, time1, time_diff;
    long int timeout_secs;
    int      state;
    struct thread_info* tinfo = arg;

    state        = 1;
    timeout_secs = 5;

    gettimeofday(&time0,NULL);
    while( 1 )
    {
        usleep(50*1000);
        gettimeofday(&time1,NULL);
        timeval_subtract(&time_diff,&time1,&time0);
        if( time_diff.tv_sec >= timeout_secs )
            break;
    }

    atomic_store(&RegistrationState,state);
    printf(COLOR_BLUE " state = %d\n" COLOR_OFF, state);

    fflush(stdout);
    fflush(stderr);
    fflush(stdin);

    {
        cmd_register_notify(tinfo->cli,"0x0018"/*TODO: use the one in cli->rwcfg*/);

        fflush(stdin);
        fflush(stdout);
        fflush(stderr);

        state = 0;        
        atomic_store(&RegistrationState,state);

        atomic_store(&PromptPrintState,PromptPrint_OFF_);
    }

    return tinfo;
}

static
int  write_string_to_handle(
        struct client* cli,
        const char* cmd,
        size_t sz,
        uint16_t Xhandle    )
{
    unsigned int idx =  0;
    int result       = -1;
    usleep(100);
    while( idx < sz )
    {
        uint8_t val;
        val    = cmd[idx];
        result = bt_gatt_client_write_value(
                      cli->gatt,
                      Xhandle,
                      &val,
                      1,
                      write_cb,
                      NULL, NULL);
        idx++;
        if( !result )
        {
            fprintf(stderr, "failed writing %s to handle %d\n",cmd,Xhandle);
            break;
        }
    }
    return result;
}

static void inject_pk_hack(struct client *cli)
{    
    {
        int cmd0_end       = 0;
        uint16_t Xhandle   = cli->rwcfg.handle_Write; //0x0015;
        const char* cmd0   = cli->rwcfg.init_WriteValues;
        printf("\n cmd0 going to write to: 0x%04x \n", Xhandle);
        while( cmd0_end >= 0 )
        {
          if(cmd0[cmd0_end] == 0)
            break;
          printf(COLOR_MAGENTA " %02x " COLOR_OFF,cmd0[cmd0_end] );
          cmd0_end++;
        }

        write_string_to_handle(cli,cmd0,strlen(cmd0),Xhandle);

        {
            int s;
            struct thread_info* tinfo;
            tinfo = malloc(sizeof(struct thread_info));
            tinfo->cli    = cli;
            s = pthread_create(&tinfo->thread_id, NULL,
                               &loop_register_later, tinfo);
            printf("pthread create result = %d\n",s);
        }
        global_client = cli;
    }
}

static void ready_cb(bool success, uint8_t att_ecode, void *user_data)
{
  struct client *cli = user_data;

  if (!success) {
    PRLOG("GATT discovery procedures failed - error code: 0x%02x\n",
                att_ecode);
    return;
  }

  PRLOG("GATT discovery procedures complete\n");

  print_services(cli);
  print_prompt();

  inject_pk_hack(cli);

}

static void service_changed_cb(uint16_t start_handle, uint16_t end_handle,
                void *user_data)
{
  struct client *cli = user_data;

  printf("\nService Changed handled - start: 0x%04x end: 0x%04x\n",
            start_handle, end_handle);

  gatt_db_foreach_service_in_range(cli->db, NULL, print_service, cli,
            start_handle, end_handle);
  print_prompt();
}

static void services_usage(void)
{
  printf("Usage: services [options]\nOptions:\n"
    "\t -u, --uuid <uuid>\tService UUID\n"
    "\t -a, --handle <handle>\tService start handle\n"
    "\t -h, --help\t\tShow help message\n"
    "e.g.:\n"
    "\tservices\n\tservices -u 0x180d\n\tservices -a 0x0009\n");
}

static bool parse_args(char *str, int expected_argc,  char **argv, int *argc)
{
  char **ap;

  for (ap = argv; (*ap = strsep(&str, " \t")) != NULL;) {
    if (**ap == '\0')
      continue;

    (*argc)++;
    ap++;

    if (*argc > expected_argc)
      return false;
  }

  return true;
}

static void cmd_services(struct client *cli, char *cmd_str)
{
  char *argv[3];
  int argc = 0;

  if (!bt_gatt_client_is_ready(cli->gatt)) {
    printf("GATT client not initialized\n");
    return;
  }

  if (!parse_args(cmd_str, 2, argv, &argc)) {
    services_usage();
    return;
  }

  if (!argc) {
    print_services(cli);
    return;
  }

  if (argc != 2) {
    services_usage();
    return;
  }

  if (!strcmp(argv[0], "-u") || !strcmp(argv[0], "--uuid")) {
    bt_uuid_t tmp, uuid;

    if (bt_string_to_uuid(&tmp, argv[1]) < 0) {
      printf("Invalid UUID: %s\n", argv[1]);
      return;
    }

    bt_uuid_to_uuid128(&tmp, &uuid);

    print_services_by_uuid(cli, &uuid);
  } else if (!strcmp(argv[0], "-a") || !strcmp(argv[0], "--handle")) {
    uint16_t handle;
    char *endptr = NULL;

    handle = strtol(argv[1], &endptr, 0);
    if (!endptr || *endptr != '\0') {
      printf("Invalid start handle: %s\n", argv[1]);
      return;
    }

    print_services_by_handle(cli, handle);
  } else
    services_usage();
}

static void read_multiple_usage(void)
{
  printf("Usage: read-multiple <handle_1> <handle_2> ...\n");
}

static void read_multiple_cb(bool success, uint8_t att_ecode,
          const uint8_t *value, uint16_t length,
          void *user_data)
{
  int i;

  if (!success) {
    PRLOG("\nRead multiple request failed: 0x%02x\n", att_ecode);
    return;
  }

  printf("\nRead multiple value (%u bytes):", length);

  for (i = 0; i < length; i++)
    printf("%02x ", value[i]);

  PRLOG("\n");
}

static void cmd_read_multiple(struct client *cli, char *cmd_str)
{
  int argc = 0;
  uint16_t *value;
  char *argv[512];
  int i;
  char *endptr = NULL;

  if (!bt_gatt_client_is_ready(cli->gatt)) {
    printf("GATT client not initialized\n");
    return;
  }

  if (!parse_args(cmd_str, sizeof(argv), argv, &argc) || argc < 2) {
    read_multiple_usage();
    return;
  }

  value = malloc(sizeof(uint16_t) * argc);
  if (!value) {
    printf("Failed to construct value\n");
    return;
  }

  for (i = 0; i < argc; i++) {
    value[i] = strtol(argv[i], &endptr, 0);
    if (endptr == argv[i] || *endptr != '\0' || !value[i]) {
      printf("Invalid value byte: %s\n", argv[i]);
      free(value);
      return;
    }
  }

  if (!bt_gatt_client_read_multiple(cli->gatt, value, argc,
            read_multiple_cb, NULL, NULL))
    printf("Failed to initiate read multiple procedure\n");

  free(value);
}

static void read_value_usage(void)
{
  printf("Usage: read-value <value_handle>\n");
}

static void read_cb(bool success, uint8_t att_ecode, const uint8_t *value,
          uint16_t length, void *user_data)
{
  int i;

  if (!success) {
    PRLOG("\nRead request failed: %s (0x%02x)\n",
        ecode_to_string(att_ecode), att_ecode);
    return;
  }

  printf("\nRead value");

  if (length == 0) {
    PRLOG(": 0 bytes\n");
    return;
  }

  printf(" (%u bytes): ", length);

  for (i = 0; i < length; i++)
    printf("%02x ", value[i]);

  PRLOG("\n");
}

static void cmd_read_value(struct client *cli, char *cmd_str)
{
  char *argv[2];
  int argc = 0;
  uint16_t handle;
  char *endptr = NULL;

  if (!bt_gatt_client_is_ready(cli->gatt)) {
    printf("GATT client not initialized\n");
    return;
  }

  if (!parse_args(cmd_str, 1, argv, &argc) || argc != 1) {
    read_value_usage();
    return;
  }

  handle = strtol(argv[0], &endptr, 0);
  if (!endptr || *endptr != '\0' || !handle) {
    printf("Invalid value handle: %s\n", argv[0]);
    return;
  }

  if (!bt_gatt_client_read_value(cli->gatt, handle, read_cb,
                NULL, NULL))
    printf("Failed to initiate read value procedure\n");
}

static void read_long_value_usage(void)
{
  printf("Usage: read-long-value <value_handle> <offset>\n");
}

static void cmd_read_long_value(struct client *cli, char *cmd_str)
{
  char *argv[3];
  int argc = 0;
  uint16_t handle;
  uint16_t offset;
  char *endptr = NULL;

  if (!bt_gatt_client_is_ready(cli->gatt)) {
    printf("GATT client not initialized\n");
    return;
  }

  if (!parse_args(cmd_str, 2, argv, &argc) || argc != 2) {
    read_long_value_usage();
    return;
  }

  handle = strtol(argv[0], &endptr, 0);
  if (!endptr || *endptr != '\0' || !handle) {
    printf("Invalid value handle: %s\n", argv[0]);
    return;
  }

  endptr = NULL;
  offset = strtol(argv[1], &endptr, 0);
  if (!endptr || *endptr != '\0') {
    printf("Invalid offset: %s\n", argv[1]);
    return;
  }

  if (!bt_gatt_client_read_long_value(cli->gatt, handle, offset, read_cb,
                NULL, NULL))
    printf("Failed to initiate read long value procedure\n");
}

static void write_value_usage(void)
{
  printf("Usage: write-value [options] <value_handle> <value>\n"
    "Options:\n"
    "\t-w, --without-response\tWrite without response\n"
    "\t-s, --signed-write\tSigned write command\n"
    "e.g.:\n"
    "\twrite-value 0x0001 00 01 00\n");
}

static struct option write_value_options[] = {
  { "without-response",	0, 0, 'w' },
  { "signed-write",	0, 0, 's' },
  { }
};

static void write_cb(bool success, uint8_t att_ecode, void *user_data)
{
  if (success) {
    PRLOG("\nWrite successful\n");
  } else {
    PRLOG("\nWrite failed: %s (0x%02x)\n",
        ecode_to_string(att_ecode), att_ecode);
  }
}

static void cmd_write_value(struct client *cli, char *cmd_str)
{
  int opt, i;
  char *argvbuf[516];
  char **argv = argvbuf;
  int argc = 1;
  uint16_t handle;
  char *endptr = NULL;
  int length;
  uint8_t *value = NULL;
  bool without_response = false;
  bool signed_write = false;

  if (!bt_gatt_client_is_ready(cli->gatt)) {
    printf("GATT client not initialized\n");
    return;
  }

  if (!parse_args(cmd_str, 514, argv + 1, &argc)) {
    printf("Too many arguments\n");
    write_value_usage();
    return;
  }

  optind = 0;
  argv[0] = "write-value";
  while ((opt = getopt_long(argc, argv, "+ws", write_value_options,
                NULL)) != -1) {
    switch (opt) {
    case 'w':
      without_response = true;
      break;
    case 's':
      signed_write = true;
      break;
    default:
      write_value_usage();
      return;
    }
  }

  argc -= optind;
  argv += optind;

  if (argc < 1) {
    write_value_usage();
    return;
  }

  handle = strtol(argv[0], &endptr, 0);
  if (!endptr || *endptr != '\0' || !handle) {
    printf("Invalid handle: %s\n", argv[0]);
    return;
  }

  length = argc - 1;

  if (length > 0) {
    if (length > UINT16_MAX) {
      printf("Write value too long\n");
      return;
    }

    value = malloc(length);
    if (!value) {
      printf("Failed to construct write value\n");
      return;
    }

    for (i = 1; i < argc; i++) {
      if (strlen(argv[i]) != 2) {
        printf("Invalid value byte: %s\n",
                argv[i]);
        goto done;
      }

      value[i-1] = strtol(argv[i], &endptr, 0);
      if (endptr == argv[i] || *endptr != '\0'
              || errno == ERANGE) {
        printf("Invalid value byte: %s\n",
                argv[i]);
        goto done;
      }
    }
  }

  if (without_response) {
    if (!bt_gatt_client_write_without_response(cli->gatt, handle,
            signed_write, value, length)) {
      printf("Failed to initiate write without response "
                "procedure\n");
      goto done;
    }

    printf("Write command sent\n");
    goto done;
  }

  if (!bt_gatt_client_write_value(cli->gatt, handle, value, length,
                write_cb,
                NULL, NULL))
    printf("Failed to initiate write procedure\n");

done:
  free(value);
}

static void write_long_value_usage(void)
{
  printf("Usage: write-long-value [options] <value_handle> <offset> "
        "<value>\n"
        "Options:\n"
        "\t-r, --reliable-write\tReliable write\n"
        "e.g.:\n"
        "\twrite-long-value 0x0001 0 00 01 00\n");
}

static struct option write_long_value_options[] = {
  { "reliable-write",	0, 0, 'r' },
  { }
};

static void write_long_cb(bool success, bool reliable_error, uint8_t att_ecode,
                void *user_data)
{
  if (success) {
    PRLOG("Write successful\n");
  } else if (reliable_error) {
    PRLOG("Reliable write not verified\n");
  } else {
    PRLOG("\nWrite failed: %s (0x%02x)\n",
        ecode_to_string(att_ecode), att_ecode);
  }
}

static void cmd_write_long_value(struct client *cli, char *cmd_str)
{
  int opt, i;
  char *argvbuf[516];
  char **argv = argvbuf;
  int argc = 1;
  uint16_t handle;
  uint16_t offset;
  char *endptr = NULL;
  int length;
  uint8_t *value = NULL;
  bool reliable_writes = false;

  if (!bt_gatt_client_is_ready(cli->gatt)) {
    printf("GATT client not initialized\n");
    return;
  }

  if (!parse_args(cmd_str, 514, argv + 1, &argc)) {
    printf("Too many arguments\n");
    write_value_usage();
    return;
  }

  optind = 0;
  argv[0] = "write-long-value";
  while ((opt = getopt_long(argc, argv, "+r", write_long_value_options,
                NULL)) != -1) {
    switch (opt) {
    case 'r':
      reliable_writes = true;
      break;
    default:
      write_long_value_usage();
      return;
    }
  }

  argc -= optind;
  argv += optind;

  if (argc < 2) {
    write_long_value_usage();
    return;
  }

  handle = strtol(argv[0], &endptr, 0);
  if (!endptr || *endptr != '\0' || !handle) {
    printf("Invalid handle: %s\n", argv[0]);
    return;
  }

  endptr = NULL;
  offset = strtol(argv[1], &endptr, 0);
  if (!endptr || *endptr != '\0' || errno == ERANGE) {
    printf("Invalid offset: %s\n", argv[1]);
    return;
  }

  length = argc - 2;

  if (length > 0) {
    if (length > UINT16_MAX) {
      printf("Write value too long\n");
      return;
    }

    value = malloc(length);
    if (!value) {
      printf("Failed to construct write value\n");
      return;
    }

    for (i = 2; i < argc; i++) {
      if (strlen(argv[i]) != 2) {
        printf("Invalid value byte: %s\n",
                argv[i]);
        free(value);
        return;
      }

      value[i-2] = strtol(argv[i], &endptr, 0);
      if (endptr == argv[i] || *endptr != '\0'
              || errno == ERANGE) {
        printf("Invalid value byte: %s\n",
                argv[i]);
        free(value);
        return;
      }
    }
  }

  if (!bt_gatt_client_write_long_value(cli->gatt, reliable_writes, handle,
              offset, value, length,
              write_long_cb,
              NULL, NULL))
    printf("Failed to initiate long write procedure\n");

  free(value);
}

static void write_prepare_usage(void)
{
  printf("Usage: write-prepare [options] <value_handle> <offset> "
        "<value>\n"
        "Options:\n"
        "\t-s, --session-id\tSession id\n"
        "e.g.:\n"
        "\twrite-prepare -s 1 0x0001 00 01 00\n");
}

static struct option write_prepare_options[] = {
  { "session-id",		1, 0, 's' },
  { }
};

static void cmd_write_prepare(struct client *cli, char *cmd_str)
{
  int opt, i;
  char *argvbuf[516];
  char **argv = argvbuf;
  int argc = 0;
  unsigned int id = 0;
  uint16_t handle;
  uint16_t offset;
  char *endptr = NULL;
  unsigned int length;
  uint8_t *value = NULL;

  if (!bt_gatt_client_is_ready(cli->gatt)) {
    printf("GATT client not initialized\n");
    return;
  }

  if (!parse_args(cmd_str, 514, argv + 1, &argc)) {
    printf("Too many arguments\n");
    write_value_usage();
    return;
  }

  /* Add command name for getopt_long */
  argc++;
  argv[0] = "write-prepare";

  optind = 0;
  while ((opt = getopt_long(argc, argv , "s:", write_prepare_options,
                NULL)) != -1) {
    switch (opt) {
    case 's':
      if (!optarg) {
        write_prepare_usage();
        return;
      }

      id = atoi(optarg);

      break;
    default:
      write_prepare_usage();
      return;
    }
  }

  argc -= optind;
  argv += optind;

  if (argc < 3) {
    write_prepare_usage();
    return;
  }

  if (cli->reliable_session_id != id) {
    printf("Session id != Ongoing session id (%u!=%u)\n", id,
            cli->reliable_session_id);
    return;
  }

  handle = strtol(argv[0], &endptr, 0);
  if (!endptr || *endptr != '\0' || !handle) {
    printf("Invalid handle: %s\n", argv[0]);
    return;
  }

  endptr = NULL;
  offset = strtol(argv[1], &endptr, 0);
  if (!endptr || *endptr != '\0' || errno == ERANGE) {
    printf("Invalid offset: %s\n", argv[1]);
    return;
  }

  /*
   * First two arguments are handle and offset. What remains is the value
   * length
   */
  length = argc - 2;

  if (length == 0)
    goto done;

  if (length > UINT16_MAX) {
    printf("Write value too long\n");
    return;
  }

  value = malloc(length);
  if (!value) {
    printf("Failed to allocate memory for value\n");
    return;
  }

  for (i = 2; i < argc; i++) {
    if (strlen(argv[i]) != 2) {
      printf("Invalid value byte: %s\n", argv[i]);
      free(value);
      return;
    }

    value[i-2] = strtol(argv[i], &endptr, 0);
    if (endptr == argv[i] || *endptr != '\0' || errno == ERANGE) {
      printf("Invalid value byte: %s\n", argv[i]);
      free(value);
      return;
    }
  }

done:
  cli->reliable_session_id =
      bt_gatt_client_prepare_write(cli->gatt, id,
              handle, offset,
              value, length,
              write_long_cb, NULL,
              NULL);
  if (!cli->reliable_session_id)
    printf("Failed to proceed prepare write\n");
  else
    printf("Prepare write success.\n"
        "Session id: %d to be used on next write\n",
            cli->reliable_session_id);

  free(value);
}

static void write_execute_usage(void)
{
  printf("Usage: write-execute <session_id> <execute>\n"
        "e.g.:\n"
        "\twrite-execute 1 0\n");
}

static void cmd_write_execute(struct client *cli, char *cmd_str)
{
  char *argvbuf[516];
  char **argv = argvbuf;
  int argc = 0;
  char *endptr = NULL;
  unsigned int session_id;
  bool execute;

  if (!bt_gatt_client_is_ready(cli->gatt)) {
    printf("GATT client not initialized\n");
    return;
  }

  if (!parse_args(cmd_str, 514, argv, &argc)) {
    printf("Too many arguments\n");
    write_value_usage();
    return;
  }

  if (argc < 2) {
    write_execute_usage();
    return;
  }

  session_id = strtol(argv[0], &endptr, 0);
  if (!endptr || *endptr != '\0') {
    printf("Invalid session id: %s\n", argv[0]);
    return;
  }

  if (session_id != cli->reliable_session_id) {
    printf("Invalid session id: %u != %u\n", session_id,
            cli->reliable_session_id);
    return;
  }

  execute = !!strtol(argv[1], &endptr, 0);
  if (!endptr || *endptr != '\0') {
    printf("Invalid execute: %s\n", argv[1]);
    return;
  }

  if (execute) {
    if (!bt_gatt_client_write_execute(cli->gatt, session_id,
              write_cb, NULL, NULL))
      printf("Failed to proceed write execute\n");
  } else {
    bt_gatt_client_cancel(cli->gatt, session_id);
  }

  cli->reliable_session_id = 0;
}

static void register_notify_usage(void)
{
  printf("Usage: register-notify <chrc value handle>\n");
}

static void notify_cb(uint16_t value_handle, const uint8_t *value,
          uint16_t length, void *user_data)
{
  int  i;
  int  state;
  bool printf_enabled;

  fflush(stderr);
  fflush(stdout);

  state = atomic_load( &RegistrationState );
  while( 0 != state )
  {
      usleep(1000*5);
      state = atomic_load( &RegistrationState );
      fprintf(stderr," state = %d ...\n", state);
      fflush(stderr);
  }

  printf_enabled = global_client->rwcfg.PRINT_NOTIFY_CB;
  if(printf_enabled)
  {
      printf(" global_serv->port=%d ... ",global_server->port);
      printf("NOTIFY_CB: ");
      printf("\n HandleValue Not/Ind: 0x%04x - ", value_handle);
  }

  if (length == 0)
  {
    PRLOG("(0 bytes)\n");
    return;
  }

  if(printf_enabled)
    printf("(%u bytes): ", length);

  for (i = 0; i < length; i++)
  {
    global_server->buffer[ global_server->line_len ] = value[i];
    global_server->line_len += 1;

    if(printf_enabled)
      printf("%02x ", value[i]);

    if( (i>0) && (value[i-1]==0x0d) && (value[i]==0x0a) ) // if line ends with CR LF
    {
        int iWrote;
        if(0)
            printf("got new line of length %03d\n",global_server->line_len);
        iWrote = tcpip_server_write_line( global_server );

        if(iWrote == global_server->line_len)
          global_server->line_len = 0;
    }
  }
  if(printf_enabled)
    PRLOG("\n");

}

static void register_notify_cb(uint16_t att_ecode, void *user_data)
{
  if (att_ecode) {
    PRLOG("Failed to register notify handler "
          "- error code: 0x%02x\n", att_ecode);
    return;
  }

  PRLOG("Registered notify handler!");
}

void cmd_register_notify(struct client *cli, char *cmd_str)
{
  char *argv[2];
  int argc = 0;
  uint16_t value_handle;
  unsigned int id;
  char *endptr = NULL;

  printf(COLOR_MAGENTA "register notify got cmd_str=%s\n" COLOR_OFF, cmd_str);

  if (!bt_gatt_client_is_ready(cli->gatt)) {
    printf("GATT client not initialized\n");
    return;
  }

  if (!parse_args(cmd_str, 1, argv, &argc) || argc != 1) {
    register_notify_usage();
    return;
  }

  value_handle = strtol(argv[0], &endptr, 0);
  if (!endptr || *endptr != '\0' || !value_handle) {
    printf("Invalid value handle: %s\n", argv[0]);
    return;
  }

  id = bt_gatt_client_register_notify(cli->gatt, value_handle,
              register_notify_cb,
              notify_cb, NULL, NULL);
  if (!id) {
    printf("Failed to register notify handler\n");
    return;
  }

  PRLOG("Registering notify handler with id: %u\n", id);
}

static void unregister_notify_usage(void)
{
  printf("Usage: unregister-notify <notify id>\n");
}

static void cmd_unregister_notify(struct client *cli, char *cmd_str)
{
  char *argv[2];
  int argc = 0;
  unsigned int id;
  char *endptr = NULL;

  if (!bt_gatt_client_is_ready(cli->gatt)) {
    printf("GATT client not initialized\n");
    return;
  }

  if (!parse_args(cmd_str, 1, argv, &argc) || argc != 1) {
    unregister_notify_usage();
    return;
  }

  id = strtol(argv[0], &endptr, 0);
  if (!endptr || *endptr != '\0' || !id) {
    printf("Invalid notify id: %s\n", argv[0]);
    return;
  }

  if (!bt_gatt_client_unregister_notify(cli->gatt, id)) {
    printf("Failed to unregister notify handler with id: %u\n", id);
    return;
  }

  printf("Unregistered notify handler with id: %u\n", id);
}

static void set_security_usage(void)
{
  printf("Usage: set_security <level>\n"
    "level: 1-3\n"
    "e.g.:\n"
    "\tset-sec-level 2\n");
}

static void cmd_set_security(struct client *cli, char *cmd_str)
{
  char *argvbuf[1];
  char **argv = argvbuf;
  int argc = 0;
  char *endptr = NULL;
  int level;

  if (!bt_gatt_client_is_ready(cli->gatt)) {
    printf("GATT client not initialized\n");
    return;
  }

  if (!parse_args(cmd_str, 1, argv, &argc)) {
    printf("Too many arguments\n");
    set_security_usage();
    return;
  }

  if (argc < 1) {
    set_security_usage();
    return;
  }

  level = strtol(argv[0], &endptr, 0);
  if (!endptr || *endptr != '\0' || level < 1 || level > 3) {
    printf("Invalid level: %s\n", argv[0]);
    return;
  }

  if (!bt_gatt_client_set_security(cli->gatt, level))
    printf("Could not set sec level\n");
  else
    printf("Setting security level %d success\n", level);
}

static void cmd_get_security(struct client *cli, char *cmd_str)
{
  int level;

  if (!bt_gatt_client_is_ready(cli->gatt)) {
    printf("GATT client not initialized\n");
    return;
  }

  level = bt_gatt_client_get_security(cli->gatt);
  if (level < 0)
    printf("Could not set sec level\n");
  else
    printf("Security level: %u\n", level);
}

static bool convert_sign_key(char *optarg, uint8_t key[16])
{
  int i;

  if (strlen(optarg) != 32) {
    printf("sign-key length is invalid\n");
    return false;
  }

  for (i = 0; i < 16; i++) {
    if (sscanf(optarg + (i * 2), "%2hhx", &key[i]) != 1)
      return false;
  }

  return true;
}

static void set_sign_key_usage(void)
{
  printf("Usage: set-sign-key [options]\nOptions:\n"
    "\t -c, --sign-key <csrk>\tCSRK\n"
    "e.g.:\n"
    "\tset-sign-key -c D8515948451FEA320DC05A2E88308188\n");
}

static bool local_counter(uint32_t *sign_cnt, void *user_data)
{
  static uint32_t cnt = 0;

  *sign_cnt = cnt++;

  return true;
}

static void cmd_set_sign_key(struct client *cli, char *cmd_str)
{
  char *argv[3];
  int argc = 0;
  uint8_t key[16];

  memset(key, 0, 16);

  if (!parse_args(cmd_str, 2, argv, &argc)) {
    set_sign_key_usage();
    return;
  }

  if (argc != 2) {
    set_sign_key_usage();
    return;
  }

  if (!strcmp(argv[0], "-c") || !strcmp(argv[0], "--sign-key")) {
    if (convert_sign_key(argv[1], key))
      bt_att_set_local_key(cli->att, key, local_counter, cli);
  } else
    set_sign_key_usage();
}

static void cmd_help(struct client *cli, char *cmd_str);

static void cmd_exit(struct client *cli, char *cmd_str);

typedef void (*command_func_t)(struct client *cli, char *cmd_str);

static struct {
  char *cmd;
  command_func_t func;
  char *doc;
} command[] = {
  { "help", cmd_help, "\tDisplay help message" },
  { "services", cmd_services, "\tShow discovered services" },
  { "read-value", cmd_read_value,
        "\tRead a characteristic or descriptor value" },
  { "read-long-value", cmd_read_long_value,
    "\tRead a long characteristic or desctriptor value" },
  { "read-multiple", cmd_read_multiple, "\tRead Multiple" },
  { "write-value", cmd_write_value,
      "\tWrite a characteristic or descriptor value" },
  { "write-long-value", cmd_write_long_value,
      "Write long characteristic or descriptor value" },
  { "write-prepare", cmd_write_prepare,
      "\tWrite prepare characteristic or descriptor value" },
  { "write-execute", cmd_write_execute,
      "\tExecute already prepared write" },
  { "register-notify", cmd_register_notify,
      "\tSubscribe to not/ind from a characteristic" },
  { "unregister-notify", cmd_unregister_notify,
            "Unregister a not/ind session"},
  { "set-security", cmd_set_security,
        "\tSet security level on le connection"},
  { "get-security", cmd_get_security,
        "\tGet security level on le connection"},
  { "set-sign-key", cmd_set_sign_key,
        "\tSet signing key for signed write command"},
  { "exit", cmd_exit,
        "\tExit this program"},
  { }
};

static void cmd_help(struct client *cli, char *cmd_str)
{
  int i;

  printf("Commands:\n");
  for (i = 0; command[i].cmd; i++)
    printf("\t%-15s\t%s\n", command[i].cmd, command[i].doc);
}

static void cmd_exit(struct client *cli, char *cmd_str)
{
    mainloop_exit_success();
    printf(COLOR_GREEN "invoked mainloop_exit_succes...\n" COLOR_OFF );
}

static void prompt_read_cb(int fd, uint32_t events, void *user_data)
{
  ssize_t read;
  size_t len = 0;
  char *line = NULL;
  char *cmd = NULL, *args;
  struct client *cli = user_data;
  int i;

  if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
    mainloop_quit();
    return;
  }

  if ((read = getline(&line, &len, stdin)) == -1)
    return;

  if (read <= 1) {
    cmd_help(cli, NULL);
    print_prompt();
    return;
  }

  line[read-1] = '\0';
  args = line;

  while ((cmd = strsep(&args, " \t")))
    if (*cmd != '\0')
      break;

  if (!cmd)
    goto failed;

  for (i = 0; command[i].cmd; i++) {
    if (strcmp(command[i].cmd, cmd) == 0)
      break;
  }

  if (command[i].cmd)
    command[i].func(cli, args);
  else
    fprintf(stderr, "Unknown command: %s\n", line);

failed:
  print_prompt();

  free(line);
}


static void signal_cb(int signum, void *user_data)
{
  switch (signum) {
  case SIGINT:
  case SIGTERM:
    mainloop_quit();
    break;
//  case SIGUSR1:
//      block_for_deferred_registration();
//      break;
  default:
    break;
  }
}

static int l2cap_le_att_connect(bdaddr_t *src, bdaddr_t *dst, uint8_t dst_type,
                  int sec)
{
  int sock;
  struct sockaddr_l2 srcaddr, dstaddr;
  struct bt_security btsec;

  if (verbose) {
    char srcaddr_str[18], dstaddr_str[18];

    ba2str(src, srcaddr_str);
    ba2str(dst, dstaddr_str);

    printf("btgatt-client: Opening L2CAP LE connection on ATT "
          "channel:\n\t src: %s\n\tdest: %s\n",
          srcaddr_str, dstaddr_str);
  }

  sock = socket(PF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
  if (sock < 0) {
    perror("Failed to create L2CAP socket");
    return -1;
  }

  /* Set up source address */
  memset(&srcaddr, 0, sizeof(srcaddr));
  srcaddr.l2_family = AF_BLUETOOTH;
  srcaddr.l2_cid = htobs(ATT_CID);
  srcaddr.l2_bdaddr_type = 0;
  bacpy(&srcaddr.l2_bdaddr, src);

  if (bind(sock, (struct sockaddr *)&srcaddr, sizeof(srcaddr)) < 0) {
    perror("Failed to bind L2CAP socket");
    close(sock);
    return -1;
  }

  /* Set the security level */
  memset(&btsec, 0, sizeof(btsec));
  btsec.level = sec;
  if (setsockopt(sock, SOL_BLUETOOTH, BT_SECURITY, &btsec,
              sizeof(btsec)) != 0) {
    fprintf(stderr, "Failed to set L2CAP security level\n");
    close(sock);
    return -1;
  }

  /* Set up destination address */
  memset(&dstaddr, 0, sizeof(dstaddr));
  dstaddr.l2_family = AF_BLUETOOTH;
  dstaddr.l2_cid = htobs(ATT_CID);
  dstaddr.l2_bdaddr_type = dst_type;
  bacpy(&dstaddr.l2_bdaddr, dst);

  printf("Connecting to device XXX...");
  fflush(stdout);

  if (connect(sock, (struct sockaddr *) &dstaddr, sizeof(dstaddr)) < 0) {
    perror(" Failed to connect");
    close(sock);
    return -1;
  }

  printf(" Done\n");

  return sock;
}

/////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{   
  int opt;
  int sec = BT_SECURITY_LOW;
  uint16_t mtu = 0;
  uint8_t dst_type = BDADDR_LE_PUBLIC;
  bool dst_addr_given = false;
  bdaddr_t src_addr, dst_addr;
  int dev_id = -1;
  int fd;
  sigset_t mask;
  unsigned int temp_ui;

  /////////////////////////////////////
  struct client *cli;
  struct tcpip_server* PKserv;
  struct readwrite_config  RWcfgTemp;

  PKserv        = tcpip_server_create();

  fprintf(stdout, "%s  %s @ %d\n",__DATE__,__FILE__,__LINE__);

  initialize_rwcfg(&RWcfgTemp);

  while ((opt = getopt_long(argc, argv, "+hvs:m:nt:d:i:P:W:N:C:Z:",
            main_options, NULL)) != -1) {
    switch (opt) {
      case 'h':
        usage();
        return EXIT_SUCCESS;
      case 'v':
        verbose = true;
        break;
      case 'n':
        RWcfgTemp.PRINT_NOTIFY_CB = true;
        atomic_store(&PromptPrintState,1);
        break;
      case 'P':
        RWcfgTemp.tcpip_Port = atoi(optarg);
        if(RWcfgTemp.tcpip_Port<=0)
          RWcfgTemp.tcpip_Enabled = 0;
        break;
      case 'W':        
        sscanf(optarg,"0x%04x",&temp_ui);
        RWcfgTemp.handle_Write = temp_ui;
        printf("got value: %d for write handle\n",RWcfgTemp.handle_Write);
        break;
      case 'N':
        sscanf(optarg,"0x%04x",&temp_ui);
        RWcfgTemp.handle_Read = temp_ui;
        printf("got value: %d for notify/read handle\n",RWcfgTemp.handle_Read);
        break;
      case 'C':
        cmd_from_arg(&RWcfgTemp,optarg);
        break;
      case 'Z':
        RWcfgTemp.tcpip_PacketSize = atoi(optarg);
        break;
      case 's':
        if (strcmp(optarg, "low") == 0)
          sec = BT_SECURITY_LOW;
        else if (strcmp(optarg, "medium") == 0)
          sec = BT_SECURITY_MEDIUM;
        else if (strcmp(optarg, "high") == 0)
          sec = BT_SECURITY_HIGH;
        else {
          fprintf(stderr, "Invalid security level\n");
          return EXIT_FAILURE;
        }
        break;
      case 'm': {
        int arg;

        arg = atoi(optarg);
        if (arg <= 0) {
          fprintf(stderr, "Invalid MTU: %d\n", arg);
          return EXIT_FAILURE;
        }

        if (arg > UINT16_MAX) {
          fprintf(stderr, "MTU too large: %d\n", arg);
          return EXIT_FAILURE;
        }

        mtu = (uint16_t)arg;
        break;
      }
      case 't':
        if (strcmp(optarg, "random") == 0)
          dst_type = BDADDR_LE_RANDOM;
        else if (strcmp(optarg, "public") == 0)
          dst_type = BDADDR_LE_PUBLIC;
        else {
          fprintf(stderr,
                  "Allowed types: random, public\n");
          return EXIT_FAILURE;
        }
        break;
      case 'd':
        if (str2ba(optarg, &dst_addr) < 0) {
          fprintf(stderr, "Invalid remote address: %s\n",
                  optarg);
          return EXIT_FAILURE;
        }

        dst_addr_given = true;
        break;

      case 'i':
        dev_id = hci_devid(optarg);
        if (dev_id < 0) {
          perror("Invalid adapter");
          return EXIT_FAILURE;
        }

        break;
      default:
        fprintf(stderr, "Invalid option: %c\n", opt);
        return EXIT_FAILURE;
    }
  }

  if (!argc) {
    usage();
    return EXIT_SUCCESS;
  }

  argc -= optind;
  argv += optind;
  optind = 0;

  if (argc) {
    usage();
    return EXIT_SUCCESS;
  }

  if (dev_id == -1)
    bacpy(&src_addr, BDADDR_ANY);
  else if (hci_devba(dev_id, &src_addr) < 0) {
    perror("Adapter not available");
    return EXIT_FAILURE;
  }

  if (!dst_addr_given) {
    fprintf(stderr, "Destination address required!\n");
    return EXIT_FAILURE;
  }

  /// configure the tcpip parameters
  global_server = setup_tcpip_server(PKserv,&RWcfgTemp);

  mainloop_init();

  fd = l2cap_le_att_connect(&src_addr, &dst_addr, dst_type, sec);
  if (fd < 0)
    return EXIT_FAILURE;

  cli = client_create(fd, mtu);
  if (!cli)
  {
    close(fd);
    return EXIT_FAILURE;
  }

  /// copy the read-write-config parameters to CLI instance
  clone_rwcfg(&RWcfgTemp,&cli->rwcfg);

  /// register callback handler
  if (mainloop_add_fd(fileno(stdin),
        EPOLLIN | EPOLLRDHUP | EPOLLHUP | EPOLLERR,
        prompt_read_cb /*register our callback*/ , cli, NULL) < 0)
  {
    fprintf(stderr, "Failed to initialize console\n");
    return EXIT_FAILURE;
  }

  sigemptyset(&mask);
  sigaddset(&mask, SIGINT);
  sigaddset(&mask, SIGTERM);  

  mainloop_set_signal(&mask, signal_cb, NULL, NULL);

  print_prompt();

  mainloop_run();

  printf("\n\nShutting down...\n");

  client_destroy(cli);

  tcpip_server_destroy(PKserv);

  return EXIT_SUCCESS;
}






