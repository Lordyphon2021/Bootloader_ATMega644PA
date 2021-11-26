


//SETUP CPU FREQ AND BAUD RATE FOR SERIAL COMMUNICATION
#define F_CPU 20000000UL


#ifndef SREG
#  if __AVR_ARCH__ >= 100
#    define SREG _SFR_MEM8(0x3F)
#  else
#    define SREG _SFR_IO8(0x3F)
#  endif
#endif


#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>	
#include <avr/boot.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h> 
#include <string.h>
#include <stdlib.h>


//GLOBAL VARIABLES FOR INTERRUPT ROUTINE
volatile uint8_t header = 0;
volatile uint32_t sram_address = 0;
volatile uint32_t address = 0;
char handshake_array[17]  = "";
char update_array[6] = "";
volatile char hex_buffer_array[64] = "";
volatile uint8_t clk = 0;
uint8_t sram_array[256] = {0};
uint8_t sram_array_full = 0;
volatile uint8_t abort_flag = 0;

//HANDSHAKE STRINGS
char handshake_call[] = "c++ is great!   ";
const char handshake_response[] = "YES!";

char update_call[]= "pdate";
const char update_response[] = "sure";

//HEADERS FOR INCOMING USART TRANSMISSIONS

const char usart_handshake_message = '!';
const char usart_update_message = 'u';
const char usart_hexfile_message = ':';
const char usart_hexfile_send_complete = 'w';
const char usart_rx_error_hexrecord = '?';
const char usart_request_data_dump = 's';
const char usart_clock_hi = '+';
const char usart_clock_low = '-';
const char usart_reset_address = '�';


//ENUMS
enum checksum_status{is_error, is_ok};

//FLAGS AND COUNTERS
_Bool display_flag = 0;
volatile _Bool checksum_status = is_error;
uint32_t animation_ctr = 0;
volatile uint32_t byte_ctr = 0;
_Bool send_sram_flag = 0;


	
//DEFINES FOR ATMEGA PORTS AND PINS
#define LCD_Port PORTC			//Define LCD Port
#define LCD_DPin  DDRC			//Define 4-Bit Pins (PD4-PD7 at PORT D)
#define RSPIN PC2				//RS Pin
#define ENPIN PC3
#define rec_button (PINA &= 0x10)






//LCD FUNCTIONS
void LCD_Action( unsigned char cmnd )
{
	LCD_Port = (LCD_Port & 0x0F) | (cmnd & 0xF0);
	LCD_Port &= ~ ( 1 << RSPIN);
	LCD_Port |= ( 1 << ENPIN);
	_delay_us(1);
	LCD_Port &= ~ (1 << ENPIN);
	_delay_us(2);
	LCD_Port = (LCD_Port & 0x0F) | (cmnd << 4);
	LCD_Port |= (1 << ENPIN);
	_delay_us(1);
	LCD_Port &= ~ (1 << ENPIN);
	_delay_ms(2);
}

void LCD_Init (void)
{
	LCD_DPin = 0xFF;		//Control LCD Pins (D4-D7)
	_delay_ms(15);			//Wait before LCD activation
	LCD_Action(0x02);		//4-Bit Control
	LCD_Action(0x28);       //Control Matrix @ 4-Bit
	LCD_Action(0x0c);       //Disable Cursor
	LCD_Action(0x06);       //Move Cursor
	LCD_Action(0x01);       //Clear LCD
	_delay_ms(2);
}

void LCD_Clear()
{
	LCD_Action (0x01);		//Clear LCD
	_delay_us(20);
	LCD_Action (0x80);		//Move to Position Line 1, Position 1
}

void LCD_Print (char *str)
{
	int i;
	for(i=0; str[i]!=0; i++){
		LCD_Port = (LCD_Port & 0x0F) | (str[i] & 0xF0);
		LCD_Port |= (1<<RSPIN);
		LCD_Port|= (1<<ENPIN);
		_delay_us(10);
		LCD_Port &= ~ (1<<ENPIN);
		_delay_us(20);
		LCD_Port = (LCD_Port & 0x0F) | (str[i] << 4);
		LCD_Port |= (1<<ENPIN);
		_delay_us(10);
		LCD_Port &= ~ (1<<ENPIN);
		_delay_us(50);
	}
}


void LCD_Printpos (char row, char pos, char *str)  //Write on a specific location
{
	if (row == 0 && pos<16)
		LCD_Action((pos & 0x0F)|0x80);
	else if (row == 1 && pos<16)
		LCD_Action((pos & 0x0F)|0xC0);
	
	LCD_Print(str);
}



void LCD_Showval16(int val) //display hex values
{
	char showval [16];
	itoa (val,showval,16);
	LCD_Print(showval);
}



//SPI INTERFACE INITIALISATION
void SPI_MasterInit(void)
{
	SPCR |= (1<<SPE)|(1<<MSTR)|(1<<SPR0)|(1<<CPOL)|(1<<CPHA);
	SPCR |= (1<<SPR0) |(1<<SPR1);
	SPCR &= ~(1<<DORD);
	SPCR &= ~(1<<SPIE);
	PORTB |=(1<<PB4);
	PORTB |=(1<<PB3);
}

//SPI SRAM FUNCTIONS

void SPI_SRAM_ByteWrite(uint32_t sram_address, uint8_t data)
{
	uint32_t address_byte_H = ((sram_address >>16) & 0xff);		// sram address is 24-bit value, split into 3 bytes for transmission
	uint32_t address_byte_M = ((sram_address >>8)  & 0xff);
	uint32_t address_byte_L = (sram_address & 0xff);
	
	PORTB &= ~0x08;												//chip select pin LOW activates SRAM
	//_delay_us(10);
	SPDR = 0x02;												//selects "byte only - operation" of sram
	while (!(SPSR & (1<<SPIF)));
	SPDR = (uint8_t)address_byte_H;
	while (!(SPSR & (1<<SPIF)));
	SPDR = (uint8_t)address_byte_M;
	while (!(SPSR & (1<<SPIF)));
	SPDR = (uint8_t)address_byte_L;
	while (!(SPSR & (1<<SPIF)));
	SPDR = data;												//stores cv/gate-data byte in 24 bit address
	while (!(SPSR & (1<<SPIF)));
	//_delay_us(10);
	PORTB |= 0x08;												//chip select pin HIGH deactivates SRAM
}



uint8_t SPI_SRAM_ByteRead(uint32_t sram_address)
{
	uint32_t address_byte_H = ((sram_address >>16) & 0xff);
	uint32_t address_byte_M = ((sram_address >>8)  & 0xff);
	uint32_t address_byte_L = (sram_address & 0xff);
	
	PORTB &= ~0x08;												//chip select pin LOW activates SRAM
	
	SPDR = 0x03;
	while (!(SPSR & (1<<SPIF)));
	SPDR = (uint8_t)address_byte_H;								//casting uint32_t to uint8_t
	while (!(SPSR & (1<<SPIF)));
	SPDR = (uint8_t)address_byte_M;
	while (!(SPSR & (1<<SPIF)));
	SPDR = (uint8_t)address_byte_L;
	while (!(SPSR & (1<<SPIF)));
	
	SPDR = 0xff;
	while (!(SPSR & (1<<SPIF)));
	uint8_t data = SPDR;										//reads data from address
	
	PORTB |= 0x08;												//chip select pin HIGH deactivates SRAM
	
	return(data);
}




//USART INTERFACE INITIALISATION
void USART_Init( uint16_t ubrr)
{
	
	
	UBRR0H = (uint8_t)((ubrr>>8) & 0xff);
	UBRR0L = (uint8_t)ubrr;
	UCSR0B |= (1<<TXEN0); 
	UCSR0B |= (1<<RXEN0);	
	UCSR0B |= (1<<RXCIE0);	
	UCSR0A |= (1<<RXC0);							
	UCSR0C = (1<<USBS0)|(3 << UCSZ00);	
	UCSR0B |= (1<<RXEN0)|(1<<RXCIE0);
	set_sleep_mode(SLEEP_MODE_IDLE);
		
}

//SERIAL SEND AND RECEIVE FUNCTIONS

void USART_transmit_byte( uint8_t data )							
{
	while ( !(  UCSR0A & (1<<UDRE0 ) ) );
	UDR0 = data;
}




void USART_transmit_string( const char* message )
{
	
	for( int idx = 0; idx != strlen( message ); ++idx )
		USART_transmit_byte(message[idx]);
	
}


uint8_t USART_receive_byte(void)
{
	while (!(UCSR0A & (1<<RXC0)))
	;   
	
	return UDR0;												
}


void (*start)( void ) = 0x0000;  //jump to main app
//void (*boot)( void ) = 0x7800;


_Bool flash_content_ok()
{
	
	_delay_ms(100);
	LCD_Clear();
	LCD_Printpos(0,0, "validating flash");
	LCD_Printpos(1,0, "please wait     ");
	
	uint16_t flash_EOF = (byte_ctr >> 1);
	uint8_t lsb = 0;
	uint8_t msb = 0;
	uint32_t sram_address = 0;
	
	/*
	for(uint16_t i = 0; i < flash_EOF; ++i){
		
		
		lsb =  SPI_SRAM_ByteRead(sram_address);
		msb =  SPI_SRAM_ByteRead(sram_address+1);
		sram_address +=2;
		
		if(pgm_read_word(i) != (lsb << 8 | msb)){
			return 0;
		}
		
	}
	*/
	
	return 1;
	
}

void write_firmware_to_flash(const uint32_t byte_ctr)
{
	uint32_t flash_EOF = ((byte_ctr >> 1) + 2048);  // => /2, and 2 pages safety
	uint32_t sram_address = 0;
	uint32_t flash_address = 0;
	cli();
	 boot_rww_enable();
	
	 while(flash_address < flash_EOF ){
		
		boot_page_erase_safe(sram_address) ;
		boot_spm_busy_wait();  
		
		for(int i = 0; i < 128; ++i){
			uint16_t lsb =  SPI_SRAM_ByteRead(sram_address);
			uint8_t msb =  SPI_SRAM_ByteRead(sram_address + 1 );
			uint16_t data = (msb << 8)  | lsb;
			boot_page_fill_safe(sram_address, data);
			sram_address += 2;
		}
		
		
			boot_page_write_safe(flash_address);
			boot_spm_busy_wait();  
			
		
			flash_address += 256;
		
	}
	
	if(flash_content_ok()){
	
		boot_rww_enable ();
	
	
		sram_address = 0;
		_delay_ms(100);
		LCD_Clear();
		LCD_Printpos(0,0, "please restart");
		LCD_Printpos(1,0, "lordyphon      ");
		_delay_ms(1000);
		//start();
		
	}
	else{
		boot_rww_enable ();
		LCD_Clear();
		LCD_Printpos(0,0, "Flash Error");
		LCD_Printpos(1,0, "try again");
		_delay_ms(1000);
		
	}
	
 
}


//MAIN FUNCTION


int main(void)
{
	
	unsigned char temp;       
	       
	
	
	
	
	
	
	
	
	//SET ATMEGA PORTS/PINS TO IN- OR OUTPUTS
	
	DDRA = 0x0f;
	PORTA = 0x00;
	DDRB = 0xbf;
	DDRD = 0xfe;
	PORTD = 0x01; //internal pull-up rx-pin
	
	PORTB = 0xbf;
	
	PORTA = 13;  // set address for record button
	
	//INIT INTERFACES 
	USART_Init(21);  // UBRR = (F_CPU/(16*BAUD))-1 
	SPI_MasterInit();
	LCD_Init();									
	_delay_ms(500);
	
	
	//ACTIVATE INTERRUPT
	sei();
	
	
	
	//CHECK IF SRAM IS ONLINE
	/*
	SPI_SRAM_ByteWrite(0x000002, 0xfa);
	
	if (SPI_SRAM_ByteRead(0x000002)==0xfa){
		
		LCD_Printpos(0, 0, "bootloader ready" );
		LCD_Printpos(1, 0, "SRAM ready...   " );
	}
	*/
	
	
	
	if(rec_button){
		//MAIN LOOP
		
		/* Interrupt Vektoren verbiegen */
		
		char sregtemp = SREG;
		temp = MCUCR;
		MCUCR = temp | (1<<IVCE);
		MCUCR = temp | (1<<IVSEL);
		SREG = sregtemp;
		
		LCD_Printpos(0, 0, "updater ready    "  );
		LCD_Printpos(1, 0, "start lordylink      " );
		
		
		
		
		while(1)  //main loop
		{
				
			
			//HANDSHAKE CALL EVALUATION
			if(strcmp(handshake_array, handshake_call) == 0){  //if call is correct, response will be sent
				_delay_ms(200);								   //give lordylink some time for startup
				USART_transmit_string(handshake_response);
				strcpy(handshake_array, "                ");   //delete input buffer			
				LCD_Printpos(0,0, "lordylink      ");
				LCD_Printpos(1,0, "connected      ");
				
			
			}
			_delay_ms(100);
			//UPDATE CALL EVALUATION
			if(strcmp(update_array, update_call) == 0){  //if call is correct, response will be sent
				_delay_ms(200);								 
				USART_transmit_string(update_response);
				strcpy(update_array, "     ");   //overwrite input buffer
				LCD_Printpos(0,0, "updater      ");
				LCD_Printpos(1,0, "enabled     ");
				
				
			}
			
			
			
			
			
			
			
			
			
			
		} //end while(1) 
	}else{
		
		start();
	
	}





} // end main 

//Interrupt Service Routine is in sleep mode unless USART message is received

ISR(USART0_RX_vect)
{
	header = USART_receive_byte();  //header will determine, which kind of message is arriving (following if/else paths)
	
	
	//HANDSHAKE MESSAGE
	
	if (header == usart_handshake_message){  // if incoming data is of handshake type...
		for(int i = 0; i < 16; ++i)
			handshake_array[i] = USART_receive_byte();  //read handshake call, if correct: response will be sent from main application
		
	}
	//HEXFILE MESSAGE - INCOMING RECORD
	
	
	//AFTER HEX-RECORD IS RECEIVED, CHECKSUM WILL BE EVALUATED AND CONFIRMATION MESSAGE ("ok" or "er") WILL BE SENT TO LORDYLINK'S BLOCKING THREAD
	//IF CONFIRMATION IS "ok", LORDYLINK SENDS NEXT RECORD AFTER CONTROLLER HAS WRITTEN DATA SECTION INTO THE SRAM
	//IF CONFIRMATION IS "er", LORDYLINK SENDS CURRENT RECORD AGAIN, EVALUATION STARTS FROM THE TOP
	//IF CONFIRMATION MESSAGE ISN'T RECOGNIZED BY LORDYLINK, CHECKSUM STATUS WILL BE RE-EVALUATED AND CONFIRMATION MESSAGE IS TRANSMITTED AGAIN.
	
	else if(header == usart_hexfile_message){   //if message is hexfile....
		
			//PARSE INCOMING MESSAGE
		
			uint8_t data_section_size = USART_receive_byte();						//this is the amount of databytes that will be written into the SRAM
			uint8_t hex_record_size = data_section_size + 5;				// add start bytes and checksum to data section length for total size of message
		
			hex_buffer_array[0] = data_section_size;						//buffer starts with data_section_size, header ':' will be discarded
		
			for(int i = 1; i < hex_record_size ; ++i )						// get rest of message data
				hex_buffer_array[ i ] = USART_receive_byte();
			
			
			
			
			//TRANSMISSION IS NOW COMPLETED AND STORED IN BUFFER
		
			switch(animation_ctr){											// display animation, tells user that all is going well
				case 1:
					LCD_Printpos(0,0, "reading file     "); 
					LCD_Printpos(1,0, "please wait      "); 
					break;
				
				case 120:
					LCD_Printpos(1,0, "  please wait    ");
					break;
				
				case 240:
					LCD_Printpos(1,0, "    please wait ");
					break;
				
				case 360:
					animation_ctr = 0; 
					break;
			}//end switch
			animation_ctr++;
		
			
			//THIS PART CALCULATES CHECKSUM FROM MESSAGE
			//AND COMPARES IT TO THE CHECKSUM IN THE HEX-RECORD
			
			uint16_t vec_sum = 0;												//local helper variable for checksum calculation
			uint8_t checksum_from_file = hex_buffer_array[hex_record_size-1];   //read checksum from file
		
			for(int i = 0; i < hex_record_size - 1; ++i)						// accumulate record for checksum calculation
				vec_sum += hex_buffer_array[ i ];
		
			uint8_t checksum_calculated =  ~(vec_sum & 0x00ff ) + 0x01;		// actual checksum calculation
		
			if(checksum_calculated == checksum_from_file){						// compare checksums
				checksum_status = is_ok;										// set boolean flag for error handling
				
				for(int i = 0; i < data_section_size; ++i){						//if checksum is ok, write data-section to SRAM
					
					do{
						SPI_SRAM_ByteWrite(address, hex_buffer_array[ i + 4 ]);	
					
					}while(SPI_SRAM_ByteRead(address) !=  hex_buffer_array[ i + 4 ]);  //additional safety-guard: write to address and read content back. if this doesn't work, sram is probably broken
				
					++address;	
					++byte_ctr;	
				}//end for
				USART_transmit_string("ok");									// confirm transmission: lordylink thread blocks until confirmation is either "ok" or "er". checksum "ok" => SRAM will be written, next record will be sent
																				// "er" => current record will be sent again. if neither "ok" nor "er" is detected by lordylink, rx_error_header will be sent
																				//to reevaluate checksum via flag variable "checksum_is"
			}//end if(checksum calculated....	
			
			else if( checksum_calculated != checksum_from_file){
				checksum_status = is_error;											// error, same record will be sent again
				LCD_Printpos(0,0,"checksum error!      ");						//LCD user feedback
				LCD_Printpos(1,0,"trying again......  ");
				
				_delay_ms(100);
				
				animation_ctr  = 0;
				USART_transmit_string("er");										
			}
	} // end else if(header == usart_hexfile_message)
	
	//THIS MESSAGE HEADER INDICATES THAT HEXFILE CONFIRMATION MESSAGE WASN'T RECOGNIZED BY LORDYLINK
	
	else if(header == usart_rx_error_hexrecord){									//lordylink didn't "understand" last confirmation	
		
		
		if(checksum_status == is_ok)											    // evaluating last checksum again...
			USART_transmit_string("ok");	
			
		else																		// if error => record will be sent again, otherwise SRAM will have been correctly written
			USART_transmit_string("er");
	}
	else if(header == usart_request_data_dump){
		{
			sram_address = 0;
			send_sram_flag = 1;
		}
	}
	else if(header == usart_clock_hi ){	//clock for sram dump, TODO: block for checksum calc
		clk = 1;
	}
	else if(header == usart_clock_low){
		clk = 0;
	}
	
	else if(header == usart_hexfile_send_complete){							//hexfile transfer is complete
		
			
			
			LCD_Clear();
			LCD_Printpos(0,0, "file complete      ");
			_delay_ms(700);
			
			LCD_Clear();
			LCD_Printpos(0,0, "burning flash       ");
			LCD_Printpos(1,0, "don't turn off      ");
			write_firmware_to_flash(byte_ctr);
			
			
		
	}	
	else if(header == usart_reset_address){
		address = 0;
		sram_address = 0;
		byte_ctr = 0;
		checksum_status = is_error;
		
	}
	else if (header == usart_update_message){  // if incoming data is of update handshake type...
		for(int i = 0; i < 5; ++i)
		update_array[i] = USART_receive_byte();  //read handshake call, if correct: response will be sent from main application
		
	}
	
	
}
 
 

 