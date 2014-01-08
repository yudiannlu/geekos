/*
 * GeekOS - x86 port IO
 */

#ifndef ARCH_IOPORT_H
#define ARCH_IOPORT_H

#include <geekos/types.h>

u8_t ioport_inb(u16_t port);
void ioport_outb(u16_t port, u8_t value);
void ioport_delay(void);

#endif /* ARCH_IOPORT_H */
