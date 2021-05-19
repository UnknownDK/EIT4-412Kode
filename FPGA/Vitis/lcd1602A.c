//
// 2x16 LCD display (HD44780 controller + I2C chip)
//
// Copyright (c) BitBank Software, Inc.
// Written by Larry Bank
// bitbank@pobox.com
// Project started 12/6/2017
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#include "lcd1602A.h"
//#include "string.h"
//
// The LCD controller is wired to the I2C port expander with the upper 4 bits
// (D4-D7) connected to the data lines and the lower 4 bits (D0-D3) used as
// control lines. Here are the control line definitions:
//
// Command (0) / Data (1) (aka RS) (D0)
// R/W                             (D1)
// Enable/CLK                      (D2) 
// Backlight control               (D3)
//
// The data must be manually clocked into the LCD controller by toggling
// the CLK line after the data has been placed on D4-D7
//
#define PULSE_PERIOD 500
#define CMD_PERIOD 4100

#define BACKLIGHT 8
#define DATA 1
static int iBackLight = BACKLIGHT;
XIic i2cSkaerm;
XIic_Config *ConfigPtr; /* Pointer to configuration data */
//XTmrCtr timerB;
unsigned char address;



static void WriteCommand(unsigned char ucCMD) {
	unsigned char uc;

	uc = (ucCMD & 0xf0) | iBackLight; // most significant nibble sent first
	XIic_DynSend(ConfigPtr->BaseAddress, address, &uc, 1, XIIC_STOP);
	usleep(PULSE_PERIOD); // manually pulse the clock line
	uc |= 4; // enable pulse
	XIic_DynSend(ConfigPtr->BaseAddress, address, &uc, 1, XIIC_STOP);
	usleep(PULSE_PERIOD);
	uc &= ~4; // toggle pulse
	XIic_DynSend(ConfigPtr->BaseAddress, address, &uc, 1, XIIC_STOP);
	usleep(CMD_PERIOD);
	uc = iBackLight | (ucCMD << 4); // least significant nibble
	XIic_DynSend(ConfigPtr->BaseAddress, address, &uc, 1, XIIC_STOP);
	usleep(PULSE_PERIOD);
	uc |= 4; // enable pulse
	XIic_DynSend(ConfigPtr->BaseAddress, address, &uc, 1, XIIC_STOP);
	usleep(PULSE_PERIOD);
	uc &= ~4; // toggle pulse
	XIic_DynSend(ConfigPtr->BaseAddress, address, &uc, 1, XIIC_STOP);
	usleep(CMD_PERIOD);

} /* WriteCommand() */

//
// Control the backlight, cursor, and blink
// The cursor is an underline and is separate and distinct
// from the blinking block option
//
void lcd1602Control(int bBacklight, int bCursor, int bBlink) {
	unsigned char ucCMD = 0xc; // display control

	iBackLight = (bBacklight) ? BACKLIGHT : 0;
	if (bCursor)
		ucCMD |= 2;
	if (bBlink)
		ucCMD |= 1;
	WriteCommand(ucCMD);
} /* lcd1602Control() */

//
// Write an ASCII string (up to 16 characters at a time)
//
int lcd1602WriteString(char *text) {
	unsigned char ucTemp[2];
	int i = 0;

	if (text == NULL){
		return 1;
	}
	
	while (i < 16 && *text) {
		ucTemp[0] = iBackLight | DATA | (*text & 0xf0);
		XIic_DynSend(ConfigPtr->BaseAddress, address, ucTemp, 1, XIIC_STOP);
		usleep(PULSE_PERIOD);
		ucTemp[0] |= 4; // pulse E
		XIic_DynSend(ConfigPtr->BaseAddress, address, ucTemp, 1, XIIC_STOP);
		usleep(PULSE_PERIOD);
		ucTemp[0] &= ~4;
		XIic_DynSend(ConfigPtr->BaseAddress, address, ucTemp, 1, XIIC_STOP);
		usleep(PULSE_PERIOD);
		ucTemp[0] = iBackLight | DATA | (*text << 4);
		XIic_DynSend(ConfigPtr->BaseAddress, address, ucTemp, 1, XIIC_STOP);
		ucTemp[0] |= 4; // pulse E
		XIic_DynSend(ConfigPtr->BaseAddress, address, ucTemp, 1, XIIC_STOP);
		usleep(PULSE_PERIOD);
		ucTemp[0] &= ~4;
		XIic_DynSend(ConfigPtr->BaseAddress, address, ucTemp, 1, XIIC_STOP);
		usleep(CMD_PERIOD);
		text++;
		i++;
	}
	return 0;
} /* WriteString() */

//
// Erase the display memory and reset the cursor to 0,0
//
void lcd1602Clear(void) {
	WriteCommand(0x01); // clear the screen
} /* lcd1602Clear() */

//
// Configure and power up the display
//
void lcd1602Powerup() {
	WriteCommand(0x02); // Set 4-bit mode of the LCD controller
	WriteCommand(0x28); // 2 lines, 5x8 dot matrix
	WriteCommand(0x0c); // display on, cursor off
	WriteCommand(0x06); // inc cursor to right when writing and don't scroll
	WriteCommand(0x80); // set cursor to row 1, column 1
}
//
// Open a file handle to the I2C device
// Set the controller into 4-bit mode and clear the display
// returns 0 for success, 1 for failure
//
void lcd1602Init(unsigned int iAddr) {

	address = iAddr;
	ConfigPtr = XIic_LookupConfig(XPAR_I2C_BUS_DEVICE_ID);

	XIic_CfgInitialize(&i2cSkaerm, ConfigPtr, ConfigPtr->BaseAddress);
	XIic_Start(&i2cSkaerm);
	XIic_SetAddress(&i2cSkaerm, XII_ADDR_TO_SEND_TYPE, iAddr);

	iBackLight = BACKLIGHT; // turn on backlight
	lcd1602Powerup();
	lcd1602Clear();     // clear the memory
} /* lcd1602Init() */

//
// Set the LCD cursor position
//
int lcd1602SetCursor(int x, int y) {
	unsigned char cCmd;

	if (x < 0 || x > 15 || y < 0 || y > 1)
		return 1;

	cCmd = (y == 0) ? 0x80 : 0xc0;
	cCmd |= x;
	WriteCommand(cCmd);
	return 0;

} /* lcd1602SetCursor() */

void lcd1602Shutdown(void) {
	iBackLight = 0; // turn off backlight
	WriteCommand(0x08); // turn off display, cursor and blink
} /* lcd1602Shutdown() */

