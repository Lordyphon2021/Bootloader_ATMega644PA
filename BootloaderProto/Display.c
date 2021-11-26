
#define F_CPU 20000000UL
#include <avr/io.h>
#include "Display.h"
#include <util/delay.h>
#include <stdlib.h>
#include "Sensor.h"



#define LCD_Port PORTC			//Define LCD Port
#define LCD_DPin  DDRC			//Define 4-Bit Pins (PD4-PD7 at PORT D)
#define RSPIN PC2				//RS Pin
#define ENPIN PC3

enum direction{up, down};

void LCD_Action( unsigned char cmnd )
{
	LCD_Port = (LCD_Port & 0x0F) | (cmnd & 0xF0);
	LCD_Port &= ~ (1<<RSPIN);
	LCD_Port |= (1<<ENPIN);
	_delay_us(1);
	LCD_Port &= ~ (1<<ENPIN);
	_delay_us(2);
	LCD_Port = (LCD_Port & 0x0F) | (cmnd << 4);
	LCD_Port |= (1<<ENPIN);
	_delay_us(1);
	LCD_Port &= ~ (1<<ENPIN);
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
	LCD_Action(0x01);       //Clean LCD
	_delay_ms(2);
}

void LCD_Clear()
{
	LCD_Action (0x01);		//Clear LCD
	_delay_us(2);
	LCD_Action (0x80);		//Move to Position Line 1, Position 1
}

void LCD_Print (char *str)
{
	int i;
	for(i=0; str[i]!=0; i++){
		LCD_Port = (LCD_Port & 0x0F) | (str[i] & 0xF0);
		LCD_Port |= (1<<RSPIN);
		LCD_Port|= (1<<ENPIN);
		_delay_us(1);
		LCD_Port &= ~ (1<<ENPIN);
		_delay_us(20);
		LCD_Port = (LCD_Port & 0x0F) | (str[i] << 4);
		LCD_Port |= (1<<ENPIN);
		_delay_us(1);
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

void LCD_Showval(int val)
{
	char showval [16];
	itoa (val,showval,10);
	LCD_Print(showval);
}

void LCD_Showval16(int val)
{
	char showval [16];
	itoa (val,showval,16);
	LCD_Print(showval);
}
void LCD_Printpos_value(char row, char pos, int val)
{
	char showval [16];
	
	if(val==0)
	LCD_Printpos(row, pos, " ");
	if(val<10)
	pos+=1;
	if(val<=9)
	LCD_Printpos(row, pos-1, " ");
	if(val<0)
	pos-=1;
	if(val<-9)
	pos-=1;
	if(val == -9)
	LCD_Printpos(row, pos-1, " ");
	
	
	if (row == 0 && pos<16)
	LCD_Action((pos & 0x0F)|0x80);
	else if (row == 1 && pos<16)
	LCD_Action((pos & 0x0F)|0xC0);
	itoa (val,showval,10);
	LCD_Print(showval);
}
void LCD_Printpos_value_32bit(char row, char pos, uint32_t val)
{
	char showval [16];
	
	
	
	
	if (row == 0 && pos<16)
	LCD_Action((pos & 0x0F)|0x80);
	else if (row == 1 && pos<16)
	LCD_Action((pos & 0x0F)|0xC0);
	itoa (val,showval,10);
	LCD_Print(showval);
}
void LCD_Printpos_value_3_digits(char row, char pos, int val)
{
	char showval [16];
	
	
	if(val<99){
		pos+=1;
		LCD_Printpos(row, pos-1, " ");
	}
	
	if(val==0)
	LCD_Printpos(row, pos, " ");
	if(val<10){
		pos+=1;
		LCD_Printpos(row, pos-1, " ");
	}
	if(val<0)
	pos-=1;
	if(val<-9){
		pos-=1;
		LCD_Printpos(row, pos-1, " ");
	}
	
	if (row == 0 && pos<16)
	LCD_Action((pos & 0x0F)|0x80);
	else if (row == 1 && pos<16)
	LCD_Action((pos & 0x0F)|0xC0);
	itoa (val,showval,10);
	LCD_Print(showval);
}



void transpose_semi_display(int8_t* value, int8_t direction)
{
	
	LCD_Printpos(0,9,"semi:");
	
	if(direction == up){
		*value += 1;
		if(*value >= 7 )
		*value = 7;
	}
	else if(direction == down){
		*value -= 1;
		if(*value <= -5 )
		*value = -5 ;
	}
	LCD_Printpos_value(0,14,*value);
}

void transpose_oct_display(int8_t* value, int8_t direction)
{
	LCD_Printpos(1,9,"oct:");
	
	if(direction == up){
		*value += 1;
		if(*value >= 3 )
		*value = 3;
	}
	else if(direction == down){
		*value -= 1;
		if(*value <= -1 )
		*value = -1 ;
	}
	LCD_Printpos_value(1,14,*value);

}
void LCD_Printpos_value_to_string(char row, char pos, int val)
{
	
	char showval [16];
	
	
	if (row == 0 && pos<16)
	LCD_Action((pos & 0x0F)|0x80);
	else if (row == 1 && pos<16)
	LCD_Action((pos & 0x0F)|0xC0);
	itoa (val,showval,10);
	LCD_Print(showval);
}
void LCD_print_page_1(uint8_t patch_number, int8_t active_sensor, int8_t display_sensor1_semi, int8_t display_sensor1_oct, int8_t display_sensor2_semi, int8_t display_sensor2_oct)
{
	
	LCD_Printpos(0, 0, "patch");
	LCD_Printpos_value_3_digits(0,5, patch_number );
	if(active_sensor == 0){
		LCD_Printpos(1, 0, "ribbon 1");
		LCD_Printpos(0, 9, "semi:");
		LCD_Printpos_value(0, 14, display_sensor1_semi);
		LCD_Printpos(1, 9, "oct: ");
		LCD_Printpos_value(1, 14, display_sensor1_oct);
		}else{
		LCD_Printpos(1, 0, "ribbon 2");
		LCD_Printpos(0, 9, "semi:");
		LCD_Printpos_value(0, 14, display_sensor2_semi);
		LCD_Printpos(1, 9, "oct: ");
		LCD_Printpos_value(1, 14, display_sensor2_oct);
	}
}

void LCD_print_updated_transpose_values(int8_t display_sensor2_semi, int8_t display_sensor2_oct)
{
	
	
	LCD_Printpos(0, 9, "semi:");
	LCD_Printpos_value(0, 14, display_sensor2_semi);
	LCD_Printpos(1, 9, "oct: ");
	LCD_Printpos_value(1, 14, display_sensor2_oct);
	
	
}