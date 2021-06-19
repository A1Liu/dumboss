ENTRY(kmain)

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
