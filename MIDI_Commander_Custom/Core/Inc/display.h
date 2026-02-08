/*
 * display.h
 *
 *  Created on: 8 Jul 2021
 *      Author: D Harvie
 */

#ifndef INC_DISPLAY_H_
#define INC_DISPLAY_H_

void display_init(void);
void display_setConfigName(void);
void display_setBankName(uint8_t bankNumber);
void display_showLabelText(const char *label);
void display_showLabelBitmap(const uint8_t *bitmap);
void display_tick(void);

#endif /* INC_DISPLAY_H_ */
