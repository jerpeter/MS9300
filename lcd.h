#ifndef LCD_H_
#define LCD_H_

////////////////////////////////////////////////////////////////////////////////
// Include Files
////////////////////////////////////////////////////////////////////////////////
#include "Typedefs.h"

////////////////////////////////////////////////////////////////////////////////
// Global Scope Variables
////////////////////////////////////////////////////////////////////////////////
//origin of LCD is top-left corner, 128 wide x 64 "pixels" high
#define UPPER	1 // use the upper ASCII table
#define LOWER	2 // use the lower ASCII table

// display constants
#define COMMAND_REGISTER	0xFF// send a command to the display
#define DATA_REGISTER		0x00 // send data to the display

// lcd command codes
#define DISPLAY_ON			0x3F // Turn the display on
#define DISPLAY_OFF			0x3E // Turn the display off
#define START_LINE_SET		0xC0
#define PAGE_ADDRESS_SET	0xB8

#define COLUMN_ADDRESS_SET	0x40

#define FIRST_HALF_DISPLAY	0x00
#define SECOND_HALF_DISPLAY	0x01

#define COLUMN_ADDRESS_HIGH 0x10
#define COLUMN_ADDRESS_LOW	0x00

#define READ_MODIFY_WRITE	0xE0

#define END					0xEE

#define RESET_DISPLAY		0xE2

#define NUMBER_OF_PAGES		8
#define NUMBER_OF_COLUMNS	128

// lcd command codes
#define REVERSE_LCD			0xff
#define NORMAL_LCD			0x00

#define LINE8	8
#define LINE7	7
#define LINE6	6
#define LINE5	5
#define LINE4	4
#define LINE3	3
#define LINE2	2
#define LINE1	1

#define LINECOLOR_BLACK 0
#define LINECOLOR_DOTTED 1
#define LINECOLOR_WHITE 2

//#define YES 0x01
//#define NO 0x00

#define LCD_BACKLIGHT_HIGH	0x8000
#define LCD_BACKLIGHT_ON	0x4000
#define LCD_RESET			0x2000
#define LCD_CS2				0x1000
#define LCD_CS1				0x0800
#define LCD_ENABLE			0x0400
#define LCD_READ_WRITE		0x0200
#define LCD_RS				0x0100

////////////////////////////////////////////////////////////////////////////////
// Function Prototypes
////////////////////////////////////////////////////////////////////////////////
void WriteLCD_Vline(uint8 x_pos, uint8 y_start, uint8 y_end, uint8 dotted);
void WriteLCD_Hline(uint8 y_pos, uint8 x_start, uint8 x_end, uint8 bLineColor);
void WriteLCD_lgText(uint8 x_pos, uint8 y_pos, const uint8 *lcd_data, uint8 polarity);
void WriteLCD_smText(uint8 x_pos, uint8 y_pos, const uint8 *lcd_data, uint8 polarity);
void Write_display(uint8 lcd_register, uint8 lcd_data, uint8 display_half);
uint8 Read_display(uint8 lcd_register, uint8 display_half);
void ClearLCDscreen(void);
void InitDisplay(void);
void Backlight_On(void);
void Backlight_Off(void);
void Backlight_High(void);
void Backlight_Low(void);
void Reset_Contrast(void);
void Set_Contrast(uint8 level);
void Write_multi_display(uint8 lcd_register, uint8 lcd_data, uint8 display_half);

#endif //LCD_H_


