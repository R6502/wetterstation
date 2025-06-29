/* Wetterstation mit CodeVision, M. Eisele, 13.02.2025 */

/******************************************************************************/

#include <stdint.h>
#include <string.h>

#include "platform.h"

/******************************************************************************/

/* Fürs die Ablaufsteuerung (Weiterrechnen der Zeit, Anzeige der Sensorwerte)
   brauchen wir eine Variable die den Zeitpunkt der vorherigen Ausführung der Funktionen
   festhält. Sobald die Differenz zwischen dieser Varable (also 'time_1000ms_prev') und
   der aktuellen Zeit aus 'millies()' >= 1000 ist also min. eine Sekunde vergangen und
   wir müssen Zeit und Anzeige wieder aktualisieren. */
uint16_t time_1000ms_prev = 0;

/* Aktuelle Zeit im BCD-Format. Das BCD-Format ist geschickt fürs Speichern und Lesen
   mit dem RTC-Chip. Auch die Anzeige geht damit recht gut, nur das Weiterrechnen ist
   nicht so geschickt da die Überläufe der Einer-Stellen extra behandelt werden müssen. */
uint8_t zeit_sekunden_bcd = 0;
uint8_t zeit_minuten_bcd = 0;
uint8_t zeit_stunden_bcd = 0;

/* Aktuelle Eingabe des Benutzers beim Stellen der Uhr */
uint8_t settime_minuten_bcd = 0;
uint8_t settime_stunden_bcd = 0;

/* Menüsteuerung */
#define MENU_NORMAL     (0)
#define MENU_SETTIME    (1)
uint8_t menu_shown = MENU_NORMAL;
uint8_t menu_mode = MENU_NORMAL;
uint8_t menu_anzeigen = 0;

/******************************************************************************/

/* Textbuffer für die Aufbereitung der Sensorwerte fürs LCD */
#define MAX_TEXT_BUFFER 16
#define TEXT_BUFFER_RIGHT 6

char text_buffer [MAX_TEXT_BUFFER];

/******************************************************************************/

void bcd_zaehler_anzeigen (uint8_t zaehler_wert);
void index_anzeigen (uint8_t v);

/******************************************************************************/

/* Verwaltung der gespeicherten Wetterdaten. Ein Datensatz wird in der Datenstruktur
   'HISTORY' gespeichert. Dafür gibt es ein Array der Dimension 16 (HISTORY_SIZE).
   Wichtig ist, dass der Wert eine 2er-Potenz ist, denn damit können wir die Überläufe
   geschickt behandeln. HISTORY_MASK ist in diesem Fall 16-1=15 (=0x0f). Damit können wir
   mit einer Und-Verknüpfung den Divisionsrest (von geteilt durch 16) schnell und
   einfach bestimmen. */
typedef struct history_s {
          uint8_t  seq;
          uint8_t  version;

          uint8_t  zeit_minuten_bcd;
          uint8_t  zeit_stunden_bcd;

          int32_t  bmp280_temp;
          uint32_t bmp280_pres;
          uint32_t bmp280_humi;
        } HISTORY;

#define HISTORY_SIZE    16
#define HISTORY_MASK    ((HISTORY_SIZE) - 1)
#define HISTORY_MAX     12

#define HISTORY_VERSION 1

/* Array mit gespeicherten Werten */
HISTORY history [HISTORY_SIZE];

/* Index des aktuellen Kopfeintrags: An dieser Stelle wird im Array ein neuer Eintrag
   angelegt, danach der Index erhöht, Zugriff aus Array mit 'HIST_HEAD & HISTORY_MASK'
   denn 'hist_head' läuft weiter als die Array-Grenze */
uint8_t  hist_head = 0;

/* Anzahl gespeicherter Wetterdaten */
uint8_t  hist_level = 0;

/* Aktueller Sequenz-Zähler, wird in der Datenstruktur in 'seq' angelegt */
uint8_t  hist_seq = 0;

/* Aktueller, angezeigter Wert aus dem Speicher, wenn == 0 dann werden die aktuellen Wetterdaten anzgezeigt */
uint8_t  hist_show = 0;

/* Ein Flag mit dem wir anzeigen ob die gemessenen Wetterdaten gültig sind,
   denn sonst kann es vokommen, dass ungültige Daten (direkt nach Start) gespeichert werden */
uint8_t  wetter_ok = 0; /* nach dem Start erst warmlaufen lassen um korrekte Werte zu speichern */

/******************************************************************************/

#define EEPROM_RECORD_SIZE     32


/* Die aktuellen Wetterdaten abspeichern: Schreiben ins history-Array sowie Ablage im EEPROM */
void hist_store (void)
{
  uint8_t index = hist_head++;

  index &= HISTORY_MASK;

  history [index] .seq = hist_seq++;
  history [index] .version = HISTORY_VERSION;
  history [index] .zeit_minuten_bcd  = zeit_minuten_bcd;
  history [index] .zeit_stunden_bcd  = zeit_stunden_bcd;
  history [index] .bmp280_temp = bmp280_temp;
  history [index] .bmp280_pres = bmp280_pres;
  history [index] .bmp280_humi = bmp280_humi;

  if (hist_level < HISTORY_MAX) {
    hist_level++;
  }

  { uint16_t addr = (uint16_t)index * EEPROM_RECORD_SIZE;
    uint8_t *p_data = (uint8_t *) &history [index];
    uint8_t n = 0;

    i2c_start ();
    i2c_write ((EEPROM_I2C_ADDR << 1) | I2C_WRITE);
    i2c_write (addr >> 8);
    i2c_write (addr);
    for (n = 0; n < sizeof (HISTORY); n++) {
      i2c_write (*p_data++);
    }
    i2c_stop ();
  }
} /* hist_store */


/******************************************************************************/


/* Ausgabe eines Festkommawert (Faktor 10) mit Dezimalpunkt und ggf. Vorzeichen */
void lcd_print_decimal10 (int32_t value)
{
  uint8_t insert_decimal_point = 1;
  uint8_t wp = TEXT_BUFFER_RIGHT; /* Schreib-Index auf Ende des Texts */
  uint8_t neg = 0;

  /* negative Werte erkennen, Betrag des Werts weiterverwenden und Flag setzen
     damit wir später das Vorzeichen schreiben */
  if (value < 0) {
    neg = 1;
    value = -value;
  }

  /* null-Byte als Terminierung schreiben */
  text_buffer [wp] = 0;

  /* Die einzelnen Ziffern im Text-Buffer ablegen ... */
  while (wp) {
    /* ... dazu ASCII '0' plus Divisionsrest modulo 10 */
    text_buffer [--wp] = '0' + value % 10;
    /* Wert für nächste Stelle durch 10 teilen */
    value /= 10;
    /* nach der ersten Stelle den Dezimalpunkt einfügen, danach dann die weitereen Stellen */
    if (insert_decimal_point) {
      text_buffer [--wp] = '.';
      insert_decimal_point = 0;
    }
    else if (value == 0) {
      /* haben wird den Wert bis auf null runtergeteilt, dann sind wir fertig */
      break;
    }
  }

  /* jetzt noch das Minus für negative Werte, fall erforderlich */
  if (neg && wp) text_buffer [--wp] = '-';

  /* die reslichen Stellen vorne mit Leerzeichen auffüllen */
  while (wp != 0) {
    text_buffer [--wp] = ' ';
  }

  /* ... und dann die Zahl aufs LCD schreiben */
  lcd_print_text (text_buffer);
} /* lcd_print_decimal10 */


/******************************************************************************/


/* Funktion zum Anzeigen der aktuellen sowie gespeicherten Wetterdaten  */
void wetterdaten_anzeigen ()
{
  /* aktuelle Daten vom Sensor holen*/
  bmp280_read ();

  /* Kennzeichnen dass wir aktuelle Daten haben, wir zählen nur bis 10, ansonsten
     würde der Zähler nach 255 überlaufen und wir wären wieder bei 0 */
  if (wetter_ok < 10) wetter_ok++;

  /* Wenn gespeicherte Werte angezeigt werden sollen, übschreiben wir die Messwerte
     durch gespeicherte Werte und zeigen diese dann an (mit einer Funktion die alles
     anzeigt sparen wir Code. */
  if (hist_show) {
    uint8_t index = hist_head;
    index -= hist_show;
    index &= HISTORY_MASK;

    /* Werte aus Speicher holen und Messdaten damit überschreiben */
    bmp280_temp = history [index] .bmp280_temp;
    bmp280_pres = history [index] .bmp280_pres;
    bmp280_humi = history [index] .bmp280_humi;

    /* Index des Speicherplatzes anzeigen */
    lcd_set_cursor (0, 3);
    index_anzeigen (hist_show);

    lcd_print_char (':');
    lcd_print_char (' ');

    /* Anzeigen der Uhrzeit des Speicherplatzes */
    bcd_zaehler_anzeigen (history [index] .zeit_stunden_bcd);
    lcd_print_char (':');
    bcd_zaehler_anzeigen (history [index] .zeit_minuten_bcd);
  }

  /* Temperatur */
  lcd_set_cursor (10, 0);
  lcd_print_decimal10 (bmp280_temp);
  lcd_print_text (" \337C");

  /* Luftdruck */
  lcd_set_cursor (10, 2);
  lcd_print_decimal10 (bmp280_pres);
  lcd_print_text (" hPa");

  /* Luftfeuchte */
  lcd_set_cursor (10, 1);
  lcd_print_decimal10 (bmp280_humi);
  lcd_print_text (" %RH");
} /* wetterdaten anzeigen */


/******************************************************************************/
/* Zeitfunktion & Sekunden hochzählen */


/* Für die Uhreit (im BCD-Format) den Zähler erhöhen. Mit dem Parameter 'limit'
   können wir die Fubktion für Stunden und Minuten verwenden */
uint8_t bcd_plus_eins (uint8_t x, uint8_t limit)
{
  x += 1;
  if ((x & 0xf) >= 0x0a) { // BCD Überlauf
    x += 0x06;
  }
  if (x >= limit) x = 0;

  return x;
} /* bcd_plus_eins */


/* Uhrzeit eine Sekunde weiterrechnen, dazu die Funktion 'bcd_plus_eins()'
   Überläufe berücksichtigen */
uint8_t zeit_weiter_eine_sekunde (void)
{
  zeit_sekunden_bcd = bcd_plus_eins (zeit_sekunden_bcd, 0x60);
  if (zeit_sekunden_bcd == 0) { /* Sekunden Überlauf */
    zeit_minuten_bcd = bcd_plus_eins (zeit_minuten_bcd, 0x60);
    if (zeit_minuten_bcd == 0) { /* Miuten-Überlauf */
      zeit_stunden_bcd = bcd_plus_eins (zeit_stunden_bcd, 0x24);
    }
  }
  return 0;
} /* zeit_weiter_eine_sekunde */


/* Uhrzeit-Zähler anzeigen, gleiche Funktion für Sekunden, Minuten und Stunden */
void bcd_zaehler_anzeigen (uint8_t zaehler_wert)
{
  /* BCD-Format ist einfach anzuzeigen: Einfach die Stellen (in diesem Fall Einer +
  Zehner auf ASCII '0' aufaddieren und aufs LCD schreiben */
  lcd_print_char ('0' + ((zaehler_wert >> 4) & 0x0f));
  lcd_print_char ('0' + ( zaehler_wert & 0x0f));
} /* bcd_zaehler_anzeigen */


/* Den Speicher-Index anzeigen, wir können nur zwei Dezimalstellen, das reicht aber */
void index_anzeigen (uint8_t v)
{
  if (v < 10) lcd_print_char (' ');
         else lcd_print_char ('0' + v / 10);
  lcd_print_char ('0' + v % 10);
} /* index_anzeigen */


/*****************************************************************************************************/


/* Menü anzeigen, wir haben nur zwei Zustände: Normalbetrieb mit Anzeige des Speicherfüllstands sowie
   die Zeit beim Stellen der Uhr */
void menu (void)
{
  if ((menu_shown == menu_mode) && (menu_anzeigen == 0)) return;

  lcd_set_cursor (0, 3);

  if (menu_mode == MENU_NORMAL) {
    // lcd_print_text ("HIST:  ");
    //
    // lcd_print_char ('0' + hist_level);
    // lcd_print_char ('/');
    //
    // bcd_zaehler_anzeigen (hist_head);
    // lcd_print_char ('/');
    // bcd_zaehler_anzeigen (hist_seq);

    lcd_print_text ("nHIST: ");
    index_anzeigen (hist_level);
  }
  else if (menu_mode == MENU_SETTIME) {
    lcd_print_text ("TIME: ");
    bcd_zaehler_anzeigen (settime_stunden_bcd);
    lcd_print_char (':');
    bcd_zaehler_anzeigen (settime_minuten_bcd);
    lcd_print_text (" #OK    ");
  }

  menu_shown = menu_mode;
  menu_anzeigen = 0;
} /* menu */


/*****************************************************************************************************/


#if defined (__AVR__)
/* AVR GCC */
int
#else
/* Codevision */
void
#endif
 main (void)
{
  /* einmalig ausführen, I2C + Zeitbasis initialisieren */
  i2c_init ();
  timebase_init ();

  /* enable interrupts */
#if defined (__AVR__)
  /* AVR GCC */
  sei ();
#else
  /* Codevision */
#asm
  sei
#endasm

#endif

  /* LCD starten */
  lcd_init ();

  /* Bosch-Sensor starten */
  bmp280_start ();

  /* Uhrzeit aus RTC auslesen (wird ab dann in der Software weitergerechnet) */
  rtc_read ();

  /* Load history */
  { uint8_t index;

    for (index = 0; index < HISTORY_SIZE; ++index) {
      uint16_t addr = (uint16_t) index * EEPROM_RECORD_SIZE;

      i2c_readmem (EEPROM_I2C_ADDR, addr, (uint8_t*) &history [index], sizeof (HISTORY));

      if (history [index].version == HISTORY_VERSION) {
        if (index == 0) {
          hist_seq = history [index] .seq;
        }
        if (hist_seq == history [index] .seq) {
          hist_head++;
          hist_seq++;
        }
      }
      else {
        break;
      }
    }

    hist_level = index;

    if (hist_level > HISTORY_MAX) {
      hist_level = HISTORY_MAX;
    }
  }

  /* Menü zum ersten Mal anzeigen */
  menu_anzeigen = 1;
  menu ();

  /* Startwert für 1-Sekunden-Intervall*/
  time_1000ms_prev = millis ();

  while (1) { /* Endlosschleife */
    uint8_t  zeit_anzeigen = 0;
    uint8_t  wetter_anzeigen = 0;
    uint16_t time_ms = 0;

    /* aktuelle Zeit in Millisekungen holen und in 'time_ms' ablegen */
    time_ms = millis ();

    /* Matrix-Tasen einlesen */
    { uint8_t  taste = 0;

      taste = keypad_read ();

      if (taste != KEYCODE_NONE) {
        /* Spezialbehandlung Taste '0', den Code überschreiben wir hier mit 0 denn das vereinfacht das Stellen der Uhrzeit */
        if (taste == KEYCODE_0) taste = 0;

        /* Wenn keine gespeicherten Daten angezeigt werden und eine numerische Taste gedrückt wurde, dann beginnen wir mit
           dem Einstellen der Uhrzeit ... */
        if ((taste < 10) && (hist_show == 0)) {
          /* ... dazu schieben wir einfach die Dezimalstellen durch die beiden settime-Variablen */
          settime_stunden_bcd <<= 4;
          settime_stunden_bcd |= settime_minuten_bcd >> 4;
          settime_minuten_bcd <<= 4;
          settime_minuten_bcd |= taste;
          menu_mode = MENU_SETTIME;
          menu_anzeigen = 1;
        }
        else if (menu_mode == MENU_SETTIME) {
          /* Wenn die OK-Taste (#) gedrückt wurde und das Stellen der Uhrzeit aktiv ist, dann
             übernehmen wir die Werte (nur wenn gültig), speichern die Zeit im RTC und wechseln in den Normal-Mode zurück */
          if (taste == KEYCODE_OK) {
            if ((settime_minuten_bcd < 0x60) && (settime_stunden_bcd < 0x24)) {
              zeit_sekunden_bcd = 0x00;
              zeit_minuten_bcd  = settime_minuten_bcd;
              zeit_stunden_bcd  = settime_stunden_bcd;
              zeit_anzeigen = 1;
              rtc_write ();
            }
          }

          settime_stunden_bcd = 0;
          settime_minuten_bcd = 0;
          menu_mode = MENU_NORMAL;
          lcd_clear ();
        }
        //else if (taste == KEYCODE_F4) {
        //  hist_store ();
        //  menu_anzeigen = 1;
        //}
        /* Blättern in den gespeicherten Wetterdaten */
        else if (taste == KEYCODE_F1) {
          if (hist_show > 0) hist_show--;
          if (hist_show == 0) menu_anzeigen = 1;
          wetter_anzeigen = 1;
        }
        else if (taste == KEYCODE_F2) {
          if (hist_show < hist_level) hist_show++;
          wetter_anzeigen = 1;
        }
      }

      menu ();
    }

    if ((time_ms - time_1000ms_prev) >= 1000) { /* alle Sekunde ausführen */
      /* Uhrzeit weiterrechnen */
      zeit_weiter_eine_sekunde ();
      zeit_anzeigen = 1;
      time_1000ms_prev += 1000;
      wetter_anzeigen = 1;

      /* Beim Übergang auf eine neue Minute das Speichern der Daten anfordern.
         Dazu ggf. die Anzeige der gespeicherten Werte beenden. */
      if (zeit_sekunden_bcd == 0) {
        if (wetter_ok > 2) {
          hist_show = 0;
          wetter_anzeigen++;
        }
        menu_anzeigen = 1;
      }
    }

    if (wetter_anzeigen) {
      wetterdaten_anzeigen ();
      if (wetter_anzeigen == 2) {
        /* neuen Eintrag mit Wetterdaten ablegen */
        hist_store ();
      }
      wetter_anzeigen = 0;
    }

    /* Aktuelle Uhrzeit anzeigen */
    if (zeit_anzeigen) {
      lcd_set_cursor ( 0, 0);
      bcd_zaehler_anzeigen (zeit_stunden_bcd);
      lcd_print_char (':');
      bcd_zaehler_anzeigen (zeit_minuten_bcd);
      lcd_print_char (':');
      bcd_zaehler_anzeigen (zeit_sekunden_bcd);

      zeit_anzeigen = 0;
    }
  }

#if defined (__AVR__)
/* AVR GCC */
  return 0;
#endif
} /* main */


/******************************************************************************/


