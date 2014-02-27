#include <interrupt_routines.h>

#define KEY_PRESSED 0

void keyboard_routine() {
  unsigned char port_value = inb(0x60);
  unsigned char key_status = port_value >> 7;
  unsigned char converted_char;
  if (key_status == KEY_PRESSED) {
    converted_char = get_converted_char(port_value&0x7F);
    if (converted_char == '\0') {
      converted_char = 'C';
    }
    printc_xy(0xF0, 0x0F, converted_char);
  }
}