/* Basisfunktionen ATmega2560 mit CodeVision, M. Eisele, 13.02.2025 */

/******************************************************************************/

#include <stdint.h>

#include "platform.h"

/******************************************************************************/

/* Diese Variable bildet unsere Zeitbasis, enthält die Zeit nach Start des mit
   einer Auflösung von einer Millisekunde, Überlauf nach 65,535 Sekunden, stört
   aber nicht, da wir nur mit relativen Zeiten arbeiten */
static volatile uint16_t timebase_ticks_ms = 0;

/******************************************************************************/

/* Timer-Interrupt, Aufruf 1000x pro Sekunde, einzige Funktion: 'timebase_ticks_ms' hochzählen */

#if defined (__AVR__)
/* AVR GCC */
ISR(TIMER0_OVF_vect)
#else
/* Codevision */
interrupt [TIM0_OVF] void timer0_overflow_isr (void)
#endif
{
  timebase_ticks_ms ++;
} /* timer0_overflow_isr */


/* Zeitbasis initialisieren, Einstellung des Timer-Interrupts */
void timebase_init (void)
{
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

#if defined (__AVR__)
  /* AVR GCC */
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
#endif
} /* timebase_init */


/* Systemzeit, 16Bit in Millisekunden, Name der Funktion in Anlehnung an die Arduino-Umgebung */
uint16_t millis (void)
{
  uint16_t now = 0;

  /* Wert aus 'timebase_ticks_ms' zurückliefern. Wir lesen mehrfach (Vergleich mit 'now') denn das
     Auslesen der 16Bit-Variable könnte vom Timer-Interrupt unterbrochen werden sodass falsche Werte
     zurückgegeben werden */
  do {
    now = timebase_ticks_ms;
  } while (now != timebase_ticks_ms);

  return now;
} /* millis */


#if defined (__AVR__)
/* AVR GCC */
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
#endif


/* Funktion für Millisekunden Verzögerung */
void mdelay_ms (uint16_t dt_ms)
{
  uint16_t t_start = millis ();
  while ((millis () - t_start) < dt_ms) {
    /* nix */
  }
} /* mdelay_ms */


/******************************************************************************/


