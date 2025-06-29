/* Wetterstation mit CodeVision, M. Eisele, 13.02.2025 */

/******************************************************************************/

#include <stdint.h>
#include <string.h>

#include "platform.h"

/******************************************************************************/

#define RTC_I2C_ADDR        0x68

/******************************************************************************/

/* RTC-Chip auslesen, da wir die Uhr im Programmcode rechnen, findet nur ein
   Aufruf beim Start statt. Die Zeit halten wir intern im BCD-Format, sodass auch
   keine weitere Umrechnung erforderlich ist.
   Die Register für Sekunden, Minuten und Stunde liegen im RTC-Chip direkt ab
   der Speicheradresse 0x00 hintereinander sodass jeweil drei aufeinanderfolgende
   I2C-Aufrufe für Schreiben und Lesen ausreichen. */
void rtc_read (void)
{
  uint8_t reg_addr = 0x00;

  i2c_start ();
  i2c_write ((RTC_I2C_ADDR << 1) | I2C_WRITE);
  i2c_write (reg_addr);
  i2c_stop ();

  i2c_start ();
  i2c_write ((RTC_I2C_ADDR << 1) | I2C_READ);
  zeit_sekunden_bcd = i2c_read (1);
  zeit_minuten_bcd  = i2c_read (1);
  zeit_stunden_bcd  = i2c_read (0);
  i2c_stop ();
} /* rtc_read */


/* Speichern der internen Zeitr im RTC-Chip. Wird nur aufgerufen wenn der Anwender
   die Uhrzeit übers Menü eingstellt hat. Auch hier keine Konvertierung erforderlich,
   da die Uhrzeit schon im BCD-Format vorliegt.
   Nebeneffekt: Mit dem Schreiben der Skeunden löschen wir auch gleich das CH Bit
   (clock halt), sodass spätestens ab dann die Uhr im Chip läuft.
 */
void rtc_write (void)
{
  uint8_t reg_addr = 0x00;

  i2c_start ();
  i2c_write ((RTC_I2C_ADDR << 1) | I2C_WRITE);
  i2c_write (reg_addr);
  i2c_write (zeit_sekunden_bcd);
  i2c_write (zeit_minuten_bcd);
  i2c_write (zeit_stunden_bcd);
  i2c_stop ();
} /* rtc_write */


/******************************************************************************/


