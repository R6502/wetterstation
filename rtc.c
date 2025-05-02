/* Wetterstation mit CodeVision, M. Eisele, 13.02.2025 */

/******************************************************************************/

#include <stdint.h>
#include <string.h>

#include "platform.h"

/******************************************************************************/

#define RTC_I2C_ADDR        0x68

/******************************************************************************/

uint8_t time_hour = 0,
        time_minutes = 0,
        time_seconds = 0;

/******************************************************************************/


void update_time (void)
{
  time_seconds += 1;
  if (time_seconds >= 60) {
    time_seconds = 0;
    time_minutes += 1;
    if (time_minutes >= 60) {
      time_minutes = 0;
      time_hour += 1;
      if (time_hour >= 24) {
        time_hour = 0;
      }
    }
  }
} /* update_time */


uint8_t uint8_to_bcd (uint8_t value)
{
  return ((value / 10) << 4) | (value % 10);
} /* uint8_to_bcd */


uint8_t uint8_from_bcd (uint8_t value)
{
  return ((value >> 4) & 0x0f) * 10 + (value & 0x0f);
} /* uint8_from_bcd */


void rtc_read (void)
{
  uint8_t reg_addr = 0x00;
  uint8_t reg_seconds = 0x00;
  uint8_t reg_minutes = 0x00;
  uint8_t reg_hours   = 0x00;

  i2c_start ();
  i2c_write ((RTC_I2C_ADDR << 1) | I2C_WRITE);
  i2c_write (reg_addr);
  i2c_stop ();

  i2c_start ();
  i2c_write ((RTC_I2C_ADDR << 1) | I2C_READ);
  reg_seconds = i2c_read (1);
  reg_minutes = i2c_read (1);
  reg_hours   = i2c_read (0);
  i2c_stop ();

  time_seconds = uint8_from_bcd (reg_seconds);
  time_minutes = uint8_from_bcd (reg_minutes);
  time_hour    = uint8_from_bcd (reg_hours);
} /* rtc_read */


void rtc_write (void)
{
  uint8_t reg_addr = 0x00;
  uint8_t reg_seconds = uint8_to_bcd (time_seconds);
  uint8_t reg_minutes = uint8_to_bcd (time_minutes);
  uint8_t reg_hours   = uint8_to_bcd (time_hour);

  i2c_start ();
  i2c_write ((RTC_I2C_ADDR << 1) | I2C_WRITE);
  i2c_write (reg_addr);
  i2c_write (reg_seconds);
  i2c_write (reg_minutes);
  i2c_write (reg_hours);
  i2c_stop ();
} /* rtc_write */


/******************************************************************************/


