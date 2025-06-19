/* Wetterstation mit CodeVision, M. Eisele, 13.02.2025 */

/******************************************************************************/

#include <stdint.h>
#include <string.h>

#include "platform.h"

/******************************************************************************/
/* Keypad */

#define KEYPAD_I2C_ADDR        0x20

/******************************************************************************/


uint8_t keypad_read (void)
{
  static uint8_t keycode_prev = KEYCODE_NONE;
  uint8_t keycode = KEYCODE_NONE;
  uint8_t col = 0;
  uint8_t num = 1;
  uint8_t reg8 = 0;

  for (col = 0; col < 4; col++) {
    /* Spalte auswÃ¤hlen */
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
      if (reg8 & 1) keycode = num;
      if (reg8 & 2) keycode = num + 1;
      if (reg8 & 4) keycode = num + 2;
      if (reg8 & 8) keycode = num + 3;
      break;
    }

    num += 4;
  }

  if (keycode == keycode_prev) {
    return KEYCODE_NONE;
  }

  keycode_prev = keycode;

  return keycode;
} /* keypad_read */


/******************************************************************************/


