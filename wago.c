#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <microhttpd.h>
#include <stdio.h>
#include <stdlib.h>
#include <byteswap.h>
#include "wago.h"
#include "typedefs.h"
#include <unistd.h>
#include <modbus/modbus.h>
#include <string.h>


static const uint16_t simulated_adc_reading[] =
{
	0x0000,
	0x1000,
	0x2000,
	0x3000,
	0x4000,
	0x5000,
	0x6000,
	0x7000,
	0x7FFF
};

// Private data
static modbus_t* ctx;
static modbus_mapping_t *mb_mapping;
static uint16_t analogue_output_segment = 0;
static uint16_t analogue_input_segment  = 0;
static int port = 1502;
static char page[1024];
static bool debug = false;


const char *get_page()
{
    sprintf(page,
        "<html> " \
        "<pre>" \
            "<body>  \t\t\t\t <b>analogue input segment:</b> 0x%04X\n</body>" \
        "</pre>" \
        "<pre>" \
            "<body>  \t\t\t\t <b>analogue output segment:</b> 0x%04X\n</body>" \
        "</pre>" \
        "</html>", analogue_input_segment, analogue_output_segment);

    return page;
}

// Enable or disable debug
//
int process_read_enableDebug ()
{
    uint16_t *address;
    uint16_t address_offset;
    int retval = MODBUS_SUCCESS; // need to figure out what this constant is

    if (debug)printf("%s\n", __PRETTY_FUNCTION__);

    address_offset = mb_mapping->start_registers + enableDebug;
    address = mb_mapping->tab_registers + address_offset;
    *address = debug;        // TBD

    return retval;
}

//
// Enable or disable debug
//
int process_write_enableDebug (uint16_t value)
{
    int retval = MODBUS_SUCCESS;
    if (debug)printf("%s - %s\n", __PRETTY_FUNCTION__, (value & 0x0001)?"TRUE":"FALSE");
    debug = value ? true : false;
    return retval;
}


int process_input_analogue_segment()
{
    uint16_t *address;
    uint16_t address_offset;
    int retval = MODBUS_SUCCESS; // need to figure out what this constant is

    if (debug)printf("%s\n", __PRETTY_FUNCTION__);
    address_offset = mb_mapping->start_registers + analogue_segment;
    address = mb_mapping->tab_registers + address_offset;
    *address = analogue_input_segment;        // TBD

    return retval;
}

int process_constant_register4()
{
    uint16_t *address;
    uint16_t address_offset;
    int retval = MODBUS_SUCCESS; // need to figure out what this constant is

    if (debug)printf("%s\n", __PRETTY_FUNCTION__);
    address_offset = mb_mapping->start_registers + constant_register4;
    address = mb_mapping->tab_registers + address_offset;
    *address = 0x1234;        // TBD

    return retval;
}

int process_output_analogue_segment(uint16_t value)
{
    int retval = MODBUS_SUCCESS; // need to figure out what this constant is

    if (debug)printf("%s - value %d\n", __PRETTY_FUNCTION__, value);
    analogue_output_segment = value;

    return retval;
}

int process_read_register(uint16_t address)
{
    int retval = 0;

    switch ( address )
    {
    case analogue_segment:
        process_input_analogue_segment();
        break;

    case enableDebug:
    	process_read_enableDebug();
        break;

    case constant_register4:
        process_constant_register4();
        break;

    default:
        retval = MODBUS_EXCEPTION_ILLEGAL_DATA_ADDRESS;
        break;
    }
    return retval;
}

int process_write_register(uint16_t address, uint16_t data)
{
    int retval = 0;

    switch ( address )
    {
    case analogue_segment:
        process_output_analogue_segment(data);
        break;

    case enableDebug:
    	process_write_enableDebug(data);
    	break;

    default:
        retval = MODBUS_EXCEPTION_ILLEGAL_DATA_ADDRESS;
        break;
    }
    return retval;
}


/*
***************************************************************************************************************
 \fn      process_handler(uint8_t* pdata)
 \brief   processess all incoming commands

 Process all input commands. The Modbus function code 0x17 which is not standard seems to exhibit non standaard
 data structure seen not belows.

 \note

      MODBUS_FC_READ_HOLDING_REGISTERS
      MODBUS_FC_WRITE_SINGLE_REGISTER - has the following data format
      ------------------------------------------------
      | TID | PID | LEN | UID | FC | [W|R]S | [W|R]Q |
      ------------------------------------------------
      0     1     3     5     7    8        11       13

      MODBUS_FC_WRITE_MULTIPLE_REGISTERS - has the following data format operation
      -------------------------------------------------------
      | TID | PID | LEN | UID | FC | WS | WQ | WC | WR x nn |
      -------------------------------------------------------
      0     1     3     5     7    8    11   13   14

      MODBUS_FC_WRITE_AND_READ_REGISTERS - has the following data format
      -----------------------------------------------------------------
      | TID | PID | LEN | UID | FC | RS | RQ | WS | WQ | WC | WR x nn |
      -----------------------------------------------------------------
      0     1     3     5     7    8    11   13   15   17   18

      TID = Transaction Id, PID = Protocol Id, LEN = Total length of message, UID = unit Id
      FC = Function Code, RS = Read start address, RQ = Read quantity, WS = Write start address,
      WQ = Write quantity, WC = Write count, WR = Write Register. nn => WQ x 2 bytes
**************************************************************************************************************
*/

void process_query(modbus_pdu_t* mb)
{
    const int convert_bytes2word_value = 256;
    int i,j,retval = MODBUS_EXCEPTION_ILLEGAL_DATA_ADDRESS;
    uint16_t address,value,count;
    int len = __bswap_16(mb->mbap.length) - 2; // len - fc - unit_id
    uint8_t fc;

    fc = mb->fcode;
    switch ( fc ){
    case MODBUS_FC_READ_HOLDING_REGISTERS:
        address = (mb->data[i++] * convert_bytes2word_value) + mb->data[i++]; // address
        retval = process_read_register(address);
        break;

    case MODBUS_FC_WRITE_SINGLE_REGISTER:
        address = (mb->data[i++] * convert_bytes2word_value) + mb->data[i++]; // address
        value   = (mb->data[i++] * convert_bytes2word_value) + mb->data[i++]; // data
        retval  = process_write_register(address, value);
        break;

    default:
        retval = MODBUS_EXCEPTION_ILLEGAL_FUNCTION;
        break;
    }

    if ( retval == MODBUS_SUCCESS)
        modbus_reply(ctx, (uint8_t*)mb, sizeof(mbap_header_t) + sizeof(fc) + len, mb_mapping); // subtract function code
    else
       modbus_reply_exception(ctx, (uint8_t*)mb, retval);
}


static int ahc_echo(void *cls, struct MHD_Connection * connection,
                         const char *url,
                         const char *method, const char *version,
                         const char *upload_data,
                         size_t *upload_data_size, void **con_cls)
{
    struct MHD_Response *response;
    int ret;

    response = MHD_create_response_from_buffer (strlen(page),
                                               (void*) get_page(), MHD_RESPMEM_PERSISTENT);

    ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
    MHD_destroy_response (response);

    return ret;
}

void *microhttpd_handler( void *ptr )
{
	int i = 0;
	int delay = 0;
    char *terminate;
    thread_param_t* param = (thread_param_t*) ptr;
    ctx = param->ctx;
    mb_mapping = param->mb_mapping;
    terminate = param->terminate;
    port = param->port + 10000;     // port = modbus port + 1000
    delay = param->delay;
    free(param);
    struct MHD_Daemon *d;

    //printf("entering handler thread\n");
    modbus_set_debug(ctx, debug);

    d = MHD_start_daemon (MHD_USE_SELECT_INTERNALLY, port, NULL, NULL,
                                         &ahc_echo, NULL, MHD_OPTION_END);

    if (NULL == d)
    {
        printf("unable to start microhttpd server\n");
        exit(1);
    }

    while (*terminate == false)
    {
    	sleep(delay);
    	analogue_input_segment = simulated_adc_reading[i++];
    	i %= (sizeof simulated_adc_reading/sizeof simulated_adc_reading[0]);
    }

    MHD_stop_daemon (d);

    //printf("exiting handler thread\n");

    return 0;
}

