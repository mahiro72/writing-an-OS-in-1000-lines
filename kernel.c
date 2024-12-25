#include "kernel.h"
#include "common.h"

typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef uint32_t size_t;

extern char __bss[], __bss_end[], __stack_top[];

/* RISC-VのSBI呼び出すをするための関数 */
struct sbiret sbi_call(long arg0, long arg1, long arg2, long arg3, long arg4, long arg5, long fid, long eid) {
    register long a0 __asm__("a0") = arg0; /* arg0をlongのa0変数として宣言し、a0レジスタと紐づける。 a0が更新されるとa0レジスタにもそれが同期される */
    register long a1 __asm__("a1") = arg1;
    register long a2 __asm__("a2") = arg2;
    register long a3 __asm__("a3") = arg3;
    register long a4 __asm__("a4") = arg4;
    register long a5 __asm__("a5") = arg5;
    register long a6 __asm__("a6") = fid;
    register long a7 __asm__("a7") = eid;

    __asm__ __volatile__("ecall" /* S-mode -> M-modeへ変更しSBI呼び出しをする. SBI呼び出しはecall命令のみ */
                        : "=r"(a0), "=r"(a1) /* ecall実行後、a0にエラーコード、a1に戻り値が格納される */
                        : "r"(a0), "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5),"r"(a6), "r"(a7) /* SBI呼び出し時の引数 */
                        : "memory");
    return (struct sbiret){.error = a0, .value = a1};
}

void putchar(char ch) {
    sbi_call(ch, 0, 0, 0, 0, 0, 0, 1 /* SBI関数ID（Console Putchar）*/);
}

void *memset(void *buf, char c, size_t n) {
    uint8_t *p = (uint8_t *) buf;
    while (n--)
        *p++ = c;
    return buf;
}

void kernel_main(void) {
    // memset(__bss, 0, (size_t) __bss_end - (size_t) __bss);

    printf("Hello, World!\n");
    printf("This is a test message. %s\n", "by mahiro");
    printf("This is a test message. %d + %d = %d\n", 1, 2, 1 + 2);

    for(;;) {
        __asm__ __volatile__("wfi");
    }
}

__attribute__((section(".text.boot"))) /* .text.bootのメモリセクションにコードを配置する */
__attribute__((naked))
void boot(void) {
    __asm__ __volatile__(
        "mv sp, %[stack_top]\n" /* spの初期値は不定なので、ここで確実に設定する */
        "j kernel_main\n"
        :
        : [stack_top] "r" (__stack_top)
    );
}
