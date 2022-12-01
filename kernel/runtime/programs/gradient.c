#include "gradient.h"

#include <drivers/serial.h>
#include <multitasking/scheduler.h>
#include <multitasking/task.h>
#include <runtime/windowmanager.h>

void gradient_on_close(window_t* window) {
    ((task_t*)window->data)->task_state=TASK_STATE_TERMINATED;
    windowmanager_destroy_window(window);
}

void gradient_main(void) {
    window_t* window = windowmanager_create_default_window(250, 250, "Gradient");
    window->data = scheduler_get_current_task();
    window->on_close = &gradient_on_close;

    uint8_t tick = 0;
    int delta = 1;
    for (;;) {
        for (size_t x = 0; x < window->buffer.width; x++) {
            for (size_t y = 0; y < window->buffer.height; y++) {
                uint32_t color = 0xFF000000 | (x << 16) | (y << 8) | tick;
                drawutils_draw_pixel(window->buffer, x, y, color);
            }
        }

        tick += delta;

        if (tick == 255) {
            delta = -1;
        } else if (tick == 0) {
            delta = 1;
        }

        scheduler_sleep(5);
    }
}
