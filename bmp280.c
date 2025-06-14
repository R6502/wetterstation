/* BMP280 mit CodeVision, M. Eisele, 13.02.2025 */

/******************************************************************************/

#include <stdint.h>
#include <string.h>

#include "platform.h"

/******************************************************************************/

#define BMP280_ADDR     0x76    // can be 0x77 or 0x76

#define BMP280_ID_REG           0xd0

#define BMP280_CAL_REG_FIRST    0x88
#define BMP280_CAL_REG_LAST     0xa1
#define BMP280_CAL_DATA_SIZE    (BMP280_CAL_REG_LAST+1 - BMP280_CAL_REG_FIRST)

/* extra BME280 */
#define BME280_CAL_HUM_REG_FIRST 0xe1
#define BME280_CAL_HUM_REG_LAST  0xe6
#define BME280_CAL_HUM_DATA_SIZE (BME280_CAL_HUM_REG_LAST + 1 - BME280_CAL_HUM_REG_FIRST)

#define BMP280_CONTROL_HUMI_REG 0xf2
#define BMP280_STATUS_REG       0xf3
#define BMP280_CONTROL_REG      0xf4
#define BMP280_CONFIG_REG       0xf5

#define BMP280_PRES_REG         0xf7
#define BMP280_TEMP_REG         0xfA
#define BMP280_HUMI_REG         0xfD

/******************************************************************************/

int32_t  bmp280_temp;
uint32_t bmp280_pres;
uint32_t bmp280_humi;
uint8_t  bmp280_id;

/******************************************************************************/

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

           /*! Calibration coefficients for the humidity sensor (2 byte extra) */
           uint8_t dig_reserved;
           uint8_t dig_h1;
         } v;
       } bmp280_cal;


static struct _bme280_cal_hum_struct {
         /*! Calibration coefficients for the humidity sensor (9 byte extra) */
         int16_t dig_h2;
         uint8_t dig_h3;
         int16_t dig_h4;
         int16_t dig_h5;
         int8_t  dig_h6;
       } bme280_cal_hum;

/******************************************************************************/


void bmp280_write_register (uint8_t reg_addr, uint8_t value)
{
  i2c_start ();
  i2c_write ((BMP280_ADDR << 1) | I2C_WRITE);
  i2c_write (reg_addr);
  i2c_write (value);
  i2c_stop ();
} /* bmp280_write_register */


uint32_t bmp280_read_register (uint8_t reg_addr, uint8_t nbytes)
{
  uint32_t result32 = 0;
  uint8_t regval = 0;

  if ((nbytes == 0) || (nbytes > 4)) return result32;

  i2c_start ();
  i2c_write ((BMP280_ADDR << 1) | I2C_WRITE);
  i2c_write (reg_addr);
  i2c_stop ();

  i2c_start ();
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


void bmp280_start (void)
{
  //uint8_t mode = 0;
  //
  //mode |= 3; /* normal mode */
  //mode |= 1 << 2; /* osrs_p = 1 -> 16bit */
  //mode |= 1 << 5; /* osrs_r = 1 -> 16bit */

  bmp280_id = bmp280_read_register (BMP280_ID_REG, 1); /* ID @d8 */

  if ((bmp280_id != BMP280_ID_VAL) && (bmp280_id != BME280_ID_VAL)) {
    bmp280_id = 0;
    return;
  }

  //bmp280_write_register (0xf4, mode);

  bmp280_readmem (BMP280_CAL_REG_FIRST,
                  bmp280_cal.bytes,
                  BMP280_CAL_DATA_SIZE);

  if (bmp280_id == BME280_ID_VAL) {
    static uint8_t bytes [BME280_CAL_HUM_DATA_SIZE];

    bmp280_readmem (BME280_CAL_HUM_REG_FIRST,
                    bytes,
                    BME280_CAL_HUM_DATA_SIZE);

    bme280_cal_hum.dig_h2 = bytes [1]; /* E2 */
    bme280_cal_hum.dig_h2 <<= 8;
    bme280_cal_hum.dig_h2 |= bytes [0]; /* E1 */

    bme280_cal_hum.dig_h3 = bytes [2]; /* E3 */

    bme280_cal_hum.dig_h4 = bytes [3]; /* E4 */
    bme280_cal_hum.dig_h4 <<= 4;
    bme280_cal_hum.dig_h4 |= bytes [4] & 0x0f; /* E5 */

    bme280_cal_hum.dig_h5 = bytes [5]; /* E6 */
    bme280_cal_hum.dig_h5 <<= 4;
    bme280_cal_hum.dig_h5 |= (bytes [4] >> 4) & 0x0f; /* E5 */

    bme280_cal_hum.dig_h6 = bytes [5]; /* E7 */
  }

  if (bmp280_id == BME280_ID_VAL) {
    /* oversampling * 16 */
    bmp280_write_register (BMP280_CONTROL_HUMI_REG, 5);
  }

  /* 0.5 ms delay, 16x filter, no 3-wire SPI */
  bmp280_write_register (BMP280_CONFIG_REG,
                            (0 << 5)  /* t_sb     = 0 */
                         |  (4 << 2)  /* filter   = 4 */
                         |   0);      /* spi3w_en = 0 */


  /* T oversample x2, P over x2, normal mode */
  bmp280_write_register (BMP280_CONTROL_REG,
                      (2 << 5)   /* osrs_t = 2 */
                    | (5 << 2)   /* osrs_p = 5 */
                    |  3 );      /* mode   = 3 */
} /* bmp280_start  */


void bmp280_read (void)
{
  uint32_t temp_raw = 0;
  uint32_t pres_raw = 0;
  uint32_t humi_raw = 0;
  int32_t  var1, var2, t_fine;//, x;

  bmp280_temp = 0;
  bmp280_pres = 0;
  bmp280_humi = 0;

  if ((bmp280_id != BMP280_ID_VAL) && (bmp280_id != BME280_ID_VAL)) {
    return;
  }

  temp_raw = bmp280_read_register (0xfa, 3); /* temp */
  pres_raw = bmp280_read_register (0xf7, 3); /* press */

  if (bmp280_id == BME280_ID_VAL) {
    humi_raw = bmp280_read_register (0xfd, 2); /* humidity */
  }

  temp_raw >>= 4;
  pres_raw >>= 4;

  /* compute the temperature */
  var1 = ((((temp_raw >> 3) - ((int32_t)bmp280_cal.v.dig_t1 << 1))) * ((int32_t)bmp280_cal.v.dig_t2)) ;
  var1 /= 2048;

  var2 = ((temp_raw >> 4) - ((int32_t)bmp280_cal.v.dig_t1));

  var2 *= var2;

  var2 >>= 12;
  var2 *= ((int32_t)bmp280_cal.v.dig_t3);
  var2 /= 16384;

  t_fine = var1 + var2;

  //bmp280_temp = (t_fine * 5 + 128) >> 8;
  bmp280_temp = (t_fine + 256) >> 9;

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
    //bmp280_pres = (((uint32_t)(((int32_t)1048576) - pres_raw) - (var2 >> 12))) * 626;
    //bmp280_pres /= (uint32_t)var1;

    bmp280_pres = (((uint32_t)(((int32_t)1048576)-pres_raw) - (var2 >> 12))) * 3125;
    //if (bmp280_pres < 0x80000000) {
    //  bmp280_pres = (bmp280_pres << 1) / ((uint32_t)var1);
    //}
    //else {
      bmp280_pres = (bmp280_pres / (uint32_t)var1) * 2;
    //}
    var1 = (((int32_t)bmp280_cal.v.dig_p9) * ((int32_t)(((bmp280_pres>>3) * (bmp280_pres >> 3)) >> 13))) >> 12;
    var2 = (((int32_t)(bmp280_pres >> 2)) * ((int32_t)bmp280_cal.v.dig_p8)) >> 13;
    bmp280_pres = (uint32_t)((int32_t)bmp280_pres + ((var1 + var2 + bmp280_cal.v.dig_p7) >> 4));
    bmp280_pres /= 10;
  }

  if (bmp280_id == BME280_ID_VAL) {
    int32_t var1;
    int32_t var2;
    int32_t var3;
    int32_t var4;
    int32_t var5;

    var1 = t_fine - ((int32_t)76800);
    //var2 = (int32_t)(humi_raw * 16384);
    var3 = (int32_t)(((int32_t)bme280_cal_hum.dig_h4) * 1048576);
    var4 = ((int32_t)bme280_cal_hum.dig_h5) * var1;

    //var5 = (((var2 - var3) - var4) + (int32_t)16384) / 32768;
    var5 = ((((humi_raw * 16384) - var3) - var4) + (int32_t)16384) / 32768;

    var2 = (var1 * ((int32_t)bme280_cal_hum.dig_h6)) / 1024;

    var3 = (var1 * ((int32_t)bme280_cal_hum.dig_h3)) / 2048;
    var4 = ((var2 * (var3 + (int32_t)32768)) / 1024) + (int32_t)2097152;

    var2 = ((var4 * ((int32_t)bme280_cal_hum.dig_h2)) + 8192) / 16384;

    var3 = var5 * var2;
    var4 = ((var3 / 32768) * (var3 / 32768)) / 128;
    var5 = var3 - ((var4 * ((int32_t)bmp280_cal.v.dig_h1)) / 16);
    //var5 = (var5 < 0 ? 0 : var5);
    //var5 = (var5 > 419430400 ? 419430400 : var5);
    bmp280_humi = (uint32_t)(var5 / 4096);

    bmp280_humi *= 10;
    bmp280_humi /= 1024;
  }
} /* bmp280_read */


/******************************************************************************/


