#include <avr/io.h>

#define SERVO_0_DEG 2000
#define SERVO_180_DEG 4000

void servo_init() {
    // Sätt PB1 (pin 9) som output
    DDRB |= (1 << PB1);

    // Fast PWM, mode 14: ICR1 som TOP
    TCCR1A |= (1 << WGM11);
    TCCR1B |= (1 << WGM12) | (1 << WGM13);

    // Non-inverting mode på OC1A
    TCCR1A |= (1 << COM1A1);

    // Prescaler = 8
    TCCR1B |= (1 << CS11);

    // Sätt period till 20 ms (50 Hz)
    ICR1 = 40000;
}

static void servo_set_angle(uint16_t pulse) {
    // pulse i ticks (0.5 µs per tick)
    OCR1A = pulse;
}

void servo_open()
{
    servo_set_angle(SERVO_0_DEG);
}
void servo_close()
{
    servo_set_angle(SERVO_180_DEG);
}