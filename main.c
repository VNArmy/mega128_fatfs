/*----------------------------------------------------------------------*/
/* Foolproof FatFs sample project for AVR              (C)ChaN, 2014    */
/*----------------------------------------------------------------------*/
#include <stdlib.h>
#include <avr/pgmspace.h>
#include <string.h>
#include <avr/io.h>	/* Device specific declarations */
#include <util/delay.h>
#include "ff.h"		/* Declarations of FatFs API */
#include "uart.h"
#include <avr/interrupt.h>
#include <stdio.h>

/* Define CPU frequency in Hz in Makefile or toolchain compiler configuration */
#ifndef F_CPU
#error "F_CPU undefined, please define CPU frequency in Hz in Makefile or compiler configuration"
#endif

/* Define UART baud rate here */
#define UART_BAUD_RATE 9600

FATFS FatFs;		/* FatFs work area needed for each volume */
FIL Fil;			/* File object needed for each open file */

char buff[256];

#define TRUE 1
#define FALSE 0
#define CHAR_NEWLINE '\n'
#define CHAR_RETURN '\r'
#define RETURN_NEWLINE "\r\n"

unsigned char data_count = 0;
unsigned char data_in[25];
char command_in[25];
int variable_A = 23; //user modifiable variable
int variable_goto = 12; //user modifiable variable
char file_name[25];

UINT bw;
FRESULT fr;

void copy_command(void);
void process_command(void);
void print_value (char *id, int value);
void uart_ok(void);
int parse_assignment (char input[16]);
void process_uart(void);
void parse_file_name (char input[16], char* file_name);
void print_file_name (char *id, char* value);
void create_file(char* fname);
void test_file(char* fname);

int parse_assignment (char input[16]) {
  char *pch;
  char cmdValue[16];
  // Find the position the equals sign is
  // in the string, keep a pointer to it
  pch = strchr(input, '=');
  // Copy everything after that point into
  // the buffer variable
  strcpy(cmdValue, pch+1);
  // Now turn this value into an integer and
  // return it to the caller.
  return atoi(cmdValue);
}

void parse_file_name (char input[25], char* file_name) {
  char *pch;
  // Find the position the equals sign is
  // in the string, keep a pointer to it
  pch = strchr(input, '=');
  uart_puts((char*)(pch+1));
  // Copy everything after that point into
  // the buffer variable
  strcpy(file_name, pch+1);
  // Now turn this value into an integer and
  // return it to the caller.
  return ;
}

void copy_command () {
  // Copy the contents of data_in into command_in
  memcpy(command_in, data_in, 25);
  // Now clear data_in, the UART can reuse it now
  memset(data_in, 0, 25);
}

void process_command() {
	uart_puts("Process command\r\n");
	if(strcasestr(command_in,"FN") != NULL){
		if(strcasestr(command_in,"?") != NULL)
			print_file_name("FN", file_name);
		else{
			uart_puts("PARSE File\r\n");
			parse_file_name(command_in, file_name);
			uart_puts(".. DONE\r\n");
		}
	}
	else if(strcasestr(command_in,"WR") != NULL){
		create_file(file_name);
	}
	else if(strcasestr(command_in,"TST") != NULL){
			test_file(file_name);
		}
	else if(strcasestr(command_in,"RD") != NULL){
				test_file(file_name);
			}
}

void print_value (char *id, int value) {
  char buffer[25];
  itoa(value, buffer, 10);
  uart_puts(id);
  uart_putc('=');
  uart_puts(buffer);
  uart_puts(RETURN_NEWLINE);
}

void print_file_name (char *id, char* value) {

  uart_puts(id);
  uart_putc('=');
  uart_puts(value);
  uart_puts(RETURN_NEWLINE);
}

void uart_ok() {
  uart_puts("OK");
  uart_puts(RETURN_NEWLINE);
}

void process_uart(){
  /* Get received character from ringbuffer
   * uart_getc() returns in the lower byte the received character and
   * in the higher byte (bitmask) the last receive error
   * UART_NO_DATA is returned when no data is available.   */
  unsigned int c = uart_getc();

  if ( c & UART_NO_DATA ){
    // no data available from UART
  }
  else {
    // new data available from UART check for Frame or Overrun error
    if ( c & UART_FRAME_ERROR ) {
      /* Framing Error detected, i.e no stop bit detected */
      uart_puts_P("UART Frame Error: ");
    }
    if ( c & UART_OVERRUN_ERROR ) {
      /* Overrun, a character already present in the UART UDR register was
       * not read by the interrupt handler before the next character arrived,
       * one or more received characters have been dropped */
      uart_puts_P("UART Overrun Error: ");
    }
    if ( c & UART_BUFFER_OVERFLOW ) {
      /* We are not reading the receive buffer fast enough,
       * one or more received character have been dropped  */
      uart_puts_P("Buffer overflow error: ");
    }

    // Add char to input buffer
    data_in[data_count] = c;

    // Return is signal for end of command input
    if (data_in[data_count] == CHAR_RETURN) {
      // Reset to 0, ready to go again
      data_count = 0;
      uart_puts(RETURN_NEWLINE);

      copy_command();
      process_command();
      uart_ok();
    }
    else {
      data_count++;
    }

    uart_putc( (unsigned char)c );
  }
}

void create_file(char* fname){
	fr = f_open(&Fil, fname, FA_WRITE | FA_CREATE_ALWAYS);	/* Create a file */
	if (fr == FR_OK)
	{
		uart_puts("CREATED!\r\n");
		fr = f_write(&Fil, "It works!\r\n", 11, &bw);	/* Write data to the file */
		sprintf(buff,"RESULT = %d\r\n",(uint8_t)fr);
		uart_puts(buff);
		fr = f_close(&Fil);
		sprintf(buff,"CLOSE_RESULT = %d\r\n",(uint8_t)fr);
		uart_puts(buff);
		/* Close the file */
		if (fr == FR_OK && bw == 11) {		/* Lights green LED if data written well */
			//			DDRB |= 0x10; PORTB |= 0x10;	/* Set PB4 high */
		}


	}
	else{
		sprintf(buff,"An error occured. (%d)\n", fr);
				uart_puts(buff);
	}
}
void test_file(char* fname){
	FILINFO fno;


	sprintf(buff,"Test for \"%s\"...\n", fname);
	uart_puts(buff);

	fr = f_stat(fname, &fno);
	switch (fr) {

	case FR_OK:
		sprintf(buff,"Size: %lu\n", fno.fsize);
		uart_puts(buff);
		sprintf(buff,"Timestamp: %u-%02u-%02u, %02u:%02u\n",
				(fno.fdate >> 9) + 1980, fno.fdate >> 5 & 15, fno.fdate & 31,
				fno.ftime >> 11, fno.ftime >> 5 & 63);
		uart_puts(buff);
		sprintf(buff,"Attributes: %c%c%c%c%c\n",
				(fno.fattrib & AM_DIR) ? 'D' : '-',
						(fno.fattrib & AM_RDO) ? 'R' : '-',
								(fno.fattrib & AM_HID) ? 'H' : '-',
										(fno.fattrib & AM_SYS) ? 'S' : '-',
												(fno.fattrib & AM_ARC) ? 'A' : '-');
		uart_puts(buff);

		memset(buff,0,256);
		fr = f_open(&Fil, fname, FA_READ);	/* Create a file */
		if (fr == FR_OK)
		{
			fr = f_read(&Fil, buff, fno.fsize, &bw);
			uart_puts("Read Data: ");
			uart_puts(buff);
			uart_puts("\r\n");
		}
		else{
				sprintf(buff,"An error occured. (%d)\n", fr);
						uart_puts(buff);
			}

//		fr = f_unlink(fname);
//		sprintf(buff,"DEL_RESULT = %d\r\n",(uint8_t)fr);
		break;

	case FR_NO_FILE:
	case FR_NO_PATH:
		sprintf(buff,"\"%s\" is not exist.\n", fname);
		uart_puts(buff);
		break;

	default:
		sprintf(buff,"An error occured. (%d)\n", fr);
		uart_puts(buff);
	}
}
int main (void)
{

	_delay_ms(5000);
	uart_init(UART_BAUD_SELECT(UART_BAUD_RATE,F_CPU));
	/*
	 * Now enable interrupt, since UART library is interrupt controlled
	 */
	sei();
	uart_puts("START!\r\n");

	f_mount(&FatFs, "", 0);		/* Give a work area to the default drive */


	for (;;) {
	    process_uart();

	}
}


