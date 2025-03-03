/* Wetterstation mit CodeVision, M. Eisele, 13.02.2025 */

/******************************************************************************/

#include <stdint.h>
#include <string.h>
#include <io.h>
#include <i2c.h>

#include "platform.h"
//#include "display.h"

/******************************************************************************/

uint16_t time_1000ms_prev = 0;
uint16_t time_100ms_prev = 0;


uint8_t tasten_feld = 0;
uint8_t tasten_feld_prev = 0;
  // von Marie


uint8_t zeit_sekunden_bcd = 0;
uint8_t zeit_minuten_bcd = 0;
uint8_t zeit_stunden_bcd = 0;

  // ende Marie


/******************************************************************************/

#define TASTE_NIX               (0)
#define TASTE_BACKLIGHT_OFF     (1)
#define TASTE_BACKLIGHT_ON      (2)
#define TASTE_BACKLIGHT_ONOFF   (3)
#define TASTE_YAY_ON            (4)
#define TASTE_YAY_OFF           (5)
#define TASTE_SEK               (6)
#define TASTE_MIN               (7)
#define TASTE_H                 (8)

// Beispiel
//#define TASTE_NAECHSTER_WERT    (20)
//#define TASTE_VORHERIGER_WERT   (21)

/******************************************************************************/


 // von Marie
uint8_t tastenabfrage (void)
 {
   uint8_t tasten_neu = 0;
   uint8_t taste = TASTE_NIX;

   tasten_feld = ~PINK;

   tasten_neu = tasten_feld_prev ^ tasten_feld;
   tasten_neu &= tasten_feld;

   //if      (tasten_neu & (1 << 0)) taste = TASTE_BACKLIGHT_ON;
   if      (tasten_neu & (1 << 0)) taste = TASTE_BACKLIGHT_ONOFF;
   //else if (tasten_neu & (1 << 1)) taste = TASTE_BACKLIGHT_OFF;
   else if (tasten_neu & (1 << 2)) taste = TASTE_YAY_ON;
   else if (tasten_neu & (1 << 3)) taste = TASTE_YAY_OFF;
   else if (tasten_neu & (1 << 4)) taste = TASTE_SEK;
   else if (tasten_neu & (1 << 5)) taste = TASTE_MIN;
   else if (tasten_neu & (1 << 6)) taste = TASTE_H;

   tasten_feld_prev = tasten_feld;
   return taste;
 }


 uint8_t bcd_plus_eins (uint8_t x)
 {
   uint8_t einer = 0;

   x += 1;
   einer = x & 0x0f;
   if (einer > 9) { // BCD Überlauf
     uint8_t zehner = 0;
     x &= ~0x0f; // Einer wieder auf 0
     x += 0x10;
     zehner = (x >> 4) & 0x0f;
     if (zehner > 9) x = 0;
   }

   return x;
 } /* bcd_plus_eins */



uint8_t zeit_weiter_eine_sekunde (void)
 {
   zeit_sekunden_bcd = bcd_plus_eins (zeit_sekunden_bcd);

   if (zeit_sekunden_bcd == 0x60) {
     zeit_sekunden_bcd = 0x00;
     zeit_minuten_bcd = bcd_plus_eins (zeit_minuten_bcd);
   }

   if (zeit_minuten_bcd == 0x60) {
     zeit_minuten_bcd = 0x00;
     zeit_stunden_bcd = bcd_plus_eins (zeit_stunden_bcd);
   }

   if (zeit_stunden_bcd == 0x24) {
     zeit_stunden_bcd = 0x00;
   }

   // Überlauf?
   /*
   uint8_t einer = 0;
   zeit_sekunden += 1;
   einer = zeit_sekunden & 0x0f;
   if (einer > 9) { // BCD Überlauf
     uint8_t zehner = 0;
     zeit_sekunden &= ~0x0f; // Einer wieder auf 0
     zeit_sekunden += 0x10;
     zehner = (zeit_sekunden >> 4) & 0x0f;
     if (zehner > 9) zeit_sekunden = 0;
   }
   */
   //zeit_sekunden ++;
   //zeit_sekunden = zeit_sekunden + 1:
   return 0;
 }


 /*****************************************************************************************************/
 // Zeitfunktion & Sekunden hochzählen


void zaehler_anzeigen (uint8_t zaehler_wert)
{
  /*
  char text [5];

  text [0] = 'a';
  text [1] = 'b';
  text [2] = 'c';
  text [3] = 'd';
  text [4] = 0;

  lcd_print_text (text);
  */

  uint8_t einer =   zaehler_wert & 0x0f;
  uint8_t zehner = (zaehler_wert >> 4) & 0x0f;

  lcd_print_char ('0' + zehner);
  lcd_print_char ('0' + einer);

  /*
  lcd_print_char (' ');

  lcd_print_char ('0' + ((einer >> 3) & 1));
  lcd_print_char ('0' + ((einer >> 2) & 1));
  lcd_print_char ('0' + ((einer >> 1) & 1));
  lcd_print_char ('0' + ((einer >> 0) & 1));

  lcd_print_char (' ');

  if (einer < 10) lcd_print_char ('0' + einer);
             else lcd_print_char ('a' + einer - 10);
  */
} /* zaehler_anzeigen */


void main (void)
{
  // einmalig ausführen
  i2c_init ();
  timebase_init ();

  DDRB |= (1 << 7);
  PORTB &= ~(1 << 7);

  sei ();

  lcd_init ();

  // write_char ('1');
  // write_char ('2');
  // write_char ('3');
  // write_char (' ');
  //
  // write_char ('H');
  // write_char ('a');
  // write_char ('l');
  // write_char ('l');
  // write_char ('o');
  //
  // while (1) {
  // }

  lcd_backlight_on ();

  // lcd_set_cursor (7, 1);
  // lcd_print_char ('H');

  lcd_set_cursor (1, 0);
  lcd_print_text ("Hallihallo:");

  //lcd_set_cursor (1, 2);
  //lcd_print_text ("blub");

  time_1000ms_prev = millis ();

  while (1) {                  // Endlosschleife
    uint8_t  taste = 0;       // Tastencode
    uint8_t  zeit_anzeigen = 0;
    uint16_t time_ms = 0;

    taste = tastenabfrage ();

    if (taste == TASTE_BACKLIGHT_OFF) {
      lcd_backlight_off ();
    }

    else if (taste == TASTE_BACKLIGHT_ON) {
      lcd_backlight_on ();
    }

    else if (taste == TASTE_BACKLIGHT_ONOFF) {
      lcd_backlight_onoff ();
    }

    else if (taste == TASTE_YAY_ON) {
      lcd_set_cursor (1, 3);
      lcd_print_text ("Yay");
    }


    else if (taste == TASTE_YAY_OFF) {
      lcd_set_cursor (1, 3);
      lcd_print_text ("   ");
    }

    else if (taste == TASTE_SEK) {
      zeit_sekunden_bcd = 0x00;
      zeit_anzeigen = 1;
    }

    else if (taste == TASTE_MIN) {
      zeit_minuten_bcd = bcd_plus_eins (zeit_minuten_bcd);
      if (zeit_minuten_bcd == 0x60) {
        zeit_minuten_bcd = 0x00;
      }
      zeit_anzeigen = 1;
    }

    else if (taste == TASTE_H) {
      zeit_stunden_bcd = bcd_plus_eins (zeit_stunden_bcd);
      if (zeit_stunden_bcd == 0x24) {
        zeit_stunden_bcd = 0x00;
      }
      zeit_anzeigen = 1;
    }

    time_ms = millis ();

    if ((time_ms - time_1000ms_prev) >= 1000) { // alle Sekunde ausführen
      zeit_weiter_eine_sekunde ();
      zeit_anzeigen = 1;
      time_1000ms_prev += 1000;
    }

    if (zeit_anzeigen != 0) {
      lcd_set_cursor (1, 2);
      zaehler_anzeigen (zeit_stunden_bcd);
      lcd_print_char (':');
      zaehler_anzeigen (zeit_minuten_bcd);
      lcd_print_char (':');
      zaehler_anzeigen (zeit_sekunden_bcd);

      zeit_anzeigen = 0;
   }


    if ((time_ms - time_100ms_prev) >= 100) {
      PORTB ^= (1 << 7);
      time_100ms_prev = time_ms;
    }

  }
} /* main */


/******************************************************************************/


