#include <8051.h>

#define True 1
#define False 0
#define T0val 112 // 6400Hz  <-- 921,6:144 = 6,4  i  256-144 = 112
#define pwmLow 0b00001111
#define pwmHigh 0b11110000

__bit __at (0x97) LED_off;
__bit t0_f;
__bit rec_f;
__bit send_f;

__xdata unsigned char *I8255b = (__xdata unsigned char *) 0xFF29; // port B
__xdata unsigned char *I8255r = (__xdata unsigned char *) 0xFF2B; // rejestr

unsigned char pwm;
unsigned char tSec;     // 64 x 100hz aby otrzymać sekundę
unsigned char t100;

void rec_serv(void);
void send_serv(void);
void t0_serv(void);

void main(){
    *I8255r = 0b10011001;
    pwm = 30;
    PCON = 0x80;
    SCON = 0b01010000;
    TMOD = 0b00100010;  // timery w tryb 2 (auto reload)
    IE = 0b10010010;    // 0b10011010 to enable inter from both timers
    TH0 = T0val; TL0 = T0val;
    TH1 = 0xFD; TL1 = 0xFD;
    tSec = 192;
    t100 = 255;

    while(True){
        if (rec_f) {         //odebrany bajt w buf. UART
            rec_f = False;   //kasuj flagę bajt odebrany
            rec_serv();         //obsłuz odebrany bajt
        }
        if (send_f)          //trzeba wysłać dane UART
            send_serv();        //wykonaj obsługę nadawania
        if (t0_f) {          //przerwanie zegarowe
            t0_f = False;    //zeruj flagę
            t0_serv();          //obsłuz przerwanie od T0
        }
    }
}

void t0_serv(void){
    if(t100 < 100){  // 6,4k Hz
        if(t100 == pwm){
            *I8255b = pwmLow;
            LED_off = True;
        }
    }
    else{   // 64 Hz
        t100 = 0;
        if(pwm) {
            *I8255b = pwmHigh;
            LED_off = False;
        }
    }
}

void rec_serv(void){
    unsigned char uc = SBUF;    //pobierz z bufara RS'a
    if (( uc >= 'a' ) && ( uc < 'z' + 1 ))
        uc += 'A' - 'a';        //zamień małą na wielką
    send_buf = uc;          //zapamiętaj w buforze
    send_f = True;          //ustaw flagę gotowości danych
}

void send_serv(void){
    if (TI)             //nadajnik nie jest gotowy
        return;
    SBUF = send_buf;    //wyślij bajt
    send_f = False;  //zeruj flagę nadawania bajtu
}

void t0_int(void) __interrupt(1){
    t100++;
    t0_f = True; //fakt wystąpienia przerwania
}

void sio_int(void) __interrupt(4)
{
    if (TI) { //gdy pusty bufor nadajnika
        TI = False;
    }
    else {
        RI = False; //zeruj flagę odbioru znaku
        rec_fl = True; //ustaw flagę odebrania znaku
    }
}