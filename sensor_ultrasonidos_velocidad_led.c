#include <p33FJ32MC202.h>
#include "config.h"

// Configuración Timer 3 (50us)
#define PR3_VALOR 1999

// UMBRAL DE VELOCIDAD
// Significa: "¿Cuántos cm ha avanzado la mano en 60ms?"
// 5 cm en 60ms equivale a unos 0.8 metros/segundo (un movimiento rápido).
// Si ves que cuesta encenderlo, baja este número a 3 o 4.
#define UMBRAL_RAPIDO 5 

// Variables globales volatiles
static volatile int estado = 0;
static volatile int contador = 0;
static volatile unsigned int tiempo_vuelo = 0;
static volatile int dato_listo = 0;

void inicializarPuertos(void) {
    AD1PCFGL = 0xFFFF; // Digitales

    // RB3 (Trigger) -> Salida (0)
    // RB14 (LED Rojo) -> Salida (0)
    // RB4 (Echo) -> Entrada (1)
    TRISB &= ~((1 << 3) | (1 << 14)); 
    TRISB |= (1 << 4);

    // Estado inicial:
    // Trigger a 0
    // RB14 a 1 (APAGADO, lógica negativa)
    PORTB &= ~(1 << 3);
    PORTB |= (1 << 14);
}

void inicializarTimer3(void) {
    T3CON = 0x0000;
    PR3 = PR3_VALOR;
    TMR3 = 0;
    IFS0bits.T3IF = 0;
    IEC0bits.T3IE = 1;
    T3CONbits.TON = 1;
}

// --- MÁQUINA DE ESTADOS (Gestiona el pulso y la espera) ---
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
        case 2: // Esperar Eco
            if (eco_actual == 1) {
                contador = 0;
                estado = 3;
            } else {
                contador++;
                if (contador > 600) estado = 4; // Timeout
            }
            break;
        case 3: // Medir
            if (eco_actual == 1) {
                contador++;
            } else {
                tiempo_vuelo = contador;
                dato_listo = 1; // ¡Dato nuevo!
                contador = 0;
                estado = 4;
            }
            break;
        case 4: // Cooldown (~60ms)
            // Esto es CLAVE: Mantiene el ritmo constante para medir velocidad
            contador++;
            if (contador > 1200) estado = 0; 
            break;
    }
}

int main(void) {
    inicializarReloj();
    inicializarPuertos();
    inicializarTimer3();

    unsigned long dist_actual = 0;
    unsigned long dist_anterior = 0;
    int diferencia = 0;
    int primera_medida = 1;

    while (1) {
        if (dato_listo == 1) {
            dato_listo = 0; // Bajamos bandera

            // 1. Calcular distancia en cm
            // (ticks * 50us) / 58
            dist_actual = ((unsigned long)tiempo_vuelo * 50) / 58;

            if (primera_medida) {
                dist_anterior = dist_actual;
                primera_medida = 0;
            } else {
                // 2. Calcular Velocidad (Cambio de posición)
                // Solo nos interesa si nos ACERCAMOS (Anterior > Actual)
                if (dist_anterior > dist_actual) {
                    diferencia = dist_anterior - dist_actual;
                } else {
                    diferencia = 0; // Nos alejamos o estamos quietos
                }

                // 3. Lógica del LED RB14
                if (diferencia >= UMBRAL_RAPIDO) {
                    // ENCENDER RB14 (Poner a 0)
                    PORTB &= ~(1 << 14);
                } else {
                    // APAGAR RB14 (Poner a 1)
                    // Esto cubre movimiento lento, estar quieto o alejarse
                    PORTB |= (1 << 14);
                }

                // Actualizar para el siguiente ciclo
                dist_anterior = dist_actual;
            }
        }
    }
    return 0;
}
