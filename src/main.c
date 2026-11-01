#include "app.h"
#include <avr/interrupt.h>

int main(void)
{
    app_init();
    sei();

    while (1)
    {
        app_run();
    }
}