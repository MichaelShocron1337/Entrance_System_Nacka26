#ifndef KEYPAD_H
#define KEYPAD_H

char keypad_get_key(void);
char keypad_get_key_debounced(void);
void keypad_init(void);

#endif