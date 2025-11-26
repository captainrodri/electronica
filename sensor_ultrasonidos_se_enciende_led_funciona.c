#include <p33FJ32MC202.h>
#include "config.h"

// Configuración del Timer para 50 microsegundos
// FCY = 40 MHz -> PR3 = 1999
#define PR3_VALOR 1999

// Variables volatiles para la interrupción
static volatile int estado = 0;         
static volatile int contador = 0;       
static volatile unsigned int tiempo_vuelo = 0; 
static volatile int dato_listo = 0;     

void inicializarPuertos(void) {
    AD1PCFGL = 0xFFFF; // Todo digital

    // --- CONFIGURACIÓN DE DIRECCIÓN (TRIS) ---
    // RB3 (Trigger) -> Salida (0)
    // RB13 (LED)    -> Salida (0)
    // RB4 (Echo)    -> Entrada (1)
    TRISB &= ~((1 << 3) | (1 << 13)); 
    TRISB |= (1 << 4);

    // --- ESTADO INICIAL ---
    // Trigger (RB3) a 0
    // LED (RB13) a 1 para que empiece APAGADO (Lógica negativa)
    PORTB &= ~(1 << 3);
    PORTB |= (1 << 13); 
}

void inicializarTimer3(void) {
    T3CON = 0x0000;         
    PR3 = PR3_VALOR;        
    TMR3 = 0;               
    IFS0bits.T3IF = 0;      
    IEC0bits.T3IE = 1;      
    T3CONbits.TON = 1;      
}

// Máquina de Estados (Igual que la anterior)
void __attribute__((interrupt, no_auto_psv)) _T3Interrupt(void) {
    IFS0bits.T3IF = 0; 
    int eco_actual = (PORTB >> 4) & 1;

    switch (estado) {
        case 0: // Disparo
            PORTB |= (1 << 3);  
            estado = 1;
            break;
        case 1: // Fin Disparo (50us)
            PORTB &= ~(1 << 3); 
            contador = 0;       
            estado = 2;
            break;
        case 2: // Esperar Subida Echo
            if (eco_actual == 1) {
                contador = 0;   
                estado = 3;
            } else {
                contador++;
                if (contador > 600) estado = 4; 
            }
            break;
        case 3: // Medir Anchura
            if (eco_actual == 1) {
                contador++; 
            } else {
                tiempo_vuelo = contador;
                dato_listo = 1; 
                contador = 0;
                estado = 4;
            }
            break;
        case 4: // Cooldown
            contador++;
            if (contador > 1200) estado = 0; 
            break;
    }
}

int main(void) {
    inicializarReloj();
    inicializarPuertos();
    inicializarTimer3();

    unsigned long distancia_cm = 0;

    while (1) {
        if (dato_listo == 1) {
            dato_listo = 0; // Reset bandera
            
            // Cálculo: (ticks * 50us) / 58
            // Usamos unsigned long para evitar desbordamiento en la multiplicación
            distancia_cm = ((unsigned long)tiempo_vuelo * 50) / 58;

            // --- LÓGICA DE CONTROL DEL LED RB13 ---
            if (distancia_cm <= 15) {
                // ENCENDER (Poner a 0)
                PORTB &= ~(1 << 13);
            } else {
                // APAGAR (Poner a 1)
                PORTB |= (1 << 13);
            }
        }
    }
    return 0;
}
