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


void wetterdaten_anzeigen ()
{
  bmp280_read ();

  /* Temperatur */
  lcd_set_cursor (11, 1);
  if (bmp280_id) {
    int32_to_text_decimal (bmp280_temp, 2);
    insert_decimal_point10 ();
    lcd_print_text (text_buffer + TEXT_BUFFER_RIGHT - 4);
    lcd_print_text (" ");
    lcd_print_char (0xdf);
    lcd_print_char ('C');
  }

  /* Luftdruck */
  lcd_set_cursor (10, 2);
  if (bmp280_id) {
    int32_to_text_decimal (bmp280_pres, 2);
    insert_decimal_point10 ();
    lcd_print_text (text_buffer + TEXT_BUFFER_RIGHT - 5);
    lcd_print_text (" hPa");
  }

  /* Luftfeuchte */
  lcd_set_cursor (0, 1);
  if (bmp280_id == BME280_ID_VAL) {
    int32_to_text_decimal (bmp280_humi, 2);
    insert_decimal_point10 ();
    lcd_print_text (text_buffer + TEXT_BUFFER_RIGHT - 5);
    lcd_print_text (" %RH");
  }
} /* wetterdaten anzeigen */


/******************************************************************************/
/* Zeitfunktion & Sekunden hochz�hlen */


uint8_t bcd_plus_eins (uint8_t x, uint8_t limit)
{
  x += 1;
  if ((x & 0xf) >= 0x0a) { // BCD �berlauf
    x += 0x06;
  }
  if (x >= limit) x = 0;

  return x;
} /* bcd_plus_eins */


uint8_t zeit_weiter_eine_sekunde (void)
{
  zeit_sekunden_bcd = bcd_plus_eins (zeit_sekunden_bcd, 0x60);
  if (zeit_sekunden_bcd == 0) { /* Sekunden �berlauf */
    zeit_minuten_bcd = bcd_plus_eins (zeit_minuten_bcd, 0x60);
    if (zeit_minuten_bcd == 0) { /* Miuten-�berlauf */
      zeit_stunden_bcd = bcd_plus_eins (zeit_stunden_bcd, 0x24);
    }
  }
  return 0;
} /* zeit_weiter_eine_sekunde */


void zaehler_anzeigen (uint8_t zaehler_wert)
{
  uint8_t einer =   zaehler_wert & 0x0f;
  uint8_t zehner = (zaehler_wert >> 4) & 0x0f;

  lcd_print_char ('0' + zehner);
  lcd_print_char ('0' + einer);
} /* zaehler_anzeigen */


/*****************************************************************************************************/


void menu (void)
{
  if ((menu_shown == menu_mode) && (menu_anzeigen ==0)) return;

  lcd_set_cursor (0, 3);

  if (menu_mode == MENU_NORMAL) {
    //lcd_print_text ("F1: PREV F2: NEXT  ");
    lcd_print_text ("HIST F1: + F2: -   ");
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
  /* einmalig ausf�hren */
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
  lcd_backlight_on ();

  bmp280_start ();

  rtc_read ();

  menu_anzeigen = 1;
  menu ();

  time_1000ms_prev = millis ();

  while (1) {                  // Endlosschleife
    uint8_t  zeit_anzeigen = 0;
    uint16_t time_ms = 0;

    time_ms = millis ();

    if ((time_ms - time_100ms_prev) >= 100) { // alle 100ms ausf�hren
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

        time_100ms_prev = time_ms;
      }

      menu ();
    }

    if ((time_ms - time_1000ms_prev) >= 1000) { // alle Sekunde ausf�hren
      zeit_weiter_eine_sekunde ();
      zeit_anzeigen = 1;
      time_1000ms_prev += 1000;
      wetterdaten_anzeigen ();
    }

    if (zeit_anzeigen != 0) {
      lcd_set_cursor (12, 0);
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


