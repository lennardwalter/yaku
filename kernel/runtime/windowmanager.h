#pragma once

#include <runtime/drawutils.h>
#include <types.h>

typedef struct {
    int x, y;
    size_t width, height, index;
    framebuffer_t buffer;
} window_t;

void windowmanager_init(void);
void windowmanager_handle_events(void);
void windowmanager_run(void);
void windowmanager_startup_screen(void);
void windowmanager_draw(void);
void windowmanager_draw_window(window_t* window);
window_t* windowmanager_get_window_at(size_t x, size_t y);
window_t* windowmanager_create_window(size_t width, size_t height);
