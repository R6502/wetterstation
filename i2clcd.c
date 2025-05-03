/* 20x4 LCD an I2C mit CodeVision, M. Eisele, 13.02.2025 */

/* Übernommen von

    https://github.com/johnrickman/LiquidCrystal_I2C.git

  und angepasst an C */

/******************************************************************************/

#include <stdint.h>

#include "platform.h"

/******************************************************************************/

/* Langsamer, aber weniger Code */
#define OPT_I2C_SINGLE_BYTE_ACCESS

/* I2C ist so langsam, dass wir uns die kurzen Delays sapren können*/
// #define OPT_LCD_SHORT_DELAYS

/******************************************************************************/

// commands
#define LCD_CLEARDISPLAY        0x01
#define LCD_RETURNHOME          0x02
#define LCD_ENTRYMODESET        0x04
#define LCD_DISPLAYCONTROL      0x08
#define LCD_CURSORSHIFT         0x10
#define LCD_FUNCTIONSET         0x20
#define LCD_SETCGRAMADDR        0x40
#define LCD_SETDDRAMADDR        0x80

// flags for display entry mode
#define LCD_ENTRYRIGHT          0x00
#define LCD_ENTRYLEFT           0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// flags for display on/off control
#define LCD_DISPLAYON           0x04
#define LCD_DISPLAYOFF          0x00
#define LCD_CURSORON            0x02
#define LCD_CURSOROFF           0x00
#define LCD_BLINKON             0x01
#define LCD_BLINKOFF            0x00

// flags for display/cursor shift
#define LCD_DISPLAYMOVE         0x08
#define LCD_CURSORMOVE          0x00
#define LCD_MOVERIGHT           0x04
#define LCD_MOVELEFT            0x00

// flags for function set
#define LCD_8BITMODE            0x10
#define LCD_4BITMODE            0x00
#define LCD_2LINE               0x08
#define LCD_1LINE               0x00
#define LCD_5x10DOTS            0x04
#define LCD_5x8DOTS             0x00

// flags for backlight control
#define LCD_BACKLIGHT           0x08
#define LCD_NOBACKLIGHT         0x00
#define LCD_CONTROL_EN          0x04   // Enable bit
#define LCD_CONTROL_RW          0x02   // Read/Write bit
#define LCD_CONTROL_RS          0x01   // Register select bit

#define LCD_INIT_COMMAND_1        ((LCD_FUNCTIONSET) | (LCD_8BITMODE))
#define LCD_INIT_COMMAND_2        ((LCD_FUNCTIONSET) | (LCD_4BITMODE))

/******************************************************************************/

uint8_t lcd_numlines = 0;
uint8_t lcd_backlightval = 0;
uint8_t lcd_address = 0;
uint8_t lcd_displaycontrol = 0;

/******************************************************************************/

/* When the display powers up, it is configured as follows:

   1. Display clear
   2. Function set:
      DL = 1; 8-bit interface data
      N = 0; 1-line display
      F = 0; 5x8 dot character font
   3. Display on/off control:
      D = 0; Display off
      C = 0; Cursor off
      B = 0; Blinking off
   4. Entry mode set:
      I/D = 1; Increment by 1
      S = 0; No shift

   Note, however, that resetting the Arduino doesn't reset the LCD, so we
   can't assume that its in that state when a sketch starts (and the
   LiquidCrystal constructor is called). */

void lcd_init (void)
{
  lcd_displaycontrol  = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
  lcd_numlines        = 4;
  lcd_address         = 0x27;
  lcd_backlightval    = LCD_NOBACKLIGHT;

  /* SEE PAGE 45/46 FOR INITIALIZATION SPECIFICATION!
     according to datasheet, we need at least 40ms after power rises above 2.7V
     before sending commands. Arduino can turn on way befer 4.5V so we'll wait 50 */
  mdelay_ms (50);

  /* Now we pull both RS and R/W low to begin commands */
  lcd_expander_write (lcd_backlightval);  // reset expanderand turn backlight off (Bit 8 =1)
  mdelay_ms (1000);

  /* put the LCD into 4 bit mode this is according to the hitachi HD44780 datasheet figure 24, pg 46 */

  /* we start in 8bit mode, try to set 4 bit mode */
  lcd_write_4bit (LCD_INIT_COMMAND_1);
  mdelay_ms (5); // wait min 4.1ms

  /* second try */
  lcd_write_4bit (LCD_INIT_COMMAND_1);
  mdelay_ms (5); // wait min 4.1ms

  /* third go! */
  lcd_write_4bit (LCD_INIT_COMMAND_1);

  mdelay_us (150);

  /* finally, set to 4-bit interface */
  lcd_write_4bit (LCD_INIT_COMMAND_2);

  // set # lines, font size, etc.
  lcd_command (LCD_FUNCTIONSET | LCD_4BITMODE | LCD_5x8DOTS | LCD_2LINE);

  /* turn the display on with no cursor or blinking default */
  lcd_display_on ();

  /* clear it off  */
  lcd_clear ();

  /* set the entry mode - Initialize to default text direction (for roman languages) */
  lcd_command (LCD_ENTRYMODESET | LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT);

  lcd_home ();
} /* lcd_init */


void lcd_clear (void)
{
  lcd_command (LCD_CLEARDISPLAY);// clear display, set cursor position to zero
  mdelay_ms (2);  // this command takes a long time!
} /* lcd_clear */


void lcd_home (void)
{
  lcd_command (LCD_RETURNHOME);  // set cursor position to zero
  mdelay_ms (2);  // this command takes a long time!
} /* lcd_home */


void lcd_set_cursor (uint8_t col, uint8_t row)
{
  static uint8_t row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };
  if (row >= lcd_numlines ) {
    row = lcd_numlines - 1;    // we count rows starting w/0
  }
  lcd_command (LCD_SETDDRAMADDR | (col + row_offsets [row]));
} /* lcd_set_cursor */


void lcd_display_off (void)
{
  lcd_displaycontrol &= ~LCD_DISPLAYON;
  lcd_command (LCD_DISPLAYCONTROL | lcd_displaycontrol);
} /* lcd_display_off */


void lcd_display_on (void)
{
  lcd_displaycontrol |= LCD_DISPLAYON;
  lcd_command (LCD_DISPLAYCONTROL | lcd_displaycontrol);
} /* lcd_display_on */


void lcd_backlight_off (void)
{
  lcd_backlightval = LCD_NOBACKLIGHT;
  lcd_expander_write (0);
} /* lcd_backlight_off */


void lcd_backlight_on (void)
{
  lcd_backlightval = LCD_BACKLIGHT;
  lcd_expander_write (0);
} /* lcd_backlight_on */


void lcd_command (uint8_t cmd)
{
  uint8_t mode = 0;
  lcd_write_8bit (cmd, mode);
} /* lcd_command */


void lcd_write_8bit (uint8_t value, uint8_t mode)
{
  uint8_t high_nibble =  value       & 0xf0;
  uint8_t low_nibble  = (value << 4) & 0xf0;

#if defined (OPT_I2C_SINGLE_BYTE_ACCESS)

  lcd_write_4bit (high_nibble | mode);
  lcd_write_4bit (low_nibble  | mode);

#else

  high_nibble |= mode;
  low_nibble  |= mode;

  high_nibble |= lcd_backlightval;
  low_nibble  |= lcd_backlightval;

  i2c_start ();
  i2c_write (lcd_address << 1);

  i2c_write (high_nibble);
  i2c_write (high_nibble | LCD_CONTROL_EN);

  mdelay_us (1); // enable pulse must be >450ns

  i2c_write (high_nibble);

  i2c_write (low_nibble);
  i2c_write (low_nibble | LCD_CONTROL_EN);

  mdelay_us (1); // enable pulse must be >450ns

  i2c_write (low_nibble);

  i2c_stop ();

#  if defined (OPT_LCD_SHORT_DELAYS)
  mdelay_us (50); // commands need > 37us to settle
#  endif

#endif
} /* lcd_write_8bit */


void lcd_write_4bit (uint8_t data)
{
#if defined (OPT_I2C_SINGLE_BYTE_ACCESS)

  lcd_expander_write (data);

  lcd_expander_write (data | LCD_CONTROL_EN);     // En high

#  if defined (OPT_LCD_SHORT_DELAYS)
  mdelay_us (1); // enable pulse must be >450ns
#  endif

  lcd_expander_write (data & ~LCD_CONTROL_EN); // En low

#else

  i2c_start ();
  i2c_write (lcd_address << 1);

  i2c_write (data);
  i2c_write (data | LCD_CONTROL_EN);

#  if defined (OPT_LCD_SHORT_DELAYS)
  mdelay_us (1); // enable pulse must be >450ns
#  endif

  i2c_write (data);

  i2c_stop ();

#endif

#  if defined (OPT_LCD_SHORT_DELAYS)
  mdelay_us (50); // commands need > 37us to settle
#  endif
} /*lcd_write_4bit  */


void lcd_expander_write (uint8_t data)
{
  i2c_start ();
  i2c_write (lcd_address << 1);
  i2c_write (data | lcd_backlightval);
  i2c_stop ();
} /* lcd_expander_write */


void lcd_print_char (uint8_t ch)
{
  uint8_t mode = LCD_CONTROL_RS;
  lcd_write_8bit (ch, mode);
} /* lcd_print_char */


void lcd_print_text (const char *text)
{
  while (*text) {
    lcd_print_char (*text++);
  }
} /* lcd_print_text */


/******************************************************************************/


