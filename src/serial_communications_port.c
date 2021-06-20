#include "kernel/serial_communications_port.h"
#include "kernel/asm.h"

// https://wiki.osdev.org/Serial_Ports

#define COM1 ((uint16_t)0x3f8)

int serial__init(void) {
  asm_outb(COM1 + 1, 0x00); // Disable all interrupts
  asm_outb(COM1 + 3, 0x80); // Enable DLAB (set baud rate divisor)
  asm_outb(COM1 + 0, 0x03); // Set divisor to 3 (lo byte) 38400 baud
  asm_outb(COM1 + 1, 0x00); //                  (hi byte)
  asm_outb(COM1 + 3, 0x03); // 8 bits, no parity, one stop bit
  asm_outb(COM1 + 2, 0xC7); // Enable FIFO, clear them, with 14-byte threshold
  asm_outb(COM1 + 4, 0x0B); // IRQs enabled, RTS/DSR set
  asm_outb(COM1 + 4, 0x1E); // Set in loopback mode, test the serial chip
  asm_outb(COM1 + 0, 0xAE); // Test serial chip (send byte 0xAE and check if
                            // serial returns same byte)

  // Check if serial is faulty (i.e: not same byte as sent)
  if (asm_inb(COM1 + 0) != 0xAE) {
    return 1;
  }

  // If serial is not faulty set it in normal operation mode
  // (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)
  asm_outb(COM1 + 4, 0x0F);
  return 0;
}

int is_transmit_empty() { return asm_inb(COM1 + 5) & 0x20; }

void serial__write(char a) {
  while (is_transmit_empty() == 0)
    ;

  asm_outb(COM1, (uint8_t)a);
}
