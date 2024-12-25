ENTRY(boot)

SECTIONS
{
    . = 0x80200000; /* ページアドレス */

    .text : {
        KEEP(*(.text.boot));
        *(.text .text.*);
    }

    .rodata : ALIGN(4) { /* ALIGN(4)で4バイト境界になるようアドレスを調整 */
        *(.rodata .rodata.*);
    }

    /* Cで定義された初期値ありの変数(int a = 1;)がコンパイル時にオブジェクトファイルの.dataセクションに配置される。
    リンカはリンク時にオブジェクトファイルの.dataセクションを集めて、ここにまとめて配置する */
    .data : ALIGN(4) {
        *(.data .data.*);
    }

    .bss : ALIGN(4) {
        __bss = .;
        *(.bss .bss.* .sbss .sbss.*);
        __bss_end = .;
    }

    . = ALIGN(4);
    . += 128 * 1024; /* 128KB */
    __stack_top = .;
}
