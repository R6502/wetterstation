/* Wetterstation mit CodeVision, M. Eisele, 13.02.2025 */

/******************************************************************************/

#include <stdint.h>
#include <string.h>

#include "platform.h"

/******************************************************************************/

#define RTC_I2C_ADDR        0x68

/******************************************************************************/

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


