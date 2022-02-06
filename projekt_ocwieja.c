#include "8051.h"

#define True 1
#define False 0
#define T0val 112 // 6400Hz  <-- 921,6:144 = 6,4  i  256-144 = 112
#define pwmLow 0b00001111
#define pwmHigh 0b11110000
#define sSize 15
__bit __at (0x97) LED_off;
__bit __at (0x96) SEG_off;
__bit t0_f;
__bit rec_f;
__bit send_f;
__bit pwm_STOP;
__bit pwm_SET;
__code unsigned char string[sSize*7] = {'>','1', '.', 'C','h','a','n', 'g', 'e',' ','s','t', 'a', 't', 'e', ' ', ' ', '>', '1', '.', '1', '.', 'S', 't', 'a', 'r', 't', ' ', ' ', ' ', ' ', ' ', '>', '1', '.', '2', '.', 'S', 't', 'o', 'p', ' ', ' ', ' ', ' ', '>','2', '.', 'S','e','t','t', 'i', 'n','g','s', ' ', ' ', ' ', ' ', ' ', ' ', '>', '2', '.', '1', '.', 'P', 'W', 'M', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>', '2', '.', '1', '.', '1', '.', ' ', '0', '3', '0', ' ', ' ', '>', '2', '.', '2', '.', 'R', 'e', 's', 'e', 't', ' ', ' ', ' '};

__xdata unsigned char * LCD_WC = (__xdata unsigned char *) 0xFF80;  // write command
__xdata unsigned char * LCD_WD = (__xdata unsigned char *) 0xFF81;  // write data
__xdata unsigned char * LCD_RS = (__xdata unsigned char *) 0xFF82;  // read status
__xdata unsigned char * LCD_RD = (__xdata unsigned char *) 0xFF83;  // read data
__xdata unsigned char *I8255b = (__xdata unsigned char *) 0xFF29; // port B
__xdata unsigned char *I8255r = (__xdata unsigned char *) 0xFF2B; // rejestr sterujący
__xdata unsigned char * led_wyb = (__xdata unsigned char *) 0xFF30; //bufor wybierajacy wyswietlacz
__xdata unsigned char * led_led = (__xdata unsigned char *) 0xFF38; //bufor wybierajacy segmenty
__xdata unsigned char * KEYBRD_L = (__xdata unsigned char *) 0xFF21; //bufor wybierajacy segmenty
__xdata unsigned char * KEYBRD_H = (__xdata unsigned char *) 0xFF22; //bufor wybierajacy segmenty

__code unsigned char WZOR[10] = {0b0111111, 0b0000110, 0b1011011, 0b1001111,
                                 0b1100110, 0b1101101, 0b1111101, 0b0000111, 0b1111111, 0b1101111};
unsigned char Aktualne[6] = {0, 0, 0, 0, 0, 0};
unsigned char iter;
unsigned char pwm;
unsigned char pwm_prev;
unsigned char tSec;     // 64 x 100hz aby otrzymać sekundę
unsigned char t100;
unsigned char send_buf;
unsigned char led_b;
unsigned char key, key_timer, key_mult, key_mult_prev;
unsigned char curr_string;
unsigned char i, j;

void rec_serv(void);
void send_serv(void);
void t0_serv(void);
void key_mult_read();
void seg_serv();
void seg_update();
void key_serv();
void display_init();
void display_strings();
void move_left();
void move_right();
void switch_line();
void return_home();
void await();
void key_mult_serve();

void main(){
    display_init();
    *I8255r = 0b10011001;
    pwm = 30;
    PCON = 0x80;
    SCON = 0b01010000;
    TMOD = 0b00100010;  // timery w tryb 2 (auto reload)
    IE = 0b10010010;    // 0b10011010 to enable inter from both timers
    TH0 = T0val;
    TL0 = T0val;
    TH1 = 0xFD;
    TL1 = 0xFD;
    tSec = 192;
    t100 = 255;
    led_b = 1;
    iter = 0;
    seg_update();
    display_strings();
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
            key_mult_read();
        }
    }
}

void key_mult_read(){
    key_mult = *KEYBRD_H;
    if(key_mult != key_mult_prev){
        key_mult_serve();
    }
    key_mult_prev = key_mult;
}
void key_mult_serve(){
    if(key_mult == 255-128){ //// enter
        if(curr_string == 1)        //start
            pwm_STOP = 0;
        else if(curr_string == 2)   //stop
            pwm_STOP = 1;
        else if(curr_string == 6){  //reset
            pwm = 30;
            seg_update();
        }
        else if(curr_string == 5){   //pwm_set
            if(pwm_SET){
                pwm_SET = 0;
            }
            else{
                pwm_SET = 1;
                pwm_prev = pwm;
            }
        }
    }else if(key_mult == 255-64){ //esc
        if(pwm_SET){
            pwm_SET = 0;
            pwm = pwm_prev;
        }
    }else if(key_mult == 255-32){ //dol
        if(pwm_SET){
            if(pwm != 0)
                pwm-=1;
        }
        else {

            if (curr_string < 6)
                curr_string += 1;
            else
                curr_string = 0;
            display_strings();
        }
    }else if(key_mult == 255-16){ //gora
        if(pwm_SET){
            if(pwm != 50)
                pwm+=1;
        }
        else {
            if (curr_string > 0)
                curr_string -= 1;
            else
                curr_string = 6;
            display_strings();
        }
    }else if(key_mult == 255-8){ //prawo
        if(pwm_SET){
            pwm += 10;
            if(pwm > 50){
                pwm = 50;
            }
        }
    }else if(key_mult == 255-4){ //lewo
        if(pwm_SET){
            pwm -= 10;
            if(pwm > 100){
                pwm = 0;
            }
        }
    }
    seg_update();
}
void seg_serv(){
    SEG_off = 1; // OFF
    *led_wyb = led_b;
    *led_led = Aktualne[iter];
    SEG_off = 0; // ON
    if(P3_5 && !key_timer) { // Odczyt klawiatury (czy wciśnięty klawisz przy danym segmencie)
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
        if(t100%10) {
            seg_serv();
            led_b += led_b;
            iter += 1;
            if (iter > 5) {
                led_b = 1;
                iter = 0;
            }
        }
        if(t100 == pwm){
            *I8255b = pwmLow;
            LED_off = True;
        }
    }
    else{   // 64 Hz
        t100 = 0;
        if(pwm && !pwm_STOP) {
            *I8255b = pwmHigh;
            LED_off = False;
        }
        else{
            *I8255b = pwmLow;
            LED_off = True;
        }
        if(!key_timer){
            if(key != 0) {
                key_serv();
                key = 0;
                key_timer = 240;
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
void display_strings(){
    await();
    return_home();
    for(j = 0; j < 2; j++) {
        for (i = 0; i < sSize; i++) {
            await();
            *LCD_WD = string[i+((j+curr_string)%7)*sSize];
        }
        return_home();
        switch_line();
    }
    return_home();
}
void move_left(){ //sc = 1 (pole), 0 (kursor) //rl = 1 (right), 0 (left)
    await();
    *LCD_WC = 0b10000;
}
void move_right(){
    await();
    *LCD_WC = 0b10100;
}
void switch_line(){
    for(i = 0; i < 40; i++){
        await();
        *LCD_WC = 0b10100;
    }
}
void display_init() {
    await();
    *LCD_WC = 0b1;
    await();
    *LCD_WC = 0b111000;
    await();
    *LCD_WC = 0b1110;
    await();
    *LCD_WC = 0b110; // Wprowadzanie danych
}
void return_home(){
    await();
    *LCD_WC = 0b10;
}
void await(){
    while(*LCD_RS >= 128); //wait
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