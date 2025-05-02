/* Wetterstation mit CodeVision, M. Eisele, 13.02.2025 */

/******************************************************************************/

#include <stdint.h>
#include <string.h>

#include "platform.h"

/******************************************************************************/

char text_buffer [MAX_TEXT_BUFFER];

/******************************************************************************/


// void insert_decimal_point100 ()
// {
//   text_buffer [TEXT_BUFFER_RIGHT + 1] = 0;
//   text_buffer [TEXT_BUFFER_RIGHT + 0] = text_buffer [TEXT_BUFFER_RIGHT - 1];
//   text_buffer [TEXT_BUFFER_RIGHT - 1] = text_buffer [TEXT_BUFFER_RIGHT - 2];
//   text_buffer [TEXT_BUFFER_RIGHT - 2] = '.';
// } /* insert_decimal_point100 */
// } /* insert_decimal_point100 */

void insert_decimal_point10 ()
{
  text_buffer [TEXT_BUFFER_RIGHT + 1] = 0;
  text_buffer [TEXT_BUFFER_RIGHT + 0] = text_buffer [TEXT_BUFFER_RIGHT - 1];
  text_buffer [TEXT_BUFFER_RIGHT - 1] = '.';
} /* insert_decimal_point10 */


void int32_to_text_decimal (int32_t value, uint8_t minlen)
{
  uint8_t wp = TEXT_BUFFER_RIGHT;
  uint8_t neg = 0;

  if (value < 0) {
    neg = 1;
    value = -value;
  }

  text_buffer [wp] = 0;

  while (wp != 0) {
    text_buffer [--wp] = '0' + value % 10;
    value /= 10;
    if (minlen) minlen--;
    if ((value == 0) && (minlen == 0)) {
      break;
    }
  }

  if (neg && wp) text_buffer [--wp] = '-';

  while (wp != 0) {
    text_buffer [--wp] = ' ';
  }
} /* int32_to_text_decimal */


//void int16_to_text_decimal (int16_t value, uint8_t minlen)
//{
//  uint8_t wp = TEXT_BUFFER_RIGHT;
//  uint8_t neg = 0;
//
//  if (value < 0) {
//    neg = 1;
//    value = -value;
//  }
//
//  text_buffer [wp] = 0;
//
//  while (wp != 0) {
//
//#if 0
//    uint16_t div10 = value;
//    uint16_t mul10;
//    uint16_t remainder;
//    div10 *= 52429;
//    div10 /= 2;
//    mul10 = div10 * 10;
//    remainder = value - mul10;
//    text_buffer [--wp] = '0' + remainder;
//    value = div10;
//#else
//    text_buffer [--wp] = '0' + value % 10;
//    value /= 10;
//#endif
//
//    if (minlen) minlen--;
//    if ((value == 0) && (minlen == 0)) {
//      break;
//    }
//  }
//
//  if (neg && wp) text_buffer [--wp] = '-';
//
//  while (wp != 0) {
//    text_buffer [--wp] = ' ';
//  }
//
//  //return text_buffer;
//} /* int16_to_text_decimal */


/******************************************************************************/


