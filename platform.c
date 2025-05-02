/* Basisfunktionen ATmega2560 mit CodeVision, M. Eisele, 13.02.2025 */

/******************************************************************************/

#include <stdint.h>

#include "platform.h"

/******************************************************************************/

//int16_t         adjust_num,
//                adjust_den,
//                adjust_control,
//                adjust_dir;

/******************************************************************************/

static volatile uint16_t timebase_ticks_ms = 0;

/******************************************************************************/


#if defined (__AVR__)
/* AVR GCC */
ISR(TIMER0_OVF_vect)
#else
/* Codevision */
interrupt [TIM0_OVF] void timer0_overflow_isr (void)
#endif
{
  timebase_ticks_ms ++;

  //if (adjust_control < adjust_den) {
  //  adjust_control += adjust_num;
  //}
  //else {
  //  adjust_control -= adjust_den;
  //  timebase_ticks_ms += adjust_dir;
  //  //uart0_tx ('.');
  //}
} /* timer0_overflow_isr */


void timebase_init (void)
{
  // //adjust_num = 25; // 0.248% auf Arduino/Marie
  // adjust_num = 31; // 0.31% auf Arduino 2
  // adjust_den = 10000;
  // adjust_control = 0;
  // adjust_dir = 1;

  /* 8-bit Timer/Counter0: Zeitbasis für 1ms Tick
     Three Independent Interrupt Sources (TOV0, OCF0A, and OCF0B)

     Timer/Counter Control:
     16.000.000 /   64 =   250.000 -> /250 = 1000

     Waveform Generation Mode = 7

      Mode WGM2 WGM1 WGM0 Timer/Counter Mode of Operation  TOP  Update of OCRx at   TOV Flag
      7    1    1    1    Fast PWM                         OCRA BOTTOM              TOP
   */

  TCCR0A = 0
           | (1 << WGM00)
           | (1 << WGM01)
           ;

  TCCR0B = 0
           | (1 << CS00) /* /64 */
           | (1 << CS01)
           | (1 << WGM02)
           ;

  TCNT0 = 0;
  OCR0A = 249;
  OCR0B = 0;

  //TIMSK0 |= (1 << OCIE0A); /* Timer/Counter0 Output Compare Match A Interrupt Enable */
  TIMSK0 |= (1 << TOIE0); /*  Timer/Counter0 Overflow Interrupt Enable */

  /* 16-bit Timer/Counter1: Freilaufend für us-Delay, 0.5us Auflösung

     To do a 16-bit write, the high byte must be written before the low byte. For a 16-bit read, the low byte must be read
     before the high byte.
   */

  TCCR1A = 0
           ;

  TCCR1B = 0
           | (1 << CS11)  /* /8 */
           ;

  TCCR1C = 0
           ;

  TCNT1H = 0;
  TCNT1L = 0;
} /* timebase_init */


uint16_t millis (void)
{
  uint16_t now = 0;

//  cli ();

//#asm
//  cli /* disable interrupts */
//#endasm

  do {
    now = timebase_ticks_ms;
  } while (now != timebase_ticks_ms);

//  sei ();

//#asm
//  sei /* enable interrupts */
//#endasm

  return now;
} /* millis */


void mdelay_us (uint16_t dt_us)
{
  uint16_t t_start = TCNT1L;
  uint16_t t_start_h = TCNT1H;

  t_start_h <<= 8;
  t_start |= t_start_h;

  dt_us <<= 1;
  if (dt_us == 0) dt_us = 1;

  while (1) {
    uint16_t t_now = TCNT1L;
    uint16_t t_now_h = TCNT1H;

    t_now_h <<= 8;
    t_now |= t_now_h;

    if ((t_now - t_start) >= dt_us) {
      break;
    }
  }
} /* mdelay_us */


void mdelay_ms (uint16_t dt_ms)
{
  uint16_t t_start = millis ();
  while ((millis () - t_start) < dt_ms) {
    /* nix */
  }
} /* mdelay_ms */



/******************************************************************************/

// #if defined(__CODEVISIONAVR__)
#if defined (__AVR__)

/* AVR GCC */
/* .... */

#else

/* Codevision */
void sei (void)
{
#asm
  sei /* enable interrupts */
#endasm
} /* sei */


void cli (void)
{
#asm
  cli /* disable interrupts */
#endasm
} /* cli */

#endif


/******************************************************************************/


