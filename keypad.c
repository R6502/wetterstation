/* Wetterstation mit CodeVision, M. Eisele, 13.02.2025 */

/******************************************************************************/

#include <stdint.h>
#include <string.h>

#include "platform.h"

/******************************************************************************/
/* Keypad */

#define KEYPAD_I2C_ADDR        0x20

/******************************************************************************/

static uint16_t keymask_prev = 0;

/******************************************************************************/


/* Keypad einlesen, Ergebnis 16Bit, für jede der 16 Tasten ein Bit */
uint16_t keypad_read_mask (void)
{
  uint16_t keymask = 0x0000;
  uint8_t col = 0;
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
    reg8 &= 0x0f; /* untere 4 Bit maskieren (eigentlich nicht nötig) */

    /* nun die neu eingelesenen Zeilenwerte zum Ergebnis hinzufügen */
    keymask <<= 4;
    keymask |= reg8;
  }

  return keymask;
} /* keypad_read_mask */


/* Liefert den ASCII-Code der letzten gedrückten Taste zurück */
uint8_t keypad_read (void)
{
  uint8_t keycode = 0;
  uint16_t keymask = 0;

  /* Tastenmatrix einlesen */
  keymask = keypad_read_mask ();

  if (keymask && (keymask_prev != keymask)) {
    static uint8_t keycodes [] = " A321B654C987D#0*";
    uint8_t keynum = 0;
    uint16_t changed = keymask_prev ^ keymask;

    changed &= keymask;

    while (changed) {
      keynum ++;
      changed >>= 1;
    }

    if (keynum < sizeof (keycodes)) {
      keycode = keycodes [keynum];
    }

    // serial_print_text ("K: ");
    // serial_print_text (rightmost (uint32_to_text_hex (keymask), 4));
    //
    // serial_print_text (" -> ");
    // serial_print_text (rightmost (int32_to_text_decimal (keynum, 0), 2));
    //
    // if (keycode) {
    //   serial_print_text (", ");
    //   uart0_tx (keycode);
    // }
    //
    // serial_print_text ("\r\n");
  }

  keymask_prev = keymask;

  return keycode;
} /* keypad_read */


/******************************************************************************/


