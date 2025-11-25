/**
 * @file main.c
 * @brief Test simple de ventilador (RB10) con pulsador (RB2) sin antirrebotes.
 */

#include <p33FJ32MC202.h>
#include "config.h" // Asumo que tienes este archivo de tu plantilla habitual

int main(void) {
    // 1. Inicialización del reloj (función estándar de tu asignatura)
    inicializarReloj();

    // 2. Configuración de Pines
    AD1PCFGL = 0xFFFF;      // Configurar todos los pines como digitales
    
    // Configurar RB2 como ENTRADA (Pulsador)
    // Ponemos el bit 2 a 1
    TRISB |= (1 << 2);

    // Configurar RB10 como SALIDA (Base del transistor del Motor)
    // Ponemos el bit 10 a 0
    TRISB &= ~(1 << 10);

    // Estado inicial: Motor Apagado (RB10 a nivel bajo)
    PORTB &= ~(1 << 10);

    // Variables para detectar el flanco
    int pulsador_actual = 1;
    int pulsador_ant = 1;

    // 3. Bucle Infinito
    while (1) {
        // Guardamos el estado anterior antes de leer el nuevo
        pulsador_ant = pulsador_actual;
        
        // Leemos el estado actual del pin RB2
        pulsador_actual = (PORTB >> 2) & 0x1;

        // Detectar Flanco de Bajada (Transición de 1 a 0)
        // Significa que acabamos de pulsar el botón
        if (pulsador_actual == 0 && pulsador_ant == 1) {
            
            // Conmutar (Toggle) el estado del motor en RB10
            // Usamos XOR (^) para invertir solo ese bit
            PORTB ^= (1 << 10);
        }
    }

    return 0;
}
