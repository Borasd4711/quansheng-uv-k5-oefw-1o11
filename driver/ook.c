
#include <string.h>   // NULL and memset

#include "bk4819.h"
#include "bsp/dp32g030/gpio.h"
#include "driver/gpio.h"
#include "driver/systick.h"
#include "radio.h"
#include "driver/ook.h"
//#include <math.h>

void OOK_CustomDelayUs(uint16_t d)
{
    SYSTICK_DelayUs( d + (uint16_t)(d/13)*(uint16_t)(d/13) );
}


void OOK_BeginTx(void)
{
	RADIO_enableTX(false);
    BK4819_SetupPowerAmplifier(g_current_vfo->txp_calculated_setting, g_current_vfo->p_tx->frequency);
    BK4819_set_GPIO_pin(BK4819_GPIO1_PIN29_PA_ENABLE, true);  // PA on
    BK4819_set_GPIO_pin(BK4819_GPIO5_PIN1_RED, true);         // turn the RED LED on
    GPIO_SetBit(&GPIOC->DATA, GPIOC_PIN_FLASHLIGHT);
}

void OOK_EndTx(void)
{
    RADIO_disableTX(false);
    BK4819_SetupPowerAmplifier(0, 0);
    BK4819_set_GPIO_pin(BK4819_GPIO1_PIN29_PA_ENABLE, false);  // PA off
    BK4819_set_GPIO_pin(BK4819_GPIO5_PIN1_RED, false);         // turn the RED LED off
    GPIO_ClearBit(&GPIOC->DATA, GPIOC_PIN_FLASHLIGHT);
}

void OOK_HardwareTxOn(void)
{
	BK4819_set_GPIO_pin(BK4819_GPIO1_PIN29_PA_ENABLE, true);  // PA on
}

void OOK_HardwareTxOff(void)
{
    BK4819_set_GPIO_pin(BK4819_GPIO1_PIN29_PA_ENABLE, false);  // PA off
}

void OOK_EncodeSymbol(OOK_t * ook_struct, bool symbol)
{    
    if (symbol)
    {
        OOK_HardwareTxOff();
        OOK_CustomDelayUs(ook_struct->period_us - ook_struct->pulse_0_us);
        OOK_HardwareTxOn();
        OOK_CustomDelayUs(ook_struct->pulse_1_us);
    }
    else
    {
        OOK_HardwareTxOff();   
        OOK_CustomDelayUs(ook_struct->period_us - ook_struct->pulse_0_us);
        OOK_HardwareTxOn();
        OOK_CustomDelayUs(ook_struct->pulse_0_us);
    }
}

void OOK_TxSequence(OOK_t * ook_struct)
{
    uint8_t len = ook_struct->sequence_len;
    uint8_t symbol_pos = 0;
    uint8_t b = 0;
    uint8_t * ptr_to_ook_sequence = ook_struct->sequence_ptr;
    bool symbol;

    OOK_HardwareTxOn(); // send initial mark for sequence transmission
    OOK_CustomDelayUs(ook_struct->sync_pulse_us);
    
    while(len--)
    {
    	if((symbol_pos++ % 8) == 0) // getting new byte from sequence every 8 bits
    	{
    		b = *(uint8_t*)ook_struct->sequence_ptr++;
    	}

		// sending msb first
		symbol = (b & 0x80) ? true : false;
		OOK_EncodeSymbol(ook_struct, symbol);
		b <<= 1;
    }

    // reset the sequence pointer to the beginning of sequence for the next call of this function
    ook_struct->sequence_ptr = ptr_to_ook_sequence;
    
    // stop transmission for the trailing delay
    OOK_HardwareTxOff();
}