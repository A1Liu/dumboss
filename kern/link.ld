/*
 * mykernel/c/link.ld
 *
 * Copyright (C) 2017 - 2021 bzt (bztsrc@gitlab)
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * This file is part of the BOOTBOOT Protocol package.
 * @brief An example linker script for sample kernel
 *
 */

mmio        = 0xfffffffff8000000;              /* these are configurable for level 2 loaders */
fb          = 0xfffffffffc000000;
bb          = 0xffffffffffe00000;
environment = 0xffffffffffe01000;
/* initstack = 1024; */
PHDRS
{
  boot PT_LOAD;                                /* one single loadable segment */
}
SECTIONS
{
    code_begin = 0xffffffffffe02000;
    . = code_begin;
    .text : {
        KEEP(*(.text.boot)) *(.text .text.*)   /* code */
        *(.rodata .rodata.*)                   /* data */
        *(.data .data.*)
        *(.got)
    } :boot

    code_end = ALIGN(4096);
    . = code_end;

    .bss (NOLOAD) : {                          /* bss */
        *(.bss .bss.*)
        *(COMMON)
    } :boot
    bss_end = ALIGN(4096);

    /DISCARD/ : { *(.eh_frame) *(.comment) }
}
