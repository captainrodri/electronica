#include <p33FJ32MC202.h>
#include "config.h"

// Configuración del Timer para 50 microsegundos (según Tema 13)
// FCY = 40 MHz (Tcy = 25ns). 50us / 25ns = 2000 ticks.
#define PR3_VALOR 1999

// Variables globales (volatile porque se cambian en la ISR)
static volatile int estado = 0;         // Máquina de estados
static volatile int contador = 0;       // Cuenta de 50us
static volatile unsigned int tiempo_vuelo = 0; // Resultado crudo
static volatile int dato_listo = 0;     // Bandera para el main

void inicializarPuertos(void) {
    // 1. Configurar pines como DIGITALES (Fundamental para RB3/AN5)
    AD1PCFGL = 0xFFFF; 

    // 2. Configurar Direcciones (TRIS)
    // RB3 (Trigger) -> Salida (0). RB4 (Echo) -> Entrada (1).
    TRISB &= ~(1 << 3); 
    TRISB |= (1 << 4);

    // 3. Estado inicial: Trigger (RB3) a 0.
    PORTB &= ~(1 << 3);
}

void inicializarTimer3(void) {
    T3CON = 0x0000;         // Apagado, Prescaler 1:1
    PR3 = PR3_VALOR;        // 50us por interrupción
    TMR3 = 0;               
    
    IFS0bits.T3IF = 0;      // Borrar bandera
    IEC0bits.T3IE = 1;      // Habilitar interrupción
    T3CONbits.TON = 1;      // Encender Timer
}

// Máquina de Estados (Basada en Tema 13 - Ejemplo 1 )
void __attribute__((interrupt, no_auto_psv)) _T3Interrupt(void) {
    IFS0bits.T3IF = 0; 

    // Lectura del pin de Eco (RB4)
    int eco_actual = (PORTB >> 4) & 1;

    switch (estado) {
        // --- Estado 0: Iniciar Disparo ---
        case 0:
            PORTB |= (1 << 3);  // Trigger RB3 ALTO
            estado = 1;
            break;

        // --- Estado 1: Finalizar Disparo (tras 50us) ---
        case 1:
            PORTB &= ~(1 << 3); // Trigger RB3 BAJO
            contador = 0;       // Reiniciar contador para timeout
            estado = 2;
            break;

        // --- Estado 2: Esperar Flanco de Subida del Eco ---
        case 2:
            if (eco_actual == 1) {
                contador = 0;   // Empezar a medir tiempo
                estado = 3;
            } else {
                // Timeout de seguridad (~30ms)
                contador++;
                if (contador > 600) estado = 4; 
            }
            break;

        // --- Estado 3: Medir Anchura del Pulso ---
        case 3:
            if (eco_actual == 1) {
                contador++; // Sigue en alto, contamos tiempo
            } else {
                // Flanco de bajada detectado: Fin de la medida
                tiempo_vuelo = contador;
                dato_listo = 1; // Avisar al main
                contador = 0;
                estado = 4;
            }
            break;

        // --- Estado 4: Tiempo de Espera (Cooldown) ---
        case 4:
            contador++;
            // Esperar aprox 60ms antes del siguiente disparo [cite: 1796]
            if (contador > 1200) { 
                estado = 0; // Volver a empezar
            }
            break;
    }
}

int main(void) {
    inicializarReloj();
    inicializarPuertos();
    inicializarTimer3();

    unsigned int distancia_aprox = 0;

    while (1) {
        // Si la interrupción dice que hay dato nuevo, lo procesamos
        if (dato_listo == 1) {
            dato_listo = 0; // Limpiamos la bandera
            
            // Cálculo simple para verificar:
            // tiempo_vuelo * 50us / 58 = cm
            // Operación aproximada con enteros:
            distancia_aprox = (tiempo_vuelo * 50) / 58;
            
            // Aquí es donde pondrías un Breakpoint para ver "distancia_aprox"
            // o usarías LEDs para visualizarlo si lo necesitas.
        }
    }
    return 0;
}
