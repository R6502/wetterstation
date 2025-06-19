/* Software-I2C mit CodeVision, M. Eisele, 13.02.2025 */

/***************************************************************************/

#include <stdio.h>
#include <string.h>

#include "platform.h"

/***************************************************************************/

/* Delay function argument values */
#define ISL12025_FULL_DELAY     (20)
#define ISL12025_HALF_DELAY     (10)
#define ISL12025_QRTR_DELAY     ( 5)

#define SCL_HIGH                { PORTB |=  (1 << scl); }
#define SCL_LOW                 { PORTB &= ~(1 << scl); }
#define SDA_HIGH                { PORTB |=  (1 << sda); }
#define SDA_LOW                 { PORTB &= ~(1 << sda); }
#define MAKE_SDA_IN             { DDRB &= ~(1 << sda); PORTB |= (1 << sda); } /* Make SDA as input + Pullup ON */
#define MAKE_SDA_OUT            { DDRB |=  (1 << sda);  } /* Make SDA as output */
#define INP_SDAT                ( (PINB & (1 << sda)) != 0)  // 1 or true if SDAT is high() FOR ACK reading

/***************************************************************************/

static void i2c_delay (uint16_t delay_us);

/***************************************************************************/

static uint8_t sda = 0,
               scl = 0;

/***************************************************************************/


void i2c_init_ext (uint8_t sda_line, uint8_t scl_line)
{
  sda = sda_line;
  scl = scl_line;

  DDRB |=  (1 << scl);
  MAKE_SDA_OUT
} /* i2c_init_ext */


void i2c_init ()
{
  //i2c_init_ext (PB5, PB4);
  i2c_init_ext (5, 4);


  //sda = sda_line;
  //scl = scl_line;
  //
  //MAKE_SDA_OUT
} /* i2c_init */


void i2c_start ()
{
  MAKE_SDA_OUT
  SDA_HIGH
  i2c_delay (ISL12025_QRTR_DELAY);
  SCL_HIGH
  i2c_delay (ISL12025_QRTR_DELAY);
  SDA_LOW
  i2c_delay (ISL12025_QRTR_DELAY);
} /* i2c_start */


void i2c_stop ()
{
  MAKE_SDA_OUT
  SDA_LOW
  SCL_HIGH
  i2c_delay (ISL12025_QRTR_DELAY);
  SDA_HIGH
  i2c_delay (ISL12025_QRTR_DELAY);
} /* i2c_stop */


uint8_t i2c_write (uint8_t data)
{
  uint8_t r = 1;
  uint8_t mask = 0x80;

  while (mask != 0) {
    SCL_LOW
    MAKE_SDA_OUT

    i2c_delay (ISL12025_QRTR_DELAY);

    if ((data & mask) != 0) {
      SDA_HIGH
    }
    else {
      SDA_LOW
    }

    i2c_delay (ISL12025_QRTR_DELAY);

    SCL_HIGH
    i2c_delay (ISL12025_HALF_DELAY);

    mask >>= 1;
  }

  SCL_LOW

  MAKE_SDA_IN
  i2c_delay (ISL12025_HALF_DELAY);

  SCL_HIGH
  i2c_delay (ISL12025_HALF_DELAY);

  if (INP_SDAT) {
    MAKE_SDA_OUT
    SCL_LOW
    i2c_delay (ISL12025_FULL_DELAY);

    /* No ACK so return with error */
    // serial_print_text ("I2C: No ACK\r\n");
    //debug_print_string ("I2C: No ACK from Device ..... ");
    //debug_print_crlf ();
    r =  0;
  }

  SCL_LOW
  i2c_delay (ISL12025_FULL_DELAY);
  MAKE_SDA_OUT

  return r;
} /* i2c_write */


uint8_t i2c_read (uint8_t send_ack)
{
  uint8_t data = 0;
  int i;

  for (i = 0; i < 8; i++) {
    data <<= 1;
    SCL_LOW
    MAKE_SDA_IN
    i2c_delay (ISL12025_HALF_DELAY);
    SCL_HIGH
    i2c_delay (ISL12025_QRTR_DELAY);
    if (INP_SDAT) data |=  0x01;
    i2c_delay (ISL12025_QRTR_DELAY);
  }

  /* Send ACK */
  SCL_LOW
  MAKE_SDA_OUT
  i2c_delay (ISL12025_HALF_DELAY);

  if (send_ack) {
    SDA_LOW
  }
  else {
    SDA_HIGH
  }

  i2c_delay (ISL12025_HALF_DELAY);
  SCL_HIGH
  i2c_delay (ISL12025_HALF_DELAY);

  return data;
} /* i2c_read */


static void i2c_delay (uint16_t delay_us)
{
  mdelay_us (delay_us);
} /* i2c_delay */


/***************************************************************************/


// static int i2c_write_data (unsigned int device_address, unsigned char *data, unsigned int length)
// {
//   int r = 0;
//
//   i2c_start ();
//
//   r = i2c_write (device_address);
//
//   if (r == 0) {
//     while (length > 0) {
//       r = i2c_write (*data++);
//       length -= 1;
//       if (r != 0) break;
//     }
//   }
//
//   i2c_stop ();
//
//   if (r != 0) {
//     debug_print_string ("SOFTI2CI failed, device_addr=");
//     debug_print_hex_byte (device_address, 1);
//     debug_print_string (", r=");
//     debug_print_int (r);
//     debug_print_crlf ();
//   }
//
//   return r;
// } /* i2c_write_data */


// static int i2c_read_data (unsigned int device_address, unsigned char *data, unsigned int length)
// {
//   int r = 0;
//
//   i2c_start ();
//
//   r = i2c_write (device_address | I2C_READ);
//
//   if (r != 0) {
//     debug_print_string ("SOFTI2CI failed, device_addr=");
//     debug_print_hex_byte (device_address, 1);
//     debug_print_string (", r=");
//     debug_print_int (r);
//     debug_print_crlf ();
//     goto skip_read;
//   }
//
//   while (length > 0) {
//     length -= 1;
//     r = i2c_read (data, (length != 0));
//     data += 1;
//   }
//
// skip_read:
//
//   i2c_stop ();
//
//   return r;
// } /* i2c_read_data */


/***************************************************************************/


