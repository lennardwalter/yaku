#pragma once

#include <input/input_event.h>
#include <runtime/drawutils.h>
#include <types.h>

typedef struct window {
    int x, y;
    size_t width, height, index;
    framebuffer_t buffer;
    char* title;
    void (*on_event)(struct window* window, input_event_t event);
    void (*on_close)(struct window* window);
    void* data;
} window_t;

void windowmanager_main(void);
void windowmanager_handle_events(void);
void windowmanager_startup_screen(void);
void windowmanager_draw(void);
void windowmanager_draw_window(window_t* window);
window_t* windowmanager_get_window_at(size_t x, size_t y);

window_t* windowmanager_create_window(size_t width, size_t height, size_t x, size_t y,
                                      char* title);
window_t* windowmanager_create_default_window(size_t width, size_t height, char* title);
void windowmanager_destroy_window(window_t* window);
void windowmanager_get_relative_cursor_pos(window_t* window, size_t* x, size_t* y);
