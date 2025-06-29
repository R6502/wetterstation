/* Wetterstation mit CodeVision, M. Eisele, 13.02.2025 */

/******************************************************************************/

#include <stdint.h>
#include <string.h>

#include "platform.h"

/******************************************************************************/
/* Keypad */

#define KEYPAD_I2C_ADDR        0x20

/******************************************************************************/

/* Matrix-Tastatur einlesen: Schrittweise eine der vier Spaltenleiting auf 0 setzen,
   dann vom Portepander die Zeilenleitungen einlesen.
   Ist eine Taste der betreffenden Spalte gedrückt so wird an der entsprechenden
   Zeilen 0 zurückgelesen, ansonsten 1
   Spalten liegen auf Bit0 - 3, Zeilen auf Bit 4 - 7

   Rückgabewerte (Tastencodes in platform.h definiert):

     KEYCODE_NONE    (0x00)

     KEYCODE_1       (0x01)
     KEYCODE_2       (0x02)
     KEYCODE_3       (0x03)
     KEYCODE_4       (0x04)
     KEYCODE_5       (0x05)
     KEYCODE_6       (0x06)
     KEYCODE_7       (0x07)
     KEYCODE_8       (0x08)
     KEYCODE_9       (0x09)
     KEYCODE_0       (0x0a)

     KEYCODE_BACK    (0x0b)
     KEYCODE_OK      (0x0c)

     KEYCODE_F4      (0x0d)
     KEYCODE_F3      (0x0e)
     KEYCODE_F1      (0x0f)
     KEYCODE_F2      (0x10)

   Der Code für eine gedrückte Taste wird nur einmal pro Betätigen der Taste zurückgeliefert.
 */

uint8_t keypad_read (void)
{
  static uint8_t keycode_prev = KEYCODE_NONE;
  uint8_t keycode = KEYCODE_NONE;
  uint8_t col = 0;
  uint8_t num = 1; /* Basiswert für Tastencode */
  uint8_t reg8 = 0;

  for (col = 0; col < 4; col++) {
    /* Spalte auswählen */
    i2c_start ();
    i2c_write ((KEYPAD_I2C_ADDR << 1) | I2C_WRITE);
    i2c_write (~(1 << col)); /* ausgewählte Spalte auf null setzen */
    i2c_stop ();

    /* und nun vom Port-Expander die Werte für die Zeilen einlesen */
    i2c_start ();
    i2c_write ((KEYPAD_I2C_ADDR << 1) | I2C_READ);

    /* in 'reg8' steht nun der 8-Bit-Lesewert vom Port-Expander */
    reg8 = i2c_read (0);

    i2c_stop ();

    reg8 = ~reg8; /* Zeilen-Bits invertieren */

    /* wir wollen nur die oberen 4 Bit (untere sind die Spaltenwerte) */
    reg8 >>= 4;

    if (reg8 != 0) {
      /* Falls eine Taste gedrückt, entsprechenden Code auf den Basiswert aufaddieren und als Tastencode zurückliefern */
      if (reg8 & 1) keycode = num;
      if (reg8 & 2) keycode = num + 1;
      if (reg8 & 4) keycode = num + 2;
      if (reg8 & 8) keycode = num + 3;
      /* Wenn eine Taste gedrückt ist, sind wir fertig */
      break;
    }

    /* Basiswert für nächste Spalte um 4 erhöhen */
    num += 4;
  }

  /* Prüfen ob die gleiche Taste wie beim vorigen Aufruf gedrückt ist, wenn ja dann liefern wir KEYCODE_NONE zurück */
  if (keycode == keycode_prev) {
    return KEYCODE_NONE;
  }

  /* Tastencode für nächsten Aufruf speichern */
  keycode_prev = keycode;

  return keycode;
} /* keypad_read */


/******************************************************************************/


