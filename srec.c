//
// srec.c
//
//  Created on: Apr 26, 2010
//	  Author: SW1
//

#if 0 /* test removal */

#include "srec.h"		   // func defs and misc defs for this file
//#include "usart.h"
#include "typedefs.h"
#include "gpio.h"
//#include "print_funcs.h"
//#include "enumes.h"
#include "string.h"
#include "lcd.h"

//#include "fsaccess.h"

#include "Globals.h"

//char srecVer[] =  "$Id: srec.c,v 1.3 2012/03/20 01:04:46 jgetz Exp $";
//char srec_c_Rev[] = "$Revision: 1.3 $";

uint16  *code;	// code space data array
uint32  codesize; // code size variable
uint16  records;
RECORD_DATA srecdata;
uint32  bytes_loaded;

#define SRAM_CODE		   (((void *)AVR32_EBI_CS1_ADDRESS) + 0x00700000)

#define SRAM_CODE_SIZE	  0x00100000

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
/* static */ char * Srec_UartGets(char *s, int channel)
{
	char *b = s;
	bool end = FALSE;
	int  data = 0;
	int  count = 0;

	do
	{
		data = usart_getchar((volatile avr32_usart_t *)channel);

		switch(data)
		{
			case '\b':
			case 0x7e:
				if (count)
				{
					count--;
					b--;

					if (channel != (int)DBG_USART)
					{
						print((volatile avr32_usart_t *)channel, "\b \b");
					}
				}
				break;
//			case '\r':
			case '\n':
				if (count)
				{
					*b++ = (char)data;
					*b = 0;

					if (channel != (int)DBG_USART)
					{
						print((volatile avr32_usart_t *)channel, "\r\n");
					}

					end = TRUE;
				}
				break;
			case CAN_CHAR:
				*b = CAN_CHAR;
				*s = CAN_CHAR;
				end = TRUE;
				break;
			default:
				if (count < 255)
				{
					count++;
					*b++ = (char)data;

					if (channel != (int)DBG_USART)
					{
						print_char((volatile avr32_usart_t *)channel, data);
					}
				}
				break;
		}
	}while(!end);

	if (*b != CAN_CHAR)
	{
		*b = 0;
	}
	return s;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int Get_and_save_srec(int file)
{
   uint16 badrecords = 0;
   int imageType = -1;
   uint16 lastrecord = FALSE;

   ASCII_SREC_DATA asciidata;
   RECORD_DATA linedata;

   records = 0;

   while(lastrecord == FALSE)
   {
	   Srec_get_line(&asciidata);
	   //Srec_xOff();

	   if ((asciidata.RecordID == 0) || (asciidata.RecordID == CAN_CHAR))
	   {
		   lastrecord = TRUE;
		   imageType = -1;
	   }

	   else
	   {
		   records++;

		   write(file, (char*)&asciidata, sizeof(asciidata));
		   file_putc(0x0D);
		   file_putc(0x0A);

		   linedata = Srec_convert_line(asciidata);

		   if ((linedata.RecordType == SREC_HEADER) ||
			   (linedata.RecordType == SREC_DATA))
		   {
			   Srec_ack();
		   }
		   else if (linedata.RecordType == SREC_END)
		   {
			   file_putc(0x00);

			   lastrecord = TRUE;
			   Srec_ack();
		   }

		   else // bad data type for now
		   {
			   records--;

#if 0 /* Does nothing because records is unsigned */
			   if (records < 0)
			   {
				   records = 0;
			   }
#endif
			   // try each data record a total of 3 times for success
			   if (badrecords++ == 2)
			   {
				   imageType = -1;
				   lastrecord = TRUE;
			   }

			   Srec_nack();
		   }
	   }
	   //Srec_xOn();
	}
	return(imageType);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int Unpack_srec(int file)
{
	uint8  lineDone;
	uint8  *tempcode;
	uint8  *fileData;
	uint16 badrecords = 0;
	int imageType = -1;
	uint8 firstEntry = 0;
	uint16 lastrecord = FALSE;
	uint32 imageAddr = 0;
	uint32 lastDataLen = 0;
	uint32 filelength;
	uint32 progress;
	char   textBuffer[25];
	volatile uint32 *eventDataBuff = SRAM_CODE;

	ASCII_SREC_DATA asciidata;
	RECORD_DATA linedata;

	fileData = (uint8*)&asciidata;
	code = (uint16*)eventDataBuff;
	tempcode = (uint8*)code;
	records = 0;

	filelength = 0; //fsaccess_file_get_size(file);

	progress = 0;
	bytes_loaded = 0;

	while(lastrecord == FALSE)
	{
		if (bytes_loaded > progress)
		{
			sprintf(textBuffer,"Loading Image..%d%c",(unsigned int)(progress/(filelength / 100)),0x25);
			//WriteLCD_smText(0, 64, (uint8*)textBuffer, NORMAL_LCD);
			//WriteLCD_smText(0, 0, (uint8*)textBuffer, NORMAL_LCD);
			progress += filelength / 100;
		}
		//Srec_file_get_line(&asciidata);
		//Srec_xOff();
		fileData = (uint8*)&asciidata;
		lineDone = FALSE;
		while(lineDone == FALSE)
		{
			*fileData = file_getc();

			if (*fileData == 0x0A)
			{
				lineDone = TRUE;
			}
			fileData++;
			bytes_loaded++;
		}

		if ((asciidata.RecordID == 0) || (asciidata.RecordID == CAN_CHAR))
		{
			lastrecord = TRUE;
			imageType = -1;
		}

		else
		{
			records++;
			linedata = Srec_convert_line(asciidata);

			if (linedata.RecordType == SREC_HEADER)
			{
			//	Srec_ack();
			}

			else if (linedata.RecordType == SREC_DATA)
			{
				if (Srec_checksum(linedata))
				{
					if (firstEntry == 0)
					{
						imageType = EXTERNAL_IMAGE;
						imageAddr = linedata.Address;
						firstEntry++;
					}
					else
					{
						tempcode = (uint8*)code + (linedata.Address - imageAddr);
					}

					Srec_get_data(linedata, tempcode);
					//tempcode = tempcode + (linedata.Length - 5);
					lastDataLen = (uint32)(linedata.Length - 5);

					// reset bad records count
					badrecords = 0;
					//Srec_ack();
				}

				else
				{
				//	  Srec_nack();
				}
			}

			else if (linedata.RecordType == SREC_END)
			{
				tempcode += lastDataLen;
				lastrecord = TRUE;
				//Srec_ack();
			}

			else // bad data type for now
			{
				records--;

#if 0 /* Does nothing because records is unsigned */
				if (records < 0)
				{
					records = 0;
				}
#endif
				// try each data record a total of 3 times for success
				if (badrecords++ == 2)
				{
					imageType = -1;
					lastrecord = TRUE;
				}
				//Srec_nack();
			}
		}
		//Srec_xOn();
	}

	codesize = (unsigned long)((uint8*)tempcode - (uint8*)code);
	return(imageType);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void Srec_get_line(ASCII_SREC_DATA * asciidata)
{
	//uint32 timercount;
	uint8  size;
	uint8  charbuffer[255];

	// fill in structure with 0x00 data
	memset(&asciidata->RecordID, 0x00, sizeof(ASCII_SREC_DATA));

	memset(&charbuffer[0], 0x00, sizeof(charbuffer));

	Srec_UartGets((char*)&charbuffer[0], (int)DBG_USART);

	if (charbuffer[0] == CAN_CHAR)
	{
		asciidata->RecordID = CAN_CHAR;
		return;
	}

	size =(unsigned char)(strlen((char*)&charbuffer[0]) - 2);

	memcpy(&asciidata->RecordID,   &charbuffer[0],   size);

	asciidata->Checksum[0] = charbuffer[size];
	asciidata->Checksum[1] = charbuffer[size+1];

	return;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
RECORD_DATA Srec_convert_line(ASCII_SREC_DATA linedata)
{
	uint8 count;
	uint8 index;
	uint8 tempbuff[10];

	// Do record type
	memset(&tempbuff[0], 0x00, sizeof(tempbuff));
	memcpy(&tempbuff[0], &linedata.RecordType, 1);
	srecdata.RecordType = Atoh_1(&tempbuff[0]);

	if (srecdata.RecordType == SREC_DATA)
	{
		// Do length
		memset(&tempbuff[0], 0x00, sizeof(tempbuff));
		memcpy(&tempbuff[0], &linedata.Count[0], 2);
		srecdata.Length = Atoh_2(&tempbuff[0]);

		// Do length
		memset(&tempbuff[0], 0x00, sizeof(tempbuff));
		memcpy(&tempbuff[0], &linedata.Addr[0], 8);
		srecdata.Address = Atoh_4(&tempbuff[0]);

		// Do data bytes
		count = (unsigned char)(srecdata.Length - 5);
		index = 0;
		while(count--)
		{
			memset(&tempbuff[0], 0x00, sizeof(tempbuff));
			memcpy(&tempbuff[0], &linedata.Data[ (index * 2) ], 2);
			srecdata.Data[index] = Atoh_2(&tempbuff[0]);
			index++;
		}

		// Do checksum
		memset(&tempbuff[0], 0x00, sizeof(tempbuff));
//		memcpy(&tempbuff[0], &linedata.Checksum[0], 8);
		memcpy(&tempbuff[0], &linedata.Data[ (index * 2) ], 2);
		srecdata.Checksum = Atoh_2(&tempbuff[0]);
	}

	return(srecdata);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8 Srec_checksum(RECORD_DATA linedata)
{
	uint8 bytecount;
	uint8 checksum = 0;
	uint8 index = 0;
	bool result = FALSE;

	bytecount = linedata.Length;

	checksum = linedata.Length;
	bytecount--;

	checksum += (uint8) (((linedata.Address & 0xFF000000) >> 24) & 0xFF);
	checksum += (uint8) (((linedata.Address & 0x00FF0000) >> 16) & 0xFF);
	checksum += (uint8) (((linedata.Address & 0x0000FF00) >> 8) & 0xFF);
	checksum += (uint8) (linedata.Address & 0xFF);

	bytecount -= 4;

	while(bytecount--)
	{
		checksum += (linedata.Data[index++]);
	}

	checksum = (unsigned char)~(0xFF & checksum);

	if (checksum == linedata.Checksum)
	{
		result = TRUE;
	}

	return(result);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void Srec_get_data(RECORD_DATA Linedata, uint8 *data)
{
	uint8 bytecount, byteindex;

	byteindex = 0;
	bytecount = (unsigned char)(Linedata.Length - (SREC_ADDR_LENGTH + SREC_CKSUM_LENGTH));

	while(bytecount--)
	{
		*data++ = Linedata.Data[byteindex++];
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void Srec_ack(void)
{
	usart_putchar(DBG_USART, ACK_CHAR);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void Srec_nack(void)
{
	usart_putchar(DBG_USART, NACK_CHAR);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8 Atoc(uint8 ch)
{
	if ((ch >= 0x30) && (ch <= 0x39))
	{
		// Got a character between '0' and '9', convert to the raw hex number
		return (uint8)(ch-0x30);
	}
	else if ((ch >= 0x41) && (ch <= 0x46))
	{
		// Got a character between 'A' and 'F', convert to the raw hex number
		return (uint8)(ch-0x37);
	}
	else if ((ch >= 0x61) && (ch <= 0x66))
	{
		// Got a character between 'a' and 'f', convert to the raw hex number
		return (uint8)(ch-0x57);
	}
	else // Should get here
	{
		return 0;
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8 Atonum(uint8 ch)
{
	if (ch == '1')
	{
		return(1);
	}

	else if (ch == '2')
	{
		return(2);
	}

	else if (ch == '3')
	{
		return(3);
	}

	else if (ch == '4')
	{
		return(4);
	}

	else if (ch == '5')
	{
		return(5);
	}

	else if (ch == '6')
	{
		return(6);
	}

	else if (ch == '7')
	{
		return(7);
	}

	else if (ch == '8')
	{
		return(8);
	}

	else if (ch == '9')
	{
		return(9);
	}

	else if (ch == 'A')
	{
		return(10);
	}

	else if (ch == 'B')
	{
		return(11);
	}

	else if (ch == 'C')
	{
		return(12);
	}

	else if (ch == 'D')
	{
		return(13);
	}

	else if (ch == 'E')
	{
		return(14);
	}

	else if (ch == 'F')
	{
		return(15);
	}

	else
	{
		return(0);
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint32 Atoh_4(uint8 * ch)
{
	uint32 result = 0;
	result = (unsigned long)(Atoc(*ch) * 0x10000000);
	ch++;
	result += Atoc(*ch) * 0x01000000;
	ch++;
	result += Atoc(*ch) * 0x00100000;
	ch++;
	result += Atoc(*ch) * 0x00010000;
	ch++;
	result += Atoc(*ch) * 0x00001000;
	ch++;
	result += Atoc(*ch) * 0x00000100;
	ch++;
	result += Atoc(*ch) * 0x00000010;
	ch++;
	result += Atoc(*ch);

	return(result);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8 Atoh_2(uint8 * ch)
{
	uint8 result = 0;
	result = (unsigned char)(Atoc(*ch) * 0x10);
	ch++;
	result += Atoc(*ch);

	return(result);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8 Atoh_1(uint8 * ch)
{
	uint8 result = 0;
	result = Atoc(*ch);

	return(result);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void Srec_xOff(void)
{
	usart_putchar(DBG_USART, XOFF_CHAR);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void Srec_xOn(void)
{
	usart_putchar(DBG_USART, XON_CHAR);
}

#endif /* test removal */