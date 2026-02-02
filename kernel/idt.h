#ifndef IDT_H
#define IDT_H

void idt_init(void);
void isr_handler(void *frame_ptr);

#endif
