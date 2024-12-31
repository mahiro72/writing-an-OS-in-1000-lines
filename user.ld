ENTRY(start)

SECTIONS {
    . = 0x1000000;

    .text : {
        KEEP(*(.text.start)); /* 最適化時に削除しないための指定 */
        *(.text .text.*);
    }

    .rodata : ALIGN(4) {
        *(.rodata .rodata.*);
    }

    .data : ALIGN(4) {
        *(.data .data.*);
    }

    .bss : ALIGN(4) {
        *(.bss .bss.* .sbss .sbss.*);

        . = ALIGN(16);
        . += 64 * 1024;
        __stack_top = .;

        ASSERT(. < 0x1800000, "too large executable");
    }
}
