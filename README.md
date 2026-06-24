# PIC16F887 Washing Machine Controller

Embedded washing machine controller built on PIC16F887 
featuring forward/reverse motor control, LCD menu system 
and interrupt driven emergency stop.

## Features
## Features
- Temperature mode selection (20C / 30C / 40C)
- Water level selection (Level 1-5)
- Motor timing modes (5s / 10s / 15s)
- Mode selection guard (blocks start without mode pick)
- Timer1-based real-time cycle timing (ISR driven)
- Forward and reverse motor control via relay
- Emergency stop via external interrupt (RB0)
- LCD 16x2 display for mode indication
  
## Tools Used
- MPLAB X IDE
- XC8 Compiler
- Proteus Simulation

## Circuit Schematic
![Simulation](proteus_simulation.png)

## Real Hardware
![Hardware](hardware.jpeg)

## Known Issues
- External interrupt stop button not working 
  reliably after first trigger - under investigation
