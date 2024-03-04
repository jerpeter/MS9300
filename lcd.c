////////////////////////////////////////////////////////////////////////////////
// Include Files
////////////////////////////////////////////////////////////////////////////////
//#include "compiler.h"
#include "Typedefs.h"
#include "lcd.h"
#include "gpio.h"
#include "Common.h"
#include "PowerManagement.h"
#include "Display.h"

#include "Globals.h"
#include "stdint.h"
#include "spi.h"
#include "mxc_delay.h"

////////////////////////////////////////////////////////////////////////////////
// Local Scope Variables
////////////////////////////////////////////////////////////////////////////////
#define PF_FIRST_TIME	0x01
#define PF_TOP_PART		0x02

#define PREPEND_BLANK_COL_TO_EACH_CHAR

#define LCD_SPI_DEASERT YES

//#ifndef BUILD_FOR_PC

// Large Numbers(2 Lines)
// LCD large numbers top lookup table
const uint8 LARGE_NUMBER_TOP[160] =
{
	0x1f,0x3f,0x70,0x60,0x60,0x61,0x62,0x74,0x3f,0x1f, // 0
	0x00,0x00,0x18,0x38,0x7F,0x7F,0x00,0x00,0x00,0x00, // 1
	0x18,0x38,0x70,0x60,0x60,0x60,0x60,0x71,0x3F,0x1F, // 2
	0x18,0x38,0x70,0x60,0x61,0x61,0x61,0x73,0x3F,0x1E, // 3
	0x01,0x03,0x06,0x0c,0x18,0x30,0x7f,0x7f,0x00,0x00, // 4
	0x7F,0x7F,0x63,0x63,0x63,0x63,0x63,0x63,0x61,0x60, // 5
	0x0F,0x1F,0x31,0x61,0x61,0x61,0x61,0x61,0x00,0x00, // 6
	0x60,0x60,0x60,0x60,0x60,0x61,0x63,0x67,0x7E,0x7C, // 7
	0x1E,0x3F,0x73,0x61,0x61,0x61,0x61,0x73,0x3F,0x1E, // 8
	0x1E,0x3F,0x73,0x61,0x61,0x61,0x61,0x71,0x3F,0x1F, // 9
	0x30,0x48,0x48,0x30,0x07,0x07,0x06,0x06,0x06,0x06, // type : to get degF
	0x30,0x48,0x48,0x30,0x03,0x07,0x06,0x06,0x07,0x03, // type ; to get degC
	0x30,0x48,0x48,0x30,0x01,0x07,0x1E,0x78,0x60,0x00, // type < to get %
	0x00,0x00,0x00,0x00,0x00,0x00,0x30,0x48,0x48,0x30, // type = to get degree symbol
	0x1F,0x3F,0x70,0x60,0x60,0x60,0x60,0x70,0x38,0x18, // type > to get C
	0x7F,0x7F,0x61,0x61,0x61,0x61,0x61,0x60,0x60,0x60 // type ? to get F
};


// Large Numbers (2 Lines)
// LCD large numbers bottom lookup table
const uint8 LARGE_NUMBER_BOT[160] =
{
	0xF8,0xFC,0x2E,0x46,0x86,0x06,0x06,0x0E,0xFC,0xF8, // 0
	0x00,0x00,0x06,0x06,0xFE,0xFE,0x06,0x06,0x00,0x00, // 1
	0x3E,0x7E,0xE6,0xC6,0xC6,0xC6,0xC6,0xC6,0x86,0x06, // 2
	0x18,0x1C,0x0E,0x06,0x86,0x86,0x86,0xCE,0xFC,0x78, // 3
	0xE0,0xE0,0x60,0x60,0x60,0x60,0xFE,0xFE,0x60,0x60, // 4
	0x18,0x1C,0x0E,0x06,0x06,0x06,0x06,0x8E,0xFC,0xF8, // 5
	0xF8,0xFC,0x8E,0x86,0x86,0x86,0x86,0xCE,0xFC,0x78, // 6
	0x0E,0x1E,0x38,0x70,0xE0,0xC0,0x80,0x00,0x00,0x00, // 7
	0x78,0xFC,0xCE,0x86,0x86,0x86,0x86,0xCE,0xFC,0x78, // 8
	0x00,0x00,0x86,0x86,0x86,0x86,0x8C,0x98,0xF0,0xE0, // 9
	0x00,0x00,0x00,0x00,0xFE,0xFE,0x60,0x60,0x00,0x00, // type : to get degF
	0x00,0x00,0x00,0x00,0xFC,0xFE,0x06,0x06,0x0E,0x0C, // type ; to get degC
	0x00,0x06,0x1E,0x78,0xE0,0x80,0x0C,0x12,0x12,0x0C, // type < to get %
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // type = to get degree symbol
	0xF8,0xFC,0x0E,0x06,0x06,0x06,0x06,0x0E,0x1C,0x18, // type > to get C
	0xFE,0xFE,0x80,0x80,0x80,0x80,0x80,0x00,0x00,0x00 // type ? to get F
};


// LCD small letters lookup table
const uint8 SMALL_CHARACTERS[670] =
{
	0x00,0x00,0x00,0x00,0x00, // space
	0x00,0x00,0xF2,0x00,0x00, // !
	0x00,0xE0,0x00,0xE0,0x00, // "
	0x28,0xFE,0x28,0xFE,0x28, // #
	0x24,0x54,0xFE,0x54,0x48, // $
	0xC4,0xC8,0x10,0x26,0x46, // %
	0x6C,0x92,0xAA,0x44,0x0A, // &
	0x00,0xA0,0xC0,0x00,0x00, // '
	0x00,0x38,0x44,0x82,0x00, // (
	0x00,0x82,0x44,0x38,0x00, // )
	0x28,0x10,0x7C,0x10,0x28, // *
	0x10,0x10,0x7C,0x10,0x10, // +
	0x00,0x0A,0x0C,0x00,0x00, // ,
	0x10,0x10,0x10,0x10,0x10, // -
	0x00,0x06,0x06,0x00,0x00, // .
	0x04,0x08,0x10,0x20,0x40, // /
	0x7C,0x8A,0x92,0xA2,0x7C, // 0
	0x00,0x42,0xFE,0x02,0x00, // 1
	0x42,0x86,0x8A,0x92,0x62, // 2
	0x84,0x82,0xA2,0xD2,0x8C, // 3
	0x18,0x28,0x48,0xFE,0x08, // 4
	0xE4,0xA2,0xA2,0xA2,0x9C, // 5
	0x3C,0x52,0x92,0x92,0x0C, // 6
	0x80,0x80,0x9E,0xA0,0xC0, // 7
	0x6C,0x92,0x92,0x92,0x6C, // 8
	0x60,0x92,0x92,0x94,0x78, // 9
	0x00,0x6C,0x6C,0x00,0x00, // :
	0x00,0x6A,0x6C,0x00,0x00, // ;
	0x10,0x28,0x44,0x82,0x00, // <
	0x28,0x28,0x28,0x28,0x28, // =
	0x00,0x82,0x44,0x28,0x10, // >
	0x40,0x80,0x8A,0x90,0x60, // ?
	0x4C,0x92,0x9E,0x82,0x7C, // @
	0x7E,0x88,0x88,0x88,0x7E, // A
	0x82,0xFE,0x92,0x92,0x6C, // B
	0x7C,0x82,0x82,0x82,0x44, // C
	0x82,0xFE,0x82,0x82,0x7C, // D
	0xFE,0x92,0x92,0x92,0x82, // E
	0xFE,0x90,0x90,0x90,0x80, // F
	0x7C,0x82,0x82,0x92,0x5E, // G
	0xFE,0x10,0x10,0x10,0xFE, // H
	0x00,0x82,0xFE,0x82,0x00, // I
	0x04,0x02,0x82,0xFC,0x80, // J
	0xFE,0x10,0x28,0x44,0x82, // K
	0xFE,0x02,0x02,0x02,0x02, // L
	0xFE,0x40,0x30,0x40,0xFE, // M
	0xFE,0x60,0x10,0x0C,0xFE, // N
	0x7C,0x82,0x82,0x82,0x7C, // O
	0xFE,0x90,0x90,0x90,0x60, // P
	0x7C,0x82,0x8A,0x84,0x7A, // Q
	0xFE,0x90,0x98,0x94,0x62, // R
	0x64,0x92,0x92,0x92,0x4C, // S
	0x80,0x80,0xFE,0x80,0x80, // T
	0xFC,0x02,0x02,0x02,0xFC, // U
	0xF8,0x04,0x02,0x04,0xF8, // V
	0xFE,0x04,0x18,0x04,0xFE, // W
	0xC6,0x28,0x10,0x28,0xC6, // X
	0xE0,0x10,0x0E,0x10,0xE0, // Y
	0x86,0x8A,0x92,0xA2,0xC2, // Z
	0x00,0xFE,0x82,0x82,0x00, // [
	0x40,0x20,0x10,0x08,0x04, // "\"
	0x00,0x82,0x82,0xFE,0x00, // ]
	0x20,0x40,0x80,0x40,0x20, // ^
	0x02,0x02,0x02,0x02,0x02, // _
	0x80,0x40,0x20,0x00,0x00, // `
	0x04,0x2A,0x2A,0x2A,0x1E, // a
	0xFE,0x12,0x22,0x22,0x1C, // b
	0x1C,0x22,0x22,0x22,0x14, // c
	0x1C,0x22,0x22,0x12,0xFE, // d
	0x1C,0x2A,0x2A,0x2A,0x18, // e
	0x00,0x10,0x7E,0x90,0x40, // f
	0x30,0x4A,0x4A,0x32,0x7C, // g
	0xFE,0x10,0x20,0x20,0x1E, // h
	0x00,0x22,0xBE,0x02,0x00, // i
	0x04,0x02,0x22,0xBC,0x00, // j
	0x00,0xFE,0x08,0x14,0x22, // k
	0x00,0x82,0xFE,0x02,0x00, // l
	0x3E,0x20,0x1E,0x20,0x1E, // m
	0x3E,0x10,0x20,0x20,0x1E, // n
	0x1C,0x22,0x22,0x22,0x1C, // o
	0x7E,0x30,0x48,0x48,0x30, // p
	0x30,0x48,0x48,0x30,0x7E, // q
	0x3E,0x10,0x20,0x20,0x10, // r
	0x1A,0x2A,0x2A,0x2A,0x26, // s
	0x20,0xFC,0x22,0x02,0x04, // t
	0x3C,0x02,0x02,0x3C,0x02, // u
	0x38,0x04,0x02,0x04,0x38, // v
	0x3C,0x02,0x0C,0x02,0x3C, // w
	0x22,0x14,0x08,0x14,0x22, // x
	0x70,0x0A,0x0A,0x12,0x7C, // y
	0x22,0x26,0x2A,0x32,0x22, // z
	0x00,0x10,0x6C,0x82,0x00, // {
	0x00,0x00,0xEE,0x00,0x00, // �
	0x00,0x82,0x6C,0x10,0x00, // }
	0x40,0x80,0x40,0x20,0x40, // ~
	0x00,0x00,0x00,0x00,0x00, // space - DEL - not used 127
	0x78,0x85,0x87,0x84,0x48, // � 128
	0x1C,0x42,0x02,0x5C,0x02, // � 129
	0x1C,0x6A,0xAA,0x2A,0x18, // �
	0x04,0x6A,0xAA,0x6A,0x1E, // �
	0x04,0xAA,0x2A,0xAA,0x1E, // �
	0x04,0x2A,0xAA,0x6A,0x1E, // �
	0x02,0x55,0xB5,0x55,0x0F, // �
	0x38,0x45,0x47,0x44,0x28, // �
	0x1C,0x6A,0xAA,0x6A,0x18, // �
	0x1C,0xAA,0x2A,0xAA,0x18, // �
	0x1C,0x2A,0xAA,0x6A,0x18, // �
	0x00,0x52,0x1E,0x42,0x00, // �
	0x00,0x52,0x9E,0x42,0x00, // �
	0x00,0x92,0x5E,0x02,0x00, // �
	0x0E,0x94,0x24,0x94,0x0E, // �
	0x07,0x4A,0xB2,0x4A,0x07, // �
	0x3E,0x6A,0xAA,0x2A,0x22, // �
	0x24,0x2A,0x3E,0x2A,0x1A, // Backwards s with a line through it
	0x7E,0x88,0xFE,0x92,0x82, // �
	0x0C,0x52,0x92,0x52,0x0C, // �
	0x0C,0x52,0x12,0x52,0x0C, // �
	0x0C,0x12,0x92,0x52,0x0C, // �
	0x1C,0x42,0x82,0x5C,0x02, // � 150
	0x1C,0x82,0x42,0x1C,0x02, // �
	0x38,0x85,0x05,0x89,0x3E, //
	0x1C,0xA2,0x22,0xA2,0x1C, // �
	0x1C,0x42,0x02,0x42,0x1C, // �
	0x38,0x44,0xFE,0x44,0x28, // �
	0x16,0x7E,0x94,0x82,0x44, // �
	0x94,0x54,0x3E,0x54,0x94, // �
	0xFE,0xA0,0x48,0x3E,0x0A, //
	0x12,0x7E,0x92,0x80,0x40, // Frequency symbol
	0x00,0x00,0x40,0xA0,0x40, // � 160 0xa0
	0xC0,0xC0,0X3E,0x28,0x20, //degree f 161 0xa1
	0xC0,0xC0,0x1C,0x22,0x22, //degree c 162 0xa2
	// 0x38,0x44,0x44,0x44,0x38, //open radio 163 0xa3
	0x00,0x00,0x00,0x00,0x00, //deselect icon 163 0xa3
	// 0x38,0x7C,0x7C,0x7C,0x38 //closed radio 164 0xa4
	// 0x08,0x04,0x06,0x18,0x60, //select check mark 164 0xa4
	// 0x00,0x7C,0x38,0x10,0x00, //select right arrow 164 0xa4
	0x08,0x1C,0x3E,0x1C,0x08, //select diamond 164 0xa4
};

const char craft_logo[] =
{
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',

'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
'\x70','\xf8','\xc4','\x84','\x84','\x1c','\x00','\x00',
'\x84','\xfc','\x7c','\x00','\x00','\x00','\x00','\x04',
'\xfc','\xfc','\x04','\x04','\x04','\x84','\xfc','\x7c',
'\x84','\x84','\x84','\xcc','\x7c','\x38','\x00','\x04',
'\x84','\xfc','\xfc','\x84','\x84','\x84','\x04','\x1c',
'\x00','\x00','\x00','\x84','\xfc','\x7c','\x84','\x84',
'\x44','\x7c','\x38','\x00','\xc0','\xe0','\x30','\x08',
'\x08','\x04','\x04','\x04','\x04','\x1c','\x0c','\x00',
'\x00','\x00','\x00','\x84','\xfc','\x7c','\x84','\x84',
'\x44','\x7c','\x38','\x00','\x00','\x00','\x00','\x00',
'\xc0','\x20','\x18','\x7c','\xfc','\x80','\x00','\x00',
'\x00','\x04','\x84','\xfc','\x7c','\x84','\x84','\x84',
'\xcc','\x7c','\x38','\x00','\x04','\xe4','\xfc','\x9c',
'\x84','\x80','\x80','\x84','\xfc','\xfc','\x04','\x04',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',

'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x0c',
'\x18','\x10','\x10','\x19','\x0f','\x07','\x00','\x00',
'\x0f','\x1f','\x10','\x10','\x10','\x10','\x08','\x1e',
'\x1f','\x11','\x00','\x10','\x10','\x1f','\x1f','\x10',
'\x00','\x00','\x00','\x00','\x00','\x00','\x10','\x10',
'\x1f','\x1f','\x10','\x10','\x10','\x10','\x1c','\x04',
'\x00','\x10','\x10','\x1f','\x1f','\x10','\x00','\x01',
'\x07','\x1e','\x18','\x10','\x07','\x0f','\x18','\x10',
'\x10','\x10','\x11','\x11','\x0f','\x0f','\x01','\x00',
'\x00','\x10','\x10','\x1f','\x1f','\x10','\x00','\x01',
'\x07','\x1e','\x18','\x10','\x10','\x18','\x16','\x11',
'\x01','\x01','\x01','\x11','\x1f','\x1f','\x10','\x00',
'\x10','\x10','\x1f','\x1f','\x10','\x00','\x00','\x00',
'\x00','\x00','\x00','\x10','\x18','\x1f','\x17','\x10',
'\x00','\x10','\x10','\x1f','\x1f','\x10','\x00','\x00',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',

'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
'\x00','\x80','\xe0','\xf0','\xf8','\x3c','\x0c','\x04',
'\x06','\x02','\x02','\x02','\x02','\x02','\x02','\x06',
'\x06','\x3c','\x04','\x00','\x02','\x02','\x02','\xfe',
'\xfe','\xfe','\xfe','\x02','\x02','\x02','\x02','\x06',
'\x86','\xfe','\xfe','\xfc','\x38','\x00','\x00','\x00',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\xc0',
'\x30','\x7c','\xfe','\xfc','\xf0','\xc0','\x00','\x00',
'\x00','\x00','\x00','\x00','\x00','\x02','\x02','\x02',
'\xfe','\xfe','\xfe','\xfe','\x02','\x02','\x02','\x02',
'\x02','\x82','\x02','\x1e','\x1e','\x02','\x02','\x02',
'\x02','\x02','\xfe','\xfe','\xfe','\xfe','\x02','\x02',
'\x02','\x02','\x02','\x1e','\x00','\x00','\x00','\x00',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',

'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
'\x00','\x1f','\x7f','\xff','\xff','\xe0','\x80','\x00',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\xff',
'\xff','\xff','\xff','\x00','\x02','\x0e','\x1e','\x3f',
'\xff','\xf9','\xf0','\xc0','\x80','\x00','\x00','\x00',
'\x00','\x00','\x00','\x80','\x60','\x1c','\x13','\x10',
'\x10','\x10','\x11','\x17','\x1f','\xff','\xff','\xfc',
'\xe0','\x80','\x00','\x00','\x00','\x00','\x00','\x00',
'\xff','\xff','\xff','\xff','\x02','\x02','\x02','\x02',
'\x02','\x0f','\x00','\x00','\x00','\x00','\x00','\x00',
'\x00','\x00','\xff','\xff','\xff','\xff','\x00','\x00',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',

'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
'\x00','\x00','\x00','\x00','\x01','\x03','\x03','\x03',
'\x06','\x06','\x04','\x04','\x04','\x04','\x04','\x04',
'\x02','\x02','\x01','\x00','\x04','\x04','\x04','\x07',
'\x07','\x07','\x07','\x04','\x04','\x04','\x00','\x00',
'\x00','\x01','\x07','\x07','\x07','\x07','\x04','\x04',
'\x04','\x04','\x06','\x07','\x04','\x04','\x00','\x00',
'\x00','\x00','\x00','\x00','\x04','\x04','\x07','\x07',
'\x07','\x07','\x06','\x04','\x04','\x04','\x04','\x04',
'\x07','\x07','\x07','\x07','\x04','\x04','\x04','\x00',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x04',
'\x04','\x04','\x07','\x07','\x07','\x07','\x04','\x04',
'\x04','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',

'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',

'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00'
}; /* end of character *bitMap */

uint16 lcd_port_image = 0;

////////////////////////////////////////////////////////////////////////////////
// Local Function Prototypes
////////////////////////////////////////////////////////////////////////////////
void Display_small_char(uint8 number, uint8 position, uint8 line, uint8 polarity, uint8 bConfig);
void Display_large_char(uint8 number, uint8 position, uint8 line, uint8 polarity);
void Clear_line(uint8 line);
void Clear_screen(void);
uint8 Bit_Swap(uint8 data);

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#if 0 /* unused */
void WriteLCD_Vline(uint8 x_pos, uint8 y_pos1, uint8 y_pos2, uint8 bLineType)
{
	uint8 ColAddr;
	uint8 v_low;
	uint8 temp;
	volatile uint8 lcd_data;
	uint8 first_time;
	uint8 even = 0;
	uint8 iMask;
	uint8 display_half;

	if (x_pos < 64)
	{
		ColAddr = x_pos;
		display_half = FIRST_HALF_DISPLAY;
	}
	else
	{
		ColAddr = x_pos - 64;
		display_half = SECOND_HALF_DISPLAY;
	}
	// assert(y_pos1 >= y_pos2);
	// calculate the horizontal position
	// temp = x_pos;
	// nHighColAddr = temp >> 4; // get the high nibble
	// nLowColAddr = temp & 0x0f; // get the low nibble

	// even =((x_pos%2)==0);

	first_time = YES; // flag to do the first byte
	while (y_pos2 <= y_pos1)
	{
		// calculate the vertical line end position, 0-7
		temp = y_pos2/8;
		v_low = temp; // get the low byte

		Write_display(COMMAND_REGISTER, COLUMN_ADDRESS_SET + ColAddr, display_half);
		Write_display(COMMAND_REGISTER, PAGE_ADDRESS_SET + v_low, display_half); // set the vertical line
		//Write_display(COMMAND_REGISTER, COLUMN_ADDRESS_HIGH + nHighColAddr); // set the horizontal position
		//Write_display(COMMAND_REGISTER, COLUMN_ADDRESS_LOW + nLowColAddr);

		// since we may be writing data to a partial line we need to read the
		// data on the existing line and add the new data to it

		//Write_display(COMMAND_REGISTER,READ_MODIFY_WRITE); // disable the auto inc on read (write will still auto-inc)
		lcd_data = Read_display(DATA_REGISTER, display_half); // get the existing data from the display
		lcd_data = Read_display(DATA_REGISTER, display_half); // get the existing data from the display
		Write_display(COMMAND_REGISTER, COLUMN_ADDRESS_SET + ColAddr, display_half);
		Write_display(COMMAND_REGISTER, PAGE_ADDRESS_SET + v_low, display_half); // set the vertical line
		//lcd_data = Bit_Swap(lcd_data);
		//Write_display(COMMAND_REGISTER,END); // enable the auto inc

		//note: LINECOLOR_WHITE is not implmented
		if (first_time == YES)
		{
			iMask = (y_pos2 % 8);
			iMask = (0xFF << (y_pos2 % 8));
			//iMask = (0xFF >> (y_pos2 % 8));
			lcd_data &= ~iMask;
			if (bLineType == LINECOLOR_DOTTED)
			{
			iMask &= (even) ? 0xAA : 0x55;
			}

			else
			{
			iMask &= 0xFF;
			}
			if (bLineType == LINECOLOR_WHITE)
			{
			lcd_data &= ~iMask;
			}

			else
			{//black or dotted
			lcd_data |= iMask;
			}

			y_pos2 += (8-(y_pos2 % 8)); //move end up to the bottom of the next 8-row up
			first_time = NO; // clear the flag
		}

		else if ((y_pos1 - y_pos2) >= 8)
		{ // not the first time & we have a whole line
			if (bLineType == LINECOLOR_DOTTED)
			{
			lcd_data |= (even) ? 0xAA : 0x55;
			}

			else
			{
				lcd_data = 0xff;
			}
			y_pos2 += 8;

		}

		else
		{// this is the last line

			//iMask = ~(0xFF << ((y_pos1 % 8)+1));
			//lcd_data &= ~iMask;
			iMask = (y_pos1 % 8);
			iMask = 7 - iMask;
			iMask = (0xFF >> (7 - (y_pos1 % 8)));
			lcd_data &= ~iMask;
			//iMask = ~iMask;

			if (bLineType == LINECOLOR_DOTTED)
			{
			iMask &= (even) ? 0xAA : 0x55;
			}

			else
			{
			iMask &= 0xFF;
			}
			lcd_data |= iMask;

			y_pos2 = y_pos1+1; //make the loop exit
		} //End else

		Write_display(DATA_REGISTER, lcd_data, display_half); // write the new data
	} //End while

} // End of function
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#if 0 /* unused */
void WriteLCD_Hline(uint8 y_pos, uint8 start, uint8 end, uint8 bLineType)
{
	uint8 bTemp;
	uint8 v_low;
	uint8 horizontal;
	uint8 bLineMask;
	volatile uint8 bCurrValue;
	uint8 display_half;

	if (start < 64)
	{
		display_half = FIRST_HALF_DISPLAY;
		horizontal = start;
	}
	else
	{
		display_half = SECOND_HALF_DISPLAY;
		start -= 64;
		end -= 64;
		horizontal = start;
	}
	// calculate the vertical line (8-row)position, 0-7
	v_low = y_pos / 8;
	Write_display(COMMAND_REGISTER, PAGE_ADDRESS_SET + v_low, SECOND_HALF_DISPLAY); // set the vertical line
	Write_display(COMMAND_REGISTER, PAGE_ADDRESS_SET + v_low, FIRST_HALF_DISPLAY); // set the vertical line

	Write_display(COMMAND_REGISTER, COLUMN_ADDRESS_SET, SECOND_HALF_DISPLAY);
	Write_display(COMMAND_REGISTER, COLUMN_ADDRESS_SET + horizontal, display_half);

	// calculate the vertical position within the line
	bTemp = y_pos % 8; // calc how far to shift
	if (bLineType == LINECOLOR_BLACK)
	{
		bLineMask = 1 << bTemp; // move the data bit to the proper location

		for (bTemp = start; bTemp <= end; bTemp++)
		{
			if (bTemp > 63)
			{
			display_half = SECOND_HALF_DISPLAY;
			bTemp = 0;
			start = 0;
			end -= 64;
			}
			bCurrValue = Read_display(DATA_REGISTER, display_half); // get the data from the display
			bCurrValue = Read_display(DATA_REGISTER, display_half); // get the data from the display
			Write_display(COMMAND_REGISTER, COLUMN_ADDRESS_SET + bTemp, display_half);
			Write_display(DATA_REGISTER, (bLineMask | bCurrValue), display_half); // write the line
		}
	}

	else if (bLineType == LINECOLOR_WHITE)
	{
		bLineMask = 1 << bTemp; // move the data bit to the proper location
		//bLineMask = 0x80 >> bTemp; // move the data bit to the proper location
		bLineMask = ~bLineMask;

		for (bTemp = start; bTemp <= end; bTemp++)
		{
			if (bTemp > 63)
			{
			display_half = SECOND_HALF_DISPLAY;
			bTemp = 0;
			start = 0;
			end -= 64;
			}
			bCurrValue = Read_display(DATA_REGISTER, display_half); // get the data from the display
			Write_display(COMMAND_REGISTER, COLUMN_ADDRESS_SET + bTemp, display_half);
			Write_display(DATA_REGISTER, (bLineMask & bCurrValue), display_half); // write the line
		}
	}//Note: dotted line currently not implemented

	//Write_display(COMMAND_REGISTER, END); // enable the auto inc

} // End of function
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#if 0 /* essentially unused */
void Display_small_char(uint8 number, uint8 h_position, uint8 v_position, uint8 polarity, uint8 bConfig)
{
	uint8 bTemp;
	uint8 v_low = 0;
	uint8 temp;
	uint8 lcd_data;
	uint8 offset;
	uint8 bDoingLeadingCol = FALSE;
	uint8 bBlankingError = FALSE;
	uint8 display_half;
	char horizontal;

	uint16 character;
	uint16 iCharacterIndex;
	uint16 iLpCnt = 0;

	horizontal = h_position;
	if (horizontal < 64)
	{
		display_half = FIRST_HALF_DISPLAY;
	}
	else
	{
		display_half = SECOND_HALF_DISPLAY;
		horizontal -= 64;
	}

	character = number - 0x20; // get rid of the offset
	character = character * 5; // this will provide the offset into the look up table

	offset = v_position%8; // determine if we are on an even line
	if (offset == 0)
	{ // we are on an even line (8-row) so no read-modify-write is needed

		if (bConfig & PF_FIRST_TIME)
		{
			// calculate the vertical position
			bTemp = v_position >> 3; // = v_position/8;
			v_low = bTemp; // get the low byte
			Write_display(COMMAND_REGISTER, PAGE_ADDRESS_SET + v_low, SECOND_HALF_DISPLAY); // set the vertical postion
			Write_display(COMMAND_REGISTER, PAGE_ADDRESS_SET + v_low, display_half); // set the vertical postion

			// calculate the horizontal position
			bTemp = horizontal;
			if (horizontal > 0)
			{//if there is a col before the char, use it
				bTemp--; //if there is a col before the char, use it
				bDoingLeadingCol = TRUE;
			}
			Write_display(COMMAND_REGISTER, COLUMN_ADDRESS_SET + bTemp, display_half);

			if (bDoingLeadingCol)
			{
				iLpCnt = -1;
				bDoingLeadingCol = FALSE;
			}
		}
		else
		{
			iLpCnt = 0;
		}

		for (; iLpCnt <6; iLpCnt++)
		{
			iCharacterIndex = iLpCnt + character;
			if (iLpCnt == 5) // ((iLpCnt == 5) || (iLpCnt == -1)) // Can't be -1 since both storage types are unsigned
			{
				temp = 0x00; //blank column following a character (or in front of first)
			}

			else
			{//read character font from memory...
				temp = SMALL_CHARACTERS[iCharacterIndex];
				temp = Bit_Swap(temp);
			}
			//if (polarity == REVERSE_LCD)
			//{
			//	temp = ~temp;
			//}
			if (bBlankingError == TRUE)
			{
				if ((uint8)(horizontal + iLpCnt) > 62)
				{
					display_half = SECOND_HALF_DISPLAY;
					Write_display(COMMAND_REGISTER, PAGE_ADDRESS_SET + v_low, display_half); // set the vertical postion
					Write_display(COMMAND_REGISTER, COLUMN_ADDRESS_SET, display_half);
					horizontal = -1 - iLpCnt;
					bBlankingError = FALSE;
				}
			}
			else
			{
				if ((uint8)(horizontal + iLpCnt) > 63)
				{
					display_half = SECOND_HALF_DISPLAY;
					Write_display(COMMAND_REGISTER, PAGE_ADDRESS_SET + v_low, display_half); // set the vertical postion
					Write_display(COMMAND_REGISTER, COLUMN_ADDRESS_SET, display_half);
					horizontal = 0 - iLpCnt;
				}
			}
			Write_display(DATA_REGISTER, (polarity == REVERSE_LCD) ? ~temp : temp, display_half);
		}
	}

	else
	{ // we are between two lines

		if (bConfig & PF_TOP_PART)
		{//do the top half of the line

			if (bConfig & PF_FIRST_TIME)
			{
				//Write_display(COMMAND_REGISTER, START_LINE_SET);

				// calculate the vertical position
				bTemp = (v_position + 8)/8 - 1;
				v_low = bTemp; // get the low byte
				//Write_display(COMMAND_REGISTER, PAGE_ADDRESS_SET + v_low + 1, SECOND_HALF_DISPLAY); // set the vertical postion
				Write_display(COMMAND_REGISTER, PAGE_ADDRESS_SET + v_low + 1, display_half); // set the vertical postion

				// calculate the horizontal position
				bTemp = horizontal;
				if (horizontal > 0)
			{//if there is a col before the char, use it
				bTemp--; //if there is a col before the char, use it
					bDoingLeadingCol = TRUE;
				}
				Write_display(COMMAND_REGISTER, COLUMN_ADDRESS_SET + bTemp, display_half);
			}

			if (bDoingLeadingCol)
			{
				iLpCnt = -1;
				bDoingLeadingCol = FALSE;
			}

			else
			{
				iLpCnt = 0;
			}

			for (; iLpCnt <6; iLpCnt++)
			{// now write the character to the upper 8-row
			iCharacterIndex = iLpCnt + character;
			if (iLpCnt == 5) // ((iLpCnt == 5) || (iLpCnt == -1)) // Can't be -1 since both storage types are unsigned
			{
				temp = 0x00; //blank column following a character (or in front of first)
			}

			else
			{//read character font from memory...
				temp = SMALL_CHARACTERS[iCharacterIndex];
					temp = Bit_Swap(temp);
			}

			if (polarity == REVERSE_LCD)
			{//if reversing the character, reverse the current column
				temp = ~temp;
				}
				if (bBlankingError == TRUE)
				{
				if ((uint8)(horizontal + iLpCnt) > 62)
					{
						display_half = SECOND_HALF_DISPLAY;
					Write_display(COMMAND_REGISTER, PAGE_ADDRESS_SET + v_low + 1, display_half); // set the vertical postion
					Write_display(COMMAND_REGISTER, COLUMN_ADDRESS_SET, display_half);
					horizontal = -1 - iLpCnt;
						bBlankingError = FALSE;
					}

				}
				else
				{
					if ((uint8)(horizontal + iLpCnt) > 63)
					{
						display_half = SECOND_HALF_DISPLAY;
					Write_display(COMMAND_REGISTER, PAGE_ADDRESS_SET + v_low + 1, display_half); // set the vertical postion
					Write_display(COMMAND_REGISTER, COLUMN_ADDRESS_SET, display_half);
					horizontal = 0 - iLpCnt;
					}
				}
			lcd_data = Read_display(DATA_REGISTER, display_half); // get the data from the display
			lcd_data = Read_display(DATA_REGISTER, display_half); // get the data from the display

			lcd_data &= (0xFF << offset);
			bTemp = temp >> (8 - offset); // shift over the new data
			bTemp = lcd_data | bTemp; // add the old and new data
				Write_display(COMMAND_REGISTER, PAGE_ADDRESS_SET + v_low + 1, display_half); // set the vertical postion
				Write_display(COMMAND_REGISTER, COLUMN_ADDRESS_SET + (horizontal + iLpCnt), display_half);
			Write_display(DATA_REGISTER, bTemp, display_half); // send it out to the display
			} // End of for
		} // End of if

		else
		{// do the bottom half of the line

			if (bConfig & PF_FIRST_TIME)
			{
				// calculate the vertical position (8-row) of the bottom part of the character
				bTemp = v_position / 8;
				v_low = bTemp; // get the low nibble
				//Write_display(COMMAND_REGISTER, PAGE_ADDRESS_SET + v_low, SECOND_HALF_DISPLAY); // set the vertical postion
				Write_display(COMMAND_REGISTER, PAGE_ADDRESS_SET + v_low, display_half); // set the vertical postion

				// calculate the horizontal position
				bTemp = horizontal;
				if (horizontal > 0)
			{//if there is a col before the char, use it
				bTemp--; //if there is a col before the char, use it
					bDoingLeadingCol = TRUE;
				}
				Write_display(COMMAND_REGISTER, COLUMN_ADDRESS_SET + bTemp, display_half);
			} // End of if

			if (bDoingLeadingCol)
			{
				iLpCnt = -1;
				bDoingLeadingCol = FALSE;
			}

			else
			{
				iLpCnt = 0;
			}

			for (; iLpCnt <6; iLpCnt++)
			{// now write the character to the upper 8-row
			iCharacterIndex = iLpCnt + character;
			if (iLpCnt == 5) // ((iLpCnt == 5) || (iLpCnt == -1)) // Can't be -1 since both storage types are unsigned
			{
				temp = 0x00; //blank column following a character
			}

			else
			{//read character font from memory...
				temp = SMALL_CHARACTERS[iCharacterIndex];
					temp = Bit_Swap(temp);
			}

			if (polarity == REVERSE_LCD)
			{//if reversing the character, reverse the current column
				temp = ~temp;
			}

				if (bBlankingError == TRUE)
				{
				if ((uint8)(horizontal + iLpCnt) > 62)
					{
						display_half = SECOND_HALF_DISPLAY;
					Write_display(COMMAND_REGISTER, PAGE_ADDRESS_SET + v_low, display_half); // set the vertical postion
					Write_display(COMMAND_REGISTER, COLUMN_ADDRESS_SET, display_half);
					horizontal = -1 - iLpCnt;
						bBlankingError = FALSE;
					}

				}
				else
				{
					if ((uint8)(horizontal + iLpCnt) > 63)
					{
						display_half = SECOND_HALF_DISPLAY;
					Write_display(COMMAND_REGISTER, PAGE_ADDRESS_SET + v_low, display_half); // set the vertical postion
					Write_display(COMMAND_REGISTER, COLUMN_ADDRESS_SET, display_half);
					horizontal = 0 - iLpCnt;
					}
				}

				lcd_data = Read_display(DATA_REGISTER, display_half); // get the data from the display
			lcd_data = Read_display(DATA_REGISTER, display_half); // get the data from the display

			lcd_data &= (0xFF >> (8-offset));
			bTemp = temp << offset; // shift over the new data
			bTemp = lcd_data | bTemp; // combine the old and new data

			Write_display(COMMAND_REGISTER, PAGE_ADDRESS_SET + v_low, display_half); // set the vertical postion
				Write_display(COMMAND_REGISTER, COLUMN_ADDRESS_SET + (horizontal + iLpCnt), display_half);
			Write_display(DATA_REGISTER, bTemp, display_half); // now write the data to the display
			} // End of for
		} // End of if
	} // End if if
} // End of function
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#if 0 /* unused */
void WriteLCD_lgText(uint8 line, uint8 position, const uint8 *lcd_data, uint8 polarity)
{
	uint8 data_byte;
	uint8 col;

	col = position;
	// now send out the data
	while (*lcd_data != 0x00) // send out data until null char
	{
		data_byte = *lcd_data++; // get the data
		Display_large_char(data_byte, col, line, polarity); // send out the data
		col += 11; // move over to the next space
	}
} // End of function
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#if 0 /* essentially unused */
void Display_large_char(uint8 number, uint8 position, uint8 line, uint8 polarity)
{
	uint8 index;
	uint8 low = 0;
	uint8 lcd_data;
	uint8 display_half;
	uint16 character;
	uint16 y;

	if (position < 64)
	{
		display_half = FIRST_HALF_DISPLAY;
	}
	else
	{
		display_half = SECOND_HALF_DISPLAY;
		position -= 64;
	}
	// calculate the horizontal position
	//dummy = position;
	//high = position >> 4; // get the high byte
	//low = position & 0x0f; // get the low byte

	character = (number - 0x30) * 10; // get rid off the offset

	// display the top half of the character
	//Write_display(COMMAND_REGISTER, START_LINE_SET);
	Write_display(COMMAND_REGISTER, PAGE_ADDRESS_SET + line-1, display_half);
	//Write_display(COMMAND_REGISTER, COLUMN_ADDRESS_HIGH + high);
	Write_display(COMMAND_REGISTER, COLUMN_ADDRESS_SET + position, display_half);
	if (polarity == REVERSE_LCD) //this will add a blank column in front of the char
	{
		if (low > 0) // do we have room?
		{
			Write_display(COMMAND_REGISTER, COLUMN_ADDRESS_SET + position - 1, display_half);
			//Write_display(COMMAND_REGISTER, COLUMN_ADDRESS_LOW + low - 1);
			Write_display(DATA_REGISTER, 0xFF, display_half);
		}
	}
	else
	{
		Write_display(COMMAND_REGISTER, COLUMN_ADDRESS_SET + position - 1, display_half);
		//Write_display(COMMAND_REGISTER, COLUMN_ADDRESS_LOW + low - 1);
		Write_display(DATA_REGISTER, 0x00, display_half);
	}
	//Write_display(COMMAND_REGISTER, COLUMN_ADDRESS_LOW + low);

	for (index = 0; index <10; index++)
	{
		y = index + character;
		lcd_data = LARGE_NUMBER_TOP[y];
		lcd_data = Bit_Swap(lcd_data);

		Write_display(DATA_REGISTER, (polarity == REVERSE_LCD) ? (0xFF-lcd_data) : lcd_data, display_half);
	}

	Write_display(DATA_REGISTER, (polarity == REVERSE_LCD) ? 0xFF : 0x00, display_half);

	// display the bottom half of the character
	//Write_display(COMMAND_REGISTER, START_LINE_SET);
	Write_display(COMMAND_REGISTER, PAGE_ADDRESS_SET + line-2, display_half);
	//Write_display(COMMAND_REGISTER, COLUMN_ADDRESS_HIGH + high);
	Write_display(COMMAND_REGISTER, COLUMN_ADDRESS_SET + position, display_half);
	if (polarity == REVERSE_LCD)
	{
		if (low > 0) // do we have room?
		{
			Write_display(COMMAND_REGISTER, COLUMN_ADDRESS_SET + position - 1, display_half);
			//Write_display(COMMAND_REGISTER, COLUMN_ADDRESS_LOW + low - 1);
			Write_display(DATA_REGISTER, 0xFF, display_half);
		}
	}
	else
	{
		Write_display(COMMAND_REGISTER, COLUMN_ADDRESS_SET + position - 1, display_half);
		//Write_display(COMMAND_REGISTER, COLUMN_ADDRESS_LOW + low - 1);
		Write_display(DATA_REGISTER, 0x00, display_half);
	}
	//Write_display(COMMAND_REGISTER, COLUMN_ADDRESS_LOW + low);

	character = (number - 0x30) * 10; // get rid off the offset

	for (index = 0; index <10; index++)
	{
		y = index + character;
		lcd_data = LARGE_NUMBER_BOT[y];
		lcd_data = Bit_Swap(lcd_data);

		Write_display(DATA_REGISTER, (polarity == REVERSE_LCD) ? (0xFF-lcd_data) : lcd_data, display_half);
	}

	Write_display(DATA_REGISTER, (polarity == REVERSE_LCD) ? 0xFF : 0x00, display_half);

} //End of function
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#if 0 /* essentially unused, only in srec */
void WriteLCD_smText(uint8 x_pos, uint8 y_pos, const uint8 *lcd_data, uint8 polarity)
{
	uint8 data_byte;
	uint8 xprint_pos;
	uint8 bPrintConfig = PF_FIRST_TIME;
	const uint8 *strPtr;

	strPtr = lcd_data;
	xprint_pos = x_pos;

	//send out the data until null char
	while (*strPtr != 0x00)
	{
		data_byte = *strPtr; // get the data
		Display_small_char(data_byte,
							xprint_pos,
						y_pos,
						polarity,
						bPrintConfig); // send out the data
		bPrintConfig = 0;
		strPtr++; // next character
		xprint_pos += 6; // move over to the next space
	}

	//if there is a top row to paint (not on an even 8-row), do it now
	if (y_pos%8)
	{
		xprint_pos = x_pos;
		bPrintConfig = (PF_FIRST_TIME | PF_TOP_PART);
		strPtr = lcd_data;
		//send out the top line string until null char
		while (*strPtr != 0x00)
		{
			data_byte = *strPtr; // get the data
			Display_small_char(data_byte,
								xprint_pos,
						y_pos,
						polarity,
						bPrintConfig); // send out the data

			bPrintConfig = PF_TOP_PART;
			strPtr++; // next character
			xprint_pos += 6; // move over to the next space
		}

	} // End of if
} // End of function
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#if 0 /* essentially unused */
void ClearLCDscreen(void)
{
	uint8 count;
	for (count = 0; count < 8; count++)
	{
		Clear_line(count);
	}
}
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#if 0 /* essentially unused */
void Clear_line(uint8 line)
{
	uint8 column;

	Write_display(COMMAND_REGISTER, PAGE_ADDRESS_SET + line, FIRST_HALF_DISPLAY);
	Write_display(COMMAND_REGISTER, PAGE_ADDRESS_SET + line, SECOND_HALF_DISPLAY);
	//Write_display(COMMAND_REGISTER, COLUMN_ADDRESS_HIGH);
	Write_display(COMMAND_REGISTER, COLUMN_ADDRESS_SET, FIRST_HALF_DISPLAY);
	Write_display(COMMAND_REGISTER, COLUMN_ADDRESS_SET, SECOND_HALF_DISPLAY);

	for (column = 0; column < 64; column++)
	{
		Write_display(DATA_REGISTER, 0x00, FIRST_HALF_DISPLAY);
	} // End if for

	for (column = 0; column < 64; column++)
	{
		Write_display(DATA_REGISTER, 0x00, SECOND_HALF_DISPLAY);
	} // End if for

	//for (column = 0; column < 64; column++)
	//{
	//	lcd_data = Read_display(DATA_REGISTER, FIRST_HALF_DISPLAY);
	//} // End if for

	//for (column = 0; column < 64; column++)
	//{
	//	lcd_data = Read_display(DATA_REGISTER, SECOND_HALF_DISPLAY);
	//} // End if for

} //End of function
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#if 0 /* unused */
void InitDisplay(void)
{
}
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#if 0 /* essentially unused */
void Write_display(uint8 lcd_register, uint8 lcd_data, uint8 display_half)
{
}
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#if 0 /* unused */
void Write_multi_display(uint8 lcd_register, uint8 lcd_data, uint8 display_half)
{
	UNUSED(lcd_register);
	UNUSED(lcd_data);
	UNUSED(display_half);
}
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#if 0 /* unused */
uint8 Read_display(uint8 lcd_register, uint8 display_half)
{
}
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#if 0 /* unused */
void Backlight_On(void)
{
}
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#if 0 /* unused */
void Backlight_Off(void)
{
}
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#if 0 /* unused */
void Backlight_High(void)
{
}
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#if 0 /* unused */
void Backlight_Low(void)
{
}
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#if 0 /* unused */
void Reset_Contrast(void)
{
}
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#if 0 /* unused */
void Set_Contrast(uint8 level)
{
}
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8 Bit_Swap(uint8 data)
{
	uint8 tempData = 0;

	if (data & 0x01)
	{
		tempData += 0x80;
	}
	if (data & 0x02)
	{
		tempData += 0x40;
	}
	if (data & 0x04)
	{
		tempData += 0x20;
	}
	if (data & 0x08)
	{
		tempData += 0x10;
	}
	if (data & 0x10)
	{
		tempData += 0x08;
	}
	if (data & 0x20)
	{
		tempData += 0x04;
	}
	if (data & 0x40)
	{
		tempData += 0x02;
	}
	if (data & 0x80)
	{
		tempData += 0x01;
	}

	return(tempData);
}

/*
	New display size: 480 x 272 (Wrong, actually 800x480)
	Old display size: 128 x 64
	Scale comparison: 3.75 x 4.25
	Memory map size difference: 8192 bytes vs 130560 bytes

	Scale width x3: 96 bits extra, 48 bits per side
	Scale length x4: 16 bits extra, 8 bits per side
	Scale length x3: 80 bits extra, 40 bits per side

	Test font character scaling:

	Original 6x8 font of 'A':
	-000--
	0---0-
	0---0-
	0---0-
	00000-
	0---0-
	0---0-
	------

	Scaled 3xH x 3xV:
	---000000000------ 1
	---000000000------ 1
	---000000000------ 1
	000---------000--- 2
	000---------000--- 2
	000---------000--- 2
	000---------000--- 3
	000---------000--- 3
	000---------000--- 3
	000---------000--- 4
	000---------000--- 4
	000---------000--- 4
	000000000000000--- 5
	000000000000000--- 5
	000000000000000--- 5
	000---------000--- 6
	000---------000--- 6
	000---------000--- 6
	000---------000--- 7
	000---------000--- 7
	000---------000--- 7
	------------------ 8
	------------------ 8
	------------------ 8

	Scaled 3xH x 4xV:
	---000000000------ 1
	---000000000------ 1
	---000000000------ 1
	---000000000------ 1
	000---------000--- 2
	000---------000--- 2
	000---------000--- 2
	000---------000--- 2
	000---------000--- 3
	000---------000--- 3
	000---------000--- 3
	000---------000--- 3
	000---------000--- 4
	000---------000--- 4
	000---------000--- 4
	000---------000--- 4
	000000000000000--- 5
	000000000000000--- 5
	000000000000000--- 5
	000000000000000--- 5
	000---------000--- 6
	000---------000--- 6
	000---------000--- 6
	000---------000--- 6
	000---------000--- 7
	000---------000--- 7
	000---------000--- 7
	000---------000--- 7
	------------------ 8
	------------------ 8
	------------------ 8
	------------------ 8
 */

///============================================================================
///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
///============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "spi.h"

/*
 * Constants/Statics/Globals
 */
// FT81X section 5.2 Chip ID
uint16_t ft81x_chip_id = 0;
// FT81X command buffer free space
uint16_t ft81x_fifo_freespace = 0;
// FT81X command buffer write pointer
uint16_t ft81x_fifo_wp = 0;
// FT81X enable QIO modes
uint8_t ft81x_qio = 0;
// FT81x screen width
uint16_t ft81x_display_width = 0;
// FT81x screen height
uint16_t ft81x_display_height = 0;

// MEDIA FIFO state vars
uint32_t mf_wp = 0;
uint32_t mf_size = 0;
uint32_t mf_base = 0;

/*
 * Core functions
 */

/*
 * Wrapper for CS to allow easier debugging
 */
void ft81x_assert_cs(bool assert)
{
	if (FT81X_SPI_2_SS_CONTROL_MANUAL)
	{
		if (assert) { MXC_GPIO_OutClr(GPIO_SPI_2_SS_0_LCD_PORT, GPIO_SPI_2_SS_0_LCD_PIN); }
		else { MXC_GPIO_OutSet(GPIO_SPI_2_SS_0_LCD_PORT, GPIO_SPI_2_SS_0_LCD_PIN); }
	}
	else // SPI2 Slave select controlled by the SPI driver
	{
		// SPI Master Slave Select Control
		// 0: Slave Select is deasserted at the end of a transmission
		// 1: Slave Select stays asserted at the end of a transmission
		// Really doesn't look like direct control, therefore the following is likely useless

		// Attempt direct control (unlikely to work)
		if (assert) { MXC_SPI2->ctrl0 |= MXC_F_SPI_CTRL0_SS_CTRL; }
		else { MXC_SPI2->ctrl0 &= ~MXC_F_SPI_CTRL0_SS_CTRL; }
	}
}

void restart_core(void)
{
	// FIXME: Host Commands must be sent in single byte mode. Just in case the ft81x is stuck
	// in QUAD mode attempt to change it to single byte mode. Set the Quad Mode flag so that
	// the Write tries to perform it in that mode, then reset the flag for single byte mode.
	ft81x_qio = 1;
	ft81x_wr16(REG_SPI_WIDTH, LCD_SPI_WIDTH_SINGLE);
	ft81x_qio = 0;
	// Put the FT81x to sleep.
	ft81x_hostcmd(CMD_SLEEP);
	// Set default clock speed.
	ft81x_hostcmd_param(CMD_CLKSEL, 0x00);
	// Performing a read at address zero will return to an Active mode. Documentation suggests
	// doing two reads followed by at least a 20ms delay when returning from a Sleep state to
	// allow things to settle.
	ft81x_rd(CMD_ACTIVE);
	ft81x_rd(CMD_ACTIVE);

	// Delay (20ms)
	MXC_Delay(MXC_DELAY_MSEC(20));

	// Select internal clock (default), which may cause a system reset.
	ft81x_hostcmd(CMD_CLKINT);
	// Power up all all ROMs.
	ft81x_hostcmd_param(CMD_PD_ROMS, 0x00);
	// Power up without resetting the things just done above.
	ft81x_hostcmd(CMD_RST_PULSE);
}

bool read_chip_id()
{
	// Read CHIP ID address until it returns a valid result.
	for (uint16_t count = 0; count < 100; count++) {
		ft81x_chip_id = ft81x_rd16(MEM_CHIP_ID);
		// Chip id: 08h, [id], 01h, 00h
		// [id]: FT8xx=10h, 11h, 12h, 13h
		if ((ft81x_chip_id) == 0x0810) { return (true); } // Chip version is FT810

		// Sleep (10ms)
		MXC_Delay(MXC_DELAY_MSEC(10));
	};

	ft81x_chip_id = 0;
	return false;
}

void select_spi_byte_width()
{
	// Enable QUAD spi mode if configured
#if (FT81X_QUADSPI)
	// Enable QAUD spi mode no dummy
	ft81x_wr16(REG_SPI_WIDTH, LCD_SPI_WIDTH_QUAD);
	ft81x_qio = 1;
#else
	// Enable single channel spi mode
	ft81x_wr16(REG_SPI_WIDTH, LCD_SPI_WIDTH_SINGLE);
	ft81x_qio = 0;
#endif
}

void ft81x_init_display_settings()
{
	/*
		Reference display settings (different LCD screen)
		// Screen specific settings
		// NHD-7.0-800480FT-CSXV-CTP
		// http://newhavendisplay.com/learnmore/EVE2_7-CSXV-CTP/
		ft81x_wr32(REG_HCYCLE, 900);
		ft81x_wr32(REG_HOFFSET, 43);
		ft81x_wr32(REG_HSIZE, FT81X_DISPLAY_WIDTH);
		ft81x_wr32(REG_HSYNC0, 0);
		ft81x_wr32(REG_HSYNC1, 41);
		ft81x_wr32(REG_VCYCLE, 500);
		ft81x_wr32(REG_VOFFSET, 12);
		ft81x_wr32(REG_VSIZE, FT81X_DISPLAY_HEIGHT);
		ft81x_wr32(REG_VSYNC0, 0);
		ft81x_wr32(REG_VSYNC1, 10);
		ft81x_wr32(REG_DITHER, 1);
		ft81x_wr32(REG_PCLK_POL, 1);
		ft81x_wr(REG_ROTATE, 0);
		ft81x_wr(REG_SWIZZLE, 0);
	*/

	/*
		// Screen specific settings
		// Pulling from: NHD-4.3-480272FT datasheet, but different package/options (display settings should be the same)
		// NHD-4.3-480272FT-CSXP-T
		ft81x_wr32(REG_HCYCLE, 548);
		ft81x_wr32(REG_HOFFSET, 43);
		ft81x_wr32(REG_HSIZE, FT81X_DISPLAY_WIDTH);
		ft81x_wr32(REG_HSYNC0, 0);
		ft81x_wr32(REG_HSYNC1, 41);
		ft81x_wr32(REG_VCYCLE, 292);
		ft81x_wr32(REG_VOFFSET, 12);
		ft81x_wr32(REG_VSIZE, FT81X_DISPLAY_HEIGHT);
		ft81x_wr32(REG_VSYNC0, 0);
		ft81x_wr32(REG_VSYNC1, 10);
		ft81x_wr32(REG_DITHER, 1);
		ft81x_wr32(REG_PCLK_POL, 1);
		ft81x_wr(REG_ROTATE, 0);
		ft81x_wr(REG_SWIZZLE, 0);
	*/

	// Screen specific settings
	// Pulling from: NHD-4.3-800480FT-CSXP-CTP reference, but different package/options (display settings should be the same)
	// NHD-4.3-800480CF-ASXP
	ft81x_wr32(REG_HCYCLE, 928);
	ft81x_wr32(REG_HOFFSET, 88);
	ft81x_wr32(REG_HSIZE, FT81X_DISPLAY_WIDTH);
	ft81x_wr32(REG_HSYNC0, 0);
	ft81x_wr32(REG_HSYNC1, 48);
	ft81x_wr32(REG_VCYCLE, 525);
	ft81x_wr32(REG_VOFFSET, 32);
	ft81x_wr32(REG_VSIZE, FT81X_DISPLAY_HEIGHT);
	ft81x_wr32(REG_VSYNC0, 0);
	ft81x_wr32(REG_VSYNC1, 3);
	ft81x_wr32(REG_DITHER, 1);
	ft81x_wr32(REG_PCLK_POL, 1);
	ft81x_wr(REG_ROTATE, 0);
	ft81x_wr(REG_SWIZZLE, 0);

	// Get screen size W,H to confirm
	ft81x_display_width = ft81x_rd16(REG_HSIZE);
	ft81x_display_height = ft81x_rd16(REG_VSIZE);
	debug("LCD: Horizontal/Width: %i, Vertical/Height: %i\r\n", ft81x_display_width, ft81x_display_height);
}

void ft81x_init_gpio()
{
	// Setup the FT81X GPIO PINS. These assume little-endian.
	// DISP = output, GPIO 0 to 2 are unconnected so set to output, GPIO 3 is not included in the 810 version so leave default (input)
	ft81x_wr16(REG_GPIOX_DIR, 0x8007);

	// Retain default settings, skip writing
	//ft81x_wr16(REG_GPIOX, 0x8000);

	// Sleep a little (100ms)
	MXC_Delay(MXC_DELAY_MSEC(100));
}

#if 1 // Test with black screen before starting display clock
void test_black_screen()
{
	// Build a black display and display it
	ft81x_stream_start(); // Start streaming
	ft81x_cmd_dlstart();  // Set REG_CMD_DL when done
	ft81x_cmd_swap();     // Set AUTO swap at end of display list
	ft81x_clear_color_rgb32(0x000000);
	ft81x_clear();
	ft81x_display();
	ft81x_getfree(0);     // trigger FT81x to read the command buffer
	ft81x_stream_stop();  // Finish streaming to command buffer
	ft81x_wait_finish();  // Wait till the GPU is finished processing the commands

	// Sleep a little (100ms)
	MXC_Delay(MXC_DELAY_MSEC(100));
}
#else
#define test_black_screen()
#endif

#if 1
// Display the built in FTDI logo animation and then calibrate
void test_logo(
)
{
	ft81x_logo();

	// Todo: look into calibrate further
	//ft81x_calibrate();
}
#else
#define test_logo()
#endif

#if 1
	// Test LOAD_IMAGE ON and OFF with a transparent PNG and update when touched
void test_load_image(void)
{
	uint32_t imgptr, widthptr, heightptr;
	uint32_t ptron, ptroff, ptrnext, width, height;

	UNUSED(ptrnext);
	UNUSED(width);
	UNUSED(height);

	// wakeup the display and set brightness
	ft81x_wake(22);

	// Load the OFF image to the MEDIA FIFO
	//// Start streaming
	ft81x_stream_start();

	//// Configure MEDIA FIFO
	ft81x_cmd_mediafifo(0x100000UL-0x40000UL, 0x40000UL);

	//// Trigger FT81x to read the command buffer
	ft81x_getfree(0);

	//// Finish streaming to command buffer
	ft81x_stream_stop();

	//// Wait till the GPU is finished
	ft81x_wait_finish();

	//// stop media fifo
	ft81x_wr32(REG_MEDIAFIFO_WRITE, 0);

	//// Load the image at address 0
	ptroff = 0;

	// Load the OFF image
	//// Start streaming
	ft81x_stream_start();

	//// USE MEDIA_FIFO
	//// Load the image at address transparent_test_file_png_len+1
	ft81x_cmd_loadimage(ptroff, OPT_RGB565 | OPT_NODL | OPT_MEDIAFIFO);

	//// Get the decompressed image properties
	ft81x_cmd_getprops(&imgptr, &widthptr, &heightptr);

	//// Trigger FT81x to read the command buffer
	ft81x_getfree(0);

	//// Finish streaming to command buffer
	ft81x_stream_stop();

	//// Send the image to the media fifo
#if 0 /* Supply image */
	ft81x_cSPOOL_MF(<pointer to png image>, <png image size>);
#endif

	//// Wait till the GPU is finished
	ft81x_wait_finish();

	//// Dump results
	ptron = ft81x_rd32(imgptr); // pointer to end of image and start of next free memory
	width = ft81x_rd32(widthptr);
	height = ft81x_rd32(heightptr);

	// Load the OFF image
	//// Start streaming
	ft81x_stream_start();

	//// USING CMD BUFFER. Max size is ~4k
	//// Load the image at address transparent_test_file_png_len+1 using CMD buffer
	ft81x_cmd_loadimage(ptron, OPT_RGB565 | OPT_NODL);

	//// spool the image to the FT81X
#if 0 /* Supply image */
	ft81x_cSPOOL(<pointer to png image>, <png image size>);
#endif

	//// Get the decompressed image properties
	ft81x_cmd_getprops(&imgptr, &widthptr, &heightptr);

	//// Trigger FT81x to read the command buffer
	ft81x_getfree(0);

	//// Finish streaming to command buffer
	ft81x_stream_stop();

	//// Wait till the GPU is finished
	ft81x_wait_finish();

	//// Dump results
	ptrnext = ft81x_rd32(imgptr); // pointer to end of image and start of next free memory
	width = ft81x_rd32(widthptr);
	height = ft81x_rd32(heightptr);

	// Capture input events and update the image if touched
	for (int x=0; x<1000; x++) {

		// Start streaming
		ft81x_stream_start();

		// Define the bitmap we want to draw
		ft81x_cmd_dlstart();  // Set REG_CMD_DL when done
		ft81x_cmd_swap();     // Set AUTO swap at end of display list

		// Clear the display
		ft81x_clear_color_rgb32(0xfdfdfd);
		ft81x_clear();
		// Draw the image
		ft81x_bitmap_source(ptroff);

		// Turn on tagging
		ft81x_tag_mask(1);

		ft81x_bitmap_layout(ARGB4, 75*2, 75);
		ft81x_bitmap_size(NEAREST, BORDER, BORDER, 75, 75);
		ft81x_begin(BITMAPS);

		ft81x_tag(3); // tag the image button #3
		ft81x_vertex2ii(100, 100, 0, 0);

		// stop tagging
		ft81x_tag_mask(0);

		// end of commands
		ft81x_end();
		ft81x_display();

		// Trigger FT81x to read the command buffer
		ft81x_getfree(0);

		// Finish streaming to command buffer
		ft81x_stream_stop();

		//// Wait till the GPU is finished
		ft81x_wait_finish();

		// Sleep (10ms)
		MXC_Delay(MXC_DELAY_MSEC(10));
	}
}
#else
#define test_load_image()
#endif

#if 1
// Test memory operation(s) and CRC32 on 6 bytes of 0x00 will be 0xB1C2A1A3
void test_memory_ops(
)
{
	// Start streaming
	ft81x_stream_start();

	// Write a predictable sequence of bytes to a memory location
	ft81x_cmd_memzero(0, 0x0006);

	// Calculate crc on the bytes we wrote
	uint32_t r = ft81x_cmd_memcrc(0, 0x0006);

	// Trigger FT81x to read the command buffer
	ft81x_getfree(0);

	// Finish streaming to command buffer
	ft81x_stream_stop();

	// Wait till the GPU is finished
	ft81x_wait_finish();

	// Dump results
	uint32_t res = ft81x_rd32(r);
	if (res == 0xB1C2A1A3) { debug("LCD: Memory operation and CRC32 verified\r\n"); }
	else { debugErr("LCD: Memory operation and CRC32 failed, expected 0xB1C2A1A3 but returned 0x%4x\r\n", res); }
}
#else
#define test_memory_ops()
#endif

#if 1
// Draw a gray screen and write Hello World, 123, button etc.
void test_display(
)
{
	ft81x_set_backlight_level(8); // Values range from 0 to 128, 0 is no backlight, 128 is max backlight

	// Wait till the GPU is finished
	for (int x=0; x<300; x++)
	{
		// Sleep (200ms)
		MXC_Delay(MXC_DELAY_MSEC(200));

		ft81x_stream_start(); // Start streaming
		ft81x_cmd_dlstart();  // Set REG_CMD_DL when done
		ft81x_cmd_swap();     // Set AUTO swap at end of display list
		ft81x_clear_color_rgb32(0xfdfdfd);
		ft81x_clear();
		ft81x_color_rgb32(0x101010);
		ft81x_bgcolor_rgb32(0xff0000);
		ft81x_fgcolor_rgb32(0x0000ff);

		// Turn off tagging
		ft81x_tag_mask(0);

		// Draw some text
		ft81x_cmd_text((FT81X_DISPLAY_WIDTH / 2), (FT81X_DISPLAY_HEIGHT / 2), 30, OPT_CENTER, "Hello World");

		ft81x_bgcolor_rgb32(0x007f7f);
		ft81x_cmd_clock((FT81X_DISPLAY_WIDTH - 40),40,30,0,12,1,2,4);

		// Turn on tagging
		ft81x_tag_mask(1);

		// Add buttons to the bottom
		ft81x_cmd_button(0, (FT81X_DISPLAY_HEIGHT - 64), (FT81X_DISPLAY_WIDTH / 4), 64, 18, 0, "OK");
		ft81x_cmd_button((FT81X_DISPLAY_WIDTH / 4), (FT81X_DISPLAY_HEIGHT - 64), (FT81X_DISPLAY_WIDTH / 4), 64, 18, 0, "ESCAPE");
		ft81x_cmd_button((FT81X_DISPLAY_WIDTH / 2), (FT81X_DISPLAY_HEIGHT - 64), (FT81X_DISPLAY_WIDTH / 4), 64, 18, 0, "MENU");
		ft81x_cmd_button((FT81X_DISPLAY_WIDTH * 3 / 4), (FT81X_DISPLAY_HEIGHT - 64), (FT81X_DISPLAY_WIDTH / 4), 64, 18, 0, "HELP");

		uint8_t tstate = rand()%((253+1)-0) + 0;
		if(tstate > 128)
			ft81x_bgcolor_rgb32(0x00ff00);
		else
			ft81x_bgcolor_rgb32(0xff0000);

		ft81x_tag(5); // tag the spinner #5
		//ft81x_cmd_toggle(70, 70, 60, 18, 0, tstate > 128 ? 0 : 65535, "YES\xffNO");

		// Turn off tagging
		ft81x_tag_mask(0);

		// Draw a keyboard
		ft81x_cmd_keys(((FT81X_DISPLAY_WIDTH / 2) - 60), ((FT81X_DISPLAY_HEIGHT / 2) + 80), 120, 30, 26, 0, "1234");

		// FIXME: Spinner if used above does odd stuff? Seems ok at the end of the display.
		ft81x_cmd_spinner(80, 80, 3, 0);

		ft81x_display();
		ft81x_getfree(0);     // trigger FT81x to read the command buffer
		ft81x_stream_stop();  // Finish streaming to command buffer

		//// Wait till the GPU is finished
		ft81x_wait_finish();
	}
}
#else
#define test_display()
#endif

#if 1
// Fill the screen with a solid color cycling colors
void test_cycle_colors(
)
{
	uint32_t rgb = 0xff0000;
	for (int x=0; x<300; x++) {
		ft81x_stream_start(); // Start streaming
		ft81x_cmd_dlstart();  // Set REG_CMD_DL when done
		ft81x_cmd_swap();     // Set AUTO swap at end of display list
		ft81x_clear_color_rgb32(rgb);
		ft81x_clear();
		ft81x_color_rgb32(0xffffff);
		ft81x_bgcolor_rgb32(0xffffff);
		ft81x_fgcolor_rgb32(0xffffff);
		ft81x_display();
		ft81x_getfree(0);     // trigger FT81x to read the command buffer
		ft81x_stream_stop();  // Finish streaming to command buffer

		// rotate colors
		rgb>>=8; if(!rgb) rgb=0xff0000;

		// Sleep (100ms)
		MXC_Delay(MXC_DELAY_MSEC(100));
	}
}
#else
#define test_cycle_colors()
#endif

#if 1
// Draw some dots of rand size, location and color.
void test_dots(
)
{
	for (int x=0; x<300; x++) {
		ft81x_stream_start(); // Start streaming
		//ft81x_alpha_funct(0b111, 0b00000000);
		//ft81x_bitmap_handle(0b10101010);
		ft81x_bitmap_layout(0b11111, 0x00, 0x00);

		ft81x_cmd_dlstart();  // Set REG_CMD_DL when done
		ft81x_cmd_swap();     // Set AUTO swap at end of display list
		ft81x_clear_color_rgb32(0x000000);
		ft81x_clear();

		ft81x_fgcolor_rgb32(0xffffff);
		ft81x_bgcolor_rgb32(0xffffff);

		uint8_t rred, rgreen, rblue;
		rred = rand()%((253+1)-0) + 0;
		rgreen = rand()%((253+1)-0) + 0;
		rblue = rand()%((253+1)-0) + 0;
		ft81x_color_rgb888(rred, rgreen, rblue);
		ft81x_begin(POINTS);
		//uint16_t size = rand()%((600+1)-0) + 0;
		uint16_t size = rand()%((360+1)-0) + 0;
		uint16_t rndx = rand()%((ft81x_display_width+1)-0) + 0;
		uint16_t rndy = rand()%((ft81x_display_height+1)-0) + 0;
		ft81x_point_size(size);
		ft81x_vertex2f(rndx<<4,rndy<<4); // defaut is 1/16th pixel precision
		ft81x_display();
		ft81x_getfree(0);     // trigger FT81x to read the command buffer
		ft81x_stream_stop();  // Finish streaming to command buffer

		// Sleep (100ms)
		MXC_Delay(MXC_DELAY_MSEC(100));
	}

	// Sleep (10ms)
	MXC_Delay(MXC_DELAY_MSEC(10));
}
#else
#define test_dots()
#endif

/*
 * Initialize the FT81x GPU
 */
uint8_t ft81x_init(void)
{
    if (GetPowerControlState(LCD_POWER_ENABLE) == OFF) { PowerControl(LCD_POWER_ENABLE, ON); }
    if (GetPowerControlState(LCD_POWER_DOWN) == ON)
	{
		PowerControl(LCD_POWER_DOWN, OFF);
		MXC_Delay(MXC_DELAY_MSEC(20)); // Per datasheet: From Sleep state, the host needs to wait at least 20ms before accessing any registers or commands
		g_lcdPowerFlag = ENABLED;
	}

	// Bring LCD Controller active, done by issuing two read commands of address 0, and possibly waiting up to 300ms
	ft81x_rd(CMD_ACTIVE);
	ft81x_rd(CMD_ACTIVE);
	MXC_Delay(MXC_DELAY_MSEC(300));

	restart_core();
	if (!read_chip_id()) {
		debugErr("LCD Controller: Failed to read chip ID\r\n");
		return false;
	}
	else { debug("LCD Controller: Chip ID verified\r\n"); }
	select_spi_byte_width();
	ft81x_backlight_off();
	ft81x_fifo_reset();
	ft81x_init_display_settings();
	test_black_screen();
	ft81x_init_gpio();
	return true;
}

/*
 * Initialize the FT81x GPU and test for a valid chip response
 */
uint8_t ft81x_init_gpu(void)
{
	restart_core();
	if (!read_chip_id()) {
		return false;
	}
	select_spi_byte_width();
	ft81x_backlight_off();
	ft81x_fifo_reset();
	ft81x_init_display_settings();
	test_black_screen();
	ft81x_init_gpio();
	test_logo();
	test_load_image();
	test_memory_ops();
	test_display();
	test_cycle_colors();
	test_dots();
	return true;
}

/*
 * Turn off the backlight
 */
void ft81x_backlight_off(void)
{
	// Set PWM to 0
	ft81x_wr(REG_PWM_DUTY, 0);
}

/*
 * Set the backlight level
 */
void ft81x_set_backlight_level(uint8_t backlightLevel)
{
	// Backlight range is 0 (off) to 128 (max brightness) per data sheet

	// Filter max range if set over level
	if (backlightLevel > 128) { backlightLevel = 128; }

	// Set PWM to desired level
	ft81x_wr(REG_PWM_DUTY, backlightLevel);
}

/*
 * Set the backlight level
 */
uint8_t ft81x_get_backlight_level(void)
{
	// Backlight range is 0 (off) to 128 (max brightness) per data sheet

	return (ft81x_rd(REG_PWM_DUTY));
}

/*
 * Turn off the display entering low power mode
 */
void ft81x_sleep()
{
  // Disable the pixel clock
  ft81x_wr32(REG_PCLK, 0);

  // Turn the backlight off
  ft81x_backlight_off();

  // Turn the display off
  ft81x_wr16(REG_GPIOX, ft81x_rd16(REG_GPIOX) | 0x7fff);
}

/*
 * Wake the display up and set pwm level
 */
void ft81x_wake(uint8_t pwm)
{
  // Enable the pixel clock
#if 0 /* NHD-7.0-800480FT-CSXV-CTP */
  ft81x_wr32(REG_PCLK, 3);
#endif
#if 0 /* From NHD-4.3-480272FT-CSXP-T reference */
	ft81x_wr32(REG_PCLK, 5);
#endif
#if 1 /* From NHD-4.3-800480FT-CSXP-CTP reference */
	ft81x_wr32(REG_PCLK, 2);
#endif

  // Set backlight PWM to pwm
  ft81x_set_backlight_level(pwm);

  // Turn the display on
  ft81x_wr16(REG_GPIOX, ft81x_rd16(REG_GPIOX) | 0x8000);
}


/*
 * Initialize our command buffer pointer state vars
 * for streaming mode.
 */
void ft81x_fifo_reset()
{
	ft81x_fifo_wp = ft81x_fifo_rp();
	ft81x_fifo_freespace = MAX_FIFO_SPACE;
}

/*
 * Get our current fifo write state location
 */
uint32_t ft81x_getwp()
{
  return RAM_CMD + (ft81x_fifo_wp & 0xffc);
}

/*
 * Write 16 bit command+arg and dummy byte
 * See 4.1.5 Host Command
 * Must never be sent in QUAD spi mode except for reset
 * A total of 3 bytes will be on the SPI BUS.
 */
void ft81x_hostcmd_param(uint8_t command, uint8_t args)
{
#if 0
	// Using the extended structure and setting SPI_TRANS_VARIABLE_CMD to specify command_bits.
	spi_transaction_ext_t trans;
	// address_bits is not used as the SPI_TRANS_VARIABLE_ADDR flag is not set.
	trans.address_bits = 0;
	// 16 bits of data: arg command byte plus dummy byte.
	uint16_t dargs = args << 8;
	trans.base.flags = SPI_TRANS_VARIABLE_CMD;
	// Fake dummy byte shift left 8
	trans.command_bits = 8;
	trans.base.cmd = command;
	trans.base.tx_buffer = &dargs;
	trans.base.length = 16;
	trans.base.rx_buffer = NULL;
	ft81x_assert_cs(true);
	spi_device_transmit(ft81x_spi, (spi_transaction_t*)&trans);
	ft81x_assert_cs(false);
#else
	uint8_t writeData[3];

	writeData[0] = command; // Top 2 bits must be 01, already built into command defines
	writeData[1] = args;
	writeData[2] = 0x00; // Dummy byte

	if (FT81X_SPI_2_SS_CONTROL_MANUAL) { ft81x_assert_cs(YES); }
	SpiTransaction(MXC_SPI2, SPI_8_BIT_DATA_SIZE, LCD_SPI_DEASERT, writeData, sizeof(writeData), NULL, 0, BLOCKING);
	if (FT81X_SPI_2_SS_CONTROL_MANUAL) { ft81x_assert_cs(NO); }
#endif
}

/*
 * Write 24 bit address + dummy byte
 * and read the 8 bit result.
 * A total of 6 bytes will be on the SPI BUS.
 */
uint8_t ft81x_rd(uint32_t addr)
{
#if 0
  // setup trans memory
  spi_transaction_ext_t trans;
  memset(&trans, 0, sizeof(trans));

  // allocate memory for our return data
  char *recvbuf=heap_caps_malloc(1, MALLOC_CAP_DMA);

  // set trans options.
  trans.base.flags = SPI_TRANS_VARIABLE_ADDR;
  if (ft81x_qio) {
    // Tell the ESP32 SPI ISR to accept MODE_XXX for QIO and DIO
    trans.base.flags |= SPI_TRANS_MODE_DIOQIO_ADDR;
    // Set this transaction to QIO
    trans.base.flags |= SPI_TRANS_MODE_QIO;
  }

  // fake dummy byte shift left 8
  trans.address_bits = 32;
  trans.base.addr = addr << 8;

  trans.base.length = 8;
  trans.base.rxlength = 8;

  // point to our RX buffer.
  trans.base.rx_buffer = recvbuf; // RX buffer

  // start the transaction ISR watches CS bit
  ft81x_assert_cs(true);

  // transmit our transaction to the ISR
  spi_device_transmit(ft81x_spi, (spi_transaction_t*)&trans);

  // grab our return data
  uint8_t ret = *((uint8_t *)recvbuf);

  // end the transaction
  ft81x_assert_cs(false);

  // cleanup
  free(recvbuf);

  return ret;
#else
	uint8_t writeData[5];
	uint8_t readData[5];

	writeData[0] = ((addr >> 16) & 0xFF);
	writeData[1] = ((addr >> 8) & 0xFF);
	writeData[2] = (addr & 0xFF);
	writeData[3] = 0x00; // Dummy byte
	writeData[4] = 0x00; // Swap byte for data read 1

	if (FT81X_SPI_2_SS_CONTROL_MANUAL) { ft81x_assert_cs(YES); }
	SpiTransaction(MXC_SPI2, SPI_8_BIT_DATA_SIZE, LCD_SPI_DEASERT, writeData, sizeof(writeData), readData, sizeof(readData), BLOCKING);
	if (FT81X_SPI_2_SS_CONTROL_MANUAL) { ft81x_assert_cs(NO); }

	return (readData[4]);
#endif
}

/*
 * Write 24 bit address + dummy byte
 * and read the 16 bit result.
 * A total of 6 bytes will be on the SPI BUS.
 */
uint16_t ft81x_rd16(uint32_t addr)
{
#if 0
  // setup trans memory
  spi_transaction_ext_t trans;
  memset(&trans, 0, sizeof(trans));

  // allocate memory for our return data
  char *recvbuf=heap_caps_malloc(4, MALLOC_CAP_DMA);

  // set trans options.
  trans.base.flags = SPI_TRANS_VARIABLE_ADDR;
  if (ft81x_qio) {
    // Tell the ESP32 SPI ISR to accept MODE_XXX for QIO and DIO
    trans.base.flags |= SPI_TRANS_MODE_DIOQIO_ADDR;
    // Set this transaction to QIO
    trans.base.flags |= SPI_TRANS_MODE_QIO;
  }

  // Set the address
  trans.address_bits = 24;
  trans.base.addr = addr;

  // 1 byte is our dummy byte we will throw it away later
  trans.base.length = 24;
  trans.base.rxlength = 24;

  // point to our RX buffer.
  trans.base.rx_buffer = recvbuf;

  // start the transaction ISR watches CS bit
  ft81x_assert_cs(true);

  // transmit our transaction to the ISR
  spi_device_transmit(ft81x_spi, (spi_transaction_t*)&trans);

  // grab our return data skip dummy byte
  uint16_t ret = *((uint16_t *)&recvbuf[1]);

  // end the transaction
  ft81x_assert_cs(false);

  // cleanup
  free(recvbuf);

  return ret;
#else
	uint8_t writeData[6];
	uint8_t readData[6];
	uint16_t result;

	writeData[0] = ((addr >> 16) & 0xFF);
	writeData[1] = ((addr >> 8) & 0xFF);
	writeData[2] = (addr & 0xFF);
	writeData[3] = 0x00; // Dummy byte
	writeData[4] = 0x00; // Swap byte for data read 1
	writeData[5] = 0x00; // Swap byte for data read 2

	if (FT81X_SPI_2_SS_CONTROL_MANUAL) { ft81x_assert_cs(YES); }
	SpiTransaction(MXC_SPI2, SPI_8_BIT_DATA_SIZE, LCD_SPI_DEASERT, writeData, sizeof(writeData), readData, sizeof(readData), BLOCKING);
	if (FT81X_SPI_2_SS_CONTROL_MANUAL) { ft81x_assert_cs(NO); }

	result = ((readData[4] << 8) | readData[5]);
	return (result);
#endif
}

/*
 * Write 24 bit address + dummy byte
 * and read the 32 bit result.
 * A total of 8 bytes will be on the SPI BUS.
 */
uint32_t ft81x_rd32(uint32_t addr)
{
#if 0
  // setup trans memory
  spi_transaction_ext_t trans;
  memset(&trans, 0, sizeof(trans));

  // allocate memory for our return data
  char *recvbuf=heap_caps_malloc(5, MALLOC_CAP_DMA);

  // set trans options.
  trans.base.flags = SPI_TRANS_VARIABLE_ADDR;
  if (ft81x_qio) {
    // Tell the ESP32 SPI ISR to accept MODE_XXX for QIO and DIO
    trans.base.flags |= SPI_TRANS_MODE_DIOQIO_ADDR;
    // Set this transaction to QIO
    trans.base.flags |= SPI_TRANS_MODE_QIO;
  }

  // Set the address
  trans.address_bits = 24;
  trans.base.addr = addr;

  // 1 byte is our dummy byte we will throw it away later
  trans.base.length = 40;
  trans.base.rxlength = 40;

  // point to our RX buffer.
  trans.base.rx_buffer = recvbuf;

  // start the transaction ISR watches CS bit
  ft81x_assert_cs(true);

  // transmit our transaction to the ISR
  spi_device_transmit(ft81x_spi, (spi_transaction_t*)&trans);

  // grab our return data skip dummy byte
  uint32_t ret = *((uint32_t *)&recvbuf[1]);

  // end the transaction
  ft81x_assert_cs(false);

  // cleanup
  free(recvbuf);

  return ret;
#else
	uint8_t writeData[8];
	uint8_t readData[8];
	uint16_t result;

	writeData[0] = ((addr >> 16) & 0xFF);
	writeData[1] = ((addr >> 8) & 0xFF);
	writeData[2] = (addr & 0xFF);
	writeData[3] = 0x00; // Dummy byte
	writeData[4] = 0x00; // Swap byte for data read 1
	writeData[5] = 0x00; // Swap byte for data read 2
	writeData[6] = 0x00; // Swap byte for data read 3
	writeData[7] = 0x00; // Swap byte for data read 4

	if (FT81X_SPI_2_SS_CONTROL_MANUAL) { ft81x_assert_cs(YES); }
	SpiTransaction(MXC_SPI2, SPI_8_BIT_DATA_SIZE, LCD_SPI_DEASERT, writeData, sizeof(writeData), readData, sizeof(readData), BLOCKING);
	if (FT81X_SPI_2_SS_CONTROL_MANUAL) { ft81x_assert_cs(NO); }

	result = ((readData[4] << 24) | (readData[5] << 16) | (readData[6] << 8) | readData[7]);
	return (result);
#endif
}

#if 0 /* only seems used for touch inputs */
/*
 * Spool a large block of memory in chunks from the FT81X
 *
 * Currently the max chunk size on ESP32 with DMA of 0 is 32 bytes.
 */
 void ft81x_rdN(uint32_t addr, uint8_t *results, int8_t len)
 {
   while(len) {
     if (len < CHUNKSIZE) {
       ft81x_rdn(addr, results, len);
       // all done
       break;
     } else {
       ft81x_rdn(addr, results, CHUNKSIZE);
       len-=CHUNKSIZE; results+=CHUNKSIZE; addr+=CHUNKSIZE;
       if(len<0) len=0;
     }
   }
 }

void ft81x_rdn(uint32_t addr, uint8_t *results, int8_t len) {
  // setup trans memory
  spi_transaction_ext_t trans;
  memset(&trans, 0, sizeof(trans));

  // allocate memory for our return data and dummy byte
  char *recvbuf=heap_caps_malloc(len+1, MALLOC_CAP_DMA);

  // set trans options.
  trans.base.flags = SPI_TRANS_VARIABLE_ADDR;
  if (ft81x_qio) {
    // Tell the ESP32 SPI ISR to accept MODE_XXX for QIO and DIO
    trans.base.flags |= SPI_TRANS_MODE_DIOQIO_ADDR;
    // Set this transaction to QIO
    trans.base.flags |= SPI_TRANS_MODE_QIO;
  }

  // Set the address
  trans.address_bits = 24;
  trans.base.addr = addr;

  // 1 byte is our dummy byte we will throw it away later
  uint32_t lenN = (len * 8) + 8;
  trans.base.length = lenN;
  trans.base.rxlength = lenN;

  // point to our RX buffer.
  trans.base.rx_buffer = recvbuf; // RX buffer

  // start the transaction ISR watches CS bit
  ft81x_assert_cs(true);

  // transmit our transaction to the ISR
  spi_device_transmit(ft81x_spi, (spi_transaction_t*)&trans);

  // grab our return data skip dummy byte
  memcpy(results, &recvbuf[1], len);

  // end the transaction
  ft81x_assert_cs(false);

  // cleanup
  free(recvbuf);
}
#endif

/*
 * Write 24 bit address + 8 bit value
 * A total of 4 bytes will be on the SPI BUS.
 */
void ft81x_wr(uint32_t addr, uint8_t byteVal)
{
#if 0
  // setup trans memory
  spi_transaction_ext_t trans;
  memset(&trans, 0, sizeof(trans));

  // set trans options.
  trans.base.flags = SPI_TRANS_VARIABLE_ADDR;
  if (ft81x_qio) {
    // Tell the ESP32 SPI ISR to accept MODE_XXX for QIO and DIO
    trans.base.flags |= SPI_TRANS_MODE_DIOQIO_ADDR;
    // Set this transaction to QIO
    trans.base.flags |= SPI_TRANS_MODE_QIO;
  }

  // no dummy byte for writes
  trans.address_bits = 24;
  trans.base.addr = addr | 0x800000; // set write bit

  trans.base.length = 8;
  trans.base.rx_buffer = NULL;
  trans.base.tx_buffer = &byteVal;

  // start the transaction ISR watches CS bit
  ft81x_assert_cs(true);

  // transmit our transaction to the ISR
  spi_device_transmit(ft81x_spi, (spi_transaction_t*)&trans);

  // end the transaction
  ft81x_assert_cs(false);
#else
	uint8_t writeData[4];

	addr &= HOST_MEMORY_WRITE_ADDR_BITS_VALID;
	addr |= HOST_MEMORY_WRITE; // set write bit
	writeData[0] = ((addr >> 16) & 0xFF);
	writeData[1] = ((addr >> 8) & 0xFF);
	writeData[2] = (addr & 0xFF);
	writeData[3] = byteVal;

	if (FT81X_SPI_2_SS_CONTROL_MANUAL) { ft81x_assert_cs(YES); }
	SpiTransaction(MXC_SPI2, SPI_8_BIT_DATA_SIZE, LCD_SPI_DEASERT, writeData, sizeof(writeData), NULL, 0, BLOCKING);
	if (FT81X_SPI_2_SS_CONTROL_MANUAL) { ft81x_assert_cs(NO); }
#endif
}

/*
 * Write 24 bit address + 16 bit value
 * A total of 5 bytes will be on the SPI BUS.
 */
void ft81x_wr16(uint32_t addr, uint16_t wordVal)
{
#if 0
  // setup trans memory
  spi_transaction_ext_t trans;
  memset(&trans, 0, sizeof(trans));

  // set trans options.
  trans.base.flags = SPI_TRANS_VARIABLE_ADDR;
  if (ft81x_qio) {
    // Tell the ESP32 SPI ISR to accept MODE_XXX for QIO and DIO
    trans.base.flags |= SPI_TRANS_MODE_DIOQIO_ADDR;
    // Set this transaction to QIO
    trans.base.flags |= SPI_TRANS_MODE_QIO;
  }

  // no dummy byte for writes
  trans.address_bits = 24;
  trans.base.addr = addr | 0x800000; // set write bit

  trans.base.length = 16;
  trans.base.rx_buffer = NULL;
  trans.base.tx_buffer = &wordVal;

  // start the transaction ISR watches CS bit
  ft81x_assert_cs(true);

  // transmit our transaction to the ISR
  spi_device_transmit(ft81x_spi, (spi_transaction_t*)&trans);

  // end the transaction
  ft81x_assert_cs(false);
#else
	uint8_t writeData[5];

	addr &= HOST_MEMORY_WRITE_ADDR_BITS_VALID;
	addr |= HOST_MEMORY_WRITE; // set write bit
	writeData[0] = ((addr >> 16) & 0xFF);
	writeData[1] = ((addr >> 8) & 0xFF);
	writeData[2] = (addr & 0xFF);
	writeData[3] = ((wordVal >> 8) & 0xFF);
	writeData[4] = (wordVal & 0xFF);

	if (FT81X_SPI_2_SS_CONTROL_MANUAL) { ft81x_assert_cs(YES); }
	SpiTransaction(MXC_SPI2, SPI_8_BIT_DATA_SIZE, LCD_SPI_DEASERT, writeData, sizeof(writeData), NULL, 0, BLOCKING);
	if (FT81X_SPI_2_SS_CONTROL_MANUAL) { ft81x_assert_cs(NO); }
#endif
}

/*
 * Write 24 bit address + 32 bit value
 * A total of 7 bytes will be on the SPI BUS.
 */
void ft81x_wr32(uint32_t addr, uint32_t longVal)
{
#if 0
  // setup trans memory
  spi_transaction_ext_t trans;
  memset(&trans, 0, sizeof(trans));

  // set trans options.
  trans.base.flags = SPI_TRANS_VARIABLE_ADDR;
  if (ft81x_qio) {
    // Tell the ESP32 SPI ISR to accept MODE_XXX for QIO and DIO
    trans.base.flags |= SPI_TRANS_MODE_DIOQIO_ADDR;
    // Set this transaction to QIO
    trans.base.flags |= SPI_TRANS_MODE_QIO;
  }

  // no dummy byte for writes
  trans.address_bits = 24;
  trans.base.addr = addr | 0x800000; // set write bit

  trans.base.length = 32;
  trans.base.rx_buffer = NULL;
  trans.base.tx_buffer = &longVal;

  // start the transaction ISR watches CS bit
  ft81x_assert_cs(true);

  // transmit our transaction to the ISR
  spi_device_transmit(ft81x_spi, (spi_transaction_t*)&trans);

  // end the transaction
  ft81x_assert_cs(false);
#else
	uint8_t writeData[7];

	addr &= HOST_MEMORY_WRITE_ADDR_BITS_VALID;
	addr |= HOST_MEMORY_WRITE; // set write bit
	writeData[0] = ((addr >> 16) & 0xFF);
	writeData[1] = ((addr >> 8) & 0xFF);
	writeData[2] = (addr & 0xFF);
	writeData[3] = ((longVal >> 24) & 0xFF);
	writeData[4] = ((longVal >> 16) & 0xFF);
	writeData[5] = ((longVal >> 8) & 0xFF);
	writeData[6] = (longVal & 0xFF);

	if (FT81X_SPI_2_SS_CONTROL_MANUAL) { ft81x_assert_cs(YES); }
	SpiTransaction(MXC_SPI2, SPI_8_BIT_DATA_SIZE, LCD_SPI_DEASERT, writeData, sizeof(writeData), NULL, 0, BLOCKING);
	if (FT81X_SPI_2_SS_CONTROL_MANUAL) { ft81x_assert_cs(NO); }
#endif
}

/*
 * Write 24 bit address leave CS open for data to be written
 * A total of 3 bytes will be on the SPI BUS.
 */
void ft81x_wrA(uint32_t addr)
{
#if 0
  // setup trans memory
  spi_transaction_ext_t trans;
  memset(&trans, 0, sizeof(trans));

  // set trans options.
  if (ft81x_qio) {
    // Tell the ESP32 SPI ISR to accept MODE_XXX for QIO and DIO
    trans.base.flags |= SPI_TRANS_MODE_DIOQIO_ADDR;
    // Set this transaction to QIO
    trans.base.flags |= SPI_TRANS_MODE_QIO;
  }

  // set write bit if rw=1
  addr |= 0x800000;
  addr = SPI_REARRANGE_DATA(addr, 24);

  trans.base.length = 24;
  trans.base.tx_buffer = &addr;

  // start the transaction ISR watches CS bit
  ft81x_assert_cs(true);

  // transmit our transaction to the ISR
  spi_device_transmit(ft81x_spi, (spi_transaction_t*)&trans);
#else
	uint8_t writeData[3];

	addr &= HOST_MEMORY_WRITE_ADDR_BITS_VALID;
	addr |= HOST_MEMORY_WRITE; // set write bit
	writeData[0] = ((addr >> 16) & 0xFF);
	writeData[1] = ((addr >> 8) & 0xFF);
	writeData[2] = (addr & 0xFF);

	// SPI write but leave slave selected for future write data
	if (FT81X_SPI_2_SS_CONTROL_MANUAL) { ft81x_assert_cs(YES); }
	SpiTransaction(MXC_SPI2, SPI_8_BIT_DATA_SIZE, NO, writeData, sizeof(writeData), NULL, 0, BLOCKING);
#endif
}

/*
 * Write bytes to the spi port no tracking.
 */
void ft81x_wrN(uint8_t *buffer, uint8_t size)
{
#if 0  
  // setup trans memory
  spi_transaction_ext_t trans;
  memset(&trans, 0, sizeof(trans));

  // set trans options.
  if (ft81x_qio) {
    // Tell the ESP32 SPI ISR to accept MODE_XXX for QIO and DIO
    trans.base.flags |= SPI_TRANS_MODE_DIOQIO_ADDR;
    // Set this transaction to QIO
    trans.base.flags |= SPI_TRANS_MODE_QIO;
  }

  trans.base.length = size * 8;
  trans.base.tx_buffer = buffer;

  // transmit our transaction to the ISR
  spi_device_transmit(ft81x_spi, (spi_transaction_t*)&trans);
#else
	// SPI write with slave select still active from before, write more data
	SpiTransaction(MXC_SPI2, SPI_8_BIT_DATA_SIZE, NO, buffer, size, NULL, 0, BLOCKING);
#endif
}

void ft81x_wrE(uint32_t addr)
{
  // end the transaction
	ft81x_assert_cs(NO);
}

/*
 * Spool a large block of memory in chunks into the FT81X
 * using the MEDIA FIFO registers. Currently the max chunk size
 * on ESP32 with DMA of 0 is 32 bytes.
 *
 * If the size is > the available space this routine will block.
 * To avoid blocking check free space before calling this routine.
 */
void ft81x_cSPOOL_MF(uint8_t *buffer, int32_t size)
{
  uint32_t fullness;
  size_t rds, ts;
  uint8_t stopped = 1; // Stopped at startup
  size_t written = 0;
  
  // Get the read pointer where the GPU is working currently
  uint32_t mf_rp = ft81x_rd32(REG_MEDIAFIFO_READ);

  // Calculate how full our fifo is based upon our read/write pointers
  fullness = (mf_wp - mf_rp) & (mf_size - 1);

  // Blocking! Keep going until all the data is sent. 
  do {

    // Wait till we have enough room to send some data
    if ( !(fullness < (mf_size - CHUNKSIZE)) )
	{
      // Release the SPI bus
      ft81x_stream_stop(); stopped = 1;
      
      // Did we write anything? If so tell the FT813
      if (written) {
        ft81x_wr32(REG_MEDIAFIFO_WRITE, mf_wp);
        written = 0;
      }

      // sleep a little let other processes go (1ms).
		MXC_Delay(MXC_DELAY_MSEC(1));

      // Get the read pointer where the GPU is working currently
      // consuming bytes.
      mf_rp = ft81x_rd32(REG_MEDIAFIFO_READ);

      // Calculate how full our fifo is based upon our read/write pointers
      fullness = (mf_wp - mf_rp) & (mf_size - 1);
      
      continue;
    }

    // resume streaming data if needed
    if (stopped) {
      // Start streaming to our fifo starting with our address
      // same as ft81x_stream_start() but different address area
      // and no auto wrapping :(
      ft81x_wrA(mf_base + mf_wp); stopped = 0;
    }

    // write up to the very end of the fifo buffer
    rds = (mf_size - mf_wp);
    if (rds > CHUNKSIZE) {
      rds = CHUNKSIZE;
    }

    // default write size to chunk size or enough for the end of the fifo
    ts = rds;

    // if we have less to send than we can then update transmit size
    if (size < ts) {
      ts = size;
    }

    // write the block to the FT81X
    ft81x_wrN((uint8_t *)buffer, ts);

    // increment the pointers/counters
    buffer+=ts;
    mf_wp+=ts;
    fullness+=ts;
    size-=ts;
    written+=ts;

	// Sleep (10ms)
	MXC_Delay(MXC_DELAY_MSEC(10));

    // loop around if we overflow the fifo.
    mf_wp&=(mf_size-1);
    
    // force flush if we reached the end of the fifo buffer
    if(!mf_wp) fullness =- mf_size;
    
  } while (size);

  // Release the SPI bus
  ft81x_stream_stop(); stopped = 1;
  
  // Did we write anything? If so tell the FT813
  if (written) {
    ft81x_wr32(REG_MEDIAFIFO_WRITE, mf_wp);
    written = 0;
  }
}

/*
 * Read the FT81x command pointer
 */
uint16_t ft81x_fifo_rp(
)
{
	uint16_t rp = ft81x_rd16(REG_CMD_READ);
	if (rp == DL_CMD_FAULT) {
		debugErr("FT81X COPROCESSOR EXCEPTION\r\n");
		// Delay 50ms?
		// Resetting co-processor sets REG_CMD_READ to zero.
		ft81x_wr(REG_CPURESET, 1);
		ft81x_wr16(REG_CMD_READ, 0);
		ft81x_wr16(REG_CMD_WRITE, 0);
		ft81x_wr(REG_CPURESET, 0);
		rp = 0;
	}
	return rp;
}

/*
 * Write out padded bits to be sure we are 32 bit aligned
 * as required by the FT81X
 */
void ft81x_align(uint32_t written)
{
  uint8_t dummy[4] = {0x00, 0x00, 0x00, 0x00};
  int8_t align = 4 - (written & 0x3);
  if (align & 0x3)
    ft81x_cN((uint8_t *)dummy, align);
}

/*
 * Start a new transactions to write to the command buffer at the current write pointer.
 * Assert CS pin.
 */
void ft81x_stream_start()
{
  // be sure we ended the last tranaction
  ft81x_stream_stop();
  // begin a new write transaction.
  ft81x_wrA(RAM_CMD + (ft81x_fifo_wp & 0xffc));
}

/*
 * Close any open transactions.
 * De-assert CS pin
 */
void ft81x_stream_stop()
{
  // end the transaction
	ft81x_assert_cs(NO);
}

/*
 * Get free space from FT81x until it reports
 * we have required space. Also triggers processing
 * of any display commands in the fifo buffer.
 */
void ft81x_getfree(uint16_t required)
{
  ft81x_fifo_wp &= 0xffc;
  ft81x_stream_stop();

  // force command to complete write to CMD_WRITE
  ft81x_wr16(REG_CMD_WRITE, ft81x_fifo_wp & 0xffc);

  do {
	// Sleep (10ms)
	MXC_Delay(MXC_DELAY_MSEC(10));

    uint16_t rp = ft81x_fifo_rp();
    uint16_t howfull = (ft81x_fifo_wp - rp) & 4095;
    ft81x_fifo_freespace = MAX_FIFO_SPACE - howfull;
  } while (ft81x_fifo_freespace < required);
  ft81x_stream_start();
}

void ft81x_checkfree(uint16_t required)
{
  // check that we have space in our fifo buffer
  // block until the FT81X says we do.
  if (ft81x_fifo_freespace < required) {
    ft81x_getfree(required);
  }
}

/*
 * wrapper to send out a 32bit command into the fifo buffer
 * while in stream() mode.
 */
void ft81x_cI(uint32_t word)
{
  //word = SPI_REARRANGE_DATA(word, 32);
  ft81x_cmd32(word);
}

/*
 * wrapper to send a 8bit command into the fifo buffer
 * while in stream() mode.
 */
void ft81x_cFFFFFF(uint8_t byte)
{
  ft81x_cmd32(byte | 0xffffff00);
}

/*
 * Write 32 bit command into our command fifo buffer
 * while in stream() mode.
 * Use tx_buffer to transmit the 32 bits.
 */
void ft81x_cmd32(uint32_t word)
{

  ft81x_fifo_wp += sizeof(word);
  ft81x_fifo_freespace -= sizeof(word);

#if 0
  // setup trans memory
  spi_transaction_ext_t trans;
  memset(&trans, 0, sizeof(trans));

  // set trans options.
  if (ft81x_qio) {
    // Tell the ESP32 SPI ISR to accept MODE_XXX for QIO and DIO
    trans.base.flags |= SPI_TRANS_MODE_DIOQIO_ADDR;
    // Set this transaction to QIO
    trans.base.flags |= SPI_TRANS_MODE_QIO;
  }

  trans.base.length = 32;
  trans.base.tx_buffer = &word;

  // transmit our transaction to the ISR
  spi_device_transmit(ft81x_spi, (spi_transaction_t*)&trans);
#else
	uint8_t writeData[4];

	writeData[0] = ((word >> 24) & 0xFF);
	writeData[1] = ((word >> 16) & 0xFF);
	writeData[2] = ((word >> 8) & 0xFF);
	writeData[3] = (word & 0xFF);

	SpiTransaction(MXC_SPI2, SPI_8_BIT_DATA_SIZE, NO, writeData, sizeof(writeData), NULL, 0, BLOCKING);
#endif
}

/*
 * Write N bytes command into our command fifo buffer
 * while in stream() mode.
 * Use tx_buffer to transmit the bits. Must be less
 * than buffer size.
 */
void ft81x_cN(uint8_t *buffer, uint16_t size)
{
  ft81x_fifo_wp += size;
  ft81x_fifo_freespace -= size;

#if 0
  // setup trans memory
  spi_transaction_ext_t trans;
  memset(&trans, 0, sizeof(trans));

  // set trans options.
  if (ft81x_qio) {
    // Tell the ESP32 SPI ISR to accept MODE_XXX for QIO and DIO
    trans.base.flags |= SPI_TRANS_MODE_DIOQIO_ADDR;
    // Set this transaction to QIO
    trans.base.flags |= SPI_TRANS_MODE_QIO;
  }

  trans.base.length = size * 8;
  trans.base.tx_buffer = buffer;

  // transmit our transaction to the ISR
  spi_device_transmit(ft81x_spi, (spi_transaction_t*)&trans);
#else
	SpiTransaction(MXC_SPI2, SPI_8_BIT_DATA_SIZE, NO, buffer, size, NULL, 0, BLOCKING);
#endif
}

/*
 * Spool a large block of memory in chunks into the FT81X
 *
 * Currently the max chunk size on ESP32 with DMA of 0 is 32 bytes.
 */
void ft81x_cSPOOL(uint8_t *buffer, int32_t size)
{
  int32_t savesize = size;

  while(size) {
    if (size < CHUNKSIZE) {
      // check that we have enough space then send command
      ft81x_checkfree(size);
      ft81x_cN((uint8_t *)buffer, size);
      // all done
      break;
    } else {
      ft81x_checkfree(CHUNKSIZE);
      ft81x_cN((uint8_t *)buffer, CHUNKSIZE);
      size-=CHUNKSIZE; buffer+=CHUNKSIZE;
      if(size<0) size=0;
    }
  }

  int8_t align = (4 - (savesize & 0x3)) & 0x3;
  ft81x_checkfree(align);
  ft81x_align(savesize);
}

/*
 * Wait until REG_CMD_READ == REG_CMD_WRITE indicating
 * the GPU has processed all of the commands in the
 * circular command buffer
 */
void ft81x_wait_finish()
{
#if 0
  uint32_t twp = ft81x_fifo_wp;
#endif
  uint16_t rp;
  ft81x_fifo_wp &= 0xffc;
  while ( ((rp=ft81x_fifo_rp()) != ft81x_fifo_wp) ) {
  }
  ft81x_fifo_freespace = MAX_FIFO_SPACE;
}

/*
 * Swap the display
 */
void ft81x_swap()
{
  ft81x_display(); // end current display list
  ft81x_cmd_swap(); // Set AUTO swap at end of display list
  //ft81x_cmd_loadidentity();
  ft81x_cmd_dlstart(); // Set REG_CMD_DL when done
  ft81x_getfree(0); // trigger display processing
}

/*
 * Programming Guide sections
 *
 */

/*
 * 4.4 ALPHA_FUNCT
 * Specify the alpha test function
 * SM20180828:QA:PASS
 */
void ft81x_alpha_funct(uint8_t func, uint8_t ref)
{
  // check that we have enough space then send command
  ft81x_checkfree(4);
  ft81x_cI(
      ( 0x09UL                 << 24) | // CMD 0x09      24 - 31
                                        // RESERVED      11 - 23
      ( (func       & 0x7L)    <<  8) | // func           8 - 10
      ( (ref        & 0xffL)   <<  0)   // ref            0 -  7
  );
}

/*
 * 4.5 BEGIN
 * Begin drawing a graphics primitive
 * SM20180828:QA:PASS
 */
void ft81x_begin(uint8_t prim)
{
  // check that we have enough space then send command
  ft81x_checkfree(4);
  ft81x_cI((0x1fUL << 24) | (prim & 0x0f));
}

/*
 * 4.6 BITMAP_HANDLE
 * Specify the bitmap handle
 * SM20180828:QA:PASS
 */
void ft81x_bitmap_handle(uint8_t handle)
{
  // check that we have enough space then send command
  ft81x_checkfree(4);
  ft81x_cI((0x05UL << 24) | (handle & 0x1f));
}

/*
 * 4.7 BITMAP_LAYOUT
 * Specify the source bitmap memory format and layout for the current handle
 * SM20180828:QA:PASS
 */
void ft81x_bitmap_layout(uint8_t format, uint16_t linestride, uint16_t height)
{
  // check that we have enough space then send command
  ft81x_checkfree(4);
  ft81x_cI(
      ( 0x07UL                 << 24) | // CMD 0x07      24 - 31
      ( (format     & 0x1fL)   << 19) | // format        19 - 23
      ( (linestride & 0x3ffL)  <<  9) | // linestride     9 - 18
      ( (height     & 0x1ffL)  <<  0)   // height         0 -  8
  );
}

/*
 * 4.8 BITMAP_LAYOUT_H
 * Specify the 2 most significant bits of the source bitmap memory format and layout for the current handle
 * SM20180828:QA:PASS
 */
void ft81x_bitmap_layout_h(uint8_t linestride, uint8_t height)
{
  // check that we have enough space then send command
  ft81x_checkfree(4);
  ft81x_cI(
      ( 0x28UL                 << 24) | // CMD 0x28      24 - 31
                                        // RESERVED       4 - 23
      ( (linestride & 0x3L)    <<  2) | // linestride     2 -  3
      ( (height     & 0x3L)    <<  0)   // height         0 -  1
  );
}

/*
 * 4.9 BITMAP_SIZE
 * Specify the screen drawing of bitmaps for the current handle
 * SM20180828:QA:PASS
 */
void ft81x_bitmap_size(uint8_t filter, uint8_t wrapx, uint8_t wrapy, uint16_t width, uint16_t height)
{
  // check that we have enough space then send command
  ft81x_checkfree(4);
  ft81x_cI(
      ( 0x08UL                 << 24) | // CMD 0x08      24 - 31
                                        // RESERVED      21 - 23
      ( (filter     & 0x1L)    << 20) | // filter        20 - 20
      ( (wrapx      & 0x1L)    << 19) | // wrapx         19 - 19
      ( (wrapy      & 0x1L)    << 18) | // wrapy         18 - 18
      ( (width      & 0x1ffL)   <<  9) | // width          9 - 17
      ( (height     & 0x1ffL)   <<  0)   // height         0 -  8
  );
}

/*
 * 4.10 BITMAP_SIZE_H
 * Specify the source address of bitmap data in FT81X graphics memory RAM_G
 * SM20180828:QA:PASS
 */
void ft81x_bitmap_size_h(uint8_t width, uint8_t height)
{
  // check that we have enough space then send command
  ft81x_checkfree(4);
  ft81x_cI((0x29UL << 24) | (((width) & 0x3) << 2) | (((height) & 0x3) << 0));
}

/*
 * 4.11 BITMAP_SOURCE
 * Specify the source address of bitmap data in FT81X graphics memory RAM_G
 * SM20180828:QA:PASS
 */
void ft81x_bitmap_source(uint32_t addr)
{
  // check that we have enough space then send command
  ft81x_checkfree(4);
  ft81x_cI((0x01UL << 24) | ((addr & 0x3fffffL) << 0));
}

/*
 * 4.12 BITMAP_TRANSFORM_A
 * Specify the A coefficient of the bitmap transform matrix
 * SM20180828:QA:PASS
 */
void ft81x_bitmap_transform_a(uint32_t a)
{
  // check that we have enough space then send command
  ft81x_checkfree(4);
  ft81x_cI((0x15UL << 24) | ((a & 0xffffL) << 0));
}

/*
 * 4.13 BITMAP_TRANSFORM_B
 * Specify the B coefficient of the bitmap transform matrix
 * SM20180828:QA:PASS
 */
void ft81x_bitmap_transform_b(uint32_t b)
{
  // check that we have enough space then send command
  ft81x_checkfree(4);
  ft81x_cI((0x16UL << 24) | ((b & 0xffffL) << 0));
}

/*
 * 4.14 BITMAP_TRANSFORM_C
 * Specify the C coefficient of the bitmap transform matrix
 * SM20180828:QA:PASS
 */
void ft81x_bitmap_transform_c(uint32_t c)
{
  // check that we have enough space then send command
  ft81x_checkfree(4);
  ft81x_cI((0x17UL << 24) | ((c & 0xffffffL) << 0));
}

/*
 * 4.15 BITMAP_TRANSFORM_D
 * Specify the D coefficient of the bitmap transform matrix
 * SM20180828:QA:PASS
 */
void ft81x_bitmap_transform_d(uint32_t d)
{
  // check that we have enough space then send command
  ft81x_checkfree(4);
  ft81x_cI((0x18UL << 24) | ((d & 0xffffL) << 0));
}

/*
 * 4.16 BITMAP_TRANSFORM_E
 * Specify the E coefficient of the bitmap transform matrix
 * SM20180828:QA:PASS
 */
void ft81x_bitmap_transform_e(uint32_t e)
{
  // check that we have enough space then send command
  ft81x_checkfree(4);
  ft81x_cI((0x19UL << 24) | ((e & 0xffffL) << 0));
}

/*
 * 4.17 BITMAP_TRANSFORM_F
 * Specify the F coefficient of the bitmap transform matrix
 * SM20180828:QA:PASS
 */
void ft81x_bitmap_transform_f(uint32_t f)
{
  // check that we have enough space then send command
  ft81x_checkfree(4);
  ft81x_cI((0x1aUL << 24) | ((f & 0xffffffL) << 0));
}

/*
 * 4.18 BLEND_FUNC
 * Specify pixel arithmetic
 * SM20180828:QA:PASS
 */
void ft81x_blend_func(uint8_t src, uint8_t dst)
{
  // check that we have enough space then send command
  ft81x_checkfree(4);
  ft81x_cI((0x0bUL << 24) | ((src & 0x7L) << 3) | ((dst & 0x7L) << 0));
}

/*
 * 4.19 CALL
 * Execute a sequence of commands at another location in the display list
 * SM20180828:QA:PASS
 */
void ft81x_call(uint16_t dest)
{
  // check that we have enough space then send command
  ft81x_checkfree(4);
  ft81x_cI((0x1dUL << 24) | ((dest & 0xffffL) << 0));
}

/*
 * 4.20 CELL
 * Specify the bitmap cell number for the VERTEX2F command
 * SM20180828:QA:PASS
 */
void ft81x_cell(uint8_t cell)
{
  // check that we have enough space then send command
  ft81x_checkfree(4);
  ft81x_cI((0x06UL << 24) | ((cell & 0x7fL) << 0));
}

/*
 * 4.21 CLEAR
 * Clear buffers to preset values
 * SM20180828:QA:PASS
 */
void ft81x_clear()
{
  // check that we have enough space then send command
  ft81x_checkfree(4);
  ft81x_cI((0x26UL << 24) | 0x7);
}

void ft81x_clearCST(uint8_t color, uint8_t stencil, uint8_t tag)
{
  uint8_t cst = 0;
  cst = color & 0x01;
  cst <<=1;
  cst |= (stencil & 0x01);
  cst <<=1;
  cst |= (tag & 0x01);

  // check that we have enough space then send command
  ft81x_checkfree(4);
  ft81x_cI((0x26UL << 24) | cst);
}

/*
 * 4.21 CLEAR_COLOR_A
 * Specify clear value for the alpha channel
 * SM20180828:QA:PASS
 */
void ft81x_clear_color_a(uint8_t alpha)
{
  // check that we have enough space then send command
  ft81x_checkfree(4);
  ft81x_cI((0x0fUL << 24) | (alpha & 0xffL));
}

/*
 * 4.23 CLEAR_COLOR_RGB
 * Specify clear values for red, green and blue channels
 * SM20180828:QA:PASS
 */
void ft81x_clear_color_rgb32(uint32_t rgb)
{
  // check that we have enough space then send command
  ft81x_checkfree(4);
  ft81x_cI((0x2UL << 24) | (rgb & 0xffffffL));
}

void ft81x_clear_color_rgb888(uint8_t red, uint8_t green, uint8_t blue)
{
   ft81x_clear_color_rgb32(((red & 0xffL) << 16) | ((green & 0xffL) << 8) | ((blue & 0xffL) << 0));
}


/*
 * 4.24 CLEAR_STENCIL
 * Specify clear value for the stencil buffer
 * SM20180828:QA:PASS
 */
void ft81x_clear_stencil(uint8_t stencil)
{
  // check that we have enough space then send command
  ft81x_checkfree(4);
  ft81x_cI((0x11UL << 24) | ((stencil & 0xffL) << 0));
}

/*
 * 4.25 CLEAR_TAG
 * Specify clear value for the tag buffer
 * SM20180828:QA:PASS
 */
void ft81x_clear_tag(uint8_t tag)
{
  // check that we have enough space then send command
  ft81x_checkfree(4);
  ft81x_cI((0x12UL << 24) | ((tag & 0xffL) << 0));
}

/*
 * 4.26 COLOR_A
 * Set the current color alpha
 * SM20180828:QA:PASS
 */
void ft81x_color_a(uint8_t alpha)
{
  // check that we have enough space then send command
  ft81x_checkfree(4);
  ft81x_cI((0x10UL << 24) | ((alpha & 0xffL) << 0));
}

/*
 * 4.27 COLOR_MASK
 * Enable or disable writing of color components
 * SM20180828:QA:PASS
 */
void ft81x_color_mask(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha)
{
  // check that we have enough space then send command
  ft81x_checkfree(4);
  ft81x_cI((0x20L << 24) | (((red & 0x1) << 3) | ((green & 0x1) << 2) | ((blue & 0x1) << 1) | ((alpha & 0x1) << 0)));
}

/*
 * 4.28 COLOR_RGB
 * Set the current color red, green, blue
 * SM20180828:QA:PASS
 */
void ft81x_color_rgb32(uint32_t rgb)
{
  // check that we have enough space then send command
  ft81x_checkfree(4);
  ft81x_cI((0x4UL << 24) | (rgb & 0xffffffL));
}

void ft81x_color_rgb888(uint8_t red, uint8_t green, uint8_t blue)
{
   ft81x_color_rgb32(((red & 0xffL) << 16) | ((green & 0xffL) << 8) | ((blue & 0xffL) << 0));
}

/*
 * 4.29 DISPLAY
 * End the display list. FT81X will ignore all commands following this command.
 * SM20180828:QA:PASS
 */
void ft81x_display()
{
  // check that we have enough space then send command
  ft81x_checkfree(4);
  ft81x_cI((0x0UL << 24));
}

/*
 * 4.30 END
 * End drawing a graphics primitive
 * SM20180828:QA:PASS
 */
void ft81x_end()
{
  // check that we have enough space then send command
  ft81x_checkfree(4);
  ft81x_cI((0x21UL << 24));
}

/*
 * 4.31 JUMP
 * Execute commands at another location in the display list
 * SM20180828:QA:PASS
 */
void ft81x_jump(uint16_t dest)
{
  // check that we have enough space then send command
  ft81x_checkfree(4);
  ft81x_cI((0x1eUL << 24) | (dest & 0xffffL));
}

/*
 * 4.32 LINE_WIDTH
 * Specify the width of lines to be drawn with primitive LINES in 1/16 pixel precision
 * SM20180828:QA:PASS
 */
void ft81x_line_width(uint16_t width)
{
  // check that we have enough space then send command
  ft81x_checkfree(4);
  ft81x_cI((0x0eUL << 24) | (width & 0xfff));
}

/*
 * 4.33 MACRO
 * Execute a single command from a macro register
 * SM20180828:QA:PASS
 */
void ft81x_macro(uint8_t macro)
{
  // check that we have enough space then send command
  ft81x_checkfree(4);
  ft81x_cI((0x25UL << 24) | (macro & 0x1L));
}

/*
 * 4.34 NOP
 * No Operation
 * SM20180828:QA:PASS
 */
void ft81x_nop()
{
  // check that we have enough space then send command
  ft81x_checkfree(4);
  ft81x_cI((0x2dUL << 24));
}

/*
 * 4.35 PALETTE_SOURCE
 * Specify the base address of the palette
 * SM20180828:QA:PASS
 */
void ft81x_palette_source(uint32_t addr)
{
  // check that we have enough space then send command
  ft81x_checkfree(4);
  ft81x_cI((0x2aUL << 24) | ((addr) & 0x3fffffUL));
}

/*
 * 4.36 POINT_SIZE
 * Specify the radius of points
 * SM20180828:QA:PASS
 */
void ft81x_point_size(uint16_t size)
{
  // check that we have enough space then send command
  ft81x_checkfree(4);
  ft81x_cI((0x0dUL << 24) | ((size & 0x1fffL) << 0));
}

/*
 * 4.37 RESTORE_CONTEXT
 * Restore the current graphics context from the context stack
 * SM20180828:QA:PASS
 */
void ft81x_restore_context()
{
  // check that we have enough space then send command
  ft81x_checkfree(4);
  ft81x_cI((0x23UL << 24));
}

/*
 * 4.38 RETURN
 * Return from a previous CALL command
 * SM20180828:QA:PASS
 */
void ft81x_return()
{
  // check that we have enough space then send command
  ft81x_checkfree(4);
  ft81x_cI((0x24UL << 24));
}

/*
 * 4.39 SAVE_CONTEXT
 * Push the current graphics context on the context stack
 * SM20180828:QA:PASS
 */
void ft81x_save_context()
{
  // check that we have enough space then send command
  ft81x_checkfree(4);
  ft81x_cI((0x22UL << 24));
}

/*
 * 4.40 SCISSOR_SIZE
 * Specify the size of the scissor clip rectangle
 * SM20180828:QA:PASS
 */
void ft81x_scissor_size(uint16_t width, uint16_t height)
{
  // check that we have enough space then send command
  ft81x_checkfree(4);
  ft81x_cI((0x1cUL << 24) | ((width & 0xfffL) << 12) | ((height & 0xfffL) << 0));
}

/*
 * 4.41 SCISSOR_XY
 * Specify the top left corner of the scissor clip rectangle
 * SM20180828:QA:PASS
 */
void ft81x_scissor_xy(uint16_t x, uint16_t y)
{
  // check that we have enough space then send command
  ft81x_checkfree(4);
  ft81x_cI((0x1bUL << 24) | ((x & 0x7ffL) << 11) | ((y & 0x7ffL) << 0));
}

/*
 * 4.42 STENCIL_FUNC
 * Set function and reference value for stencil testing
 * SM20180828:QA:PASS
 */
void ft81x_stencil_func(uint8_t func, uint8_t ref, uint8_t mask)
{
  // check that we have enough space then send command
  ft81x_checkfree(4);
  ft81x_cI((0x0aUL << 24) | ((func & 0xfL) << 16) | ((ref & 0xffL) << 8) | ((mask & 0xffL) << 0));
}

/*
 * 4.43 STENCIL_MASK
 * Control the writing of individual bits in the stencil planes
 * SM20180828:QA:PASS
 */
void ft81x_stencil_mask(uint8_t mask)
{
  // check that we have enough space then send command
  ft81x_checkfree(4);
  ft81x_cI((0x013UL << 24) | ((mask & 0xffL) << 0));
}

/*
 * 4.44 STENCIL_OP
 * Set stencil test actions
 * SM20180828:QA:PASS
 */
void ft81x_stencil_op(uint8_t sfail, uint8_t spass)
{
  // check that we have enough space then send command
  ft81x_checkfree(4);
  ft81x_cI((0x0cUL << 24) | ((sfail & 0x7L) << 3) | ((spass & 0x7L) << 0));
}

/*
 * 4.45 TAG
 * Attach the tag value for the following graphics objects
 * drawn on the screen. The initial tag buffer value is 255.
 * SM20180828:QA:PASS
 */
void ft81x_tag(uint8_t s)
{
  // check that we have enough space then send command
  ft81x_checkfree(4);
  ft81x_cI((0x3UL << 24) | ((s & 0xffL) << 0));
}

/*
 * 4.46 TAG_MASK
 * Control the writing of the tag buffer
 * SM20180828:QA:PASS
 */
void ft81x_tag_mask(uint8_t mask)
{
  // check that we have enough space then send command
  ft81x_checkfree(4);
  ft81x_cI((0x14UL << 24) | ((mask & 1L) << 0));
}

/*
 * 4.47 VERTEX2F
 * Start the operation of graphics primitives at the specified
 * screen coordinate, in the pixel precision defined by VERTEX_FORMAT
 * SM20180828:QA:PASS
 */
void ft81x_vertex2f(int16_t x, int16_t y)
{
  // check that we have enough space then send command
  ft81x_checkfree(4);
  ft81x_cI((0x1UL << 30) | ((x & 0x7fffL) << 15) | ((y & 0x7fffL) << 0));
}

/*
 * 4.48 VERTEX2ii
 * Start the operation of graphics primitives at the specified
 * screen coordinate, in the pixel precision defined by VERTEX_FORMAT
 * SM20180828:QA:PASS
 */
void ft81x_vertex2ii(int16_t x, int16_t y, uint8_t handle, uint8_t cell)
{
  // check that we have enough space then send command
  ft81x_checkfree(4);
  ft81x_cI(
      ( 0x02UL                 << 30) | // CMD 0x02      30 - 31
      ( (x          & 0x1ffL)  << 21) | // x             21 - 29
      ( (y          & 0x1ffL)  << 12) | // y             12 - 20
      ( (handle     & 0x1fL)   <<  7) | // handle         7 - 11
      ( (cell       & 0x7fL)   <<  0)   // cell           0 -  6
  );
}

/*
 * 4.49 VERTEX_FORMAT
 * Set the precision of VERTEX2F coordinates
 * SM20180828:QA:PASS
 */
void ft81x_vertex_format(int8_t frac)
{
  // check that we have enough space then send command
  ft81x_checkfree(4);
  ft81x_cI((0x27UL << 24) | (((frac) & 0x7) << 0));
}

/*
 * 4.50 VERTEX_TRANSLATE_X
 * Specify the vertex transformation’s X translation component
* SM20180828:QA:PASS
 */
void ft81x_vertex_translate_x(uint32_t x)
{
  // check that we have enough space then send command
  ft81x_checkfree(4);
  ft81x_cI((0x2bUL << 24) | (((x) & 0x1ffffUL) << 0));
}

/*
 * 4.51 VERTEX_TRANSLATE_Y
 * Specify the vertex transformation’s Y translation component
* SM20180828:QA:PASS
 */
void ft81x_vertex_translate_y(uint32_t y)
{
  // check that we have enough space then send command
  ft81x_checkfree(4);
  ft81x_cI((0x2cUL << 24) | (((y) & 0x1ffffUL) << 0));
}

/*
 * 5.11 CMD_DLSTART
 * Start a new display list
 * SM20180828:QA:PASS
 */
void ft81x_cmd_dlstart()
{
  // check that we have enough space then send command
  ft81x_checkfree(4);
  ft81x_cFFFFFF(0x00);
}

/*
 * 5.12 CMD_SWAP
 * Swap the current display list
 * SM20180828:QA:PASS
 */
void ft81x_cmd_swap()
{
  // check that we have enough space then send command
  ft81x_checkfree(4);
  ft81x_cFFFFFF(0x01);
}

/*
 * 5.13 CMD_COLDSTART
 * This command sets the co-processor engine to default reset states
 * SM20180828:QA:PASS
 */
void ft81x_cmd_coldstart()
{
  // check that we have enough space then send command
  ft81x_checkfree(4);
  ft81x_cFFFFFF(0x32);
}

/*
 * 5.14 CMD_INTERRUPT
 * trigger interrupt INT_CMDFLAG
 * SM20180828:QA:PASS
 */
void ft81x_cmd_interrupt(uint32_t ms)
{
  // check that we have enough space then send command
  ft81x_checkfree(8);
  ft81x_cFFFFFF(0x02);
  ft81x_cI(ms);
}

/*
 * 5.15 CMD_APPEND
 * Append more commands to current display list
 * SM20180828:QA:PASS
 */
void ft81x_cmd_append(uint32_t ptr, uint32_t num)
{
  // check that we have enough space then send command
  ft81x_checkfree(12);
  ft81x_cFFFFFF(0x1e);
  ft81x_cI(ptr);
  ft81x_cI(num);
}

/*
 * 5.16 CMD_REGREAD
 * Read a register value
 * FIXME
 */
void ft81x_cmd_regread(uint32_t ptr, uint32_t* result)
{
  // check that we have enough space then send command
  ft81x_checkfree(8);
  ft81x_cFFFFFF(0x19);
  ft81x_cI(ptr);

  // The data will be written starting here in the buffer so get the pointer
  uint32_t r = ft81x_getwp();

  // Fill in memory where results will go with dummy data
  ft81x_cI(0xffffffff); // Will be result

  // report back memory locations of the results to caller
  *result = r;
}

/*
 * 5.17 CMD_MEMWRITE
 * Write bytes into memory
 */
void ft81x_cmd_memwrite(uint32_t ptr, uint32_t num)
{
  // check that we have enough space then send command
  ft81x_checkfree(12);
  ft81x_cFFFFFF(0x1a);
  ft81x_cI(ptr);
  ft81x_cI(num);
}

/*
 * 5.18 CMD_INFLATE
 * Decompress data into memory
 */
void ft81x_cmd_inflate(uint32_t ptr)
{
  // check that we have enough space then send command
  ft81x_checkfree(8);
  ft81x_cFFFFFF(0x22);
  ft81x_cI(ptr);
}

/*
 * 5.19 CMD_LOADIMAGE
 * Load a JPEG or PNG image
 * SM20180828:QA:PASS
 */
void ft81x_cmd_loadimage(uint32_t ptr, uint32_t options)
{
  // check that we have enough space then send command
  ft81x_checkfree(12);
  ft81x_cFFFFFF(0x24);
  ft81x_cI(ptr);
  ft81x_cI(options);
}

/*
 * 5.20 CMD_MEDIAFIFO
 * set up a streaming media FIFO in RAM_G
 * SM20180828:QA:PASS
 */
void ft81x_cmd_mediafifo(uint32_t base, uint32_t size)
{
  mf_wp = 0;
  mf_size = size;
  mf_base = base;
  
  // check that we have enough space then send command
  ft81x_checkfree(12);
  ft81x_cFFFFFF(0x39);
  ft81x_cI(base);
  ft81x_cI(size);
}

/*
 * 5.21 CMD_PLAYVIDEO
 * Video playback
 */
void ft81x_cmd_playvideo(uint32_t options)
{
  // check that we have enough space then send command
  ft81x_checkfree(8);
  ft81x_cFFFFFF(0x3a);
  ft81x_cI(options);
}

/*
 * 5.22 CMD_VIDEOSTART
 * Initialize the AVI video decoder
 */
void ft81x_cmd_videostart()
{
  // check that we have enough space then send command
  ft81x_checkfree(4);
  ft81x_cFFFFFF(0x40);
}

/*
 * 5.23 CMD_VIDEOFRAME
 * Loads the next frame of video
 */
void ft81x_cmd_videoframe(uint32_t dst, uint32_t ptr)
{
  // check that we have enough space then send command
  ft81x_checkfree(12);
  ft81x_cFFFFFF(0x40);
  ft81x_cI(dst);
  ft81x_cI(ptr);
}

/*
 * 5.24 CMD_MEMCRC
 * Compute a CRC-32 for memory
 */
uint32_t ft81x_cmd_memcrc(uint32_t ptr, uint32_t num)
{
  // check that we have enough space then send command
  ft81x_checkfree(16);

  ft81x_cFFFFFF(0x18);
  ft81x_cI(ptr);
  ft81x_cI(num);

  // The data will be written here in the buffer so get the pointer
  uint32_t r = ft81x_getwp();

  // Dummy data where our results will go
  ft81x_cI(0xffffffff);
  return r;
}

/*
 * 5.25 CMD_MEMZERO
 * Write zero to a block of memory
 */
void ft81x_cmd_memzero(uint32_t ptr, uint32_t num)
{
  // check that we have enough space then send command
  ft81x_checkfree(12);
  ft81x_cFFFFFF(0x1c);
  ft81x_cI(ptr);
  ft81x_cI(num);
}

/*
 * 5.26 CMD_MEMSET
 * Fill memory with a byte value
 */
void ft81x_cmd_memset(uint32_t ptr, uint32_t value, uint32_t num)
{
  // check that we have enough space then send command
  ft81x_checkfree(12);
  ft81x_cFFFFFF(0x1b);
  ft81x_cI(value);
  ft81x_cI(num);
}

/*
 * 5.27 CMD_MEMCPY
 * Copy a block of memory
 */
void ft81x_cmd_memcpy(uint32_t dest, uint32_t src, uint32_t num)
{
  // check that we have enough space then send command
  ft81x_checkfree(16);
  ft81x_cFFFFFF(0x1d);
  ft81x_cI(dest);
  ft81x_cI(src);
  ft81x_cI(num);
}

/*
 * 5.28 CMD_BUTTON
 * Draw a button
 */
void ft81x_cmd_button(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t font, uint16_t options, const char *s)
{
  uint16_t b[6];
  b[0] = x;
  b[1] = y;
  b[2] = w;
  b[3] = h;
  b[4] = font;
  b[5] = options;
  uint32_t len = strlen(s)+1;
  int8_t align = (4 - (len & 0x3)) & 0x3;

  // check that we have enough space then send command
  ft81x_checkfree(4+sizeof(b)+len+align);
  ft81x_cFFFFFF(0x0d);
  ft81x_cN((uint8_t *)b, sizeof(b));
  ft81x_cN((uint8_t *)s, len);
  ft81x_align(len);
}

/*
 * 5.29 CMD_CLOCK
 * Draw an analog clock
 */
void ft81x_cmd_clock(uint16_t x, uint16_t y, uint16_t r, uint16_t options, uint16_t h, uint16_t m, uint16_t s, uint16_t ms)
{
  uint16_t b[8];
  b[0] = x;
  b[1] = y;
  b[2] = r;
  b[3] = options;
  b[4] = h;
  b[5] = m;
  b[6] = s;
  b[7] = ms;

  // check that we have enough space then send command
  ft81x_checkfree(4+sizeof(b));
  ft81x_cFFFFFF(0x14);
  ft81x_cN((uint8_t *)b, sizeof(b));
}


/*
 * 5.30 CMD_FGCOLOR
 * set the foreground color
* SM20180828:QA:PASS
 */
void ft81x_fgcolor_rgb32(uint32_t rgb)
{
  // check that we have enough space then send command
  ft81x_checkfree(8);
  ft81x_cFFFFFF(0x0a);
  ft81x_cI(rgb);
}
void ft81x_fgcolor_rgb888(uint8_t red, uint8_t green, uint8_t blue)
{
  ft81x_fgcolor_rgb32(((red & 255L) << 16) | ((green & 255L) << 8) | ((blue & 255L) << 0));
}

/*
 * 5.31 CMD_BGCOLOR
 * Set the background color
 */
void ft81x_bgcolor_rgb32(uint32_t rgb)
{
  // check that we have enough space then send command
  ft81x_checkfree(8);
  ft81x_cFFFFFF(0x09);
  ft81x_cI(rgb);
}
void ft81x_bgcolor_rgb888(uint8_t red, uint8_t green, uint8_t blue)
{
   ft81x_bgcolor_rgb32(((red & 255L) << 16) | ((green & 255L) << 8) | ((blue & 255L) << 0));
}

/*
 * 5.32 CMD_GRADCOLOR
 * Set the 3D button highlight color
 */
void ft81x_gradcolor_rgb32(uint32_t rgb)
{
  // check that we have enough space then send command
  ft81x_checkfree(8);
  ft81x_cFFFFFF(0x34);
  ft81x_cI(rgb);
}
void ft81x_gradcolor_rgb888(uint8_t red, uint8_t green, uint8_t blue)
{
   ft81x_gradcolor_rgb32(((red & 255L) << 16) | ((green & 255L) << 8) | ((blue & 255L) << 0));
}

/*
 * 5.33 CMD_GAUGE
 * Draw a gauge
 */
void ft81x_cmd_gauge(int16_t x, int16_t y, int16_t r, uint16_t options, uint16_t major, uint16_t minor, uint16_t val, uint16_t range)
{
  uint16_t b[7];
  b[0] = x;
  b[1] = y;
  b[2] = r;
  b[3] = options;
  b[4] = major;
  b[4] = minor;
  b[5] = val;
  b[6] = range;
  // check that we have enough space then send command
  ft81x_checkfree(4+sizeof(b));
  ft81x_cFFFFFF(0x13);
  ft81x_cN((uint8_t *)b,sizeof(b));
}

/*
 * 5.34 CMD_GRADIENT
 * Draw a smooth color gradient
 */
void ft81x_cmd_gradient(int16_t x0, int16_t y0, uint32_t rgb0, int16_t x1, int16_t y1, uint32_t rgb1)
{
  uint16_t b[8];
  b[0] = x0;
  b[1] = y0;
  b[2] = rgb0 >> 16;
  b[3] = rgb0 & 0xffff;
  b[4] = x1;
  b[5] = y1;
  b[6] = rgb1 >> 16;
  b[7] = rgb1 & 0xffff;

  // check that we have enough space then send command
  ft81x_checkfree(4+sizeof(b));
  ft81x_cFFFFFF(0x0b);
  ft81x_cN((uint8_t *)b,sizeof(b));
}

/*
 * 5.35 CMD_KEYS
 * Draw a row of keys
 */
void ft81x_cmd_keys(int16_t x, int16_t y, int16_t w, int16_t h, int16_t font, uint16_t options, const char *s)
{
  uint16_t b[6];
  b[0] = x;
  b[1] = y;
  b[2] = w;
  b[3] = h;
  b[4] = font;
  b[5] = options;

  uint32_t len = strlen(s)+1;
  int8_t align = (4 - (len & 0x3)) & 0x3;

  // check that we have enough space then send command
  ft81x_checkfree(4+sizeof(b)+len+align);
  ft81x_cFFFFFF(0x0e);
  ft81x_cN((uint8_t *)b,sizeof(b));
  ft81x_cN((uint8_t *)s, len);
  ft81x_align(len);
}

/*
 * 5.36 CMD_PROGRESS
 * Draw a progress bar
 */
void ft81x_cmd_progress(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t options, uint16_t val, uint16_t range)
{
  uint16_t b[8];
  b[0] = x;
  b[1] = y;
  b[2] = w;
  b[3] = h;
  b[4] = options;
  b[5] = val;
  b[6] = range;
  b[7] = 0; // dummy pad

  // check that we have enough space then send command
  ft81x_checkfree(4+sizeof(b));
  ft81x_cFFFFFF(0x0f);
  ft81x_cN((uint8_t *)b,sizeof(b));
}

/*
 * 5.37 CMD_SCROLLBAR
 * Draw a scroll bar
 */
void ft81x_cmd_scrollbar(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t options, uint16_t val, uint16_t size, uint16_t range)
{
  uint16_t b[8];
  b[0] = x;
  b[1] = y;
  b[2] = w;
  b[3] = h;
  b[4] = options;
  b[5] = val;
  b[6] = size;
  b[7] = range;

  // check that we have enough space then send command
  ft81x_checkfree(4+sizeof(b));
  ft81x_cFFFFFF(0x11);
  ft81x_cN((uint8_t *)b,sizeof(b));
}

/*
 * 5.38 CMD_SLIDER
 * Draw a slider
 */
void ft81x_cmd_slider(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t options, uint16_t val, uint16_t range)
{
  uint16_t b[8];
  b[0] = x;
  b[1] = y;
  b[2] = w;
  b[3] = h;
  b[4] = options;
  b[5] = val;
  b[6] = range;
  b[7] = 0; // dummy pad

  // check that we have enough space then send command
  ft81x_checkfree(4+sizeof(b));
  ft81x_cFFFFFF(0x10);
  ft81x_cN((uint8_t *)b,sizeof(b));
}

/*
 * 5.39 CMD_DIAL
 * Draw a rotary dial control
 */
void ft81x_cmd_dial(int16_t x, int16_t y, int16_t r, uint16_t options, uint16_t val)
{
  uint16_t b[6];
  b[0] = x;
  b[1] = y;
  b[2] = r;
  b[3] = options;
  b[4] = val;
  b[5] = 0; // dummy pad

  // check that we have enough space then send command
  ft81x_checkfree(4+sizeof(b));
  ft81x_cFFFFFF(0x2d);
  ft81x_cN((uint8_t *)b,sizeof(b));
}

/*
 * 5.40 CMD_TOGGLE
 * Draw a toggle switch
 */
void ft81x_cmd_toggle(int16_t x, int16_t y, int16_t w, int16_t font, uint16_t options, uint16_t state, const char *s)
{
  uint16_t b[6];
  b[0] = x;
  b[1] = y;
  b[2] = w;
  b[3] = font;
  b[4] = options;
  b[5] = state;
  uint32_t len = strlen(s)+1;
  int8_t align = (4 - (len & 0x3)) & 0x3;

   // check that we have enough space then send command
  ft81x_checkfree(4+sizeof(b)+len+align);
  ft81x_cFFFFFF(0x12);
  ft81x_cN((uint8_t *)b, sizeof(b));
  ft81x_cN((uint8_t *)s, len);
  ft81x_align(len);
}

/*
 * 5.41 CMD_TEXT
 * Draw text
 */
void ft81x_cmd_text(int16_t x, int16_t y, int16_t font, uint16_t options, const char *s)
{
  uint16_t b[4];
  b[0] = x;
  b[1] = y;
  b[2] = font;
  b[3] = options;
  uint32_t len = strlen(s)+1;
  int8_t align = (4 - (len & 0x3)) & 0x3;

  // check that we have enough space then send command
  ft81x_checkfree(4+sizeof(b)+len+align);
  ft81x_cFFFFFF(0x0c);
  ft81x_cN((uint8_t *)b, sizeof(b));
  ft81x_cN((uint8_t *)s, len);
  ft81x_align(len);
}

/*
 * 5.42 CMD_SETBASE
 * Set the base for number output
 */
void ft81x_cmd_setbase(uint32_t b)
{
  // check that we have enough space then send command
  ft81x_checkfree(8);
  ft81x_cFFFFFF(0x38);
  ft81x_cI(b);
}

/*
 * 5.43 CMD_NUMBER
 * Draw number
 */
void ft81x_cmd_number(int16_t x, int16_t y, int16_t font, uint16_t options, int32_t n)
{
   uint16_t b[6];
   b[0] = x;
   b[1] = y;
   b[2] = font;
   b[3] = options;
   b[4] = n;
   b[5] = 0; // dummy pad

   // check that we have enough space then send command
   ft81x_checkfree(sizeof(b)+4);
   ft81x_cFFFFFF(0x2e);
   ft81x_cN((uint8_t *)&b,sizeof(b));
}

/*
 * 5.44 CMD_LOADIDENTITY
 * Set the current matrix to the identity matrix
 */
void ft81x_cmd_loadidentity()
{
  // check that we have enough space then send command
  ft81x_checkfree(4);
  ft81x_cFFFFFF(0x26);
}

/*
 * 5.45 CMD_SETMATRIX
 * Write the current matrix to the display list
 */
void ft81x_cmd_setmatrix()
{
  // check that we have enough space then send command
  ft81x_checkfree(4);
  ft81x_cFFFFFF(0x2a);
}

/*
 * 5.46 CMD_GETMATRIX
 * Retrieves the current matrix within the context of the co-processor engine
 */
void ft81x_cmd_getmatrix(int32_t *a, int32_t *b, int32_t *c, int32_t *d, int32_t *e, int32_t *f)
{
  // check that we have enough space then send command
  ft81x_checkfree(4);
  ft81x_cFFFFFF(0x33);

  // The data will be written starting here in the buffer so get the pointer
  uint32_t r = ft81x_getwp();

  // Fill in memory where results will go with dummy data
  ft81x_cI(0xffffffff); // Will be a
  ft81x_cI(0xffffffff); // Will be b
  ft81x_cI(0xffffffff); // Will be c
  ft81x_cI(0xffffffff); // Will be d
  ft81x_cI(0xffffffff); // Will be e
  ft81x_cI(0xffffffff); // Will be f

  // report back memory locations of the results to caller
  *a = r;
  r+=4;
  *b = r;
  r+=4;
  *c = r;
  r+=4;
  *d = r;
  r+=4;
  *e = r;
  r+=4;
  *f = r;
}

/*
 * 5.47 CMD_GETPTR
 * Get the end memory address of data inflated by CMD_INFLATE
 */
void ft81x_cmd_getptr(uint32_t *result)
{
  // check that we have enough space then send command
  ft81x_checkfree(8);
  ft81x_cFFFFFF(0x23);

  // The data will be written starting here in the buffer so get the pointer
  uint32_t r = ft81x_getwp();

  // Fill in memory where results will go with dummy data
  ft81x_cI(0xffffffff); // Will be ptr

  // report back memory locations of the results to caller
  *result = r;
}

/*
 * 5.48 CMD_GETPROPS
 * Get the image properties decompressed by CMD_LOADIMAGE
 * BLOCKING CALL, expects to be in streaming mode
 */
void ft81x_cmd_getprops(uint32_t *ptr, uint32_t *width, uint32_t *height)
{

  // check that we have enough space then send command
  ft81x_checkfree(16);
  ft81x_cFFFFFF(0x25);

  // The data will be written starting here in the buffer so get the pointer
  uint32_t r = ft81x_getwp();

  // Fill in memory where results will go with dummy data
  ft81x_cI(0xffffffff); // Will be ptr
  ft81x_cI(0xffffffff); // Will be width
  ft81x_cI(0xffffffff); // Will be height

  // report back memory locations of the results to caller
  *ptr = r;
  r+=4;
  *width = r;
  r+=4;
  *height = r;
}

/*
 * 5.49 CMD_SCALE
 * Apply a scale to the current matrix
 */
void ft81x_cmd_scale(int32_t sx, int32_t sy)
{
  // check that we have enough space then send command
  ft81x_checkfree(12);
  ft81x_cFFFFFF(0x28);
  ft81x_cI(sx);
  ft81x_cI(sy);
}

/*
 * 5.50 CMD_ROTATE
 * Apply a rotation to the current matrix
 */
void ft81x_cmd_rotate(int32_t a)
{
  // check that we have enough space then send command
  ft81x_checkfree(8);
  ft81x_cFFFFFF(0x29);
  ft81x_cI(a);
}

/*
 * 5.51 CMD_TRANSLATE
 * Apply a translation to the current matrix
 */
void ft81x_cmd_translate(int32_t tx, int32_t ty)
{
  // check that we have enough space then send command
  ft81x_checkfree(12);
  ft81x_cFFFFFF(0x27);
  ft81x_cI(tx);
  ft81x_cI(tx);
}

/*
 * 5.52 CMD_CALIBRATE
 * Execute the touch screen calibration routine
 */
void ft81x_cmd_calibrate(uint32_t *result)
{
  // check that we have enough space then send command
  ft81x_checkfree(4);
  ft81x_cFFFFFF(0x15);

  // The data will be written starting here in the buffer so get the pointer
  uint32_t r = ft81x_getwp();

  // Fill in memory where results will go with dummy data
  ft81x_cI(0xffffffff); // Will be result

  // report back memory locations of the results to caller
  *result = r;
}

void ft81x_calibrate()
{
  ft81x_stream_start();  // Start streaming
  ft81x_clear_color_rgb32(0xffffff);
  ft81x_color_rgb32(0xffffff);
  ft81x_bgcolor_rgb32(0x402000);
  ft81x_fgcolor_rgb32(0x703800);
  ft81x_cmd_dlstart();   // Set REG_CMD_DL when done
  ft81x_clear();         // Clear the display
  ft81x_getfree(0);      // trigger FT81x to read the command buffer

  ft81x_cmd_text(180, 30, 40, OPT_CENTER, "Please tap on the dot..");
  //ft81x_cmd_calibrate(0);// Calibration command
  //ft81x_cmd_swap();      // Set AUTO swap at end of display list

  ft81x_stream_stop();   // Finish streaming to command buffer
  // Wait till the Logo is finished
  ft81x_wait_finish();
}

/*
 * 5.53 CMD_SETROTATE
 * Rotate the screen
 */
void ft81x_cmd_setrotate(uint32_t r)
{
  // check that we have enough space then send command
  ft81x_checkfree(8);
  ft81x_cFFFFFF(0x36);
  ft81x_cI(r);

  // Get our screen size W,H to confirm
  ft81x_display_width = ft81x_rd16(REG_HSIZE);
  ft81x_display_height = ft81x_rd16(REG_VSIZE);

  // portrait mode swap w & h
  if (r & 2) {
    int t = ft81x_display_height;
    ft81x_display_height = ft81x_display_width;
    ft81x_display_width = t;
  }
}

/*
 * 5.54 CMD_SPINNER
 * Start an animated spinner
 */
void ft81x_cmd_spinner(int16_t x, int16_t y, int16_t style, int16_t scale)
{
  uint16_t b[4];
  b[0] = x;
  b[1] = y;
  b[2] = style;
  b[3] = scale;

  // check that we have enough space to run the command
  ft81x_checkfree(sizeof(b)+4);
  ft81x_cFFFFFF(0x16);
  ft81x_cN((uint8_t *)&b,sizeof(b));
}

/*
 * 5.55 CMD_SCREENSAVER
 * Start an animated screensaver
 */
void ft81x_cmd_screensaver()
{
  // check that we have enough space then send command
  ft81x_checkfree(4);
  ft81x_cFFFFFF(0x2f);
}

/*
 * 5.56 CMD_SKETCH
 * Start a continuous sketch update
 */
void ft81x_cmd_sketch(int16_t x, int16_t y, int16_t w, int16_t h, int16_t ptr, int16_t format)
{
  uint16_t b[6];
  b[0] = x;
  b[1] = y;
  b[2] = w;
  b[3] = h;
  b[4] = ptr;
  b[5] = format; // dummy pad

  // check that we have enough space then send command
  ft81x_checkfree(sizeof(b)+4);
  ft81x_cFFFFFF(0x30);
  ft81x_cN((uint8_t *)&b,sizeof(b));
}

/*
 * 5.57 CMD_STOP
 * Stop any active spinner, screensaver or sketch
 */
void ft81x_cmd_stop()
{
  // check that we have enough space then send command
  ft81x_checkfree(4);
  ft81x_cFFFFFF(0x17);
}

/*
 * 5.58 CMD_SETFONT
 * Set up a custom font
 */
void ft81x_cmd_setfont(uint32_t font, uint32_t ptr)
{
  // check that we have enough space then send command
  ft81x_checkfree(12);
  ft81x_cFFFFFF(0x2b);
  ft81x_cI(font);
  ft81x_cI(ptr);
}

/*
 * 5.59 CMD_SETFONT2
 * Set up a custom font
 */
void ft81x_cmd_setfont2(uint32_t handle, uint32_t font, uint32_t ptr, uint32_t firstchar)
{
  // check that we have enough space then send command
  ft81x_checkfree(16);
  ft81x_cFFFFFF(0x3b);
  ft81x_cI(font);
  ft81x_cI(ptr);
  ft81x_cI(firstchar);
}

/*
 * 5.60 CMD_SETSCRATCH
 * Set the scratch bitmap for widget use
 */
void ft81x_cmd_setscratch(uint32_t handle)
{
  // check that we have enough space then send command
  ft81x_checkfree(8);
  ft81x_cFFFFFF(0x3c);
  ft81x_cI(handle);
}

/*
 * 5.61 CMD_ROMFONT
 * Load a ROM font into bitmap handle
 */
void ft81x_cmd_romfont(uint32_t font, uint32_t slot)
{
  // check that we have enough space then send command
  ft81x_checkfree(12);
  ft81x_cFFFFFF(0x3f);
  ft81x_cI(font);
  ft81x_cI(slot);
}


/*
 * 5.62 CMD_TRACK
 * Track touches for a graphics object
 */
void ft81x_cmd_track(int16_t x, int16_t y, int16_t width, int16_t height, int16_t tag)
{
  uint16_t b[6];
  b[0] = x;
  b[1] = y;
  b[2] = width;
  b[3] = height;
  b[4] = tag;
  b[5] = 0; // dummy pad

  // check that we have enough space then send command
  ft81x_checkfree(4+sizeof(b));
  ft81x_cFFFFFF(0x2c);
  ft81x_cN((uint8_t *)&b,sizeof(b));
}

/*
 * 5.63 CMD_SNAPSHOT
 * Take a snapshot of the current screen
 */
void ft81x_cmd_snapshot(uint32_t ptr)
{
  // check that we have enough space then send command
  ft81x_checkfree(8);
  ft81x_cFFFFFF(0x1f);
  ft81x_cI(ptr);
}

/*
 * 5.64 CMD_SNAPSHOT2
 * Take a snapshot of the current screen
 */
void ft81x_cmd_snapshot2(uint32_t fmt, uint32_t ptr, uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
  uint16_t b[4];
  b[0] = x;
  b[1] = y;
  b[2] = width;
  b[3] = height;

  // check that we have enough space then send command
  ft81x_checkfree(12+sizeof(b));
  ft81x_cFFFFFF(0x37);
  ft81x_cI(fmt);
  ft81x_cI(ptr);
  ft81x_cN((uint8_t *)&b,sizeof(b));
}

/*
 * 5.65 CMD_SETBITMAP
 * Set up display list for bitmap
 */
void ft81x_cmd_setbitmap(uint32_t addr, uint16_t fmt, uint16_t width, uint16_t height)
{
  uint16_t b[4];
  b[0] = fmt;
  b[1] = width;
  b[2] = height;
  b[3] = 0;

  // check that we have enough space then send command
  ft81x_checkfree(8+sizeof(b));
  ft81x_cFFFFFF(0x43);
  ft81x_cI(addr);
  ft81x_cN((uint8_t *)&b,sizeof(b));
}

/*
 * 5.66 CMD_LOGO
 * Play FTDI logo animation wait till it is done
 * FIXME: freespace()
 */
void ft81x_logo()
{
  ft81x_stream_start(); // Start streaming
  ft81x_cmd_dlstart();  // Set REG_CMD_DL when done
  ft81x_cFFFFFF(0x31);  // Logo command
  ft81x_cmd_swap();     // Set AUTO swap at end of display list
  ft81x_getfree(0);     // trigger FT81x to read the command buffer
  ft81x_stream_stop();  // Finish streaming to command buffer
  // Wait till the Logo is finished
  ft81x_wait_finish();
  // AFAIK the only command that will set the RD/WR to 0 when finished
  ft81x_fifo_reset();
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void TestLCD(void)
{
    debug("LCD: Test device access...\r\n");

    if (GetPowerControlState(LCD_POWER_ENABLE) == OFF)
	{
		debug("Power Control: LCD Power enable bring turned on\r\n");
		PowerControl(LCD_POWER_ENABLE, ON);
		MXC_Delay(MXC_DELAY_MSEC(500));
	}
	else { debugWarn("Power Control: LCD Power enable already on\r\n"); }

    if (GetPowerControlState(LCD_POWER_DOWN) == ON)
	{
		debug("Power Control: LCD Power down being turned off\r\n");
		PowerControl(LCD_POWER_DOWN, OFF);
		MXC_Delay(MXC_DELAY_MSEC(500));
	}
	else { debugWarn("Power Control: LCD Power down already off\r\n"); }

    debug("LCD: Restart core...\r\n");
	restart_core();

	// Read CHIP ID address until it returns a valid result.
	for (uint16_t count = 0; count < 100; count++)
	{
		ft81x_chip_id = ft81x_rd16(MEM_CHIP_ID);
		// Chip id: 08h, [id], 01h, 00h
		// [id]: FT8xx=10h, 11h, 12h, 13h
		if ((ft81x_chip_id) == 0x0810) { break; } // Chip version is FT810

		MXC_Delay(MXC_DELAY_MSEC(10));
	}

	if ((ft81x_chip_id) == 0x0810) { debug("LCD: Chip ID is FT%3x\r\n", ft81x_chip_id); }
	else { debugErr("LCD: Chip ID problem, reports 0x%04x\r\n", ft81x_chip_id); }

    debug("LCD: Single byte width selected\r\n");
	select_spi_byte_width();

    debug("LCD: Turning the backlight off (PWM set to 0)\r\n");
	ft81x_backlight_off();

    debug("LCD: FIFO reset\r\n");
	ft81x_fifo_reset();

    debug("LCD: Init display settings\r\n");
	ft81x_init_display_settings();

    debug("LCD: Test black screen\r\n");
	test_black_screen();

    debug("LCD: Init GPIO\r\n");
	ft81x_init_gpio();

    debug("LCD: Test logo\r\n");
	test_logo();

    debug("LCD: Test memory operation\r\n");
	test_memory_ops();

    debug("LCD: Test display\r\n");
	test_display();

    debug("LCD: Test color cycle\r\n");
	test_cycle_colors();

    debug("LCD: Test dots\r\n");
	test_dots();
}
