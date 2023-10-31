#ifndef DRIVER_OOK_H
#define DRIVER_OOK_H

typedef struct OOK_s {
	uint8_t *sequence_ptr; 
	uint8_t  sequence_len;	// number of symbols in sequence
	uint16_t sync_pulse_us;	// tx duration at beginning of transmission (us)
	uint16_t pulse_0_us;	// tx duration for mark (us)
	uint16_t pulse_1_us;	// tx duration for space (us)
	uint16_t period_us;		// time between two symbols (us)
} __attribute__((packed)) OOK_t;

void OOK_BeginTx(void);
void OOK_EndTx(void);
void OOK_HardwareTxOn(void);
void OOK_HardwareTxOff(void);
void OOK_EncodeSymbol(OOK_t * ook_struct, bool symbol);
void OOK_TxSequence(OOK_t * ook_struct);

/*** usage (theoretically...)
#include "driver/ook.h"

OOK_t rc433_spazioGajaBarra = {
	.sequence_len = 37,
	.sync_pulse_us = 200,
	.pulse_0_us = 290,
	.pulse_1_us = 90,
	.period_us = 400
};
uint8_t temp1[] = {0x00, 0x67, 0x43, 0x79, 0xF8}; // {0b00000000, 0b01100111, 0b01000011, 0b01111001, 0b11111000}
	
{
	OOK_BeginTx();
	OOK_TxSequence(&rc433_spazioGajaBarra);
	OOK_EndTx();
	SYSTEM_DelayMs(100);
}
***/

#endif // DRIVER_OOK_H