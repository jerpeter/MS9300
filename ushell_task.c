/**************************************************************************
 *
 * \file
 *
 * \brief uShell command line interpreter.
 *
 * Copyright (c) 2009 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel microcontroller product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 ***************************************************************************/

#if 0 /* old hw */

//_____  I N C L U D E S ___________________________________________________

#if (defined __GNUC__)
  //#include "nlao_usart.h"
#endif
#include <stdio.h>
#include <string.h>
//#include "compiler.h"
#include "board.h"
#include "gpio.h"
//#include "usart.h"
//#include "ctrl_access.h"
//#include "fat.h"
//#include "file.h"
//#include "navigation.h"
//#include "conf_usb.h"
//#include "usb_drv.h"
//#include "cycle_counter.h"
#if USB_HOST_FEATURE == TRUE
  #include "usb_host_enum.h"
  #include "usb_host_task.h"
  #include "host_mass_storage_task.h"
#endif
#include "ushell_task.h"
#include "EventProcessing.h"

//_____ M A C R O S ________________________________________________________

/*! \name Shell USART Configuration
 */
//! @{
#if BOARD == EVK1100
#  define SHL_USART               (&AVR32_USART0)
#  define SHL_USART_RX_PIN        AVR32_USART0_RXD_0_0_PIN
#  define SHL_USART_RX_FUNCTION   AVR32_USART0_RXD_0_0_FUNCTION
#  define SHL_USART_TX_PIN        AVR32_USART0_TXD_0_0_PIN
#  define SHL_USART_TX_FUNCTION   AVR32_USART0_TXD_0_0_FUNCTION
#  define SHL_USART_BAUDRATE      115200
#else
#  error Unsupported board
#endif
//! @}

#define fputs(a, b)				usart_write_line((&AVR32_USART0), a)
#define putchar_special(a)		usart_putchar((&AVR32_USART0), a)

#define USHELL_HISTORY        10 // Must be 1 or more
#define USHELL_NB_LINE        20
#define USHELL_NB_COL         80

#define USHELL_SIZE_CMD_LINE  70
#define USHELL_MAX_NB_ARG     2

//_____ D E C L A R A T I O N S ____________________________________________

// Debug print buffer
static char pBuffer[120];

// Manage task
static Bool     g_b_ushell_task_run = FALSE;
static uint32_t g_u32_ushell_pba_hz;

// To manage command line
static uint8_t  g_u8_escape_sequence=0;
static uint8_t  g_u8_cmd_size=0;
static uint8_t  g_u8_history_pos=0;
static uint8_t  g_u8_history_pos_search=0;
static char     g_s_cmd_his[USHELL_HISTORY][USHELL_SIZE_CMD_LINE];
static char     g_s_cmd[USHELL_SIZE_CMD_LINE];
static char     g_s_arg[USHELL_MAX_NB_ARG][USHELL_SIZE_CMD_LINE];

// To manage a file system shortcut
static Fs_index g_mark_index;

// Internal sub routines
Bool    ushell_cmd_scan         ( void );
uint8_t ushell_cmd_decode       ( void );
void    ushell_clean_cmd_line   ( void );
void    ushell_history_up       ( void );
void    ushell_history_down     ( void );
void    ushell_history_display  ( void );
Bool    ushell_more_wait        ( void );
// Internal sub routines for file system commands
void  ushell_cmd_nb_drive     ( void );
void  ushell_cmd_free_space   ( void );
void  ushell_cmd_format       ( void );
void  ushell_cmd_mount        ( void );
void  ushell_cmd_space        ( void );
void  ushell_cmd_ls           ( Bool b_more );
void  ushell_cmd_cd           ( void );
void  ushell_cmd_gotoparent   ( void );
void  ushell_cmd_cat          ( Bool b_more);
void  ushell_cmd_help         ( void );
void  ushell_cmd_mkdir        ( void );
void  ushell_cmd_touch        ( void );
void  ushell_cmd_rm           ( void );
void  ushell_cmd_append_file  ( void );
void  ushell_cmd_copy         ( void );
void  ushell_cmd_rename       ( void );
Bool  ushell_cmd_sync         ( void );
void  ushell_cmd_perform      ( void );
void  ushell_path_valid_syntac( char *path );
// Internal sub routines for USB commands
void  ushell_cmdusb_ls        ( void );
void  ushell_cmdusb_suspend   ( void );
void  ushell_cmdusb_resume    ( void );
Bool  ushell_cmd_syncevents   ( uint8_t, uint16_t*, uint16_t*, uint16_t*, uint16_t* );


//! @brief This function initializes the hardware/software resources required for ushell task.
//!
void ushell_task_init(uint32_t pba_hz)
{
  uint8_t u8_i;

#if 0 /* Done elsewhere */
  //** Initialize the USART used by uShell with the configured parameters
  static const gpio_map_t SHL_USART_GPIO_MAP =
  {
    {SHL_USART_RX_PIN, SHL_USART_RX_FUNCTION},
    {SHL_USART_TX_PIN, SHL_USART_TX_FUNCTION}
  };
#if (defined __GNUC__)
  set_usart_base((void *)SHL_USART);
  gpio_enable_module(SHL_USART_GPIO_MAP,
                     sizeof(SHL_USART_GPIO_MAP) / sizeof(SHL_USART_GPIO_MAP[0]));
  usart_init(SHL_USART_BAUDRATE);
#elif (defined __ICCAVR32__)
  static const usart_options_t SHL_USART_OPTIONS =
  {
    .baudrate = SHL_USART_BAUDRATE,
    .charlength = 8,
    .paritytype = USART_NO_PARITY,
    .stopbits = USART_1_STOPBIT,
    .channelmode = USART_NORMAL_CHMODE
  };

  extern volatile avr32_usart_t *volatile stdio_usart_base;
  stdio_usart_base = SHL_USART;
  gpio_enable_module(SHL_USART_GPIO_MAP,
                     sizeof(SHL_USART_GPIO_MAP) / sizeof(SHL_USART_GPIO_MAP[0]));
  usart_init_rs232(SHL_USART, &SHL_USART_OPTIONS, pba_hz);
#endif
#endif

  //** Configure standard I/O streams as unbuffered.
#if (defined __GNUC__)
  setbuf(stdin, NULL);
#endif
  setbuf(stdout, NULL);

  // Set default state of ushell
  g_b_ushell_task_run = FALSE;
  for( u8_i=0; u8_i<USHELL_HISTORY; u8_i++ ) {
     g_s_cmd_his[u8_i][0] = 0;  // Set end of line for all cmd line history
  }

  //fputs(MSG_EXIT, stdout );

  g_u32_ushell_pba_hz = pba_hz;  // Save value to manage a time counter during perform command

}


/*! \brief Entry point of the explorer task management.
 *
 * This function performs uShell decoding to access file-system functions.
 */
void ushell_task(void)
{
    uint16_t totalFilesCopied;
    uint16_t totalFilesSkipped;
    uint16_t totalFilesReplaced;
    uint16_t totalFilesDuplicated;

   //** No loop with the basic scheduler
   {

   //** Check the USB mode and authorize/unauthorize ushell
   if(!g_b_ushell_task_run)
   {
      if( Is_usb_id_device() )
         return;     // Exit of the task scheduled

      g_b_ushell_task_run = TRUE;
      // Display shell startup
      fputs(MSG_WELCOME, stdout);
      ushell_cmd_nb_drive();
      fputs(MSG_PROMPT, stdout);

      // Reset the embedded FS on ushell navigator and on first drive
      nav_reset();
      nav_select( FS_NAV_ID_USHELL_CMD );
      nav_drive_set( 0 );
   }else{
      if( Is_usb_id_device() )
      {
         g_b_ushell_task_run = FALSE;
         fputs(MSG_EXIT, stdout );
         nav_exit();
         return;     // Exit of the task scheduled
      }
   }

   //** Scan shell command
   if( !ushell_cmd_scan() )
      return;     // Exit of the task scheduled

   //** Command ready then decode and execute this one
   switch( ushell_cmd_decode() )
   {
      // Displays number of  drives
      case CMD_NB_DRIVE:
      ushell_cmd_nb_drive();
      break;

      // Displays free space information for all connected drives
      case CMD_DF:
      ushell_cmd_free_space();
      break;

      // Formats disk
      case CMD_FORMAT:
      ushell_cmd_format();
      break;

      // Mounts a drive (e.g. "b:")
      case CMD_MOUNT:
      ushell_cmd_mount();
      break;

      // Displays the space information for current drive
      case CMD_SPACE:
      ushell_cmd_space();
      break;

      // Lists the files present in current directory (e.g. "ls")
      case CMD_LS:
      ushell_cmd_ls(FALSE);
      break;
      case CMD_LS_MORE:
      ushell_cmd_ls(TRUE);
      break;

      // Enters in a directory (e.g. "cd folder_toto")
      case CMD_CD:
      ushell_cmd_cd();
      break;

      // Enters in parent directory ("cd..")
      case CMD_UP:
      ushell_cmd_gotoparent();
      break;

      // Displays a text file
      case CMD_CAT:
      ushell_cmd_cat(FALSE);
      break;
      case CMD_CAT_MORE:
      ushell_cmd_cat(TRUE);
      break;

      // Displays the help
      case CMD_HELP:
      ushell_cmd_help();
      break;

      // Creates directory
      case CMD_MKDIR:
      ushell_cmd_mkdir();
      break;

      // Creates file
      case CMD_TOUCH:
      ushell_cmd_touch();
      break;

      // Deletes files or directories
      case CMD_RM:
      ushell_cmd_rm();
      break;

      // Appends char to selected file
      case CMD_APPEND:
      ushell_cmd_append_file();
      break;

      // Index routines (= specific shortcut from ATMEL FileSystem)
      case CMD_SET_ID:
      g_mark_index = nav_getindex();
      break;
      case CMD_GOTO_ID:
      nav_gotoindex( &g_mark_index );
      break;

      // Copies file to other location
      case CMD_CP:
      ushell_cmd_copy();
      break;

      // Renames file
      case CMD_MV:
      ushell_cmd_rename();
      break;

      // Synchronize folders
      case CMD_SYNC:
      ushell_cmd_sync();
      break;

      case CMD_PERFORM:
      ushell_cmd_perform();
      break;

      // USB commands
#if USB_HOST_FEATURE == TRUE
      case CMD_LS_USB:
      ushell_cmdusb_ls();
      break;
      case CMD_USB_SUSPEND:
      ushell_cmdusb_suspend();
      break;
      case CMD_USB_RESUME:
      ushell_cmdusb_resume();
      break;
#endif

      case CMD_SYNCEVENTS:
	  ushell_cmd_syncevents(USB_SYNC_FROM_SHELL, &totalFilesCopied, &totalFilesSkipped, &totalFilesReplaced, &totalFilesDuplicated);
      break;

      case CMD_NONE:
      break;

      // Unknown command
      default:
      fputs(MSG_ER_CMD_NOT_FOUND, stdout);
      break;
   }

   fputs(MSG_PROMPT, stdout);

   }
}


//! @brief Get the full command line to be interpreted.
//!
//! @return TRUE, if a command is ready
//!
Bool ushell_cmd_scan(void)
{
   int c_key;

   // Something new of the UART ?
   if (usart_read_char(SHL_USART, &c_key) != USART_SUCCESS)
   {
      usart_reset_status(SHL_USART);
      return FALSE;
   }

   if( 0 != g_u8_escape_sequence )
   {
      //** Decode escape sequence
      if( 1 == g_u8_escape_sequence )
      {
         if( 0x5B != c_key )
         {
            g_u8_escape_sequence=0;
            return FALSE;  // Escape sequence cancel
         }
         g_u8_escape_sequence=2;
      }
      else
      {
         // Decode value of the sequence
         switch (c_key)
         {
/*
Note: OVERRUN error on USART with an RTOS and USART without interrupt management
If you want support "Escape sequence", then you have to implement USART interrupt management
            case 0x41:     // UP command
            ushell_clean_cmd_line();
            ushell_history_up();
            ushell_history_display();
            break;
            case 0x42:     // DOWN command
            ushell_clean_cmd_line();
            ushell_history_down();
            ushell_history_display();
            break;
*/
            default:       // Ignore other command
            break;
         }
         g_u8_escape_sequence=0; // End of Escape sequence
      }
      return FALSE;
   }

   //** Normal sequence
   switch (c_key)
   {
      //** Command validation
      case ASCII_CR:
      putchar_special(ASCII_CR);         // Echo
      putchar_special(ASCII_LF);         // Add new line flag
      g_s_cmd_his[g_u8_history_pos][g_u8_cmd_size]=0;  // Add NULL terminator at the end of command line
      return TRUE;

      //** Enter in escape sequence
      case ASCII_ESCAPE:
      g_u8_escape_sequence=1;
      break;

      //** backspace
      case ASCII_BKSPACE:
      if(g_u8_cmd_size>0)        // Beginning of line ?
      {
         // Remove the last character on terminal
         putchar_special(ASCII_BKSPACE); // Send a backspace to go in previous character
         putchar_special(' ');           // Send a space to erase previous character
         putchar_special(ASCII_BKSPACE); // Send a backspace to go in new end position (=previous character position)
         // Remove the last character on cmd line buffer
         g_u8_cmd_size--;
      }
      break;

      // History management
      case '!':
      ushell_clean_cmd_line();
      ushell_history_up();
      ushell_history_display();
      break;
      case '$':
      ushell_clean_cmd_line();
      ushell_history_down();
      ushell_history_display();
      break;

      //** Other char
      default:
      if( (0x1F<c_key) && (c_key<0x7F) && (USHELL_SIZE_CMD_LINE!=g_u8_cmd_size) )
      {
         // Accept char
         putchar_special(c_key);                                          // Echo
         g_s_cmd_his[g_u8_history_pos][g_u8_cmd_size++] = c_key;  // append to cmd line
      }
      break;
   }
   return FALSE;
}


//! @brief decodes full command line into command type and arguments
//!
//!
//! @return the command type decoded
//!
//! @verbatim
//! The arguments are storage in g_s_arg global array
//! @endverbatim
//!
uint8_t ushell_cmd_decode( void )
{
   uint8_t cmd_type;
   uint8_t u8_i,u8_j,u8_k;
   Bool b_arg_include_space;

   if(0==g_u8_cmd_size)
   {
      // Command line empty
      return CMD_NONE;
   }

   // Get command string and Change command to lower case
   for( u8_i=0; (g_s_cmd_his[g_u8_history_pos][u8_i]!=' ') && (u8_i<=g_u8_cmd_size); u8_i++)
   {
      g_s_cmd[u8_i] = g_s_cmd_his[g_u8_history_pos][u8_i];
      if( ('A'<=g_s_cmd[u8_i]) && (g_s_cmd[u8_i]<='Z') )
         g_s_cmd[u8_i] += ('a'-'A');
   }
   g_s_cmd[u8_i]=0;

   // Get arguments strings
   for( u8_j=0; u8_j<USHELL_MAX_NB_ARG; u8_j++ )
   {
      u8_i++;     // Jump space character
      // Check "
      b_arg_include_space = ( g_s_cmd_his[g_u8_history_pos][u8_i] == '"' );
      if( b_arg_include_space ) {
        u8_i++;
      }
      for( u8_k=0;
           (b_arg_include_space || (g_s_cmd_his[g_u8_history_pos][u8_i] != ' '))
           && ((!b_arg_include_space) || (g_s_cmd_his[g_u8_history_pos][u8_i] != '"'))
           && (u8_i<=g_u8_cmd_size);
           u8_i++, u8_k++ )
      {
         g_s_arg[u8_j][u8_k] = g_s_cmd_his[g_u8_history_pos][u8_i];
      }
      if( b_arg_include_space ) {
        u8_i++;   // Jump last "
      }
      g_s_arg[u8_j][u8_k] = 0;
   }

   // Reset command size and update history
   g_u8_cmd_size=0;
   g_u8_history_pos++;
   if( g_u8_history_pos == USHELL_HISTORY)
      g_u8_history_pos = 0;
   g_u8_history_pos_search = g_u8_history_pos;

   // Decode command type
   if (!strcmp(g_s_cmd, STR_DISK ))
   {  cmd_type=CMD_NB_DRIVE; }
   else if ( !strcmp(g_s_cmd, STR_DF))
   {  cmd_type=CMD_DF; }
   else if ( !strcmp(g_s_cmd, STR_FORMAT))
   {  cmd_type=CMD_FORMAT; }
   else if ( !strcmp(g_s_cmd, STR_MOUNT))
   {  cmd_type=CMD_MOUNT; }
   else if ( g_s_cmd[1]==':' )
   {  cmd_type=CMD_MOUNT; g_s_arg[0][0]=g_s_cmd[0];g_s_arg[0][1]='0'; }
   else if ( !strcmp(g_s_cmd, STR_SPACE))
   {  cmd_type=CMD_SPACE; }
   else if ( !strcmp(g_s_cmd, STR_LS))
   {  cmd_type=CMD_LS; }
   else if ( !strcmp(g_s_cmd, STR_LS_MORE))
   {  cmd_type=CMD_LS_MORE; }
   else if (!strcmp(g_s_cmd, STR_CD))
   {  cmd_type=CMD_CD; }
   else if ( !strcmp(g_s_cmd, STR_UP))
   {  cmd_type=CMD_UP; }
   else if ( !strcmp(g_s_cmd, STR_CAT))
   {  cmd_type=CMD_CAT; }
   else if ( !strcmp(g_s_cmd, STR_CAT_MORE))
   {  cmd_type=CMD_CAT_MORE; }
   else if ( !strcmp(g_s_cmd, STR_HELP))
   {  cmd_type=CMD_HELP; }
   else if ( !strcmp(g_s_cmd, STR_MKDIR))
   {  cmd_type=CMD_MKDIR; }
   else if ( !strcmp(g_s_cmd, STR_TOUCH))
   {  cmd_type=CMD_TOUCH; }
   else if ( !strcmp(g_s_cmd, STR_RM))
   {  cmd_type=CMD_RM; }
   else if ( !strcmp(g_s_cmd, STR_APPEND))
   {  cmd_type=CMD_APPEND; }
   else if ( !strcmp(g_s_cmd, STR_MARK))
   {  cmd_type=CMD_SET_ID; }
   else if ( !strcmp(g_s_cmd, STR_GOTO))
   {  cmd_type=CMD_GOTO_ID; }
   else if ( !strcmp(g_s_cmd, STR_CP))
   {  cmd_type=CMD_CP; }
   else if ( !strcmp(g_s_cmd, STR_MV))
   {  cmd_type=CMD_MV; }
   else if ( !strcmp(g_s_cmd, STR_SYNC))
   {  cmd_type=CMD_SYNC; }
   else if ( !strcmp(g_s_cmd, STR_PERFORM))
   {  cmd_type=CMD_PERFORM; }
#if USB_HOST_FEATURE == TRUE
   else if ( !strcmp(g_s_cmd, STR_LS_USB))
   {  cmd_type=CMD_LS_USB; }
   else if ( !strcmp(g_s_cmd, STR_USB_SUSPEND))
   {  cmd_type=CMD_USB_SUSPEND; }
   else if ( !strcmp(g_s_cmd, STR_USB_RESUME))
   {  cmd_type=CMD_USB_RESUME; }
#endif
   else if ( !strcmp(g_s_cmd, STR_SYNCEVENTS))
   {  cmd_type=CMD_SYNCEVENTS; }
   else
   {
      fputs(MSG_ER_CMD_NOT_FOUND, stdout);
      return CMD_NONE;
   }
   return cmd_type;
}


//! @brief Cleans the command line on the display
//!
void ushell_clean_cmd_line( void )
{
   // Clean command line display
   while( 0 != g_u8_cmd_size )
   {
      // Remove the last character on cmd line buffer
      putchar_special(ASCII_BKSPACE); // Send a backspace to go in previous character
      putchar_special(' ');           // Send a space to erase previous character
      putchar_special(ASCII_BKSPACE); // Send a backspace to go in new end position (=previous character position)
      g_u8_cmd_size--;
   }
}


//! @brief Selects the previous command in history list
//!
void ushell_history_up( void )
{
   if( g_u8_history_pos_search == 0 )
   {
      if( (USHELL_HISTORY-1) == g_u8_history_pos )
         return;  // End of history list
      g_u8_history_pos_search = USHELL_HISTORY-1;
   }else{
      if( (g_u8_history_pos_search-1) == g_u8_history_pos )
         return;  // End of history list
      g_u8_history_pos_search--;
   }
   if( 0 == g_s_cmd_his[g_u8_history_pos_search][0] )
   {
      // History empty then go to previous selection
      ushell_history_down();
   }
}


//! @brief Selects the next command in history list
//!
void ushell_history_down( void )
{
   if( g_u8_history_pos_search == g_u8_history_pos )
      return;  // End of history list
   if( g_u8_history_pos == 0 )
   {
      if( (USHELL_HISTORY-1) == g_u8_history_pos_search )
         return;  // End of history list
      g_u8_history_pos_search++;
   }else{
      if( (g_u8_history_pos_search+1) == g_u8_history_pos )
         return;  // End of history list
   }
   g_u8_history_pos_search++;
   if( USHELL_HISTORY == g_u8_history_pos_search )
      g_u8_history_pos_search = 0;
}


//! @brief Displays the current history
//!
void ushell_history_display( void )
{
   g_u8_cmd_size=0;
   while( g_s_cmd_his[g_u8_history_pos_search][g_u8_cmd_size] != 0 )
   {
      putchar_special( g_s_cmd_his[g_u8_history_pos_search][g_u8_cmd_size] );
      g_s_cmd_his[g_u8_history_pos][g_u8_cmd_size] = g_s_cmd_his[g_u8_history_pos_search][g_u8_cmd_size];
      g_u8_cmd_size++;
   }
   g_s_cmd_his[g_u8_history_pos][g_u8_cmd_size] = 0;
}


//! @brief This function wait a key press
//!
//! @return TRUE, if the action must be continue
//!
Bool ushell_more_wait( void )
{
   int c_key;
   usart_write_line((&AVR32_USART0), "\n\r-- space for more--");
   c_key=0;
   while( (c_key!='q') && (c_key!=' ') )
   {
      usart_reset_status(SHL_USART);
      while(usart_read_char(SHL_USART, &c_key) != USART_SUCCESS);
   }
   usart_write_line((&AVR32_USART0), "\r                 \r");
   return (c_key==' ');
}


//! @brief This function display all drives present
//!
void ushell_cmd_nb_drive( void )
{
   uint8_t u8_tmp;

   usart_write_line((&AVR32_USART0), "Memory interface available:\r\n");
   for( u8_tmp=0; u8_tmp<nav_drive_nb(); u8_tmp++ )
   {
      // Display drive letter name (a, b...)
	  //printf_special("%c: %s\r\n", 'a'+u8_tmp, mem_name(u8_tmp) );
      sprintf((char*)&pBuffer[0], "%c: %s\r\n", 'a'+u8_tmp, mem_name(u8_tmp)); usart_write_line((&AVR32_USART0), (char*)&pBuffer[0]);
   }
}


//! @brief This function displays the free space of each drive present
//!
void ushell_cmd_free_space( void )
{
   uint8_t u8_tmp;
   Fs_index sav_index = nav_getindex();      // Save current position
   for( u8_tmp=0; u8_tmp<nav_drive_nb(); u8_tmp++ )
   {
      nav_drive_set( u8_tmp );      // Select drive
      if( !nav_partition_mount() )  // Mount drive
         continue;

      // Display drive letter name (a, b...)
      //printf_special("%c: %s\r\n", 'a'+u8_tmp, mem_name(u8_tmp) );
      sprintf((char*)&pBuffer[0], "%c: %s\r\n", 'a'+u8_tmp, mem_name(u8_tmp)); usart_write_line((&AVR32_USART0), (char*)&pBuffer[0]);

      if( g_s_arg[0][0]=='l' )        // Choose command option
      {
         // Long and exact function
         //printf_special("Free space: %llu Bytes / %llu Bytes\n\r", (uint64_t)(nav_partition_freespace() * SECTOR_SIZE_IN_BYTES), (uint64_t)(nav_partition_space() * SECTOR_SIZE_IN_BYTES));
	      sprintf((char*)&pBuffer[0], "Free space: %llu Bytes / %llu Bytes\n\r", (uint64_t)(nav_partition_freespace() * SECTOR_SIZE_IN_BYTES), (uint64_t)(nav_partition_space() * SECTOR_SIZE_IN_BYTES)); usart_write_line((&AVR32_USART0), (char*)&pBuffer[0]);
      }
      else
      {
         // Otherwise use fast command
         //printf_special("Free space: %u %%\n\r", nav_partition_freespace_percent() );
	      sprintf((char*)&pBuffer[0], "Free space: %u %%\n\r", nav_partition_freespace_percent()); usart_write_line((&AVR32_USART0), (char*)&pBuffer[0]);
      }
   }
   nav_gotoindex(&sav_index);       // Restore position
}


//! @brief This function formats a drive
//!
void ushell_cmd_format( void )
{
   if( g_s_arg[0][0] == 0 )
      return;

   // Select drive to format
   nav_drive_set( g_s_arg[0][0]-'a');
   if( !nav_drive_format(FS_FORMAT_DEFAULT) )
   {
      fputs(MSG_ER_FORMAT, stdout);
      return;
   }
}


//! @brief This function mount a drive
//!
void ushell_cmd_mount( void )
{
   uint8_t u8_drive_lun;
   Fs_index sav_index;

   if( g_s_arg[0][0] == 0 )
      return;

   // Compute the logical unit number of drive
   u8_drive_lun=g_s_arg[0][0]-'a';
   // Check lun number
   if( u8_drive_lun >= nav_drive_nb() )
   {
      fputs(MSG_ER_DRIVE, stdout);
      return;
   }

   // Mount drive
   sav_index = nav_getindex();      // Save previous position
   if( nav_drive_set(u8_drive_lun))
   {
      if( nav_partition_mount() )
         return;                    // Here, drive mounted
   }
   fputs(MSG_ER_MOUNT, stdout);
   nav_gotoindex(&sav_index);       // Restore previous position
}


//! @brief This function displays the disk space of current drive
//!
void ushell_cmd_space( void )
{
   uint32_t u32_space;
   // Display drive letter name (a, b...)
   fputs( mem_name(nav_drive_get()), stdout);
   putchar_special(' ');
   putchar_special( nav_drive_get()+'a');
   // Otherwise use fast command
   u32_space = nav_partition_space();
   if( 1024 >(u32_space % (2*1024)) )
   {
      u32_space = u32_space/(2*1024);
   }else{
      u32_space = (u32_space/(2*1024))+1;
   }
   //printf_special(": space: %luMB \n\r", u32_space );
	sprintf((char*)&pBuffer[0], ": space: %luMB \n\r", u32_space); usart_write_line((&AVR32_USART0), (char*)&pBuffer[0]);
}


//! @brief This function manages the ls command
//!
//! @param  b_more  enable the '|more' management when TRUE otherwise no '|more' management
//!
void ushell_cmd_ls( Bool b_more )
{
   uint8_t str_char[MAX_FILE_PATH_LENGTH];
   uint16_t u16_i,u16_nb_file,u16_nb_dir,last_i;
   uint8_t ext_filter=FALSE;

   //** Print drive name
   //printf_special("%c: volume is %s\r\n", 'a'+nav_drive_get(), mem_name(nav_drive_get()) );
	sprintf((char*)&pBuffer[0], "%c: volume is %s\r\n", 'a'+nav_drive_get(), mem_name(nav_drive_get())); usart_write_line((&AVR32_USART0), (char*)&pBuffer[0]);

   usart_write_line((&AVR32_USART0), "Drive uses ");
   switch (nav_partition_type())
   {
      case FS_TYPE_FAT_12:
      usart_write_line((&AVR32_USART0), "FAT12\n\r");
      break;

      case FS_TYPE_FAT_16:
      usart_write_line((&AVR32_USART0), "FAT16\n\r");
      break;

      case FS_TYPE_FAT_32:
      usart_write_line((&AVR32_USART0), "FAT32\n\r");
      break;

      default:
      usart_write_line((&AVR32_USART0), "an unknown partition type\r\n");
      return;
   }

   //** Print directory name
   if( !nav_dir_name( (FS_STRING)str_char, MAX_FILE_PATH_LENGTH ) )
   {
		usart_write_line((&AVR32_USART0), "NAV: Dir name failed\r\n");
	    return;
   }
   //usart_write_line((&AVR32_USART0), "Dir name is %s\n\r",str_char);
	sprintf((char*)&pBuffer[0], "Dir name is %s\n\r",str_char); usart_write_line((&AVR32_USART0), (char*)&pBuffer[0]);

   //** Check extension filter in extra parameters
   if(g_s_arg[0][0]!=0)
   {
      if(g_s_arg[0][0] == '*' && g_s_arg[0][1]=='.')
      {
         ext_filter=TRUE;
         for(u16_i=2; u16_i<USHELL_SIZE_CMD_LINE; u16_i++)
         {
            g_s_arg[0][u16_i-2]=g_s_arg[0][u16_i];
         }
      }
   }

   //** Print files list
   usart_write_line((&AVR32_USART0), "          Size  Name\n\r");
   // Init loop at the beginning of directory
   nav_filelist_reset();
   u16_nb_file=0;
   u16_nb_dir=0;
   last_i=0;
   // For each file in list
   while( nav_filelist_set(0,FS_FIND_NEXT) )
   {
      if(!ext_filter)
      {
         // No extension filter
         if( nav_file_isdir() )
         {
            usart_write_line((&AVR32_USART0), "Dir ");
            u16_nb_dir++;              // count the number of directory
         }else{
            usart_write_line((&AVR32_USART0), "    ");
         }
      }
      else
      {
         // If extension filter then ignore directories
         if(nav_file_isdir())
            continue;
         // Check extension
         if(!nav_file_checkext((FS_STRING)g_s_arg[0]))
            continue;
      }
      u16_nb_file++;                   // count the total of files (directories and files)

      // Check 'more' step
      if( b_more && ((u16_nb_file%USHELL_NB_LINE)==0) && (u16_nb_file!=0) && (last_i != u16_nb_file) )
      {
         last_i=u16_nb_file;
         if( !ushell_more_wait() )
            return;  // Exit LS command
      }

      // Display file
      nav_file_name((FS_STRING)str_char, MAX_FILE_PATH_LENGTH, FS_NAME_GET, TRUE);
      //printf_special("%10lu  %s\n\r", nav_file_lgt(), str_char);
	sprintf((char*)&pBuffer[0], "%10lu  %s\n\r", nav_file_lgt(), str_char); usart_write_line((&AVR32_USART0), (char*)&pBuffer[0]);
   }
   // Display total number
   //printf_special(" %4i Files\r\n", u16_nb_file-u16_nb_dir );
	sprintf((char*)&pBuffer[0], " %4i Files\r\n", u16_nb_file-u16_nb_dir); usart_write_line((&AVR32_USART0), (char*)&pBuffer[0]);
   //printf_special(" %4i Dir\r\n", u16_nb_dir );
	sprintf((char*)&pBuffer[0], " %4i Dir\r\n", u16_nb_dir); usart_write_line((&AVR32_USART0), (char*)&pBuffer[0]);
}


//! @brief This function enter in a directory
//!
void ushell_cmd_cd( void )
{
   if( g_s_arg[0][0] == 0 )
      return;

   // Add '\' at the end of path, else the nav_setcwd select the directory but don't enter into.
   ushell_path_valid_syntac( g_s_arg[0] );

   // Call file system routine
   if( nav_setcwd((FS_STRING)g_s_arg[0],TRUE,FALSE) == FALSE )
   {
      fputs(MSG_ER_UNKNOWN_FILE, stdout);
   }
}


//! @brief This function go back to parent directory
//!
void ushell_cmd_gotoparent( void )
{
   nav_dir_gotoparent();
}


//! @brief Manage cat command
//!
//! @param b_more   enable the '|more' management
//!
//! @todo more management not fully functional with file without CR
//!
void ushell_cmd_cat(Bool b_more)
{
   char c_file_character;
   uint8_t n_line=0;

   if( g_s_arg[0][0] == 0 )
      return;

   // Select file
   if( !nav_setcwd((FS_STRING)g_s_arg[0],TRUE,FALSE) )
   {
      fputs(MSG_ER_UNKNOWN_FILE, stdout);
      return;
   }

   // Open file
   file_open(FOPEN_MODE_R);
   while (file_eof()==FALSE)
   {
      // Check 'b_more' option
      if( b_more && (n_line >= USHELL_NB_LINE))
      {
         n_line = 0;
         if( !ushell_more_wait() )
            break;   // Stop cat command
      }

      // Display a character
      c_file_character = file_getc();
      putchar_special( c_file_character );

      // Count the line number
      if (c_file_character==ASCII_LF)
         n_line++;
   }
   file_close();

   // Jump in a new line
   putchar_special(ASCII_CR);putchar_special(ASCII_LF);
}


//! @brief This function display the help
//!
void ushell_cmd_help( void )
{
   fputs(MSG_HELP, stdout);
}


//! @brief This function create a directory
//!
void ushell_cmd_mkdir( void )
{
   if( g_s_arg[0][0] == 0 )
      return;

   if( !nav_dir_make((FS_STRING)g_s_arg[0]) )
      fputs(MSG_KO, stdout);
}


//! @brief This function create a file
//!
void ushell_cmd_touch( void )
{
   if( g_s_arg[0][0] == 0 )
      return;

   nav_file_create((FS_STRING)g_s_arg[0]);
}


//! @brief This function delete a file or directory
//!
void ushell_cmd_rm( void )
{
   uint8_t u8_i = 0;
   Fs_index sav_index;

   if( g_s_arg[0][0] == 0 )
      return;

   // Save the position
   sav_index = nav_getindex();

   while( 1 )
   {
      // Restore the position
      nav_gotoindex(&sav_index);
      // Select file or directory
      if( !nav_setcwd( (FS_STRING)g_s_arg[0], TRUE, FALSE ) )
         break;
      // Delete file or directory
      if( !nav_file_del( FALSE ) )
      {
         fputs(MSG_KO, stdout);
         break;
      }
      u8_i++;
   }
   //printf_special( "%u file(s) deleted\n\r", u8_i );
	sprintf((char*)&pBuffer[0], "%u file(s) deleted\n\r", u8_i); usart_write_line((&AVR32_USART0), (char*)&pBuffer[0]);
}


//! @brief Minimalist file editor to append char to a file
//!
//! @verbatim
//! hit ^q to exit and save file
//! @endverbatim
//!
void ushell_cmd_append_file( void )
{
   int c_key;

   if( g_s_arg[0][0] == 0 )
      return;

   // Select file or directory
   if( !nav_setcwd( (FS_STRING)g_s_arg[0], TRUE, FALSE ) )
   {
      fputs(MSG_ER_UNKNOWN_FILE, stdout);
      return;
   }
   // Open file
   if( !file_open(FOPEN_MODE_APPEND) )
   {
      fputs(MSG_KO, stdout);
      return;
   }

   // Append file
   fputs(MSG_APPEND_WELCOME, stdout);
   while( 1 )
   {
      usart_reset_status(SHL_USART);
      while(usart_read_char(SHL_USART, &c_key) != USART_SUCCESS);

      if( c_key == ASCII_CTRL_Q )
         break;   // ^q to quit

      putchar_special( c_key );
      file_putc( c_key );
      if( c_key == ASCII_CR )
      {
         putchar_special(ASCII_LF);
         file_putc(ASCII_LF);
      }
   }

   // Close file
   file_close();
   putchar_special(ASCII_CR);putchar_special(ASCII_LF);
}


//! @brief This function copies a file to other location
//!
void ushell_cmd_copy( void )
{
   Fs_index sav_index;
   uint8_t u8_status_copy;

   if( g_s_arg[0][0] == 0 )
      return;

   // Save the position
   sav_index = nav_getindex();

   // Select source file
   if( !nav_setcwd( (FS_STRING)g_s_arg[0], TRUE, FALSE ) )
   {
      fputs(MSG_ER_UNKNOWN_FILE, stdout);
      return;
   }
   // Get name of source to be used as same destination name
   nav_file_name( (FS_STRING)g_s_arg[0], MAX_FILE_PATH_LENGTH, FS_NAME_GET, TRUE );
   // Mark this selected file like source file
   if( !nav_file_copy())
   {
      fputs(MSG_KO, stdout);
      goto cp_end;
   }

   // Select destination
   if( g_s_arg[1][0]==0 )
   {
      // g_s_arg[1] is NULL, using mark
      if( !nav_gotoindex(&g_mark_index) )
         goto cp_end;
   }
   else
   {
      // g_s_arg[1] exists, then go to this destination
      if( !nav_setcwd( (FS_STRING)g_s_arg[1], TRUE, FALSE ) )
      {
         fputs(MSG_ER_UNKNOWN_FILE, stdout);
         goto cp_end;
      }
   }

   // Set the name destination and start paste
   if( !nav_file_paste_start((FS_STRING)g_s_arg[0]) )
   {
      fputs(MSG_ER_PASTE, stdout);
      goto cp_end;
   }

   // Performs copy
   do
   {
      u8_status_copy = nav_file_paste_state( FALSE );
   }while( u8_status_copy == COPY_BUSY );

   // Check status of copy action
   if( u8_status_copy == COPY_FAIL )
   {
      fputs(MSG_ER_PASTE, stdout);
      goto cp_end;
   }

cp_end:
   // Restore the position
   nav_gotoindex(&sav_index);
}


//! @brief This function renames a file or a directory
//!
void ushell_cmd_rename( void )
{
   if( g_s_arg[0][0] == 0 )
      return;
   if( g_s_arg[1][0] == 0 )
      return;

   // Select source file
   if( !nav_setcwd( (FS_STRING)g_s_arg[0], TRUE, FALSE ) )
   {
      fputs(MSG_ER_UNKNOWN_FILE, stdout);
      return;
   }
   // Rename file or directory
   if( !nav_file_rename( (FS_STRING)g_s_arg[1] ) )
   {
      fputs(MSG_KO, stdout);
      return;
   }
}


//! @brief Synchronize a path with an other path
//!
//! @return TRUE if success
//!
Bool ushell_cmd_sync( void )
{
   Fs_index sav_index;
   uint8_t u8_folder_level = 0;
	int c_key;

   if( g_s_arg[0][0] == 0 )
      return FALSE;
   if( g_s_arg[1][0] == 0 )
      return FALSE;
   // Add '\' at the end of path, else the nav_setcwd select the directory but don't enter into.
   ushell_path_valid_syntac( g_s_arg[0] );
   ushell_path_valid_syntac( g_s_arg[1] );

   usart_write_line((&AVR32_USART0), "Synchronize folders:\n\r");
   sav_index = nav_getindex();   // Save the position

   // Select source directory in COPYFILE navigator handle
   nav_select( FS_NAV_ID_COPYFILE );
   sprintf((char*)&pBuffer[0], "Source directory: %s\n\r", g_s_arg[0]);
   usart_write_line((&AVR32_USART0), (char*)&pBuffer[0]);
   if( !nav_setcwd( (FS_STRING)g_s_arg[0], TRUE, FALSE ) )
      goto ushell_cmd_sync_error;
   nav_filelist_reset();

   // Select destination directory in USHELL navigator handle
   nav_select( FS_NAV_ID_USHELL_CMD );
   sprintf((char*)&pBuffer[0], "Destination directory: %s\n\r", g_s_arg[1]);
   usart_write_line((&AVR32_USART0), (char*)&pBuffer[0]);
   if( !nav_setcwd( (FS_STRING)g_s_arg[1], TRUE, TRUE ) )
      goto ushell_cmd_sync_error;
   nav_filelist_reset();

   // loop to scan and create ALL folders and files
   while(1)
   {
      while(1)
      {
		if (usart_test_hit(SHL_USART))
		{
			c_key = (SHL_USART->rhr & AVR32_USART_RHR_RXCHR_MASK) >> AVR32_USART_RHR_RXCHR_OFFSET;

			if ((c_key == 0x03) || (c_key == 0x1B))
			{
				goto ushell_cmd_sync_cancel;
			}
		}

         // Loop to Search files or directories
         // Reselect Source
         nav_select( FS_NAV_ID_COPYFILE );
         if( nav_filelist_set( 0 , FS_FIND_NEXT ) )
            break;   // a next file and directory is found

         // No other dir or file in current dir then go to parent dir on Source and Destination disk
         if( 0 == u8_folder_level )
         {
            // end of update folder
            //********* END OF COPY **************
            goto ushell_cmd_sync_finish;
         }

         usart_write_line((&AVR32_USART0), "Go to parent\n\r");
         // Remark, nav_dir_gotoparent() routine go to in parent dir and select the children dir in list
         u8_folder_level--;
         if( !nav_dir_gotoparent() )
            goto ushell_cmd_sync_error;
         // Select Destination navigator and go to the same dir of Source
         nav_select( FS_NAV_ID_USHELL_CMD );
         if( !nav_dir_gotoparent() )
            goto ushell_cmd_sync_error;
      } // end of while (1)

      if( nav_file_isdir())
      {
         usart_write_line((&AVR32_USART0), "Dir found - create dir: ");
         //** here, a new directory is found and is selected
         // Get name of current selection (= dir name on Source)
         if( !nav_file_name( (FS_STRING)g_s_arg[0], USHELL_SIZE_CMD_LINE, FS_NAME_GET, FALSE ))
            goto ushell_cmd_sync_error;
         // Enter in dir (on Source)
         if( !nav_dir_cd())
            goto ushell_cmd_sync_error;
         u8_folder_level++;
         // Select Destination disk
         nav_select( FS_NAV_ID_USHELL_CMD );
         // Create folder in Destination disk
         usart_write_line((&AVR32_USART0), (char*)g_s_arg[0]);
         usart_write_line((&AVR32_USART0), "\n\r");
         if( !nav_dir_make( (FS_STRING )g_s_arg[0] ))
         {
            if( FS_ERR_FILE_EXIST != fs_g_status )
               goto ushell_cmd_sync_error;
            // here, error the name exist
         }
         // Here the navigator have selected the folder on Destination
         if( !nav_dir_cd())
         {
            if( FS_ERR_NO_DIR == fs_g_status )
            {
               // FYC -> Copy impossible, because a file have the same name of folder
            }
            goto ushell_cmd_sync_error;
         }
         // here, the folder is created and the navigator is entered in this dir
      }
      else
      {
         usart_write_line((&AVR32_USART0), "File found - copy file: ");
         //** here, a new file is found and is selected
         // Get name of current selection (= file name on Source)
         if( !nav_file_name( (FS_STRING)g_s_arg[0], USHELL_SIZE_CMD_LINE, FS_NAME_GET, FALSE ))
            goto ushell_cmd_sync_error;
         usart_write_line((&AVR32_USART0), (char*)g_s_arg[0]);
         usart_write_line((&AVR32_USART0), "\n\r");
         if( !nav_file_copy())
            goto ushell_cmd_sync_error;

         // Paste file in current dir of Destination disk
         nav_select( FS_NAV_ID_USHELL_CMD );
         while( !nav_file_paste_start( (FS_STRING)g_s_arg[0] ) )
         {
            // Error
            if( fs_g_status != FS_ERR_FILE_EXIST )
               goto ushell_cmd_sync_error;
#if 0 /* Original */
            // File exists then deletes this one
            usart_write_line((&AVR32_USART0), "File exists then deletes this one.\n\r");
            if( !nav_file_del( TRUE ) )
               goto ushell_cmd_sync_error;
            // here, retry PASTE
#else
            // File exists so skip it
            usart_write_line((&AVR32_USART0), "File exists - skipping copy of this file.\n\r");
			break;
#endif
         }
#if 1 /* Test */
		if (fs_g_status != FS_ERR_FILE_EXIST)
		{
#endif
			// Set file creation and last access for new copied file
			SetFileDateTimestamp(FS_DATE_CREATION);
			SetFileDateTimestamp(FS_DATE_LAST_WRITE);

         // Copy running
         {
         uint8_t status;
         do{
            status = nav_file_paste_state(FALSE);
         }while( COPY_BUSY == status );

         if( COPY_FINISH != status )
            goto ushell_cmd_sync_error;
#if 1 /* Test */
		 }
#endif
         }
      } // if dir OR file
   } // end of first while(1)

ushell_cmd_sync_error:
   // Restore the position
   nav_select( FS_NAV_ID_USHELL_CMD );
   nav_gotoindex(&sav_index);
   usart_write_line((&AVR32_USART0), "!!!Copy fail\n\r");
   return FALSE;

ushell_cmd_sync_finish:
   // Restore the position
   nav_select( FS_NAV_ID_USHELL_CMD );
   nav_gotoindex(&sav_index);
   usart_write_line((&AVR32_USART0), "End of copy\n\r");
   return TRUE;

ushell_cmd_sync_cancel:
   // Restore the position
   nav_select( FS_NAV_ID_USHELL_CMD );
   nav_gotoindex(&sav_index);
   usart_write_line((&AVR32_USART0), "Command canceled!\n\r");
   return TRUE;
}

//! @brief Synchronize a path with an other path
//!
//! @return TRUE if success
//!
#include "Globals.h"
extern void HandleSystemEvents(void);
Bool ushell_cmd_syncevents(uint8_t requestSource, uint16_t* filesCopied, uint16_t* filesSkipped, uint16_t* filesReplaced, uint16_t* filesDuplicated)
{
   Fs_index sav_index;
   uint8_t u8_folder_level = 0;
	int c_key;
	unsigned int duplicateCount = 0;
	char fileExtension[255];
	char* fileExtensionStartPtr;
	uint8 lastSyncAction;

	*filesCopied = 0;
	*filesSkipped = 0;
	*filesReplaced = 0;
	*filesDuplicated = 0;

#if 1 /* New method to prompt the user what action for a file that already exists */
extern USER_MENU_STRUCT syncFileExistsMenu[];
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	g_syncFileExistsAction = g_unitConfig.usbSyncMode;
#endif

#if 0
   if( g_s_arg[0][0] == 0 )
      return FALSE;
   if( g_s_arg[1][0] == 0 )
      return FALSE;
   // Add '\' at the end of path, else the nav_setcwd select the directory but don't enter into.
   ushell_path_valid_syntac( g_s_arg[0] );
   ushell_path_valid_syntac( g_s_arg[1] );
#else
	if (VIRTUAL_MEM == ENABLE) { strcpy(g_s_arg[0], "b:\\Events\\"); }
	else { strcpy(g_s_arg[0], "a:\\Events\\"); }

	if ((g_factorySetupRecord.invalid) || (g_factorySetupRecord.unitSerialNumber[0] == '\0')
		|| ((g_factorySetupRecord.unitSerialNumber[0] == ' ') && (g_factorySetupRecord.unitSerialNumber[1] == '\0')))
	{
		if (VIRTUAL_MEM == ENABLE) { strcpy(g_s_arg[1], "c:\\NS8100-No-Serial-Events\\"); }
		else { strcpy(g_s_arg[1], "b:\\NS8100-No-Serial-Events\\"); }
	}
	else // Serial number is valid
	{
		if (VIRTUAL_MEM == ENABLE) { sprintf(g_s_arg[1], "%s%s%s", "c:\\NS8100-", g_factorySetupRecord.unitSerialNumber, "-Events\\"); }
		else { sprintf(g_s_arg[1], "%s%s%s", "b:\\NS8100-", g_factorySetupRecord.unitSerialNumber, "-Events\\"); }
	}
#endif

   usart_write_line((&AVR32_USART0), "Synchronize folders:\n\r");
   sav_index = nav_getindex();   // Save the position

   // Select source directory in COPYFILE navigator handle
   nav_select( FS_NAV_ID_COPYFILE );
   sprintf((char*)&pBuffer[0], "Source directory: %s\n\r", g_s_arg[0]);
   usart_write_line((&AVR32_USART0), (char*)&pBuffer[0]);
   if( !nav_setcwd( (FS_STRING)g_s_arg[0], TRUE, FALSE ) )
      goto ushell_cmd_syncevents_error;
   nav_filelist_reset();

   // Select destination directory in USHELL navigator handle
   nav_select( FS_NAV_ID_USHELL_CMD );
   sprintf((char*)&pBuffer[0], "Destination directory: %s\n\r", g_s_arg[1]);
   usart_write_line((&AVR32_USART0), (char*)&pBuffer[0]);
#if 1 /* Normal */
   if( !nav_setcwd( (FS_STRING)g_s_arg[1], TRUE, TRUE ) )
      goto ushell_cmd_syncevents_error;
#else /* Test */
	// Set CWD to directory
   if( !nav_setcwd( (FS_STRING)g_s_arg[1], TRUE, FALSE ) )
   {
	   // Directory not found, try to create
	   if( !nav_setcwd( (FS_STRING)g_s_arg[1], TRUE, FALSE ) )
	   {
			goto ushell_cmd_syncevents_error;
	   }
	   else // Created, now add creation time
	   {
			SetFileDateTimestamp(FS_DATE_CREATION);
	   }
   }
#endif
   nav_filelist_reset();

   // loop to scan and create ALL folders and files
   while(1)
   {
      while(1)
      {
#if 0
		if (usart_read_char(SHL_USART, &c_key) == USART_SUCCESS)
		{
			if ((c_key == 0x03) || (c_key == 0x1B))
			{
				goto ushell_cmd_syncevents_cancel;
			}
		}
#else
		if (usart_test_hit(SHL_USART))
		{
			c_key = (SHL_USART->rhr & AVR32_USART_RHR_RXCHR_MASK) >> AVR32_USART_RHR_RXCHR_OFFSET;

			if ((c_key == 0x03) || (c_key == 0x1B))
			{
				goto ushell_cmd_syncevents_cancel;
			}
		}
#endif
         // Loop to Search files or directories
         // Reselect Source
         nav_select( FS_NAV_ID_COPYFILE );
         if( nav_filelist_set( 0 , FS_FIND_NEXT ) )
            break;   // a next file and directory is found

         // No other dir or file in current dir then go to parent dir on Source and Destination disk
         if( 0 == u8_folder_level )
         {
            // end of update folder
            //********* END OF COPY **************
            goto ushell_cmd_syncevents_finish;
         }

         usart_write_line((&AVR32_USART0), "Go to parent\n\r");
         // Remark, nav_dir_gotoparent() routine go to in parent dir and select the children dir in list
         u8_folder_level--;
         if( !nav_dir_gotoparent() )
            goto ushell_cmd_syncevents_error;
         // Select Destination navigator and go to the same dir of Source
         nav_select( FS_NAV_ID_USHELL_CMD );
         if( !nav_dir_gotoparent() )
            goto ushell_cmd_syncevents_error;
      } // end of while (1)

      if( nav_file_isdir())
      {
         usart_write_line((&AVR32_USART0), "Dir found - create dir: ");
         //** here, a new directory is found and is selected
         // Get name of current selection (= dir name on Source)
         if( !nav_file_name( (FS_STRING)g_s_arg[0], USHELL_SIZE_CMD_LINE, FS_NAME_GET, FALSE ))
            goto ushell_cmd_syncevents_error;
         // Enter in dir (on Source)
         if( !nav_dir_cd())
            goto ushell_cmd_syncevents_error;
         u8_folder_level++;
         // Select Destination disk
         nav_select( FS_NAV_ID_USHELL_CMD );
         // Create folder in Destination disk
         usart_write_line((&AVR32_USART0), (char*)g_s_arg[0]);
         usart_write_line((&AVR32_USART0), "\n\r");
         if( !nav_dir_make( (FS_STRING )g_s_arg[0] ))
         {
            if( FS_ERR_FILE_EXIST != fs_g_status )
               goto ushell_cmd_syncevents_error;
            // here, error the name exist
         }
         // Here the navigator have selected the folder on Destination
         if( !nav_dir_cd())
         {
            if( FS_ERR_NO_DIR == fs_g_status )
            {
               // FYC -> Copy impossible, because a file have the same name of folder
            }
            goto ushell_cmd_syncevents_error;
         }
         // here, the folder is created and the navigator is entered in this dir
      }
      else
      {
         usart_write_line((&AVR32_USART0), "File found - copy file: ");
         //** here, a new file is found and is selected
         // Get name of current selection (= file name on Source)
         if( !nav_file_name( (FS_STRING)g_s_arg[0], USHELL_SIZE_CMD_LINE, FS_NAME_GET, FALSE ))
            goto ushell_cmd_syncevents_error;
         usart_write_line((&AVR32_USART0), (char*)g_s_arg[0]);
         usart_write_line((&AVR32_USART0), "\n\r");
         if( !nav_file_copy())
            goto ushell_cmd_syncevents_error;

		duplicateCount = 0;

		memset(g_spareBuffer, 0, MAX_FILE_NAME_CHARS);
		sprintf((char*)g_spareBuffer, "%s: %s %s", getLangText(FILE_TEXT), (char*)g_s_arg[0], getLangText(SYNCING_TEXT));
		OverlayMessage(getLangText(SYNC_PROGRESS_TEXT), (char*)g_spareBuffer, 0);

		//=========================================================================
		// Start copy operation
		//-------------------------------------------------------------------------
         // Paste file in current dir of Destination disk
         nav_select(FS_NAV_ID_USHELL_CMD);
         while (!nav_file_paste_start((FS_STRING)g_s_arg[0]))
         {
            // Check if the error code if anything but the file exists
            if (fs_g_status != FS_ERR_FILE_EXIST)
			{
				goto ushell_cmd_syncevents_error;
			}

			//=========================================================================
			// Destination file already exists
			//-------------------------------------------------------------------------
            usart_write_line((&AVR32_USART0), "File exists - Add prompt for action\n\r");
			
			if (requestSource == USB_SYNC_FROM_SHELL)
			{
				g_syncFileExistsAction = SKIP_ALL_OPTION;
			}

			//=========================================================================
			// New method to prompt the user what action for a file that already exists
			//-------------------------------------------------------------------------
			// Offer options the first time when the file already exists on the destination (skip, replace, duplicate, skip all, replace all, duplicate all)
			if ((g_syncFileExistsAction < SKIP_ALL_OPTION) && (!duplicateCount))
			{
				lastSyncAction = g_syncFileExistsAction;
				g_syncFileExistsAction = 0;

				memset(g_menuTags[FILENAME_TAG].text, 0, MENU_TAGS_MAX_CHARS);
				strncpy(g_menuTags[FILENAME_TAG].text, (char*)g_s_arg[0], (MENU_TAGS_MAX_CHARS - 1));

				SoftUsecWait(750 * SOFT_MSECS);

				SETUP_USER_MENU_MSG(&syncFileExistsMenu, lastSyncAction);
				JUMP_TO_ACTIVE_MENU();

				// Wait for user input, need key input
				while (g_syncFileExistsAction == 0)
				{
					HandleSystemEvents();
				}
			}

			//=========================================================================
			// Skip option
			//-------------------------------------------------------------------------
			if ((g_syncFileExistsAction == SKIP_OPTION) || (g_syncFileExistsAction == SKIP_ALL_OPTION))
			{
				if (g_syncFileExistsAction == SKIP_OPTION)
				{
					memset(g_spareBuffer, 0, MAX_FILE_NAME_CHARS);
					sprintf((char*)g_spareBuffer, "%s: %s %s, %s", getLangText(FILE_TEXT), (char*)g_s_arg[0], getLangText(EXISTS_TEXT), getLangText(SKIPPED_TEXT));
					OverlayMessage(getLangText(SYNC_PROGRESS_TEXT), (char*)g_spareBuffer, (1 * SOFT_SECS));
				}

				*filesSkipped += 1;
				break;
			}
			//=========================================================================
			// Replace option
			//-------------------------------------------------------------------------
			else if ((g_syncFileExistsAction == REPLACE_OPTION) || (g_syncFileExistsAction == REPLACE_ALL_OPTION))
			{
				memset(g_spareBuffer, 0, MAX_FILE_NAME_CHARS);
				sprintf((char*)g_spareBuffer, "%s: %s %s", getLangText(FILE_TEXT), (char*)g_s_arg[0], getLangText(REPLACED_TEXT));
				OverlayMessage(getLangText(SYNC_PROGRESS_TEXT), (char*)g_spareBuffer, 0);

				if (!nav_file_del(TRUE))
				{
					goto ushell_cmd_syncevents_error;
				}

				*filesReplaced += 1;
			}
			//=========================================================================
			// Duplicate option
			//-------------------------------------------------------------------------
			else if ((g_syncFileExistsAction == DUPLICATE_OPTION) || (g_syncFileExistsAction == DUPLICATE_ALL_OPTION))
			{
				// Check if this is the first duplicate match
				if (!duplicateCount)
				{
					memset(g_spareBuffer, 0, MAX_FILE_NAME_CHARS);
					sprintf((char*)g_spareBuffer, "%s: %s %s", getLangText(FILE_TEXT), (char*)g_s_arg[0], getLangText(DUPLICATED_TEXT));
					OverlayMessage(getLangText(SYNC_PROGRESS_TEXT), (char*)g_spareBuffer, 0);

					memset(fileExtension, 0, sizeof(fileExtension));
					memset(g_spareFileName, 0, sizeof(g_spareFileName));

					fileExtensionStartPtr = strstr(g_s_arg[0], ".");

					// Check if the '.' separator was found
					if (fileExtensionStartPtr)
					{
						// Skip past '.' separator
						sscanf((fileExtensionStartPtr + 1), "%s", fileExtension);

						// Terminate the string at the '.' separator
						*fileExtensionStartPtr = '\0';
					}

					// Copy the file name (either up to the '.' separator or the entire string if no separator was found)
					strcpy(g_spareFileName, (char*)g_s_arg[0]);

					*filesDuplicated += 1;
				}

				if (strlen(fileExtension))
				{
					sprintf((char*)g_s_arg[0], "%s (%d).%s", g_spareFileName, ++duplicateCount, fileExtension);
				}
				else
				{
					sprintf((char*)g_s_arg[0], "%s (%d)", g_spareFileName, ++duplicateCount);
				}
			}

#if 0 /* Exception testing (Prevent non-ISR soft loop watchdog from triggering) */
			g_execCycles++;
#endif
         }

		if (fs_g_status != FS_ERR_FILE_EXIST)
		{
			// Set file creation and last access for new copied file
			SetFileDateTimestamp(FS_DATE_CREATION);
			SetFileDateTimestamp(FS_DATE_LAST_WRITE);

			// Copy running
			{
				uint8_t status;

				do
				{
					status = nav_file_paste_state(FALSE);
				} while (COPY_BUSY == status);

				if (COPY_FINISH != status)
				{
					goto ushell_cmd_syncevents_error;
				}

#if 0 /* Exception testing (Prevent non-ISR soft loop watchdog from triggering) */
				g_execCycles++;
#endif
				*filesCopied += 1;
			}
         }
      } // if dir OR file
   } // end of first while(1)

ushell_cmd_syncevents_error:
   // Restore the position
   nav_select( FS_NAV_ID_USHELL_CMD );
   nav_gotoindex(&sav_index);
   usart_write_line((&AVR32_USART0), "!!!Copy fail\n\r");
   return FALSE;

ushell_cmd_syncevents_finish:
   // Restore the position
   nav_select( FS_NAV_ID_USHELL_CMD );
#if 0 /* Add last access timestamp */
	SetFileDateTimestamp(FS_DATE_LAST_WRITE);
#endif
   nav_gotoindex(&sav_index);
   sprintf((char*)&pBuffer[0], "Events copied: %d\r\nEnd of copy\n\r", *filesCopied);
   usart_write_line((&AVR32_USART0), pBuffer);
   return TRUE;

ushell_cmd_syncevents_cancel:
   // Restore the position
   nav_select( FS_NAV_ID_USHELL_CMD );
   nav_gotoindex(&sav_index);
   usart_write_line((&AVR32_USART0), "Command canceled!\n\r");
   return TRUE;
}

// File alloc space (unit sector 512B)
#define  FILE_ALLOC_SIZE      ((1024*1024L)/512L)      // 1MB

Fs_file_segment ushell_cmd_perform_alloc( uint8_t lun, uint16_t size_alloc )
{
   const FS_STRING file_tmp_name = "tmp.bin";
   Fs_file_segment g_recorder_seg;
   g_recorder_seg.u16_size = 0;

   if( !nav_drive_set(lun))
      return g_recorder_seg;

   if( !nav_partition_mount() )
      return g_recorder_seg;

   if( !nav_file_create((FS_STRING)file_tmp_name))
   {
      if( FS_ERR_FILE_EXIST != fs_g_status)
         return g_recorder_seg;
      nav_file_del(FALSE);
      if( !nav_file_create((FS_STRING)file_tmp_name))
         return g_recorder_seg;
   }
   // Open file
   if( !file_open(FOPEN_MODE_W) )
   {
      nav_file_del(FALSE);
      return g_recorder_seg;
   }
   // Define the size of segment to alloc (unit 512B)
   // Note: you can alloc more in case of you don't know total size
   g_recorder_seg.u16_size = size_alloc;
   // Alloc in FAT a cluster list equal or inferior at segment size
   if( !file_write( &g_recorder_seg ))
   {
      g_recorder_seg.u16_size = 0;
      file_close();
      nav_file_del(FALSE);
   }
   return g_recorder_seg;   //** File open and FAT allocated
}

void ushell_cmd_perform_transfer( Fs_file_segment seg_src, Fs_file_segment seg_dest )
{
   uint8_t id_trans_memtomem;
   Ctrl_status status_stream;
   uint16_t u16_i, u16_trans_max;
   uint32_t u32_tmp, u32_time;

   u16_trans_max = ( seg_src.u16_size < seg_dest.u16_size )?  seg_src.u16_size : seg_dest.u16_size;
   for( u16_i=2; u16_i<=u16_trans_max; u16_i*=10 )
   {
      u32_time = Get_sys_count();
	  id_trans_memtomem = stream_mem_to_mem( seg_src.u8_lun , seg_src.u32_addr , seg_dest.u8_lun , seg_dest.u32_addr , u16_i );
      if( ID_STREAM_ERR == id_trans_memtomem )
      {
         usart_write_line((&AVR32_USART0), "Transfer error\r\n");
         return;
      }
      while(1)
      {
         status_stream = stream_state( id_trans_memtomem );
         if( CTRL_BUSY == status_stream ) continue;
         if( CTRL_GOOD == status_stream ) break;
         if( CTRL_FAIL == status_stream ) {
            usart_write_line((&AVR32_USART0), "Transfer error\r\n");
            return;
         }
      }
      u32_time = cpu_cy_2_us(Get_sys_count()-u32_time, g_u32_ushell_pba_hz );
      u32_tmp = ((uint32_t)u16_i*(1000000/2))/u32_time;
      //printf_special( "Transfer rate %4luKB/s - stream size %4iKB\r\n", u32_tmp, u16_i/2 );
	sprintf((char*)&pBuffer[0], "Transfer rate %4luKB/s - stream size %4iKB\r\n", u32_tmp, u16_i/2); usart_write_line((&AVR32_USART0), (char*)&pBuffer[0]);

      if( (8000000) < u32_time )
      {
         // The test time must be inferior at 8s
         break;
      }
   }
}

void ushell_cmd_perform_access( Bool b_sens_write, Fs_file_segment seg )
{
   uint16_t u16_trans;
   uint32_t u32_tmp, u32_time;

   fat_cache_flush();
   fat_cache_reset();
   u32_time = Get_sys_count();
   for( u16_trans=0; u16_trans<seg.u16_size; u16_trans++ )
   {
      if( b_sens_write )
      {
         if( CTRL_GOOD != ram_2_memory( seg.u8_lun , seg.u32_addr , fs_g_sector )) {
            usart_write_line((&AVR32_USART0), "Transfer error\r\n");
            return;
         }
      }else{
         if( CTRL_GOOD != memory_2_ram( seg.u8_lun , seg.u32_addr , fs_g_sector )) {
            usart_write_line((&AVR32_USART0), "Transfer error\r\n");
            return;
         }
      }
      seg.u32_addr++;
      if( 8000000 < cpu_cy_2_us(Get_sys_count()-u32_time, g_u32_ushell_pba_hz) )
      {
         // Stop access after 8s
         break;
      }
   }
   u32_time = cpu_cy_2_us(Get_sys_count()-u32_time, g_u32_ushell_pba_hz);
   u32_tmp = ((uint32_t)u16_trans*(1000000/2))/u32_time;
   if( b_sens_write )
      { 
		  //printf_special( "Transfer rate - WRITE %4luKB/s\r\n", u32_tmp ); 
		sprintf((char*)&pBuffer[0], "Transfer rate - WRITE %4luKB/s\r\n", u32_tmp); usart_write_line((&AVR32_USART0), (char*)&pBuffer[0]);
	}
   else
      { 
		  //printf_special( "Transfer rate - READ %4luKB/s\r\n", u32_tmp );
		sprintf((char*)&pBuffer[0], "Transfer rate - READ %4luKB/s\r\n", u32_tmp); usart_write_line((&AVR32_USART0), (char*)&pBuffer[0]);
	}
}

#define SIZE_OF_EXT_BUFFER 8
static   uint8_t  u8_ext_buffer[512*SIZE_OF_EXT_BUFFER];

void ushell_cmd_perform_extaccess( Bool b_sens_write, Fs_file_segment seg )
{
   uint16_t u16_trans;
   uint32_t u32_tmp, u32_time;
   uint8_t  u8_nb_trans_usb=0;

   fat_cache_flush();
   fat_cache_reset();
   u32_time = Get_sys_count();
   u16_trans=0;
   while( seg.u16_size!=0 )
   {
      if( 0 == (seg.u32_addr % SIZE_OF_EXT_BUFFER) )
      {
         u8_nb_trans_usb = SIZE_OF_EXT_BUFFER;
      }else{
         u8_nb_trans_usb = SIZE_OF_EXT_BUFFER - (seg.u32_addr % SIZE_OF_EXT_BUFFER);  // to align access with usual memory mapping
      }
      if (u8_nb_trans_usb > seg.u16_size)
        u8_nb_trans_usb = seg.u16_size;

      if( b_sens_write )
      {
         if( CTRL_GOOD != host_write_10_extram( seg.u32_addr , u8_ext_buffer, u8_nb_trans_usb )) {
            usart_write_line((&AVR32_USART0), "Transfer error\r\n");
            return;
         }
      }else{
         if( CTRL_GOOD != host_read_10_extram( seg.u32_addr , u8_ext_buffer, u8_nb_trans_usb )) {
            usart_write_line((&AVR32_USART0), "Transfer error\r\n");
            return;
         }
      }
      seg.u16_size -= u8_nb_trans_usb;
      u16_trans    += u8_nb_trans_usb;
      seg.u32_addr += u8_nb_trans_usb;
      if( 8000000 < cpu_cy_2_us(Get_sys_count()-u32_time, g_u32_ushell_pba_hz) )
      {
         // Stop access after 8s
         break;
      }
   }
   u32_time = cpu_cy_2_us(Get_sys_count()-u32_time, g_u32_ushell_pba_hz);
   u32_tmp = ((uint32_t)u16_trans*(1000000/2))/u32_time;
   if( b_sens_write )
      { 
		  //printf_special( "Transfer rate - WRITE %4luKB/s\r\n", u32_tmp);
		sprintf((char*)&pBuffer[0], "Transfer rate - WRITE %4luKB/s\r\n", u32_tmp); usart_write_line((&AVR32_USART0), (char*)&pBuffer[0]);
	}
   else
      { 
		  //printf_special( "Transfer rate - READ %4luKB/s\r\n", u32_tmp );
		sprintf((char*)&pBuffer[0], "Transfer rate - READ %4luKB/s\r\n", u32_tmp); usart_write_line((&AVR32_USART0), (char*)&pBuffer[0]);
	}
}


//! @brief Perform transfer between two devices
//!
void ushell_cmd_perform( void )
{
   Fs_index sav_index;
   Fs_file_segment seg1, seg2;

   if( g_s_arg[0][0] == 0 )
      return;

   sav_index = nav_getindex();   // Save the position

   // Alloc a file on each devices
   usart_write_line((&AVR32_USART0), "Alloc a file on each devices\n\r");
   seg1 = ushell_cmd_perform_alloc( (g_s_arg[0][0]-'a') , FILE_ALLOC_SIZE );
   if( seg1.u16_size == 0 )
   {
      usart_write_line((&AVR32_USART0), "!!!Error allocation on device 1\n\r");
      // Restore the position
      nav_gotoindex(&sav_index);
      return;
   }
   if( g_s_arg[1][0] != 0 )
   {
      nav_select( FS_NAV_ID_COPYFILE );
      seg2 = ushell_cmd_perform_alloc( (g_s_arg[1][0]-'a') , FILE_ALLOC_SIZE );
      if( seg2.u16_size == 0 )
      {
         nav_select( FS_NAV_ID_USHELL_CMD );
         file_close();
         nav_file_del(FALSE);
         usart_write_line((&AVR32_USART0), "!!!Error allocation on device 2\n\r");
         // Restore the position
         nav_gotoindex(&sav_index);
         return;
      }

      // Transfer data from device 1 to device 2
      usart_write_line((&AVR32_USART0), "Transfer data from device 1 to device 2\r\n");
      ushell_cmd_perform_transfer(seg1,seg2);
      usart_write_line((&AVR32_USART0), "Transfer data from device 2 to device 1\r\n");
      ushell_cmd_perform_transfer(seg2,seg1);
      // Delete files allocated
      nav_select( FS_NAV_ID_COPYFILE );
      file_close();
      nav_file_del(FALSE);
      nav_select( FS_NAV_ID_USHELL_CMD );
   }
   else
   {
      ushell_cmd_perform_access( FALSE, seg1 );
      ushell_cmd_perform_access( TRUE, seg1 );
      if( LUN_ID_USB <= nav_drive_get() )
      {
         usart_write_line((&AVR32_USART0), "Transfer large buffer on USB\r\n");
         ushell_cmd_perform_extaccess( FALSE, seg1 );
         ushell_cmd_perform_extaccess( TRUE, seg1 );
      }

   }

   file_close();
   nav_file_del(FALSE);
   // Restore the position
   nav_gotoindex(&sav_index);
   usart_write_line((&AVR32_USART0), "End of test\n\r");
   return;
}




//! @brief Appends the '\' char at the end of path
//!
void ushell_path_valid_syntac( char *path )
{
   uint8_t u8_tmp;

   // Compute size of substitute
   for( u8_tmp=0; u8_tmp<MAX_FILE_PATH_LENGTH; u8_tmp++ )
   {
      if( path[u8_tmp]==0)
         break;
   }
   // Append the '\' char for the nav_setcwd to enter the found directory
   if ( path[u8_tmp-1] != '\\')
   {
      path[u8_tmp]='\\';
      path[u8_tmp+1]=0;
   }
}


#if USB_HOST_FEATURE == TRUE
//! @brief In host mode, display basic low level information about the device connected
//!
//! @note The device should be supported by the host (configured)
//!
void ushell_cmdusb_ls(void)
{
   uint8_t i,j;

   // Check USB host status
   if( (!Is_host_ready()) && (!Is_host_suspended()) )
   {
      fputs(MSG_NO_DEVICE, stdout);
      return;
   }
   if( Is_host_suspended() )
   {
      fputs(MSG_USB_SUSPENDED, stdout);
   }

   //printf_special("VID:%04X, PID:%04X, ",Get_VID(),Get_PID());
	sprintf((char*)&pBuffer[0], "VID:%04X, PID:%04X, ",Get_VID(),Get_PID()); usart_write_line((&AVR32_USART0), (char*)&pBuffer[0]);
   //printf_special("MaxPower is %imA, ",2*Get_maxpower());
	sprintf((char*)&pBuffer[0], "MaxPower is %imA, ",2*Get_maxpower()); usart_write_line((&AVR32_USART0), (char*)&pBuffer[0]);

   if (Is_device_self_powered())
   {  fputs(MSG_SELF_POWERED, stdout);}
   else
   {  fputs(MSG_BUS_POWERED, stdout); }
   if (Is_usb_full_speed_mode())
   {  fputs(MSG_DEVICE_FULL_SPEED, stdout);}
   else
#if (USB_HIGH_SPEED_SUPPORT==FALSE)
   {  fputs(MSG_DEVICE_LOW_SPEED, stdout); }
#else
   {  fputs(MSG_DEVICE_HIGH_SPEED, stdout); }
#endif
   if (Is_device_supports_remote_wakeup())
   {  fputs(MSG_REMOTE_WAKEUP_OK, stdout);}
   else
   {  fputs(MSG_REMOTE_WAKEUP_KO, stdout); }
   //printf_special("Supported interface(s):%02i\n\r",Get_nb_supported_interface());
	sprintf((char*)&pBuffer[0], "Supported interface(s):%02i\n\r",Get_nb_supported_interface()); usart_write_line((&AVR32_USART0), (char*)&pBuffer[0]);

   for(i=0;i<Get_nb_supported_interface();i++)
   {
      //printf_special("Interface nb:%02i, AltS nb:%02i, Class:%02i, SubClass:%02i, Protocol:%02i\n\r", Get_interface_number(i), Get_altset_nb(i), Get_class(i), Get_subclass(i), Get_protocol(i));
	sprintf((char*)&pBuffer[0], "Interface nb:%02i, AltS nb:%02i, Class:%02i, SubClass:%02i, Protocol:%02i\n\r", Get_interface_number(i), Get_altset_nb(i), Get_class(i), Get_subclass(i), Get_protocol(i)); usart_write_line((&AVR32_USART0), (char*)&pBuffer[0]);

      usart_write_line((&AVR32_USART0), " Endpoint(s) Addr:");
      if(Get_nb_ep(i))
      {
         for(j=0;j<Get_nb_ep(i);j++)
         {
            //printf_special(" %02lX", Get_ep_nbr(i,j));
			sprintf((char*)&pBuffer[0], " %02lX", Get_ep_nbr(i,j)); usart_write_line((&AVR32_USART0), (char*)&pBuffer[0]);
         }
      }
      else
      {
         usart_write_line((&AVR32_USART0), "None");
      }
      putchar_special(ASCII_CR);putchar_special(ASCII_LF);
   }
}

//! @brief In host mode, set host in suspend mode
//!
void ushell_cmdusb_suspend(void)
{
   if( !Is_host_ready() )
   {
      fputs(MSG_NO_DEVICE, stdout);
   }
   Host_request_suspend();
}

//! @brief In host mode, resume host from suspend mode
//!
void ushell_cmdusb_resume(void)
{
   if( !Is_host_suspended() )
   {
      fputs(MSG_NO_DEVICE, stdout);
   }
   Host_request_resume();
}

#endif  // USB_HOST_FEATURE == TRUE
#endif