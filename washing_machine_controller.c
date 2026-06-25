/*******************************************************************************
 * File: main.c
 * * Description: 
 * Firmware for an Embedded Washing Machine Controller using PIC16F887.
 * Features include temperature/water level selection, interrupt-driven 
 * real-time cycle timing, forward/reverse relay motor control, and an 
 * external interrupt-driven emergency stop.
 * * Target: PIC16F887
 * Compiler: XC8
 * Oscillator: 4 MHz (Internal)
 ******************************************************************************/

#include <xc.h>

// === CONFIGURATION BITS ===
#pragma config FOSC = INTRC_NOCLKOUT // Internal oscillator, no clock out
#pragma config WDTE = OFF            // Watchdog Timer disabled
#pragma config PWRTE = ON            // Power-up Timer enabled
#pragma config MCLRE = ON            // RE3/MCLR pin function is MCLR
#pragma config CP = OFF              // Code protection disabled
#pragma config CPD = OFF             // Data code protection disabled
#pragma config BOREN = OFF           // Brown Out Reset disabled
#pragma config IESO = ON             // Internal External Switchover mode enabled
#pragma config FCMEN = ON            // Fail-Safe Clock Monitor enabled
#pragma config LVP = OFF             // Low Voltage Programming disabled
#pragma config BOR4V = BOR40V        // Brown-out Reset set to 4.0V
#pragma config WRT = OFF             // Flash Memory Self-Write Protection off

// === MACROS & DEFINES ===
#define _XTAL_FREQ 4000000           // Define system clock for __delay_ms()

// === GLOBAL VARIABLES ===
// UI State Counters
int countb1 = 0; // Tracks selected temperature mode
int countb2 = 0; // Tracks selected water level
int countb3 = 0; // Tracks selected motor timing mode
int modeSelected = 0; // Flag: 1 if timing mode is locked in, 0 otherwise

// System State Flags
volatile int motorRunning = 0; 
volatile int tick = 0;         // Half-second counter incremented by Timer1
volatile int sec = 0;          // Full-second counter

// LCD Display Strings
char *temp[] = {"WATER TEMP 20C", "WATER TEMP 30C", "WATER TEMP 40C"};
char *waterlvl[] = {"WATER LEVEL 01", "WATER LEVEL 02", "WATER LEVEL 03",
                    "WATER LEVEL 04", "WATER LEVEL 05"};
char *power1[] = {"LOW MODE", "MEDIUM MODE", "HIGH MODE"};
char *power2[] = {"FOR&REV - 5S", "FOR&REV - 10S", "FOR&REV - 15S"};

// === FUNCTION PROTOTYPES ===
void pulse(void);
void lcd(char rn, char data);
void lcdclear(void);
void lcdline(char addr, char *str);
void lcdshow(char *top, char *bot);
void stopMotor(void);
void runMotor(void);

// === LCD DRIVER FUNCTIONS ===

/* Generates an Enable (EN) pulse for the LCD on pin RE1 */
void pulse(void) {
    RE1 = 1;
    __delay_ms(5);
    RE1 = 0;
    __delay_ms(5);
}

/* Sends Command (rn=0) or Data (rn=1) to LCD. RS is on RE0, Data on PORTD */
void lcd(char rn, char data) {
    RE0 = rn;
    PORTD = data;
    pulse();
}

/* Clears the LCD display and returns cursor to home */
void lcdclear(void) {
    lcd(0, 0x01);
    __delay_ms(2); // Clear command requires a longer delay
}

/* Writes a string up to 16 characters to a specific DDRAM address */
void lcdline(char addr, char *str) {
    int i = 0;
    lcd(0, addr); // Set cursor address
    while (str[i] && i < 16) {
        lcd(1, str[i]);
        i++;
    }
    // Pad remaining characters with spaces to clear old text
    while (i < 16) {
        lcd(1, ' ');
        i++;
    }
}

/* Helper function to update both rows of the 16x2 LCD simultaneously */
void lcdshow(char *top, char *bot) {
    lcdline(0x80, top); // 0x80: Line 1 starting address
    lcdline(0xC0, bot); // 0xC0: Line 2 starting address
}

// === MOTOR CONTROL FUNCTIONS ===

/* Safely disengages all motor driving relays */
void stopMotor(void) {
    motorRunning = 0;
    RC0 = 0; // Relay 1 OFF
    RC1 = 0; // Relay 2 OFF
    RC2 = 0; // Main Power OFF
    RC3 = 1; // System Standby state indicator
}

/* Executes the selected washing cycle sequence using Timer1 for timing */
void runMotor(void) {
    int halfTime, endTime;
    int mode = countb3;

    // Determine cycle duration based on selected mode
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

    // Reset timing variables before starting
    sec = 0;
    tick = 0;
    RC3 = 0; // Standby indicator OFF
    RC2 = 1; // Main Power ON
    motorRunning = 1;

    lcdclear();
    lcdshow("RUNNING...", "MOTOR ON");

    // Main cycle loop: Executes forward and reverse directions
    while (motorRunning == 1) {
        // Cycle complete condition
        if (sec >= endTime) {
            stopMotor();
            break;
        }
        
        // Directional control (Forward for first half, Reverse for second half)
        if (sec < halfTime) {
            RC0 = 1; // Forward relay engaged
            RC1 = 0;
        } else {
            RC0 = 0;
            RC1 = 1; // Reverse relay engaged
        }
    }
    stopMotor(); // Ensure relays are off if loop exits unexpectedly
}

// === INTERRUPT SERVICE ROUTINE ===

void __interrupt() ISR(void) {
    // External Interrupt (RB0/INT) - Emergency Stop Triggered
    if (INTF == 1) {
        stopMotor(); // Immediate hardware halt
        INTF = 0;    // Clear interrupt flag
    }
    
    // Timer1 Overflow Interrupt - Handles 1-second system clock
    if (TMR1IF == 1) {
        TMR1 = 3036; // Preload for 500ms delay at 4MHz (65536 - 62500)
        TMR1IF = 0;  // Clear interrupt flag
        
        tick++;
        if (tick >= 2) { // 2 ticks = 1 full second
            tick = 0;
            sec++;
        }
    }
}

// === MAIN APPLICATION ENTRY ===

void main(void) {
    // --- Port Initialization ---
    PORTA = PORTB = PORTC = PORTD = PORTE = 0x00;
    
    // TRIS Config: 1 = Input, 0 = Output
    TRISA = 0x0F; // RA0-RA3 as inputs (Push buttons)
    TRISB = 0x01; // RB0 as input (External Interrupt / Emergency Stop)
    TRISC = TRISD = TRISE = 0x00; // Output lines for LCD and Relays
    
    // Disable analog inputs, configure all pins as digital I/O
    ANSEL = ANSELH = 0x00;

    // --- Interrupt & Timer Configuration ---
    
    // Enable Global (GIE), Peripheral (PEIE), and External RB0 (INTE) interrupts
    INTCON = 0xD0; 
    
    // Configure Timer1: 1:8 Prescaler, Internal clock, Timer ON
    T1CON = 0x31; 
    
    // Preload Timer1 for a 500ms overflow and enable its interrupt
    TMR1 = 3036; 
    TMR1IE = 1; 
    
    // Configure OPTION_REG: Enable PORTB pull-ups, Interrupt on Rising Edge
    OPTION_REG = 0x40; 

    // --- LCD Initialization Sequence ---
    lcd(0, 0x38); // 8-bit mode, 2 lines, 5x8 dots
    lcd(0, 0x0E); // Display ON, Cursor ON
    lcdclear();                     
    lcdshow("SELECT MODES", "");

    // --- Main Control Loop ---
    while (1) {
        
        // Button 1: Temperature Selection
        if (RA1 == 1) {
            lcdclear();            
            lcdshow(temp[countb1], "TEMP SELECT");
            countb1++;
            if (countb1 > 2) countb1 = 0;
            while (RA1 == 1); // Blocking wait for button release (Debounce strategy)
        }
        
        // Button 2: Water Level Selection
        else if (RA2 == 1) {
            lcdclear();             
            lcdshow(waterlvl[countb2], "LEVEL SELECT");
            countb2++;
            if (countb2 > 4) countb2 = 0;
            while (RA2 == 1); 
        }
        
        // Button 3: Motor Timing Mode Selection
        else if (RA3 == 1) {
            modeSelected = 1; // Unlock start sequence
            lcdclear();          
            lcdshow(power1[countb3], power2[countb3]);
            countb3++;
            if (countb3 > 2) countb3 = 0;
            while (RA3 == 1); 
        }

        // Button 4: Start Cycle
        if (RA0 == 1) {
            while (RA0 == 1); 
            
            // Safety Check: Prevent start if timing mode isn't chosen
            if (modeSelected == 0) {
                lcdclear();       
                lcdshow("SELECT MODE", "FIRST!");
                __delay_ms(1000);
                lcdclear();          
                lcdshow("SELECT MODES", "");
            } else {
                // Execute Cycle
                runMotor();
                
                // Reset State post-cycle
                lcdclear();         
                lcdshow("CYCLE DONE", "SELECT MODE");
                modeSelected = 0;
                countb3 = 0; // Reset mode selection logic
            }
        }
    }
}
