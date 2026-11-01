#include <avr/io.h>
#include "buzzer.h"

/*
    Arduino Uno / ATmega328P

    Timer2 används i CTC-läge.
    Utgång: OC2B = PD3 = Arduino digital pin 3

    Frekvens:
        f = F_CPU / (2 * prescaler * (1 + OCR2A))

    Med:
        F_CPU = 16 MHz
        prescaler = 128
        OCR2A = 141

        f ≈ 440 Hz
*/

void buzzer_init(void)
{
    // PD3 / OC2B / Arduino pin 3 som output
    DDRD |= (1 << DDD3);

    // Stoppa timer under konfiguration
    TCCR2A = 0;
    TCCR2B = 0;

    // CTC mode
    TCCR2A |= (1 << WGM21);

    // Compare value för ~440Hz
    OCR2A = 141;

    buzzer_quiet();
}

void buzzer_scream(void)
{
    // Toggle OC2B on compare match
    TCCR2A |= (1 << COM2B0);

    // Prescaler 128
    TCCR2B &= ~((1 << CS22) | (1 << CS21) | (1 << CS20));
    TCCR2B |= (1 << CS22) | (1 << CS20);
}

void buzzer_quiet(void)
{
    // Disconnect OC2B
    TCCR2A &= ~(1 << COM2B0);

    // Stop timer
    TCCR2B &= ~((1 << CS22) | (1 << CS21) | (1 << CS20));

    // Sätt pin låg
    PORTD &= ~(1 << PORTD3);
}