#include <8051.h>

#define FALSE 0
#define TRUE 1
#define T0_DAT 65535-921 // przerwanie T0 co 1ms
#define TL_0 T0_DAT%256
#define TH_0 T0_DAT/256
#define T100 100 // 100 * 10 = 1000ms = 1s
#define T10 10


__code unsigned char WZOR[10] = {0b0111111, 0b0000110, 0b1011011, 0b1001111,
                                 0b1100110, 0b1101101, 0b1111101, 0b0000111, 0b1111111, 0b1101111
                                };
__xdata unsigned char * led_wyb = (__xdata unsigned char *) 0xFF30; //bufor wybierajacy wyswietlacz
__xdata unsigned char * led_led = (__xdata unsigned char *) 0xFF38; //bufor wybierajacy segmenty
unsigned char timer_buf, timer_buf2; // czasomierz programowy (zliczający do 100), dodatkowy czasomierz programowy (doliczający do 1000)
unsigned char hh, mm, ss;
unsigned char led_b, iter, ustawiany_segment;
unsigned char key, key_timer;
unsigned char Aktualne[6] = {0b11111111, 0b11111111, 0b11111111, 0b11111111, 0b11111111, 0b11111111 };
__bit __at (0x96) SEG_OFF;
__bit t0_flag;
__bit stop;
__bit time_increment;
__bit key_served;

void t0_serv(void);
void obsluga_klawiatury(void);
void update(void);


void main() {
    stop = FALSE;
    ustawiany_segment = 0;
    key_timer = 0;
    hh = 12;
    mm = 43;
    ss = 52;
    TMOD = 0b00100001;
    TL0 = TL_0;
    TH0 = TH_0;
    time_increment = FALSE;
    timer_buf = T100;
    timer_buf2 = T10;
    t0_flag = FALSE;
    ET0 = TRUE;
    ES = TRUE;
    EA = TRUE;
    TR0 = TRUE;
    led_b = 1;
    iter = 0;
    while (TRUE) {
        if (t0_flag) { //przerwanie zegarowe co 1ms
            t0_flag = FALSE;
            t0_serv();
            if (time_increment && !stop) { //wykonanie co 1s
                if (++ss == 60) {
                    mm++;
                    ss = 0;
                }
                if (mm == 60) {
                    hh++;
                    mm = 0;
                }
                if (hh == 24) {
                    hh = 0;
                }
                time_increment = FALSE;
                update();
            }
            if(iter > 5) {
                led_b = 1;
                iter = 0;
            }
            SEG_OFF = 1; // OFF
            *led_wyb = led_b;
            *led_led = Aktualne[iter];
            SEG_OFF = 0; // ON
            if(P3_5 && key_timer == 0) { // Odczyt klawiatury (czy wciśnięty klawisz przy danym segmencie)
                key = led_b;
                obsluga_klawiatury();
                key_timer = 180;
                if(led_b == 4 || led_b == 32) {
                    key_timer = 250;
                    key_served = TRUE;
                }
            }
            led_b += led_b;
            iter++;
        }
    }
}

void t0_serv(void) {
    if (key_timer) {
        key_timer--;
    }
    else {
        if(key_served) {
            key_served = FALSE;
            key_timer = 250;
        }
    }
    if (timer_buf) {
        timer_buf--;        //zmniejsz stan czasomierza
    }
    else {
        timer_buf = T100;
        timer_buf2--;
        if (!timer_buf2) {
            timer_buf2 = T10;   //regeneruj licznik (1s)
            time_increment = TRUE;
        }
    }
}

void obsluga_klawiatury(void) {
    if(key == 1) { // ENTER
        if (stop) {
            timer_buf = T100;
            timer_buf2 = T10;
            stop = FALSE;
            time_increment = FALSE;
        }
        update();
        return;
    }
    if(key == 2) // ESC
        stop = TRUE; // set_time mode on
    if (key == 4 && stop) { //PRAWO
        ustawiany_segment = (ustawiany_segment+1)%3;
    }
    if (key == 32 && stop) { // LEFT
        ustawiany_segment = (ustawiany_segment+2)%3;
        update();
    }
    if (key == 8 && stop) { // UP
        if (ustawiany_segment == 0) {
            if(++hh == 24) {
                hh = 0;
            }
        }
        if (ustawiany_segment == 1) {
            if(++mm == 60) {
                mm = 0;
            }
        }
        if (ustawiany_segment == 2) {
            if(++ss == 60) {
                ss = 0;
            }
        }
    }
    if (key == 16 && stop) { // DOWN
        if (ustawiany_segment == 0) {
            if(--hh > 24) {
                hh = 23;
            }
        }
        if (ustawiany_segment == 1) {
            if(--mm > 60) {
                mm = 59;
            }
        }
        if (ustawiany_segment == 2) {
            if(--ss > 60) {
                ss = 59;
            }
        }
    }
    update();
}

void update(void) {
    Aktualne[0] = WZOR[ss % 10];
    Aktualne[1] = WZOR[ss / 10];
    Aktualne[2] = WZOR[mm % 10];
    Aktualne[3] = WZOR[mm / 10];
    Aktualne[4] = WZOR[hh % 10];
    Aktualne[5] = WZOR[hh / 10];
    if(stop) {
        if(ustawiany_segment == 0) {
            Aktualne[4] = WZOR[hh % 10] | 0b10000000;
            Aktualne[5] = WZOR[hh / 10] | 0b10000000;
        }
        else if(ustawiany_segment == 1) {
            Aktualne[2] = WZOR[mm % 10] | 0b10000000;
            Aktualne[3] = WZOR[mm / 10] | 0b10000000;
        }
        else if(ustawiany_segment == 2) {
            Aktualne[0] = WZOR[ss % 10] | 0b10000000;
            Aktualne[1] = WZOR[ss / 10] | 0b10000000;
        }
    }
}

void t0_int(void) __interrupt(1) {
    TL0 = TL0 | TL_0;
    TH0 = TH_0;
    t0_flag = TRUE;
}