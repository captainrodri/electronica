#include <p33FJ32MC202.h>
#include "config.h"


static int ticks = 0;
static int periodo = 0;
static int temp;
static unsigned int distancia=18;
static unsigned int velocidad;

int main(void) {
    inicializarReloj();
    inicializarPuertos();
    inicializarTimer1(); // Temporizador para ventilador
   

    while(1){
       
        // distancia = medirDistancia();
        // velocidad = medirVelocidad();
        // displayLEDs();
        // temp = medirTemp();

    }
    return 0;
}

void inicializarPuertos(void){
    AD1PCFGL = 0xFFFF;
   
    // Sensor Distancia:
    TRISB &= ~(1 << 12); // Salida para Trig
    PORTB &= ~(1<<12); // RB11 nivel bajo
   

    return;
}

void inicializarTimer1(void){
    T1CON = 0x0000;
    PR1 = 20000;
   
    TMR1 = 0;
    IFS0bits.T1IF = 0;
    IEC0bits.T1IE = 1;
    IPC0bits.T1IP = 4;
    T1CON |= (1<<15);

    return;
}

void __attribute__((__interrupt__, no_auto_psv)) _T1Interrupt(void)
{
    IFS0bits.T1IF = 0;
    ticks++;
    if(distancia<15){
        periodo=2;
    }else if(distancia<20){
        periodo=20;
    }else{
        periodo=0;
    }
   
   
    if (ticks == periodo/2){
        PORTB ^= (1<<12);
    }
    if (ticks==periodo){
        ticks=0;
    }
    return;
}
