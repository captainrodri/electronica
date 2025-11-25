/*
 * PROGRAMA DE TEST: HC-SR04 CON DIVISOR DE TENSIÓN
 *
 * Mide la distancia y enciende un LED según la proximidad.
 * - RB13 (Cerca):  distancia < 10 cm
 * - RB14 (Lejos):  distancia >= 10 cm
 *
 * CONEXIONES (Según diagrama):
 * - VCC  -> +5V
 * - GND  -> GND
 * - TRIG -> RB3 (Salida directa del PIC al Sensor)
 * - ECHO -> RB4 (Entrada al PIC protegida por divisor)
 * * ECHO del Sensor -> R1 (10k) -> RB4
 * * RB4 -> R2 (22k) -> GND
 * (Voltaje en RB4 = ~3.4V, Nivel lógico Alto válido)
 */

#include <p33FJ32MC202.h>
#include "config.h"       // Asegúrate de que FCY esté definido aquí (ej. 40000000UL)
#include <libpic30.h>     // Para __delay_us y __delay_ms

// --- Definición de Pines ---
// Según tu foto: TRIG va a RB3 y ECHO va a RB4
#define TRIG_PIN (1 << 3) // RB3
#define ECHO_PIN (1 << 4) // RB4 

/**
 * @brief Configura pines del Sensor y LEDs
 */
void inicializarPuertos(void) {
    AD1PCFGL = 0xFFFF; // Todos los pines como Digitales (Importante para RB3/RB4 que son analógicos)

    // --- Configuración de LEDs (Salidas) ---
    TRISB &= ~((1 << 13) | (1 << 14));
    PORTB |= ((1 << 13) | (1 << 14)); // Apagados (asumiendo lógica negativa/sink)

    // --- Configuración del Sensor ---
    
    // TRIG (RB3) como Salida
    TRISB &= ~TRIG_PIN;
    PORTB &= ~TRIG_PIN; // Empezar en BAJO

    // ECHO (RB4) como Entrada
    TRISB |= ECHO_PIN;
}

/**
 * @brief Configura el Timer 2 como cronómetro
 * Cálculo: FCY=40MHz, Prescaler 1:8 -> Tcy = 25ns * 8 = 200ns por tick
 */
void inicializarTimer2_Cronometro(void) {
    T2CON = 0x0000;         // Parar Timer
    T2CONbits.TCKPS = 0b01; // Prescaler 1:8
    TMR2 = 0;               // Limpiar contador
    PR2 = 0xFFFF;           // Periodo máximo
    // El timer se enciende (TON=1) solo cuando lo necesitamos
}

/**
 * @brief Programa Principal
 */
int main(void) {
    inicializarReloj(); // Asegúrate de tener esta función en tu config.c
    inicializarPuertos();
    inicializarTimer2_Cronometro();

    unsigned int pulso_ticks = 0;
    unsigned int distancia_cm = 0;
    unsigned int portb_buffer;

    while (1) {
        
        // --- 1. Enviar pulso TRIG de 10us ---
        PORTB |= TRIG_PIN;  // RB3 Alto
        __delay_us(10);
        PORTB &= ~TRIG_PIN; // RB3 Bajo

        // --- 2. Esperar flanco de subida en ECHO (RB4) ---
        TMR2 = 0;
        T2CONbits.TON = 1; // Encender Timer
        
        // Leer RB4: ((PORTB >> 4) & 1)
        while (((PORTB >> 4) & 1) == 0) {
            if (TMR2 > 50000) { // Timeout de seguridad (~10ms)
                break;
            }
        }
        
        // --- 3. Medir duración del pulso en ALTO ---
        TMR2 = 0; // Reiniciar cuenta para medir el ancho del pulso
        
        while (((PORTB >> 4) & 1) == 1) {
            if (TMR2 > 60000) { // Timeout (~12ms, fuera de rango > 4m)
                break;
            }
        }
        
        pulso_ticks = TMR2; // Capturar ticks
        T2CONbits.TON = 0;  // Apagar Timer para ahorrar energía

        // --- 4. Calcular distancia ---
        // Fórmula: Distancia = (Tiempo_us) / 58
        // 1 tick = 0.2us (200ns).
        // Ticks necesarios para 1 cm = 58us / 0.2us = 290 ticks.
        
        if (pulso_ticks < 60000) {
            distancia_cm = pulso_ticks / 290;
        } else {
            distancia_cm = 999; // Código de error (muy lejos o desconectado)
        }
        
        // --- 5. Lógica de LEDs ---
        portb_buffer = PORTB;
        portb_buffer |= ((1 << 13) | (1 << 14)); // Apagar ambos primero

        if (distancia_cm < 10) {
            // Cerca (<10cm): Encender LED RB13
            portb_buffer &= ~(1 << 13);
        } else {
            // Lejos (>=10cm): Encender LED RB14
            portb_buffer &= ~(1 << 14);
        }
        
        PORTB = portb_buffer; // Actualizar puerto

        // --- 6. Retardo para estabilidad ---
        __delay_ms(100);
    }

    return 0;
}
