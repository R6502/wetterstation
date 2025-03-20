#ifndef DISPLAY_H
#define DISPLAY_H


#pragma used+
void write_char (unsigned char);
void write_instr (unsigned char);
void lcd_init (void);
void display_pos (unsigned char pos);

#pragma used-
#pragma library display.lib

#endif

