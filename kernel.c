#include "kernel.h"
#include "x86-64.h"
#include <stddef.h>
#include <stdint.h>

// VGA text-mode buffer
static uint16_t *const VGA_BUFFER = (uint16_t *)0xB8000;
static const int VGA_WIDTH = 80;
static const int VGA_HEIGHT = 25;


x86_64_pagetable kernel_pagetable[5];

// VGA color attributes
enum vga_color {
  COLOR_BLACK = 0,
  COLOR_BLUE = 1,
  COLOR_GREEN = 2,
  COLOR_CYAN = 3,
  COLOR_RED = 4,
  COLOR_MAGENTA = 5,
  COLOR_BROWN = 6,
  COLOR_LIGHT_GREY = 7,
  COLOR_DARK_GREY = 8,
  COLOR_LIGHT_BLUE = 9,
  COLOR_LIGHT_GREEN = 10,
  COLOR_LIGHT_CYAN = 11,
  COLOR_LIGHT_RED = 12,
  COLOR_LIGHT_MAGENTA = 13,
  COLOR_LIGHT_BROWN = 14,
  COLOR_WHITE = 15,
};

// Combine foreground and background colors into a VGA attribute byte
static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) {
  return fg | bg << 4;
}

// Create a VGA entry (character and color)
static inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
  return (uint16_t)uc | (uint16_t)color << 8;
}

// Clear the VGA buffer
void clear_vga_buffer(uint16_t *buffer, uint8_t color) {
  for (int y = 0; y < VGA_HEIGHT; y++) {
    for (int x = 0; x < VGA_WIDTH; x++) {
      const size_t index = y * VGA_WIDTH + x;
      buffer[index] = vga_entry(' ', color);
    }
  }
}

// Print a string to the VGA buffer
void vga_print(const char *str, uint8_t color) {
  static size_t index = 0;
  for (size_t i = 0; str[i] != '\0'; i++) {
    if (index >= VGA_WIDTH * VGA_HEIGHT) {
      // If we reach the end of the screen, reset the index (simple scroll
      // simulation)
      index = 0;
    }
    VGA_BUFFER[index++] = vga_entry(str[i], color);
  }
}

void kernel_exception(regstate *regs) {
  // TODO: save registers to 'current' process
  // TODO: maybe some optional logging

  switch (regs->reg_intno) {
  case INT_IRQ: {
    // TODO: track ticks, handle lapic state
    // TODO: schedule next process
    break;
    }
    case INT_PF: {
    // TODO: implement page fault logic
    break;
    }
    default:
    // TODO: unhandled exception, put an error here
    return;
  }


  // TODO: schedule here in case we fall through case statement
}

int kernel_main() {
  // Clear the VGA buffer with black background and light grey text
  clear_vga_buffer(VGA_BUFFER, vga_entry_color(COLOR_LIGHT_GREY, COLOR_BLACK));

  // Print a welcome message in light green text
  const char *welcome_message = "Welcome to SignalOS!";
  vga_print(welcome_message, vga_entry_color(COLOR_LIGHT_GREEN, COLOR_BLACK));

  // Infinite loop
  for (;;) {
  }
}
