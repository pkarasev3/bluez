#include "pk-btgatt-rwcfg.h"
#include "src/shared/att-types.h"
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
  arg->PRINT_NOTIFY_CB  = false;

  memset(&arg->next_WriteValues[0],0,sizeof(arg->next_WriteValues));
}

static void  set_command_string_to_null(struct readwrite_config* arg)
{
  memset(arg->next_WriteValues,0, sizeof(arg->next_WriteValues));
}

//! @author{yasser boumenir} @author{peter karasev}
size_t cmd_from_arg(struct readwrite_config* arg, const uint8_t* cmd)
{
  size_t i;
  if(!cmd) {
    usage();
    return 0;
  }

  set_command_string_to_null(arg); // first get a clean slate...

  for(i=0;i<(sizeof(arg->next_WriteValues)-2);i++)
  {
    if( 0 == cmd[i] )
      break;
    arg->next_WriteValues[i] = cmd[i]; // then fill in our garbage...
  }
  arg->next_WriteValues[i++] = 0x0d;
  arg->next_WriteValues[i++] = 0x0a;
  printf(COLOR_BLUE "from %s string, generated hex commands = %s ... \n" COLOR_OFF,
         cmd, arg->next_WriteValues);
  return i;
}

void clone_rwcfg(const struct readwrite_config* src,struct readwrite_config* dst)
{
  dst->handle_Read=src->handle_Read;
  dst->handle_Write=src->handle_Write;
  dst->tcpip_Enabled=src->tcpip_Enabled;
  dst->tcpip_PacketSize=src->tcpip_PacketSize;
  dst->tcpip_Port=src->tcpip_Port;
  dst->PRINT_NOTIFY_CB=src->PRINT_NOTIFY_CB;

  memcpy(&dst->next_WriteValues[0],&src->next_WriteValues[0],sizeof(src->next_WriteValues));
}

size_t put_string_as_hex_values_command(struct readwrite_config* arg, const uint8_t* cmd)
{
  return cmd_from_arg(arg,cmd);
}


const char *ecode_to_string(uint8_t ecode)
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



//////////////////////////////////////////////

void usage()
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


////////////////////////////

bool parse_args(char* str, int expected_argc, char** argv, int* argc)
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

/**                      ..,co88oc.oo8888cc,..
  o8o.               ..,o8889689ooo888o"88/88888oooc..
.88888             .o888896888".8*8*8#88o'?88/8*8*8#889ooo....
a888P          ..c6888969""..,"o8*8*8#888o.?8888\88\88"".ooo8888oo.
088P        ..atc88889"".,oo8o.868*8*8#888o 889\8|89",o8*8*8#8*8*8#.
888t  ...coo688889"'.ooo88o88b.'86988988889 8688\88'o8888896989^888o
 8*8*8#8*8*8#"..ooo88896888/888  "9o688888' "888988 8888868888'o88888
  ""G8889""'ooo8*8*8#888/88889 .d8o9889""'   "8688o."88888988"o8*8*8#o .
           o8888'""""""""""'   o8688"          88868. 8*8*8#.68988888"o8o.
           88888o.              "8888ooo.        '8888. 88888.8898888o"888o.
           "8*!*8#!'             "8*!*8#!'          '""8o"8888.886988?\o8888o .
      . :.:::::::::::.: .     . :.::::::::.: .   . : ::.:."8888 "88888>>$*\88o
####################################################     :..8888,. "8*8*8#88888.
                                                        .:o888.o8o.  "866o9888o
                                                         :888.o8888.  "88$$&9".
                                                        . 89  8*8*8#    "88":.
                                                   :.    '!#8#!o!
                                                         .       "8888..
                                                                   8*8*8#o.
                                                                    "!#!#!#,
                                                             . : :.:::::::.: :.   */
