//Obsługa wyświetlacza ciekłokrystalicznego
#include "8051.h"
#define TRUE 1
#define FALSE 0
#define SSIZE 11
#define Tbuff 3

__code unsigned char string[SSIZE] = {'K', 'o', 'c','h','a','m', ' ', 'D','S','M','!'};
__code unsigned char string2[SSIZE] = {' ', '.', '.', '.', 'c', 'z', 'a', 's', 'a', 'm', 'i'};

__xdata unsigned char * LCD_WC = (__xdata unsigned char *) 0xFF80;  // write command
__xdata unsigned char * LCD_WD = (__xdata unsigned char *) 0xFF81;  // write data
__xdata unsigned char * LCD_RS = (__xdata unsigned char *) 0xFF82;  // read status
__xdata unsigned char * LCD_RD = (__xdata unsigned char *) 0xFF83;  // read data
__xdata unsigned char * led_wyb = (__xdata unsigned char *) 0xFF30; //bufor wybierajacy wyswietlacz
unsigned char i, iter, led_b, tbuff, pressed_key;
unsigned char c;
unsigned char waiter;
__bit t0_flag;
__bit key_pressed;
__bit ready;

void display_init();
void display_strings();
void key_serv();
void move_left();
void move_right();
void switch_line();
void return_home();
void wait_some();
void await();

void main(){
    display_init();
    TMOD = 0b00100001; //przerwania co ok. 70 ms
    TL0 = 0; TH0 = 0;
    tbuff = Tbuff;
    t0_flag = FALSE;
    ET0 = TRUE;
    ES = TRUE;
    EA = TRUE;
    TR0 = TRUE;
    iter = 0;
    led_b = 1;
    pressed_key = FALSE;
    key_pressed = FALSE;
    ready = TRUE;
    waiter = 32;
    display_strings();
    while(TRUE) {
        if (iter == 6) {
            iter = 0;
            led_b = 1;
        }
        *led_wyb = led_b;
        if (P3_5 && !key_pressed && ready) {
            pressed_key = led_b;
            key_pressed = TRUE;
        }
        iter++;
        led_b += led_b;
        if(t0_flag){
            t0_flag = FALSE;
            tbuff--;
            if(!tbuff){
                tbuff = Tbuff;
                if(key_pressed && ready) {
                    if (pressed_key != 0) {
                        key_serv();
                        pressed_key = 0;
                    }
                    key_pressed = FALSE;
                    ready = FALSE;
                }
                else if (!ready){
                    ready = TRUE;
                }
            }
        }
    }
}

void key_serv(){
    await();
    if(pressed_key == 1){ // ENT
        c = *LCD_RD;
        if(c >= 'a' && c <= 'z'){
            c -= 32;
        }
        await();
        *LCD_WD = c;
    }
    else if(pressed_key == 2){ // ESC
        c = *LCD_RD;
        if(c >= 'A' && c <= 'Z'){
            c += 32;
        }
        await();
        *LCD_WD = c;
    }
    else if(pressed_key == 4){ // R
        move_right();
    }
    else if(pressed_key == 8){ // Up
        switch_line();
    }
    else if(pressed_key == 16){ // D
        switch_line();
    }
    else if(pressed_key == 32){ // L
        move_left();
    }
    await();
}

void display_strings(){
    for(i = 0; i < SSIZE; i++){
        await();
        *LCD_WD = string[i];
    }
    wait_some();
    return_home();
    switch_line();
    for(i = 0; i < SSIZE; i++){
        wait_some();
        await();
        *LCD_WD = string2[i];
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
void wait_some(){
    while(TRUE){
        if(t0_flag){
            t0_flag = FALSE;
            if(!waiter--){
                waiter = 1;
                break;
            }
        }
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
    TL0 = 0;
    TH0 = 0;
    t0_flag = TRUE;
}