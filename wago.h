/*
 * Copyright © kiwipower 2017
 *
 * Header file to simulate battery charge/discharge profile
 */
#ifndef BATTSIM_DOT_H
#define BATTSIM_DOT_H

#include <stdint.h>
#include <modbus/modbus.h>
#include "typedefs.h"

#define analogue_segment              0
#define enableDebug                   1
#define constant_register4            0x2002

// proclet
int process_input_analogue_segment();
int process_output_analogue_segment(uint16_t data);
int process_constant_register4();

int  process_read_register(uint16_t address);
int  process_write_register(uint16_t address, uint16_t data);
void process_query(modbus_pdu_t*);
void *handler( void *ptr );
#endif
