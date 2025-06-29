/* BMP280 mit CodeVision, M. Eisele, 13.02.2025 */

/******************************************************************************/

#include <stdint.h>
#include <string.h>

#include "platform.h"

/******************************************************************************/

#define BMP280_ID_REG           0xd0

#define BMP280_CAL_REG_FIRST    0x88
#define BMP280_CAL_REG_LAST     0xa1
//#define BMP280_CAL_DATA_SIZE    (BMP280_CAL_REG_LAST+1 - BMP280_CAL_REG_FIRST)

/* extra BME280 */
#define BME280_CAL_HUM_REG_FIRST 0xe1
#define BME280_CAL_HUM_REG_LAST  0xe6
#define BME280_CAL_HUM_DATA_SIZE (BME280_CAL_HUM_REG_LAST + 1 - BME280_CAL_HUM_REG_FIRST)
#define BME280_CAL_HUM_OFFS      (BME280_CAL_HUM_REG_FIRST - BMP280_CAL_REG_FIRST)

#define BMP280_CAL_DATA_SIZE_ALLES    (BME280_CAL_HUM_REG_LAST+1 - BMP280_CAL_REG_FIRST)

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

/* Kalibrierdaten des Sensors, werden einmal beim Start ausgelesen und sind für die
   nachfolgenden Korrekturrechnungen erforderlich */
static union _bmp280_cal_union {
         uint8_t bytes [BMP280_CAL_DATA_SIZE_ALLES];
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

/* Schreiben eines Registers des BME280, übergeben wird die Register-Adresse sowie
   der zuschreibende Wert, jeweils 8Bit */
void bmp280_write_register (uint8_t reg_addr, uint8_t value)
{
  i2c_start ();
  i2c_write ((BMP280_I2C_ADDR << 1) | I2C_WRITE);
  i2c_write (reg_addr);
  i2c_write (value);
  i2c_stop ();
} /* bmp280_write_register */


/* Lesen eines Registers des BME280, übergeben wird die Register-Adresse sowie
   die Anzahl der zulesenden Bytes. Gelesen werden Registerwerte bis maximal
   vier Byte (32Bit) */
uint32_t bmp280_read_register (uint8_t reg_addr, uint8_t nbytes)
{
  uint32_t result32 = 0;
  uint8_t regval = 0;

  /* Parameterprüfung: nbytes mindestens ein Byte, max vier Byte */
  if ((nbytes == 0) || (nbytes > 4)) return result32;

  /* Dem Sensor die Register-Adresse mitteilen */
  i2c_start ();
  i2c_write ((BMP280_I2C_ADDR << 1) | I2C_WRITE);
  i2c_write (reg_addr);
  i2c_stop ();

  /* dann das Lesen der Daten beginnen */
  i2c_start ();
  i2c_write((BMP280_I2C_ADDR << 1) | I2C_READ);

  while (nbytes > 0) {
    uint8_t ack = 1;

    /* Beim letzten Byte senden wir kein ACK */
    if (nbytes == 1) ack = 0;

    /* ein Byte lesen ... */
    regval = i2c_read (ack);

    /* ... und zum Gesamtergebnis hinzufügen. Dazu den bisher gelesenen Wert um 8 Bit nach links schieben ... */
    result32 <<= 8;
    /* ... und dann das neue Byte einsetzen */
    result32 |= regval;

    nbytes -= 1;
  }

  i2c_stop ();

  return result32;
} /* bmp280_read_register */


void i2c_readmem (uint8_t i2caddr, uint16_t memaddr, uint8_t buff[], uint8_t bytes)
{
  uint8_t i = 0;

  i2c_start ();
  i2c_write ((i2caddr << 1) | I2C_WRITE);
  if (i2caddr == EEPROM_I2C_ADDR) {
    i2c_write (memaddr >> 8);
  }
  i2c_write (memaddr);
  i2c_stop ();

  i2c_start ();
  i2c_write ((i2caddr << 1) | I2C_READ);

  for (i = 0; i < bytes; i++) {
    uint8_t ack = 1;
    if (i == (bytes - 1)) ack = 0;
    buff[i] = i2c_read (ack);
  }

  i2c_stop();
} /* i2c_readmem */


/* Den Sensor starten, dazu die Kalibrierwerte auslesen und dann den Messmode starten */
void bmp280_start (void)
{
  bmp280_id = bmp280_read_register (BMP280_ID_REG, 1); /* ID @d8 */

  if ((bmp280_id != BMP280_ID_VAL) && (bmp280_id != BME280_ID_VAL)) {
    return;
  }

  /* Auslesen des kompletten Datenblocks mit den Kalibrierdaten */
  i2c_readmem (BMP280_I2C_ADDR,
               BMP280_CAL_REG_FIRST,
               bmp280_cal.bytes,
               BMP280_CAL_DATA_SIZE_ALLES);

  /* Umrechnung entsprechend den Vorgaben von Bosch */
  bme280_cal_hum.dig_h2 = bmp280_cal.bytes [BME280_CAL_HUM_OFFS + 1]; /* E2 */
  bme280_cal_hum.dig_h2 <<= 8;
  bme280_cal_hum.dig_h2 |= bmp280_cal.bytes [BME280_CAL_HUM_OFFS + 0]; /* E1 */

  bme280_cal_hum.dig_h3 = bmp280_cal.bytes [BME280_CAL_HUM_OFFS + 2]; /* E3 */

  bme280_cal_hum.dig_h4 = bmp280_cal.bytes [BME280_CAL_HUM_OFFS + 3]; /* E4 */
  bme280_cal_hum.dig_h4 <<= 4;
  bme280_cal_hum.dig_h4 |= bmp280_cal.bytes [BME280_CAL_HUM_OFFS + 4] & 0x0f; /* E5 */

  bme280_cal_hum.dig_h5 = bmp280_cal.bytes [BME280_CAL_HUM_OFFS + 5]; /* E6 */
  bme280_cal_hum.dig_h5 <<= 4;
  bme280_cal_hum.dig_h5 |= (bmp280_cal.bytes [BME280_CAL_HUM_OFFS + 4] >> 4) & 0x0f; /* E5 */

  bme280_cal_hum.dig_h6 = bmp280_cal.bytes [BME280_CAL_HUM_OFFS + 5]; /* E7 */

  /* oversampling * 16 einstellen */
  bmp280_write_register (BMP280_CONTROL_HUMI_REG, 5);

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


/* Auslesen und Umrechnen der Umweltdaten. Wird vom Hauptprogramm zyklisch aufgerufen
   Die Umrechnung erfolgt in Anlehnung an Referenz-Implementiereun von Bosch. Teile davon
   wurden deaktiviert um die Limitierung der Codevision Demo-Software einzuhalten.
   Auf den ersten Blick ist dadurch keine Verschlechterung der Sensordaten ersichtlich.
 */
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

  /* jeweils 24Bit Rohdaten für Temperatur und Luftdruck lesen */
  temp_raw = bmp280_read_register (0xfa, 3); /* temp */
  pres_raw = bmp280_read_register (0xf7, 3); /* press */


  if (bmp280_id == BME280_ID_VAL) {
    /* 16Bit Rohdaten für Luftfeuchtigkeit */
    humi_raw = bmp280_read_register (0xfd, 2); /* humidity */
  }

  /* Ab hier Umrechnung aus Referenz-Implementierung, Details unklar .... */

  /* Division durch 16 */
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

  bmp280_temp = (t_fine + 256) >> 9;

  /* compute the pressure */
  var1 = (((int32_t)t_fine) >> 1) - (int32_t)64000;
  var2 = (((var1 >> 2) * (var1 >> 2)) >> 11) * ((int32_t)bmp280_cal.v.dig_p6);
  var2 = var2 + ((var1 * ((int32_t)bmp280_cal.v.dig_p5)) << 1);
  var2 = (var2 >> 2) + (((int32_t)bmp280_cal.v.dig_p4) << 16);
  var1 = (((bmp280_cal.v.dig_p3 * (((var1 >> 2) * (var1 >> 2)) >> 13)) >> 3)
          + ((((int32_t)bmp280_cal.v.dig_p2) * var1) >> 1)) >> 18;
  var1 = ((((32768 + var1)) * ((int32_t)bmp280_cal.v.dig_p1)) >> 15);

  if (var1 != 0) {
    bmp280_pres = (((uint32_t)(((int32_t)1048576) - pres_raw) - (var2 >> 12))) * 625;
    bmp280_pres = (bmp280_pres / (uint32_t)var1);
  }

  /* compute humidity (nur wenn wir den BME280 haben) */
  if (bmp280_id == BME280_ID_VAL) {
    int32_t var1;
    int32_t var2;
    int32_t var3;
    int32_t var4;
    int32_t var5;

    var1 = t_fine - ((int32_t)76800);
    var3 = (int32_t)(((int32_t)bme280_cal_hum.dig_h4) * 1048576);
    var4 = ((int32_t)bme280_cal_hum.dig_h5) * var1;

    var5 = ((((humi_raw * 16384) - var3) - var4) + (int32_t)16384) / 32768;

    var2 = (var1 * ((int32_t)bme280_cal_hum.dig_h6)) / 1024;

    var3 = (var1 * ((int32_t)bme280_cal_hum.dig_h3)) / 2048;
    var4 = ((var2 * (var3 + (int32_t)32768)) / 1024) + (int32_t)2097152;

    var2 = ((var4 * ((int32_t)bme280_cal_hum.dig_h2)) + 8192) / 16384;

    var3 = var5 * var2;
    var5 = var3; // - ((var4 * ((int32_t)bmp280_cal.v.dig_h1)) / 16);
    bmp280_humi = (uint32_t)(var5 / 419439ul);
  }
} /* bmp280_read */


/******************************************************************************/


