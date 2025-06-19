/* Wetterstation mit CodeVision, M. Eisele, 13.02.2025 */

/******************************************************************************/

#include <stdint.h>
#include <string.h>

#include "platform.h"

/******************************************************************************/

uint16_t time_1000ms_prev = 0;
uint16_t time_100ms_prev = 0;

uint8_t zeit_sekunden_bcd = 0;
uint8_t zeit_minuten_bcd = 0;
uint8_t zeit_stunden_bcd = 0;

uint8_t settime_minuten_bcd = 0;
uint8_t settime_stunden_bcd = 0;

#define MENU_NORMAL     (0)
#define MENU_SETTIME    (1)
uint8_t menu_shown = MENU_NORMAL;
uint8_t menu_mode = MENU_NORMAL;
uint8_t menu_anzeigen = 0;

/******************************************************************************/

#define MAX_TEXT_BUFFER 16
#define TEXT_BUFFER_RIGHT 6

char text_buffer [MAX_TEXT_BUFFER];

/******************************************************************************/

void zaehler_anzeigen (uint8_t zaehler_wert);

/******************************************************************************/


typedef struct history_s {
          uint8_t  seq;

          uint8_t  zeit_minuten_bcd;
          uint8_t  zeit_stunden_bcd;

          int32_t  bmp280_temp;
          uint32_t bmp280_pres;
          uint32_t bmp280_humi;
        } HISTORY;

#define HISTORY_SIZE    16
#define HISTORY_MASK    0xf
#define HISTORY_MAX     9

HISTORY history [HISTORY_SIZE];

uint8_t  hist_head = 0;
uint8_t  hist_level = 0;
uint8_t  hist_seq = 0;
uint8_t  hist_show = 0;

/******************************************************************************/


void hist_store (void)
{
  uint8_t index = hist_head++;

  index &= HISTORY_MASK;
  history [index] .seq = hist_seq++;
  history [index] .zeit_minuten_bcd  = zeit_minuten_bcd;
  history [index] .zeit_stunden_bcd  = zeit_stunden_bcd;
  history [index] .bmp280_temp = bmp280_temp;
  history [index] .bmp280_pres = bmp280_pres;
  history [index] .bmp280_humi = bmp280_humi;

  if (hist_level < HISTORY_MAX) {
    hist_level++;
  }
  hist_show = 0;
} /* hist_store */


/******************************************************************************/


void lcd_print_decimal10 (int32_t value)
{
  uint8_t insert_decimal_point = 1;
  uint8_t wp = TEXT_BUFFER_RIGHT;
  uint8_t neg = 0;

  if (value < 0) {
    neg = 1;
    value = -value;
  }

  text_buffer [wp] = 0;

  while (wp) {
    text_buffer [--wp] = '0' + value % 10;
    value /= 10;
    if (insert_decimal_point) {
      text_buffer [--wp] = '.';
      insert_decimal_point = 0;
    }
    else if (value == 0) {
      break;
    }
  }

  if (neg && wp) text_buffer [--wp] = '-';

  while (wp != 0) {
    text_buffer [--wp] = ' ';
  }

  lcd_print_text (text_buffer);
} /* lcd_print_decimal10 */


/******************************************************************************/


void wetterdaten_anzeigen ()
{
  //lcd_set_cursor (0, 2);
  //lcd_print_char ('0'+ hist_level);

  if (hist_show) {
    uint8_t index = hist_head;
    index -= hist_show;
    index &= HISTORY_MASK;

    //history [index] .zeit_minuten_bcd  = zeit_minuten_bcd;
    //history [index] .zeit_stunden_bcd  = zeit_stunden_bcd;

    bmp280_temp = history [index] .bmp280_temp;
    bmp280_pres = history [index] .bmp280_pres;
    bmp280_humi = history [index] .bmp280_humi;

    lcd_set_cursor (0, 2);
    //lcd_print_char ('/');
    lcd_print_char ('0'+ hist_show);

    lcd_print_char (':');
    lcd_print_char (' ');

    zaehler_anzeigen (history [index] .zeit_stunden_bcd);
    lcd_print_char (':');
    zaehler_anzeigen (history [index] .zeit_minuten_bcd);
  }
  else {
    lcd_print_text ("         ");
    bmp280_read ();
  }

  /* Temperatur */
  lcd_set_cursor (10, 0);
  if (bmp280_id) {
    lcd_print_decimal10 (bmp280_temp);
    lcd_print_text (" \337C");
    //lcd_print_char (0xdf);
    //lcd_print_char ('C');
  }

  /* Luftdruck */
  lcd_set_cursor (10, 2);
  if (bmp280_id) {
    lcd_print_decimal10 (bmp280_pres);
    lcd_print_text (" hPa");
  }

  /* Luftfeuchte */
  lcd_set_cursor (10, 1);
  if (bmp280_id == BME280_ID_VAL) {
    lcd_print_decimal10 (bmp280_humi);
    lcd_print_text (" %RH");
  }
} /* wetterdaten anzeigen */


/******************************************************************************/
/* Zeitfunktion & Sekunden hochzählen */


uint8_t bcd_plus_eins (uint8_t x, uint8_t limit)
{
  x += 1;
  if ((x & 0xf) >= 0x0a) { // BCD Überlauf
    x += 0x06;
  }
  if (x >= limit) x = 0;

  return x;
} /* bcd_plus_eins */


uint8_t zeit_weiter_eine_sekunde (void)
{
  zeit_sekunden_bcd = bcd_plus_eins (zeit_sekunden_bcd, 0x60);
  if (zeit_sekunden_bcd == 0) { /* Sekunden Überlauf */
    zeit_minuten_bcd = bcd_plus_eins (zeit_minuten_bcd, 0x60);
    if (zeit_minuten_bcd == 0) { /* Miuten-Überlauf */
      zeit_stunden_bcd = bcd_plus_eins (zeit_stunden_bcd, 0x24);
    }
  }
  return 0;
} /* zeit_weiter_eine_sekunde */


void zaehler_anzeigen (uint8_t zaehler_wert)
{
  uint8_t zehner = (zaehler_wert >> 4) & 0x0f;
  uint8_t einer =   zaehler_wert & 0x0f;

  lcd_print_char ('0' + zehner);
  lcd_print_char ('0' + einer);
} /* zaehler_anzeigen */


/*****************************************************************************************************/


void menu (void)
{
  if ((menu_shown == menu_mode) && (menu_anzeigen == 0)) return;

  lcd_set_cursor (0, 3);

  if (menu_mode == MENU_NORMAL) {
    lcd_print_text ("HIST: ");
    lcd_print_char ('0'+ hist_level);
    lcd_print_text (" F1: < F2: > ");
  }
  else if (menu_mode == MENU_SETTIME) {
    lcd_print_text ("TIME: ");
    zaehler_anzeigen (settime_stunden_bcd);
    lcd_print_char (':');
    zaehler_anzeigen (settime_minuten_bcd);
    lcd_print_text (" #OK    ");
  }

  menu_shown = menu_mode;
  menu_anzeigen = 0;
} /* menu */


/*****************************************************************************************************/


#if defined (__AVR__)
/* AVR GCC */
int
#else
/* Codevision */
void
#endif
 main (void)
{
  /* einmalig ausführen */
  i2c_init ();
  timebase_init ();

  /* enable interrupts */
#if defined (__AVR__)
  /* AVR GCC */
  sei ();
#else
  /* Codevision */
#asm
  sei
#endasm

#endif

  lcd_init ();
  //lcd_backlight_on ();

  bmp280_start ();

  rtc_read ();

  menu_anzeigen = 1;
  menu ();

  time_1000ms_prev = millis ();

  while (1) {                  // Endlosschleife
    uint8_t  zeit_anzeigen = 0;
    uint8_t  wetter_anzeigen = 0;
    uint16_t time_ms = 0;

    time_ms = millis ();

    if ((time_ms - time_100ms_prev) >= 100) { // alle 100ms ausführen
      uint8_t  taste = 0;

      taste = keypad_read ();

      if (taste != KEYCODE_NONE) {
        if (taste == KEYCODE_0) taste = 0;

        if (taste < 10) {
          settime_stunden_bcd <<= 4;
          settime_stunden_bcd |= settime_minuten_bcd >> 4;
          settime_minuten_bcd <<= 4;
          settime_minuten_bcd |= taste;
          menu_mode = MENU_SETTIME;
          menu_anzeigen = 1;
        }
        else if (menu_mode == MENU_SETTIME) {
          if (taste == KEYCODE_OK) {
            if ((settime_minuten_bcd < 0x60) && (settime_stunden_bcd < 0x24)) {
              zeit_sekunden_bcd = 0x00;
              zeit_minuten_bcd  = settime_minuten_bcd;
              zeit_stunden_bcd  = settime_stunden_bcd;
              zeit_anzeigen = 1;
              rtc_write ();
            }
          }

          settime_stunden_bcd = 0;
          settime_minuten_bcd = 0;
          menu_mode = MENU_NORMAL;
        }
        else if (taste == KEYCODE_F4) {
          hist_store ();
          menu_anzeigen = 1;
        }
        else if (taste == KEYCODE_F1) {
          if (hist_show > 0) hist_show--;
          wetter_anzeigen = 1;
        }
        else if (taste == KEYCODE_F2) {
          if (hist_show < hist_level) hist_show++;
          wetter_anzeigen = 1;
        }

        time_100ms_prev = time_ms;
      }

      menu ();
    }

    if ((time_ms - time_1000ms_prev) >= 1000) { // alle Sekunde ausführen
      zeit_weiter_eine_sekunde ();
      zeit_anzeigen = 1;
      time_1000ms_prev += 1000;
      //wetterdaten_anzeigen ();
      wetter_anzeigen = 1;
    }

    if (wetter_anzeigen) {
      wetterdaten_anzeigen ();
      wetter_anzeigen = 0;
    }

    if (zeit_anzeigen) {
      //lcd_set_cursor (12, 0);
      lcd_set_cursor ( 0, 0);
      zaehler_anzeigen (zeit_stunden_bcd);
      lcd_print_char (':');
      zaehler_anzeigen (zeit_minuten_bcd);
      lcd_print_char (':');
      zaehler_anzeigen (zeit_sekunden_bcd);

      zeit_anzeigen = 0;
    }
  }

#if defined (__AVR__)
/* AVR GCC */
  return 0;
#endif
} /* main */


/******************************************************************************/


