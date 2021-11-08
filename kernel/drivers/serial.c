#include "serial.h"

#include <io.h>
#include <types.h>

void serial_init(void) {
    io_outb(SERIAL_PORT + 1, 0x00);
    io_outb(SERIAL_PORT + 3, 0x80);
    io_outb(SERIAL_PORT + 0, 0x03);
    io_outb(SERIAL_PORT + 1, 0x00);
    io_outb(SERIAL_PORT + 3, 0x03);
    io_outb(SERIAL_PORT + 2, 0xC7);
    io_outb(SERIAL_PORT + 4, 0x0B);

    // serial faultyness check
    io_outb(SERIAL_PORT + 4, 0x1E);
    io_outb(SERIAL_PORT + 0, 0xAE);
    if (io_inb(SERIAL_PORT) != 0xAE) {
        // panic
    }

    // not faulty -> set serial port to normal operation mode
    io_outb(SERIAL_PORT + 4, 0x0F);
}

bool serial_is_transmit_empty(void) {
    return io_inb(SERIAL_PORT + 5) & 0x20;
}

void serial_putc(char c) {
    while (serial_is_transmit_empty() == 0) {}
    io_outb(SERIAL_PORT, c);
}

void serial_puts(char* s) {
    while (*s) {
        // also do carriage return, not only line feed
        if (*s == '\n') {
            serial_putc('\r');
        }

        serial_putc(*s);
        s++;
    }
}
