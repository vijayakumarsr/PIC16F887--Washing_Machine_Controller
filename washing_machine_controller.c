#include <xc.h>

#pragma config FOSC = INTRC_NOCLKOUT
#pragma config WDTE = OFF
#pragma config PWRTE = ON
#pragma config MCLRE = ON
#pragma config CP = OFF
#pragma config CPD = OFF
#pragma config BOREN = OFF
#pragma config IESO = ON
#pragma config FCMEN = ON
#pragma config LVP = OFF
#pragma config BOR4V = BOR40V
#pragma config WRT = OFF

#define _XTAL_FREQ 4000000

int countb1 = 0;
int countb2 = 0;
int countb3 = 0;
volatile int motorRunning = 0;
int modeSelected = 0;

char *temp[] = {"WATER TEMP 20C", "WATER TEMP 30C", "WATER TEMP 40C"};
char *waterlvl[] = {"WATER LEVEL 01", "WATER LEVEL 02", "WATER LEVEL 03",
    "WATER LEVEL 04", "WATER LEVEL 05"};
char *power1[] = {"LOW MODE", "MEDIUM MODE", "HIGH MODE"};
char *power2[] = {"FOR&REV - 5S", "FOR&REV - 10S", "FOR&REV - 15S"};

volatile int tick = 0;
volatile int sec = 0;

void pulse(void) {
    RE1 = 1;
    __delay_ms(5);
    RE1 = 0;
    __delay_ms(5);
}

void lcd(char rn, char data) {
    RE0 = rn;
    PORTD = data;
    pulse();
}

void lcdclear(void) {
    lcd(0, 0x01);
    __delay_ms(2);
}

void lcdline(char addr, char *str) {
    int i = 0;
    lcd(0, addr);
    while (str[i] && i < 16) {
        lcd(1, str[i]);
        i++;
    }
    while (i < 16) {
        lcd(1, ' ');
        i++;
    }
}

void lcdshow(char *top, char *bot) {
    lcdline(0x80, top);
    lcdline(0xC0, bot);
}

void stopMotor(void) {
    motorRunning = 0;
    RC0 = 0;
    RC1 = 0;
    RC2 = 0;
    RC3 = 1;
}

void runMotor(void) {
    int halfTime, endTime;
    int mode = countb3;

    if (mode == 1) {
        halfTime = 5;
        endTime = 10;
    } else if (mode == 2) {
        halfTime = 10;
        endTime = 20;
    } else {
        halfTime = 15;
        endTime = 30;
    }

    sec = 0;
    tick = 0;
    RC3 = 0;
    RC2 = 1;
    motorRunning = 1;

   
    lcdclear();
    lcdshow("RUNNING...", "MOTOR ON");

    while (motorRunning == 1) {

        if (sec >= endTime) {
            stopMotor();
            break;
        }
        if (sec < halfTime) {
            RC0 = 1;
            RC1 = 0;
        } else {
            RC0 = 0;
            RC1 = 1;
        }
    }
    stopMotor();
}

void __interrupt() ISR(void) {
    if (INTF == 1) {
        stopMotor();
        INTF = 0;
    }
    if (TMR1IF == 1) {
        TMR1 = 3036;
        TMR1IF = 0;
        tick++;
        if (tick >= 2) {
            tick = 0;
            sec++;
        }
    }
}

void main(void) {
    PORTA = PORTB = PORTC = PORTD = PORTE = 0x00;
    TRISA = 0x0F;
    TRISB = 0x01;
    TRISC = TRISD = TRISE = 0x00;
    ANSEL = ANSELH = 0x00;

    INTCON = 0xD0;
    T1CON = 0x31;
    TMR1 = 3036;
    TMR1IE = 1;
    OPTION_REG = 0x40;

    lcd(0, 0x38);
    lcd(0, 0x0E);
    lcdclear();                     
    lcdshow("SELECT MODES", "");

    while (1) {
        if (RA1 == 1) {
            lcdclear();            
            lcdshow(temp[countb1], "TEMP SELECT");
            countb1++;
            if (countb1 > 2) countb1 = 0;
            while (RA1 == 1);
        }
        else if (RA2 == 1) {
            lcdclear();             
            lcdshow(waterlvl[countb2], "LEVEL SELECT");
            countb2++;
            if (countb2 > 4) countb2 = 0;
            while (RA2 == 1);
        }
        else if (RA3 == 1) {
            modeSelected = 1;
            lcdclear();          
            lcdshow(power1[countb3], power2[countb3]);
            countb3++;
            if (countb3 > 2) countb3 = 0;
            while (RA3 == 1);
            
        }

        if (RA0 == 1) {
            while (RA0 == 1);
            if (modeSelected == 0) {
                lcdclear();       
                lcdshow("SELECT MODE", "FIRST!");
                __delay_ms(1000);
                lcdclear();          
                lcdshow("SELECT MODES", "");
            } else {
                runMotor();
                lcdclear();         
                lcdshow("CYCLE DONE", "SELECT MODE");
                modeSelected = 0;
                countb3 = 0;
            }
        }
    }
}
