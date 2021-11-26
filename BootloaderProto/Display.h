#ifndef Display   /* Include guard */
#define Display


void LCD_Action( unsigned char cmnd );
void LCD_Init (void);
void LCD_Clear();
void LCD_Print (char *str);
void LCD_Printpos (char row, char pos, char *str);
void LCD_Showval(int val);
void LCD_Showval16(int val);
void LCD_Showval16(int val);
void LCD_Printpos_value(char row, char pos, int val);
char *  itoa ( int value, char * str, int base );
void transpose_semi_display(int8_t* value, int8_t direction);
void transpose_oct_display(int8_t* value, int8_t direction);
void LCD_Printpos_value_3_digits(char row, char pos, int val);
void LCD_Printpos_value_to_string(char row, char pos, int val);
void LCD_print_page_1(uint8_t patch_number, int8_t active_sensor, int8_t display_sensor1_semi, int8_t display_sensor1_oct, int8_t display_sensor2_semi, int8_t display_sensor2_oct);
void LCD_print_updated_transpose_values(int8_t display_sensor2_semi, int8_t display_sensor2_oct);
void LCD_Printpos_value_32bit(char row, char pos, uint32_t val);
#endif // Display