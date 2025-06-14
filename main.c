/* Wetterstation mit CodeVision, M. Eisele, 13.02.2025 */

/******************************************************************************/

#include <stdint.h>
#include <string.h>

#include "platform.h"

/******************************************************************************/

uint16_t time_1000ms_prev = 0;
uint16_t time_100ms_prev = 0;

uint8_t tasten_feld = 0;
uint8_t tasten_feld_prev = 0;

uint8_t zeit_sekunden_bcd = 0;
uint8_t zeit_minuten_bcd = 0;
uint8_t zeit_stunden_bcd = 0;

/******************************************************************************/

#define TASTE_NIX               (0)
#define TASTE_SEK               (1)
#define TASTE_MIN               (2)
#define TASTE_H                 (3)
#define TASTE_STORE_RTC         (4)

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
  lcd_set_cursor (10, 3);
  if (bmp280_id == BME280_ID_VAL) {
    int32_to_text_decimal (bmp280_humi, 2);
    insert_decimal_point10 ();
    lcd_print_text (text_buffer + TEXT_BUFFER_RIGHT - 5);
    lcd_print_text (" %RH");
  }
} /* wetterdaten anzeigen */


/******************************************************************************/


uint8_t tastenabfrage (void)
{
  uint8_t tasten_neu = 0;
  uint8_t taste = TASTE_NIX;

  tasten_feld = ~PINK;

  tasten_neu = tasten_feld_prev ^ tasten_feld;
  tasten_neu &= tasten_feld;

  if      (tasten_neu & (1 << 4)) taste = TASTE_SEK;
  else if (tasten_neu & (1 << 5)) taste = TASTE_MIN;
  else if (tasten_neu & (1 << 6)) taste = TASTE_H;
  else if (tasten_neu & (1 << 7)) taste = TASTE_STORE_RTC;

  tasten_feld_prev = tasten_feld;
  return taste;
}


uint8_t bcd_plus_eins (uint8_t x, uint8_t limit)
{
  //uint8_t einer = 0;

  x += 1;
  //einer = x & 0x0f;
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


 /*****************************************************************************************************/
 // Zeitfunktion & Sekunden hochzählen


void zaehler_anzeigen (uint8_t zaehler_wert)
{
  uint8_t einer =   zaehler_wert & 0x0f;
  uint8_t zehner = (zaehler_wert >> 4) & 0x0f;

  lcd_print_char ('0' + zehner);
  lcd_print_char ('0' + einer);
} /* zaehler_anzeigen */


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
  // einmalig ausführen
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

  time_1000ms_prev = millis ();

  while (1) {                  // Endlosschleife
    uint8_t  taste = 0;       // Tastencode
    uint8_t  zeit_anzeigen = 0;
    uint16_t time_ms = 0;

    taste = tastenabfrage ();

    //if (taste == TASTE_BACKLIGHT_OFF) {
    //  lcd_backlight_off ();
    //}
    //
    //else if (taste == TASTE_BACKLIGHT_ON) {
    //  lcd_backlight_on ();
    //}

    //else if (taste == TASTE_YAY_ON) {
    //  lcd_set_cursor (1, 3);
    //  lcd_print_text ("Yay");
    //}

    //else if (taste == TASTE_YAY_OFF) {
    //  lcd_set_cursor (1, 3);
    //  lcd_print_text ("   ");
    //}

    if (taste == TASTE_SEK) {
      zeit_sekunden_bcd = 0x00;
      zeit_anzeigen = 1;
    }

    else if (taste == TASTE_MIN) {
      zeit_minuten_bcd = bcd_plus_eins (zeit_minuten_bcd, 0x60);
      zeit_anzeigen = 1;
    }

    else if (taste == TASTE_H) {
      zeit_stunden_bcd = bcd_plus_eins (zeit_stunden_bcd, 0x24);
      zeit_anzeigen = 1;
    }

    else if (taste == TASTE_STORE_RTC) {
      rtc_write ();
    }

    time_ms = millis ();

    if ((time_ms - time_1000ms_prev) >= 1000) { // alle Sekunde ausführen
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
} /* main */


/******************************************************************************/


