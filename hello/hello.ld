ENTRY(hello)
MEMORY
{
  FLASH (rx) : ORIGIN = 0x08010000, LENGTH = 4K
}
SECTIONS
{
  .text :
  {
    . = ALIGN(4);
    _stext = .;
    KEEP(*(.func_table));
    *(.text)                        /* remaining code */
    *(.text.*)                      /* remaining code */
    *(.rodata)                      /* read-only data (constants) */
    *(.rodata*)
    *(.glue_7)
    *(.glue_7t)
    *(.gnu.linkonce.t*)
    _etext = .;
  } > FLASH
}