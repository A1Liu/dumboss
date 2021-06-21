ENTRY(kmain)
OUTPUT(binary)

SECTIONS {
  .text :
  {
      *(.kmain)
      *(.text)
  }
  .rodata :
  {
      *(.rodata)
  }
  .data :
  {
      *(.data)
  }
  .bss :
  {
      *(.bss)
  }
}
