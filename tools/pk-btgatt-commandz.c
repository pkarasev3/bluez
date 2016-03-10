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
#include "src/shared/att.h"
#include "src/shared/queue.h"
#include "src/shared/gatt-db.h"
#include "src/shared/gatt-client.h"

#include "tools/pk-btgatt-rwcfg.h"
#include "tools/pk-btgatt-commandz.h"

#include <getopt.h>

int NoArg = no_argument;
int RequiredArg = required_argument;
int OptionalArg = optional_argument;

//! @note{Setting Options is very confusing;
//!       see this page: http://linux.die.net/man/3/getopt_long    }


struct option write_string_options[] =
{
  { "write-to-handle",        1, 0, 'W' },
  { "delay-for-milliseconds", 1, 0, 'd' },
  { }
};

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

void send_stringAsHexValueSequence(struct client* cli)
{
  int cmd0_end       = 0;
  uint16_t Xhandle   = cli->rwcfg.handle_Write; //0x0015;
  const char* cmd0   = cli->rwcfg.next_WriteValues;
  printf("\n cmd0 going to write to: 0x%04x \n", Xhandle);
  while( cmd0_end >= 0 )
  {
    if(cmd0[cmd0_end] == 0)
      break;
    printf(COLOR_MAGENTA " %02x " COLOR_OFF,cmd0[cmd0_end] );
    cmd0_end++;
  }

  write_string_to_handle(cli,cmd0,strlen(cmd0),Xhandle);
}



void cmd_write_string(struct client *cli, char *cmd_str)
{
  int option_index;
  char *argvbuf[516];
  char **argv = argvbuf;
  int argc = 1;
  struct readwrite_config* rwcfg;
  uint16_t handle;
  char *endptr = NULL;
  int remaining_length;
  uint8_t *command_string = NULL;
  unsigned int temp_ui;

  if (!bt_gatt_client_is_ready(cli->gatt))
    RETURN_PRINTF_ERROR("GATT client not initialized; did not send command",cmd_str);

  if (!parse_args(cmd_str, 514, argv + 1, &argc))
    RETURN_PRINTF_ERROR("Too many arguments in command: \n",cmd_str);

  rwcfg   = &(cli->rwcfg);
  temp_ui = rwcfg->handle_Write;
  optind  = 0;

  argv[0] = str2hex_cmd;

  while(1) {
    int opt_result = 0;
    opt_result = getopt_long(argc, argv,
                       "W:", write_string_options,
                             &option_index);
    printf( "optarg=%s, optind = %d, opt_result = %d, opts index = %d, argv[index] = %s \n",optarg, optind, opt_result, option_index, argv[option_index]);
    switch (opt_result)
    {
    case 'W':
        sscanf(optarg,"0x%04x",&temp_ui);
        break;
    case 'd':
        usleep( 1000 * atoi(optarg) );
        break;
    default:
        break;
    }

    if( opt_result < 0 )
      break;
  }

  rwcfg->handle_Write = temp_ui;
  printf("options index = %02d \n    setting write handle: %d \n",option_index, rwcfg->handle_Write);

  argc -= optind;
  argv += optind;

  if (argc < 1) {
    write_string_usage();
    return;
  }

  if( temp_ui > 0 )
    handle = temp_ui;
  else
  {
    handle = strtol(argv[0], &endptr, 0);
    if (!endptr || *endptr != '\0' || !handle) {
      printf("Invalid handle: %s\n", argv[0]);
      return;
    }
  }

  remaining_length                 = strlen(argvbuf[optind]); //whats left in argv after we stripped out command itself and -W  0xYZXW  "remainder"
  command_string                   = malloc(remaining_length+1);
  command_string[remaining_length] = 0;

  printf(COLOR_MAGENTA " remaining length of cmd: %d \n     Extracting from remaining argv:    %s \n" COLOR_OFF,
         remaining_length, argvbuf[optind]);
  memcpy(command_string, argvbuf[optind], remaining_length);

  put_string_as_hex_values_command( rwcfg, command_string );

  if( &cli->rwcfg != rwcfg) {
    clone_rwcfg(rwcfg, &cli->rwcfg);
    printf(COLOR_RED  "mismatched RWconfig objects!?\n" COLOR_OFF);
  }

  send_stringAsHexValueSequence(cli);

  free(command_string);

}

#define PRLOG(...) \
     {printf(__VA_ARGS__); \
      fflush(stdout); fflush(stderr); \
      printf(COLOR_BLUE "#" COLOR_OFF);}

void write_cb(bool success, uint8_t att_ecode, void *user_data)
{
  if (success) {
    PRLOG("\nWrite successful\n");
  } else {
    PRLOG("\nWrite failed: %s (0x%02x)\n",
        ecode_to_string(att_ecode), att_ecode);
  }
}

void write_string_usage()
{
  printf("Usage: " str2hex_cmd " -W  <value_handle>   <command string>\n"
    " Overview: "  str2hex_doc "\n"
    " Options:\n"
    "\t-W, set write handle (e.g., make it different than it was at startup)\n"
    "Example: \n"
    "\t" str2hex_cmd "  -W  0x0666  transmogrify\n");
}
