/* Copyright 2023 Motorello
 * https://github.com/motorello
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 */

#include "bk4819.h"
#include "bsp/dp32g030/gpio.h"
#include "driver/gpio.h"
#include "driver/systick.h"
#include "radio.h"
#include "driver/ook.h"

#define OOK_TX_FREQUENCY (433920000 / 10) // standard EU frequency for ISM
#define OOK_TX_BIAS      10 // low power

uint32_t vfo_frequency_to_restore = 0;
BK4819_filter_bandwidth_t bandwidth_to_restore = 0;
uint8_t txp_to_restore = 0;

void OOK_CustomDelayUs(uint16_t d)
{
    SYSTICK_DelayUs( d + (uint16_t)(d/13)*(uint16_t)(d/13) );
}


void OOK_BeginTx(void)
{
    // save the current vfo parameters to be restored later (because RADIO_enableTX() uses this value to setup all tx stuff)
    vfo_frequency_to_restore = g_current_vfo->p_tx->frequency; 
    bandwidth_to_restore = g_current_vfo->channel_bandwidth;
    txp_to_restore = g_current_vfo->txp_calculated_setting;

    // override the current vfo in order to avoid to TX on unwanted bands
    g_current_vfo->p_tx->frequency = OOK_TX_FREQUENCY;
    g_current_vfo->channel_bandwidth = BK4819_FILTER_BW_WIDE;
    g_current_vfo->txp_calculated_setting = OOK_TX_BIAS;

	RADIO_enableTX(false);
    GPIO_SetBit(&GPIOC->DATA, GPIOC_PIN_FLASHLIGHT);          // turn the FLASHLIGHT on just for fancy
}

void OOK_EndTx(void)
{
    RADIO_disableTX(false);
    GPIO_ClearBit(&GPIOC->DATA, GPIOC_PIN_FLASHLIGHT);         // turn the FLASHLIGHT off, fancy over

    // restore the correct vfo parameters
    g_current_vfo->p_tx->frequency = vfo_frequency_to_restore;
    g_current_vfo->channel_bandwidth = bandwidth_to_restore;
    g_current_vfo->txp_calculated_setting = txp_to_restore;
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
        OOK_CustomDelayUs(ook_struct->period_us - ook_struct->pulse_1_us);
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