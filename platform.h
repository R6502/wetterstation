/* Basisfunktionen ATmega2560 mit CodeVision, M. Eisele, 13.02.2025 */

#ifndef PLATFORM_H
#define PLATFORM_H

/******************************************************************************/

#if defined (__AVR__)
/* AVR GCC */
#  include <avr/io.h>
#  include <avr/interrupt.h>
#else
/* Codevision */
#  include <io.h>
#  include <i2c.h>
#endif

/******************************************************************************/

#if defined(__CODEVISIONAVR__)
#pragma used+
#endif

void timebase_init (void);
uint16_t millis (void);
void mdelay_us (uint16_t dt_us);
void mdelay_ms (uint16_t dt_ms);

void int32_to_text_decimal (int32_t value, uint8_t minlen);
// void int16_to_text_decimal (int16_t value, uint8_t minlen);
void insert_decimal_point10 ();

#if defined(__CODEVISIONAVR__)
void sei (void);
void cli (void);
#endif

void lcd_init (void);
void lcd_clear (void);
void lcd_home (void);
void lcd_set_cursor (uint8_t col, uint8_t row);
void lcd_display_off (void);
void lcd_display_on (void);
void lcd_backlight_off (void);
void lcd_backlight_on (void);
void lcd_command (uint8_t cmd);
void lcd_write_8bit (uint8_t value, uint8_t mode);
void lcd_write_4bit (uint8_t data);
void lcd_expander_write (uint8_t data);
void lcd_print_char (uint8_t ch);
void lcd_print_text (const char *text);

void bmp280_start (void);
void bmp280_read (void);

#if defined(__CODEVISIONAVR__)
#pragma used-
#endif

#define I2C_WRITE               0
#define I2C_READ                1

#if defined (__AVR__)
void i2c_init ();
void i2c_init_ext (uint8_t sda_line, uint8_t scl_line);
void i2c_start ();
void i2c_stop ();
uint8_t i2c_write (uint8_t ch);
uint8_t i2c_read (uint8_t ch);
#endif

/******************************************************************************/

#define BMP280_ID_VAL           0x58
#define BME280_ID_VAL           0x60

extern int32_t  bmp280_temp;
extern uint32_t bmp280_pres;
extern uint32_t bmp280_humi;
extern uint8_t  bmp280_id;

/******************************************************************************/

uint8_t keypad_read (void);

/******************************************************************************/

extern uint8_t zeit_sekunden_bcd;
extern uint8_t zeit_minuten_bcd;
extern uint8_t zeit_stunden_bcd;

void rtc_read (void);
void rtc_write (void);

/******************************************************************************/

#define MAX_TEXT_BUFFER 16
#define TEXT_BUFFER_RIGHT 10
extern char text_buffer [MAX_TEXT_BUFFER];

/******************************************************************************/

#endif /* PLATFORM_H */

