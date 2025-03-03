/* Basisfunktionen ATmega2560 mit CodeVision, M. Eisele, 13.02.2025 */

#ifndef PLATFORM_H
#define PLATFORM_H

/******************************************************************************/

#pragma used+
void timebase_init (void);
uint16_t millis (void);
void mdelay_us (uint16_t dt_us);
void mdelay_ms (uint16_t dt_ms);

void uart0_init (unsigned int ubrr);
void uart0_tx (uint8_t txbyte);
uint8_t uart0_rx (void);

void serial_print_text (const char *text);

const char *uint32_to_text_decimal (uint32_t value);
const char *int32_to_text_decimal (int32_t value);

void sei (void);
void cli (void);

void lcd_init (void);
void lcd_clear (void);
void lcd_home (void);
void lcd_set_cursor (uint8_t col, uint8_t row);
void lcd_display_off (void);
void lcd_display_on (void);
void lcd_cursor_off (void);
void lcd_cursor_on (void);
void lcd_blink_off (void);
void lcd_blink_on (void);
void lcd_scroll_left (void);
void lcd_scroll_right (void);
void lcd_backlight_off (void);
void lcd_backlight_on (void);
void lcd_backlight_onoff (void);
void lcd_command (uint8_t cmd);
void lcd_write_8bit (uint8_t value, uint8_t mode);
void lcd_write_4bit (uint8_t data);
void lcd_expander_write (uint8_t data);
void lcd_print_char (uint8_t ch);
void lcd_print_text (const char *text);

void bmp280_start (void);
void bmp280_read (void);
#pragma used-

/******************************************************************************/

extern int32_t  bmp280_temp;
extern uint32_t bmp280_pres;
extern uint8_t  bmp280_ok;

/******************************************************************************/

#endif /* PLATFORM_H */

