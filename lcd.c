////////////////////////////////////////////////////////////////////////////////
// Include Files
////////////////////////////////////////////////////////////////////////////////
//#include "compiler.h"
#include "Typedefs.h"
#include "lcd.h"
#include "M23018.h"
#include "gpio.h"
#include "Common.h"
#include "PowerManagement.h"
#include "Display.h"

////////////////////////////////////////////////////////////////////////////////
// Local Scope Variables
////////////////////////////////////////////////////////////////////////////////
#define PF_FIRST_TIME	0x01
#define PF_TOP_PART		0x02

#define PREPEND_BLANK_COL_TO_EACH_CHAR

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
#if 0
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
#if 0
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

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
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

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
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

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
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

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void ClearLCDscreen(void)
{
	uint8 count;
	for (count = 0; count < 8; count++)
	{
		Clear_line(count);
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void Clear_line(uint8 line)
{
	uint8 column;

	Write_display(COMMAND_REGISTER, PAGE_ADDRESS_SET + line, FIRST_HALF_DISPLAY);
	Write_display(COMMAND_REGISTER, PAGE_ADDRESS_SET + line, SECOND_HALF_DISPLAY);
	//Write_display(COMMAND_REGISTER, COLUMN_ADDRESS_HIGH);
	Write_display(COMMAND_REGISTER, COLUMN_ADDRESS_SET, FIRST_HALF_DISPLAY);
	Write_display(COMMAND_REGISTER, COLUMN_ADDRESS_SET, SECOND_HALF_DISPLAY);
	//TODO: check for second half of display then do that half
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

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void InitDisplay(void)
{
#if 0 /* old hw */
	volatile unsigned short *lcd = ((void *)AVR32_EBI_CS0_ADDRESS);

	//Set reset low
	lcd_port_image &= ~LCD_RESET;
	*lcd = lcd_port_image;

	//Set E low
	lcd_port_image &= ~LCD_ENABLE;
	*lcd = lcd_port_image;

	//Set A0, R/W, CS1, CS2 high
	lcd_port_image |= (LCD_RS | LCD_READ_WRITE | LCD_CS1 | LCD_CS2);
	*lcd = lcd_port_image;

	//Set reset high
	lcd_port_image |= LCD_RESET;
	*lcd = lcd_port_image;

	Write_display(COMMAND_REGISTER, DISPLAY_ON, FIRST_HALF_DISPLAY);
	Write_display(COMMAND_REGISTER, START_LINE_SET, FIRST_HALF_DISPLAY);
	Write_display(COMMAND_REGISTER, PAGE_ADDRESS_SET, FIRST_HALF_DISPLAY);
	Write_display(COMMAND_REGISTER, COLUMN_ADDRESS_SET, FIRST_HALF_DISPLAY);

	Write_display(COMMAND_REGISTER, DISPLAY_ON, SECOND_HALF_DISPLAY);
	Write_display(COMMAND_REGISTER, START_LINE_SET, SECOND_HALF_DISPLAY);
	Write_display(COMMAND_REGISTER, PAGE_ADDRESS_SET, SECOND_HALF_DISPLAY);
	Write_display(COMMAND_REGISTER, COLUMN_ADDRESS_SET, SECOND_HALF_DISPLAY);

	// now clear the display
	ClearLCDscreen();
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void Write_display(uint8 lcd_register, uint8 lcd_data, uint8 display_half)
{
#if 0 /* old hw */
	volatile unsigned short *lcd = ((void *)AVR32_EBI_CS0_ADDRESS);

	//Write data
	lcd_port_image = ((lcd_port_image & 0xFF00) | lcd_data);
	*lcd = lcd_port_image;

	if (lcd_register == COMMAND_REGISTER)
	{
		//Set RS low
		lcd_port_image &= ~LCD_RS;
		*lcd = lcd_port_image;
	}
	else
	{
		//Set RS high
		lcd_port_image |= LCD_RS;
		*lcd = lcd_port_image;
	}

	if (display_half == FIRST_HALF_DISPLAY)
	{
		//Set write low and CS2 low
		lcd_port_image &= (~LCD_READ_WRITE & ~LCD_CS2);
		*lcd = lcd_port_image;
	}
	else
	{
		//Set write low and CS1 low
		lcd_port_image &= (~LCD_READ_WRITE & ~LCD_CS1);
		*lcd = lcd_port_image;
	}

	//Set E high
	lcd_port_image |= LCD_ENABLE;
	*lcd = lcd_port_image;

	SoftUsecWait(100);

	//Set E low
	lcd_port_image &= ~LCD_ENABLE;
	*lcd = lcd_port_image;

	SoftUsecWait(100);

	//Set write, CS1, CS2 and address high
	lcd_port_image |= (LCD_READ_WRITE | LCD_CS1 | LCD_CS2 | LCD_RS);
	*lcd = lcd_port_image;

	SoftUsecWait(100);
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void Write_multi_display(uint8 lcd_register, uint8 lcd_data, uint8 display_half)
{
	UNUSED(lcd_register);
	UNUSED(lcd_data);
	UNUSED(display_half);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8 Read_display(uint8 lcd_register, uint8 display_half)
{
#if 0 /* old hw */
	uint16 lcd_data;

	volatile unsigned short *lcd = ((void *)AVR32_EBI_CS0_ADDRESS);

	if (lcd_register == COMMAND_REGISTER)
	{
		//Set RS low
		lcd_port_image &= ~LCD_RS;
	}
	else
	{
		//Set RS high
		lcd_port_image |= LCD_RS;
	}

	//Set RD/WR high for read
	lcd_port_image |= LCD_READ_WRITE;
	*lcd = lcd_port_image;

	if (display_half == FIRST_HALF_DISPLAY)
	{
		//Set CS2 low
		lcd_port_image &= ~LCD_CS2;
	}
	else
	{
		//Set CS1 low
		lcd_port_image &= ~LCD_CS1;
	}

	//Set E high
	lcd_port_image |= LCD_ENABLE;
	*lcd = lcd_port_image;

	//Read data
	lcd_data = *lcd;

	//Set E low
	lcd_port_image &= ~LCD_ENABLE;

	//Set RD/WR, CS1, CS2 and RS high
	lcd_port_image |= (LCD_ENABLE | LCD_CS1 | LCD_CS2 | LCD_RS);
	*lcd = lcd_port_image;

	return((uint8)(lcd_data & 0xFF));
#endif
}// End of function

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void Backlight_On(void)
{
#if 0 /* old hw */
	volatile unsigned short *lcd = ((void *)AVR32_EBI_CS0_ADDRESS);

	lcd_port_image |= LCD_BACKLIGHT_ON;
	*lcd = lcd_port_image;
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void Backlight_Off(void)
{
#if 0 /* old hw */
	volatile unsigned short *lcd = ((void *)AVR32_EBI_CS0_ADDRESS);

	lcd_port_image &= ~LCD_BACKLIGHT_ON;
	*lcd = lcd_port_image;
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void Backlight_High(void)
{
#if 0 /* old hw */
	volatile unsigned short *lcd = ((void *)AVR32_EBI_CS0_ADDRESS);

	lcd_port_image |= LCD_BACKLIGHT_HIGH;
	*lcd = lcd_port_image;
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void Backlight_Low(void)
{
#if 0 /* old hw */
	volatile unsigned short *lcd = ((void *)AVR32_EBI_CS0_ADDRESS);

	lcd_port_image &= ~LCD_BACKLIGHT_HIGH;
	*lcd = lcd_port_image;
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void Reset_Contrast(void)
{
	PowerControl(LCD_POWER_ENABLE, OFF);
	PowerControl(LCD_CONTRAST_ENABLE, OFF);

	PowerControl(LCD_CONTRAST_ENABLE, ON);
	PowerControl(LCD_POWER_ENABLE, ON);
	SoftUsecWait(LCD_ACCESS_DELAY);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void Set_Contrast(uint8 level)
{
	uint8 counts = 0;
	uint8 i;

	Reset_Contrast();

	if (level < 32)
	{
		counts = level + 32;
	}
	else
	{
		counts = level - 32;
	}

	for (i = 0; i < counts; i++)
	{
		// Toggle to adjust
		PowerControl(LCD_CONTRAST_ENABLE, OFF);
		PowerControl(LCD_CONTRAST_ENABLE, ON);
	}
}

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
