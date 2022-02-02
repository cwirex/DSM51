#include "8051.h"

#define True 1
#define False 0
#define T0val 112 // 6400Hz  <-- 921,6:144 = 6,4  i  256-144 = 112
#define pwmLow 0b00001111
#define pwmHigh 0b11110000

__bit __at (0x97) LED_off;
__bit __at (0x96) SEG_off;
__bit t0_f;
__bit rec_f;
__bit send_f;

__xdata unsigned char *I8255b = (__xdata unsigned char *) 0xFF29; // port B
__xdata unsigned char *I8255r = (__xdata unsigned char *) 0xFF2B; // rejestr sterujący
__xdata unsigned char * led_wyb = (__xdata unsigned char *) 0xFF30; //bufor wybierajacy wyswietlacz
__xdata unsigned char * led_led = (__xdata unsigned char *) 0xFF38; //bufor wybierajacy segmenty

__code unsigned char WZOR[10] = {0b0111111, 0b0000110, 0b1011011, 0b1001111,
                                 0b1100110, 0b1101101, 0b1111101, 0b0000111, 0b1111111, 0b1101111};
unsigned char Aktualne[6] = {0, 0, 0, 0, 0, 0};

unsigned char pwm;
unsigned char tSec;     // 64 x 100hz aby otrzymać sekundę
unsigned char t100;
unsigned char send_buf;
unsigned char led_b;
unsigned char key, key_timer;

void rec_serv(void);
void send_serv(void);
void t0_serv(void);
void seg_serv();
void seg_update();
void key_serv();

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
    led_b = 1;
    TR0 = True;
    TR1 = True;
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

void seg_serv(){
    SEG_OFF = 1; // OFF
    *led_wyb = led_b;
    *led_led = Aktualne[iter];
    SEG_OFF = 0; // ON
    if(P3_5) { // Odczyt klawiatury (czy wciśnięty klawisz przy danym segmencie)
        key = led_b;
    }
}

void key_serv(void) {
    if(key == 1) { // ENTER

    }
    else if(key == 2){ // ESC

    }

    else if (key == 4) { //PRAWO
        pwm += 10;
        if(pwm > 50){
            pwm = 50;
        }
    }
    else if (key == 32) { // LEFT
        pwm -= 10;
        if(pwm > 100){
            pwm = 0;
        }
    }
    else if (key == 8) { // UP
        if(pwm != 50)
            pwm+=1;
    }
    else if (key == 16) { // DOWN
        if(pwm != 0)
            pwm-=1;
    }
    seg_update();
}

void seg_update(void) {
    Aktualne[0] = WZOR[pwm % 10];
    Aktualne[1] = WZOR[pwm / 10];
}

void t0_serv(void){
    if(t100 < 100){  // 6,4k Hz
        if(t100%17==2){ // przydzielenie ok. 64hz na każdy z segmentów
            seg_serv();
            led_b += led_b;
        }
        if(t100 == pwm){
            *I8255b = pwmLow;
            LED_off = True;
        }
    }
    else{   // 64 Hz
        t100 = 0;
        led_b = 1;
        if(pwm) {
            *I8255b = pwmHigh;
            LED_off = False;
        }
        if(!key_timer){
            if(key != 0) {
                key_serv();
                key = 0;
                key_timer = 234;
            }
        }
        else{
            key_timer+=1;
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
    t100+=1;
    t0_f = True; //fakt wystąpienia przerwania
}

void sio_int(void) __interrupt(4)
{
    if (TI) { //gdy pusty bufor nadajnika
        TI = False;
    }
    else {
        RI = False; //zeruj flagę odbioru znaku
        rec_f = True; //ustaw flagę odebrania znaku
    }
}