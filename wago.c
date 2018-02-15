#include <stdio.h>
#include <stdlib.h>
#include <byteswap.h>
#include "wago.h"
#include "typedefs.h"
#include <unistd.h>
#include <modbus/modbus.h>
#include <string.h>


// Private data
static modbus_t* ctx;
static modbus_mapping_t *mb_mapping;

static uint16_t analogue_value = 0;


int process_input_analogue_segment()
{
    uint16_t *address;
    uint16_t address_offset;
    int retval = MODBUS_SUCCESS; // need to figure out what this constant is

    address_offset = mb_mapping->start_registers + analogue_segment;
    address = mb_mapping->tab_registers + address_offset;
    *address = analogue_value;        // TBD

    return retval;
}

int process_constant_register4()
{
    uint16_t *address;
    uint16_t address_offset;
    int retval = MODBUS_SUCCESS; // need to figure out what this constant is

    address_offset = mb_mapping->start_registers + constant_register4;
    address = mb_mapping->tab_registers + address_offset;
    *address = 0x1234;        // TBD

    return retval;
}

int process_output_analogue_segment(uint16_t value)
{
    int retval = MODBUS_SUCCESS; // need to figure out what this constant is

    analogue_value = value;

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

//
// Thread handler
//
void *handler( void *ptr )
{
    char *terminate;
    char status[12] = "idle";
    thread_param_t* param = (thread_param_t*) ptr;
    ctx = param->ctx;
    mb_mapping = param->mb_mapping;
    terminate = param->terminate;
    free(param);

    while ( *terminate == false )
    {

    }
}

