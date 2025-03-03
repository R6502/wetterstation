/* Basisfunktionen ATmega2560 mit CodeVision, M. Eisele, 13.02.2025 */

/******************************************************************************/

#include <stdint.h>
#include <io.h>
#include <i2c.h>

#include "platform.h"

/******************************************************************************/

// 16 -> 115200 bit/s bei  U2X0 == 1

void uart0_init (unsigned int ubrr)
{
  /* set bitrate */
  UBRR0L  = (unsigned char) (ubrr & 0xff);
  UBRR0H  = (unsigned char) (ubrr >> 8);

  DDRE |= (1 << PORTE1);

  UCSR0A = 0
           | (1 << U2X0) /*  Double the USART transmission speed */
           ;

  UCSR0C = 0        /* Asynchronous USART, 1 stop bit, no parity */
           | (1 << UCSZ01)  /* Set 8-bit frame format */
           | (1 << UCSZ00)
           ;

  /* Enable transmitter */
  UCSR0B = 0
           | (1 << TXEN0)
           | (1 << RXEN0)
           ;
} /* uart0_init */


void uart0_tx (uint8_t txbyte)
{
  /* Wait until the previous transmission has ended */
  while ((UCSR0A & (1 << UDRE0)) == 0) {
    /* empty */
  }
  /* Put the data into tx buffer */
  UDR0 = txbyte;
} /* uart0_tx */


uint8_t uart0_rx (void)
{
  uint8_t rxbyte = 0;

  /* Wait for data to be received */
  if ((UCSR0A & (1 << RXC0)) != 0) {
    /* Get and return received data from buffer */
    rxbyte = UDR0;
  }

  return rxbyte;
} /* uart0_rx */


void serial_print_text (const char *text)
{
  while (*text) {
    uart0_tx (*text++);
  }
} /* serial_print_text */


/******************************************************************************/


static volatile uint16_t timebase_ticks_ms = 0;


//interrupt [TIM0_COMPA] void timer0_compa_isr (void)
//{
//  // TIFR0
//  timebase_ticks_ms ++;
//  //uart0_tx ('*');
//} /* timer0_compa_isr */


interrupt [TIM0_OVF] void timer0_overflow_isr (void)
{
  timebase_ticks_ms ++;
  //uart0_tx ('*');
} /* timer0_overflow_isr */


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

  //cli ();

#asm
  cli /* disable interrupts */
#endasm

  now = timebase_ticks_ms;

  //sei ();
#asm
  sei /* enable interrupts */
#endasm

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


// void mdelay_us (uint8_t dt_us)
// {
//   uint8_t t_start = TCNT0; // TODO: anderen Timer (16bit) verwenden
//   dt_us >>= 2;
//   if (dt_us == 0) dt_us = 1;
//   while ((TCNT0 - t_start) < dt_us) {
//     /* nix */
//   }
// } /* mdelay_us */


void mdelay_ms (uint16_t dt_ms)
{
  uint16_t t_start = millis ();
  while ((millis () - t_start) < dt_ms) {
    /* nix */
  }
} /* mdelay_ms */



/******************************************************************************/


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


/******************************************************************************/


