#ifndef KEYBOARD_H
#define KEYBOARD_H
#include <stdbool.h>

void keyboard_init(void);
void irq_keyboard_handler(void);
bool keyboard_getchar(char *c);

#endif
