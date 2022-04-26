#include "ps2.h"

#include <drivers/serial.h>
#include <interrupts/pic.h>
#include <io.h>
#include <types.h>

bool ps2_data_response_req = false; // if true don't handle irq as normal
static bool dual_channel;
static bool ps2_port1;
static bool ps2_port2;
uint8_t ps2_response_count = 0;
uint8_t ps2_responses_awaited = 1; // ps2 irqs aren't handled ps2_response_awaited times

/*
this snippet was copied and adapted from:
K Lange; ps2hid.c; static int ps2_wait_input(void), 17.10.2021
*/
// Wait until the PS/2 controller's input buffer is clear.
// Use this before WRITING to the controller.
uint8_t ps2_wait_input(void) {
    uint64_t timeout = 100000UL;
    while (--timeout) {
        if (!(io_inb(PS2_STATUS) & 2)) {
            return 0;
        }
    }
    return 1;
}

/*
this snippet was copied and adapted from:
K Lange; ps2hid.c; static int ps2_wait_output(void), 17.10.2021
*/
// Wait until the PS/2 controller's output buffer is filled.
// Use this before READING from the controller.
uint8_t ps2_wait_output(void) {
    uint64_t timeout = 100000UL;
    while (--timeout) {
        if (io_inb(PS2_STATUS) & 1) {
            return 0;
        }
    }
    return 1;
}

void ps2_disable(void) {
    ps2_wait_input();
    io_outb(PS2_COMMAND, PS2_DISABLE_PORT1);
    ps2_wait_input();
    io_outb(PS2_COMMAND, PS2_DISABLE_PORT2);
    pic_mask_irq(1);
    pic_mask_irq(12);
}

void ps2_enable(void) {
    pic_unmask_irq(1);
    pic_unmask_irq(12);
    ps2_wait_input();
    io_outb(PS2_COMMAND, PS2_ENABLE_PORT1);
    ps2_wait_input();
    io_outb(PS2_COMMAND, PS2_ENABLE_PORT2);
}

void ps2_write_command(uint8_t cmdbyte) {
    ps2_disable();
    ps2_wait_input();
    io_outb(PS2_COMMAND, cmdbyte);
    ps2_enable();
}

uint8_t ps2_write_command_read_data(uint8_t cmdbyte) {
    ps2_write_command(cmdbyte);
    return ps2_read_data();
}

uint8_t ps2_read_status(void) {
    ps2_wait_output();
    return io_inb(PS2_STATUS);
}

uint8_t ps2_read_data(void) {
    ps2_wait_output();
    return io_inb(PS2_DATA);
}

uint8_t ps2_write_data(uint8_t cmdbyte) {
    ps2_data_response_req = true;
    ps2_wait_input();
    io_outb(PS2_DATA, cmdbyte);
    return ps2_read_data();
}

void ps2_write_command_arg(uint8_t cmdbyte, uint8_t arg) {
    ps2_write_command(cmdbyte);
    ps2_write_data(arg);
}

uint8_t ps2_write_data_arg(uint8_t cmdbyte, uint8_t arg) {
    uint8_t cmdbyte_response = ps2_write_data(cmdbyte);

    // if cmdbyte isn't acknowledged, return response
    if (cmdbyte_response != 0xFA) {
        return cmdbyte_response;
    }
    return ps2_write_data(arg);
}


/*
the idea to this snippet is from:
https://wiki.osdev.org/index.php?title=%228042%22_PS/2_Controller&action=history; "8042" PS/2 Controller; https://wiki.osdev.org/%228042%22_PS/2_Controller, 17.10.2021
*/
void ps2_init(void) {

    ps2_write_command(PS2_DISABLE_PORT1);
    ps2_write_command(PS2_DISABLE_PORT2);

    // Clear the input buffer.
    size_t timeout = 1024;
    while ((io_inb(PS2_STATUS) & 1) && timeout > 0) {
        timeout--;
        io_inb(PS2_DATA);
    }

    if (timeout == 0) {
        // panic("ps2: prob. no existing PS/2");
        return;
    }

    // Enable interrupt lines, enable translation.
    uint8_t status = ps2_write_command_read_data(PS2_READ_CONFIG);
    status |= (PS2_PORT1_IRQ | PS2_PORT2_IRQ | PS2_PORT1_TLATE);
    ps2_write_command_arg(PS2_WRITE_CONFIG, status);
    status = ps2_write_command_read_data(PS2_READ_CONFIG);
    // if (ps2_write_command_read_data(0xAA) != 0x55) {
    //     // panic(ps2: self-test on init failed)
    //     ;
    // }
    // checks if it's a dual-channel ps2-controller
    if (!(ps2_write_command_read_data(0xAE) & (1 >> 5))) {
        ps2_write_command(PS2_DISABLE_PORT2);
        dual_channel = true;
    } else {
        dual_channel = false;
    }
    // test PS/2 ports and exit init if both fail
    // port 1
    if (ps2_write_command_read_data(0xAB) == 0x00) {
        ps2_port1 = true;
    }

    if (dual_channel) {
        // port 2
        if (ps2_write_command_read_data(0xA9) == 0x00) {
            ps2_port2 = true;
        }
    }

    if (!ps2_port1 && !ps2_port2) {
        // panic(ps2 init: neither port passed test);
        return;
    }
    if (ps2_port1) {
        ps2_write_command(PS2_ENABLE_PORT1);
        ps2_write_data(0xFF);
    }

    // mouse not implemented yet
    if (ps2_port2) {
        ps2_write_command(PS2_ENABLE_PORT2);
        ps2_responses_awaited = 2;
        ps2_write_command(0xD4);
        ps2_write_data(0xFF); // reset
        ps2_responses_awaited = 1;
        ps2_write_command(0xD4);
        ps2_write_data(0xF6); // default

        // enable scroll wheel (with weird sequence-> sample rate to 200, 100, 80)
        ps2_write_command(0xD4);
        ps2_write_data(0xF3); // set sample rate
        ps2_write_command(0xD4);
        ps2_write_data(200); // 20 samples per second
        ps2_write_command(0xD4);
        ps2_write_data(0xF3);
        ps2_write_command(0xD4);
        ps2_write_data(100);
        ps2_write_command(0xD4);
        ps2_write_data(0xF3);
        ps2_write_command(0xD4);
        ps2_write_data(80);

        ps2_responses_awaited = 2;

        // get mouse id
        ps2_write_command(0xD4);
        ps2_write_data(0xF2);
        ps2_responses_awaited = 1;

        ps2_write_command(0xD4);
        ps2_write_data(0xF4); // enable data reporting
        // 0xD4: sends next byte to PS/2-Port: 2
    }
    ps2_responses_awaited = 0;
    ps2_data_response_req = false;
}
