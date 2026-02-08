/*
 * display.c
 *
 *  Created on: 8 Jul 2021
 *      Author: D Harvie
 */
#include "main.h"
#include "flash_midi_settings.h"
#include <string.h>
#include <stdio.h>
#include "ssd1306.h"
#include "logo_atc.h"
#include "label_bitmaps.h"

static uint32_t label_restore_deadline = 0;
static uint8_t label_active = 0;

void display_setBankName(uint8_t bankNumber);

static uint8_t center_x(const char *text, FontDef font){
	uint16_t w = (uint16_t)strlen(text) * font.FontWidth;
	if (w >= SSD1306_WIDTH) {
		return 0;
	}
	return (uint8_t)((SSD1306_WIDTH - w) / 2);
}

static void draw_bitmap_fullscreen(const uint8_t *bmp){
	uint8_t row_bytes = (LABEL_W + 7) / 8;
	for (uint16_t y = 0; y < LABEL_H; y++) {
		for (uint16_t x = 0; x < LABEL_W; x++) {
			uint32_t idx = y * row_bytes + (x >> 3);
			uint8_t byte = bmp[idx];
			if (byte & (1 << (7 - (x & 7)))) {
				ssd1306_DrawPixel(x, y, White);
			}
		}
	}
}

void display_init(void){
    ssd1306_Init();

    ssd1306_Fill(Black);
    ssd1306_SetCursor(center_x("Modded by", Font_7x10), 18);
    ssd1306_WriteString("Modded by", Font_7x10, White);
    ssd1306_SetCursor(center_x("YEAHWAAH", Font_7x10), 30);
    ssd1306_WriteString("YEAHWAAH", Font_7x10, White);
    ssd1306_DrawRectangle(8, 52, 119, 59, White);
    ssd1306_UpdateScreen();

    for (uint8_t x = 10; x < 118; x++) {
    	ssd1306_Line(x, 54, x, 57, White);
    	if ((x % 4) == 0) {
    		ssd1306_UpdateScreen();
    		HAL_Delay(15);
    	}
    }
    ssd1306_UpdateScreen();
    HAL_Delay(200);

    __NOP();
}

void display_setConfigName(void){
    ssd1306_SetCursor(10, 34);
    for(int i=0; i<16; i++){
    	ssd1306_WriteChar(pGlobalSettings[16+i], Font_7x10, White);
    }
    ssd1306_UpdateScreen();
}

void display_showLabelText(const char *label){
    ssd1306_Fill(Black);
    ssd1306_SetCursor(center_x(label, Font_11x18), 24);
    ssd1306_WriteString((char *)label, Font_11x18, White);
    ssd1306_UpdateScreen();

    label_active = 1;
    label_restore_deadline = HAL_GetTick() + 3000;
}

void display_showLabelBitmap(const uint8_t *bitmap){
    ssd1306_Fill(Black);
    draw_bitmap_fullscreen(bitmap);
    ssd1306_UpdateScreen();

    label_active = 1;
    label_restore_deadline = HAL_GetTick() + 3000;
}

void display_tick(void){
    if(label_active && HAL_GetTick() >= label_restore_deadline){
        label_active = 0;
        display_setBankName(0);
    }
}

void display_setBankName(uint8_t bankNumber){
	(void)bankNumber;
	ssd1306_Fill(Black);

	for (uint16_t y = 0; y < LOGO_ATC_HEIGHT; y++) {
		for (uint16_t x = 0; x < LOGO_ATC_WIDTH; x++) {
			uint32_t idx = y * LOGO_ATC_WIDTH + x;
			uint8_t byte = logo_atc_bits[idx >> 3];
			if (byte & (1 << (7 - (idx & 7)))) {
				ssd1306_DrawPixel(x, y, White);
			}
		}
	}

	ssd1306_UpdateScreen();
}
