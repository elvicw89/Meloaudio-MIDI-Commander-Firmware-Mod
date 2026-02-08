/*
 * flash_midi_settings.c
 *
 *  Created on: 12 Jul 2021
 *      Author: D Harvie
 */

#include "main.h"
#include "flash_midi_settings.h"
#include "midi_defines.h"


#define FLASH_SETTINGS_OFFSET	(1024*128)
#define FLASH_SETTINGS_START	(FLASH_BASE + FLASH_SETTINGS_OFFSET)

#define FLASH_SETTINGS_NO_PAGES	(3)

#define NO_CMD_BYTES 0, 0, 0, 0
#define CC_CMD_BYTES(chan, cc, on_val, off_val) \
	(uint8_t)(CMD_CC_NIBBLE | ((chan) - 1)), (uint8_t)((cc) & 0x7F), (uint8_t)((on_val) & 0x7F), (uint8_t)((off_val) & 0x7F)

static uint8_t g_global_settings[32] = {
	[GLOBAL_SETTINGS_CHANNEL] = 1,
	[GLOBAL_SETTINGS_REALTIME_PASS] = 0,
	[16] = 'C', [17] = 'C', [18] = '2', [19] = '5',
	[20] = ' ', [21] = 'M', [22] = 'O', [23] = 'M',
	[24] = 'E', [25] = 'N', [26] = 'T', [27] = 'A',
	[28] = 'R', [29] = 'Y', [30] = ' ', [31] = ' '
};

static uint8_t g_bank_strings[12] = {
	'B', '1', ' ', ' ',
	'C', 'C', '2', '5', ' ', 'A', 'B', 'C'
};

static uint8_t g_switch_cmds[MIDI_NUM_COMMANDS_PER_SWITCH * MIDI_ROM_CMD_SIZE * 8 * MIDI_NUM_BANKS] = {
	// SW_1
	CC_CMD_BYTES(1, 25, 1, 0), NO_CMD_BYTES, NO_CMD_BYTES, NO_CMD_BYTES, NO_CMD_BYTES,
	NO_CMD_BYTES, NO_CMD_BYTES, NO_CMD_BYTES, NO_CMD_BYTES, NO_CMD_BYTES,
	// SW_2
	CC_CMD_BYTES(1, 25, 2, 0), NO_CMD_BYTES, NO_CMD_BYTES, NO_CMD_BYTES, NO_CMD_BYTES,
	NO_CMD_BYTES, NO_CMD_BYTES, NO_CMD_BYTES, NO_CMD_BYTES, NO_CMD_BYTES,
	// SW_3
	CC_CMD_BYTES(1, 25, 3, 0), NO_CMD_BYTES, NO_CMD_BYTES, NO_CMD_BYTES, NO_CMD_BYTES,
	NO_CMD_BYTES, NO_CMD_BYTES, NO_CMD_BYTES, NO_CMD_BYTES, NO_CMD_BYTES,
	// SW_4
	(uint8_t)(CMD_CC_NIBBLE | 0), (uint8_t)(28 | 0x80), 127, 0, NO_CMD_BYTES, NO_CMD_BYTES, NO_CMD_BYTES, NO_CMD_BYTES,
	NO_CMD_BYTES, NO_CMD_BYTES, NO_CMD_BYTES, NO_CMD_BYTES, NO_CMD_BYTES,
	// SW_A
	(uint8_t)(CMD_CC_NIBBLE | 0), (uint8_t)(79 | 0x80), 127, 0, NO_CMD_BYTES, NO_CMD_BYTES, NO_CMD_BYTES, NO_CMD_BYTES,
	NO_CMD_BYTES, NO_CMD_BYTES, NO_CMD_BYTES, NO_CMD_BYTES, NO_CMD_BYTES,
	// SW_B
	(uint8_t)(CMD_CC_NIBBLE | 0), (uint8_t)(80 | 0x80), 127, 0, NO_CMD_BYTES, NO_CMD_BYTES, NO_CMD_BYTES, NO_CMD_BYTES,
	NO_CMD_BYTES, NO_CMD_BYTES, NO_CMD_BYTES, NO_CMD_BYTES, NO_CMD_BYTES,
	// SW_C
	(uint8_t)(CMD_CC_NIBBLE | 0), (uint8_t)(81 | 0x80), 127, 0, NO_CMD_BYTES, NO_CMD_BYTES, NO_CMD_BYTES, NO_CMD_BYTES,
	NO_CMD_BYTES, NO_CMD_BYTES, NO_CMD_BYTES, NO_CMD_BYTES, NO_CMD_BYTES,
	// SW_D
	(uint8_t)(CMD_CC_NIBBLE | 0), (uint8_t)(60 | 0x80), 127, 0, NO_CMD_BYTES, NO_CMD_BYTES, NO_CMD_BYTES, NO_CMD_BYTES,
	NO_CMD_BYTES, NO_CMD_BYTES, NO_CMD_BYTES, NO_CMD_BYTES, NO_CMD_BYTES
};

uint8_t *pGlobalSettings = g_global_settings;
uint8_t *pBankStrings = g_bank_strings;
uint8_t *pSwitchCmds = g_switch_cmds;


void flash_settings_erase(void){
	// Erase flash sectors containing the settings
	// This must be done before re-writing them.

	uint32_t pageError;

	FLASH_EraseInitTypeDef eraseInit ={
			.TypeErase = FLASH_TYPEERASE_PAGES,
			.Banks = FLASH_BANK_1,
			.PageAddress = FLASH_SETTINGS_START,
			.NbPages = FLASH_SETTINGS_NO_PAGES
	};

	HAL_FLASH_Unlock();

	HAL_StatusTypeDef status = HAL_FLASHEx_Erase(&eraseInit, &pageError);

	HAL_FLASH_Lock();

	if(status != HAL_OK){
		// TODO: Display a message of the page that's in error
		Error("Flash erase error");
	}

}


void flash_settings_write(uint8_t* data, uint32_t offset){
	uint32_t flash_address = offset + FLASH_SETTINGS_START;

	HAL_FLASH_Unlock();

	// Programming 16bytes, so 8 iterations of 16bit
	for(int i=0; i<8; i++){
		uint16_t write_data = data[2*i] + (data[2*i+1] << 8);
		HAL_StatusTypeDef status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, flash_address + 2*i, write_data);
		if(status != HAL_OK){
			Error("Flash write error");
		}
	}

	HAL_FLASH_Lock();

}
