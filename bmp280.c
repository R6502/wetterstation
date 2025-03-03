/* BMP280 mit CodeVision, M. Eisele, 13.02.2025 */

/******************************************************************************/

#include <stdint.h>
#include <string.h>
#include <io.h>
#include <i2c.h>

#include "platform.h"

/******************************************************************************/

#define BMP280_ADDR     0x76    // can be 0x77 or 0x76



#define I2C_WRITE               0
#define I2C_READ                1

#define BMP280_ID_REG           0xD0
#define BMP280_ID_VAL           0x58

#define BMP280_CAL_REG_FIRST    0x88
#define BMP280_CAL_REG_LAST     0xA1
#define BMP280_CAL_DATA_SIZE    (BMP280_CAL_REG_LAST+1 - BMP280_CAL_REG_FIRST)

#define BMP280_STATUS_REG       0xF3
#define BMP280_CONTROL_REG      0xF4
#define BMP280_CONFIG_REG       0xF5

#define BMP280_PRES_REG         0xF7
#define BMP280_TEMP_REG         0xFA
// #define BMP280_RAWDATA_BYTES    6       // 3 bytes pressure, 3 bytes temperature

#define BMP280_DEBUG(name, val) serial_print_text (name); \
                                serial_print_text ("="); \
                                serial_print_text (int16_to_text_decimal (val)); \
                                serial_print_text ("\r\n");

/******************************************************************************/

int32_t  bmp280_temp;
uint32_t bmp280_pres;
uint8_t  bmp280_ok;


// we create a struct for this, so that we can add support for two
// sensors on a I2C bus
static union _bmp280_cal_union {
         uint8_t bytes [BMP280_CAL_DATA_SIZE];
         struct {
           uint16_t dig_t1;
           int16_t  dig_t2;
           int16_t  dig_t3;
           uint16_t dig_p1;
           int16_t  dig_p2;
           int16_t  dig_p3;
           int16_t  dig_p4;
           int16_t  dig_p5;
           int16_t  dig_p6;
           int16_t  dig_p7;
           int16_t  dig_p8;
           int16_t  dig_p9;
         } v;
       } bmp280_cal;

/******************************************************************************/

void bmp280_write_register (/*uint8_t i2c_addr,*/ uint8_t reg_addr, uint8_t value)
{
  i2c_start ();
  //i2c_write (i2c_addr << 1);
  i2c_write ((BMP280_ADDR << 1) | I2C_WRITE);
  i2c_write (reg_addr);
  i2c_write (value);
  i2c_stop ();
} /* bmp280_write_register */


uint32_t bmp280_read_register (/*uint8_t i2c_addr,*/ uint8_t reg_addr, uint8_t nbytes)
{
  uint32_t result32 = 0;
  uint8_t regval = 0;

  if ((nbytes == 0) || (nbytes > 4)) return result32;

  i2c_start ();
  i2c_write ((BMP280_ADDR << 1) | I2C_WRITE);
  //i2c_write (i2c_addr << 1);
  i2c_write (reg_addr);
  i2c_stop ();

  i2c_start ();
  //i2c_write ((i2c_addr << 1) | 1);
  i2c_write((BMP280_ADDR << 1) | I2C_READ);

  while (nbytes > 0) {
    uint8_t ack = 1;

    if (nbytes == 1) ack = 0;

    regval = i2c_read (ack);

    result32 <<= 8;
    result32 |= regval;

    nbytes -= 1;
  }

  i2c_stop ();

  return result32;
} /* bmp280_read_register */


void bmp280_readmem (uint8_t reg, uint8_t buff[], uint8_t bytes)
{
  uint8_t i = 0;

  i2c_start ();
  i2c_write ((BMP280_ADDR << 1) | I2C_WRITE);
  i2c_write (reg);
  i2c_stop ();

  i2c_start ();
  i2c_write ((BMP280_ADDR << 1) | I2C_READ);

  for (i = 0; i < bytes; i++) {
    uint8_t ack = 1;
    if (i == (bytes - 1)) ack = 0;
    buff[i] = i2c_read (ack);
  }

  i2c_stop();
} /* bmp280_readmem */


// static void bmp280_getcalibration (void)
// {
//   //memset (bmp280_cal.bytes, 0, sizeof (bmp280_cal));
//
//   bmp280_readmem (BMP280_CAL_REG_FIRST,
//                   bmp280_cal.bytes,
//                   BMP280_CAL_DATA_SIZE);
//
//   //BMP280_DEBUG("T1", bmp280_cal.v.dig_t1);
//   //BMP280_DEBUG("T2", bmp280_cal.v.dig_t2);
//   //BMP280_DEBUG("T3", bmp280_cal.v.dig_t3);
//
//   //BMP280_DEBUG("P1", bmp280_cal.v.dig_p1);
//   //BMP280_DEBUG("P2", bmp280_cal.v.dig_p2);
//   //BMP280_DEBUG("P3", bmp280_cal.v.dig_p3);
//   //BMP280_DEBUG("P4", bmp280_cal.v.dig_p4);
//   //BMP280_DEBUG("P5", bmp280_cal.v.dig_p5);
//   //BMP280_DEBUG("P6", bmp280_cal.v.dig_p6);
//   //BMP280_DEBUG("P7", bmp280_cal.v.dig_p7);
//   //BMP280_DEBUG("P8", bmp280_cal.v.dig_p8);
//   //BMP280_DEBUG("P9", bmp280_cal.v.dig_p9);
// } /* bmp280_getcalibration */


void bmp280_set_ctrl (uint8_t osrs_t, uint8_t osrs_p, uint8_t mode)
{
  bmp280_write_register (BMP280_CONTROL_REG,
                      ((osrs_t & 0x7) << 5)
                    | ((osrs_p & 0x7) << 2)
                    | (mode & 0x3)
        );
} /* bmp280_set_ctrl */


void bmp280_set_config (uint8_t t_sb, uint8_t filter, uint8_t spi3w_en)
{
  bmp280_write_register (BMP280_CONFIG_REG,
                            ((t_sb & 0x7) << 5)
                         | ((filter & 0x7) << 2)
                         | (spi3w_en & 1)
        );
} /* bmp280_set_config */


void bmp280_start (void)
{
  uint8_t mode = 0;

  mode |= 3; /* normal mode */
  mode |= 1 << 2; /* osrs_p = 1 -> 16bit */
  mode |= 1 << 5; /* osrs_r = 1 -> 16bit */

  bmp280_write_register (0xf4, mode);

  //bmp280_getcalibration ();
  bmp280_readmem (BMP280_CAL_REG_FIRST,
                  bmp280_cal.bytes,
                  BMP280_CAL_DATA_SIZE);
  bmp280_set_config (0, 4, 0); // 0.5 ms delay, 16x filter, no 3-wire SPI
  bmp280_set_ctrl (2, 5, 3); // T oversample x2, P over x2, normal mode
} /* bmp280_start  */


//uint8_t bmp280_get_status (void)
//{
//  uint8_t data[1];
//  bmp280_readmem (BMP280_STATUS_REG, data, 1);
//  //BMP280_DEBUG ("Status", data[0]);
//  return data[0];
//}


void bmp280_read (void)
{
  uint32_t temp_raw = 0;
  uint32_t pres_raw = 0;
  uint8_t value8 = 0x00;
  int32_t  var1, var2, t_fine;//, x;

  bmp280_ok = 1;

  value8   = bmp280_read_register (0xd0, 1); /* ID */


  if (value8 != BMP280_ID_VAL) {
    bmp280_ok = 0;
    bmp280_temp = 0;
    bmp280_pres = 0;
    return;
  }

  temp_raw = bmp280_read_register (0xfa, 3); /* temp */
  pres_raw = bmp280_read_register (0xf7, 3); /* press */

  temp_raw >>= 4;
  pres_raw >>= 4;

  // serial_print_text ("B:");
  // serial_print_text (uint8_to_text_hex (value8));
  // serial_print_text (", ");
  // serial_print_text (uint32_to_text_decimal (temp_raw));
  // serial_print_text (", ");
  // serial_print_text (uint32_to_text_decimal (pres_raw));
  // serial_print_text ("\r\n");

  /* The following code is based on a 32-bit integer code from the BMP280 datasheet */

  /* compute the temperature */
  var1 = ((((temp_raw >> 3) - ((int32_t)bmp280_cal.v.dig_t1 << 1)))
          * ((int32_t)bmp280_cal.v.dig_t2)); // >> 11;

  if (var1 < 0) {
    var1 = -var1;
    var1 >>= 11;
    var1 = -var1;
  }
  else {
    var1 >>= 11;
  }

  var2 = ((temp_raw >> 4) - ((int32_t)bmp280_cal.v.dig_t1));
  var2 *= var2;
  var2 >>= 12;
  var2 *= ((int32_t)bmp280_cal.v.dig_t3);

  //var2 = (((((temp_raw >> 4) - ((int32_t)bmp280_cal.v.dig_t1))
  //        * ((temp_raw >> 4) - ((int32_t)bmp280_cal.v.dig_t1))) >> 12)
  //        * ((int32_t)bmp280_cal.v.dig_t3));
  //        //* ((int32_t)bmp280_cal.v.dig_t3)) >> 14;

  if (var2 < 0) {
    var2 = -var2;
    var2 >>= 14;
    var2 = -var2;
  }
  else {
    var2 >>= 14;
  }

  t_fine = var1 + var2;

  bmp280_temp = (t_fine * 5 + 128) >> 8;


   //serial_print_text ("t=");
   //serial_print_text (int32_to_text_decimal (bmp280_temp));
   //serial_print_text ("\r\n");

#if 1
  // compute the pressure
  var1 = (((int32_t)t_fine) >> 1) - (int32_t)64000;
  var2 = (((var1 >> 2) * (var1 >> 2)) >> 11) * ((int32_t)bmp280_cal.v.dig_p6);
  var2 = var2 + ((var1 * ((int32_t)bmp280_cal.v.dig_p5)) << 1);
  var2 = (var2 >> 2) + (((int32_t)bmp280_cal.v.dig_p4) << 16);
  var1 = (((bmp280_cal.v.dig_p3 * (((var1 >> 2) * (var1 >> 2)) >> 13)) >> 3)
          + ((((int32_t)bmp280_cal.v.dig_p2) * var1) >> 1)) >> 18;
  var1 = ((((32768 + var1)) * ((int32_t)bmp280_cal.v.dig_p1)) >> 15);

  if (var1 == 0) {
    bmp280_pres = 0;
  }
  else {
    bmp280_pres = (((uint32_t)(((int32_t)1048576)-pres_raw) - (var2 >> 12))) * 3125;
    if (bmp280_pres < 0x80000000) {
      bmp280_pres = (bmp280_pres << 1) / ((uint32_t)var1);
    }
    else {
      bmp280_pres = (bmp280_pres / (uint32_t)var1) * 2;
    }
    var1 = (((int32_t)bmp280_cal.v.dig_p9) * ((int32_t)(((bmp280_pres>>3) * (bmp280_pres >> 3)) >> 13))) >> 12;
    var2 = (((int32_t)(bmp280_pres >> 2)) * ((int32_t)bmp280_cal.v.dig_p8)) >> 13;
    bmp280_pres = (uint32_t)((int32_t)bmp280_pres + ((var1 + var2 + bmp280_cal.v.dig_p7) >> 4));
  }

#endif
   //serial_print_text ("p =");
   //serial_print_text (uint32_to_text_decimal (bmp280_pres));
   //serial_print_text ("\r\n");
} /* bmp280_read */


/******************************************************************************/


