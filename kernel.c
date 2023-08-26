#include "kernel.h"
#include "common.h"

extern char __bss[], __bss_end[], __stack_top[];

struct sbiret sbi_call(long arg0,long arg1,long arg2,long arg3,long arg4,long arg5,long fid,long eid) {
	/* fid=funcid, eid=extid(extention id) */	
	/* ref: https://github.com/riscv-software-src/opensbi/blob/0ad866067d7853683d88c10ea9269ae6001bcf6f/lib/sbi/sbi_ecall_legacy.c#L64 */
	register long a0 __asm__("a0") = arg0;
	register long a1 __asm__("a1") = arg1;
	register long a2 __asm__("a2") = arg2;
	register long a3 __asm__("a3") = arg3;
	register long a4 __asm__("a4") = arg4;
	register long a5 __asm__("a5") = arg5;
	register long a6 __asm__("a6") = fid;
	register long a7 __asm__("a7") = eid;
	
	__asm__ __volatile__ ("ecall"
				: "=r"(a0), "=r"(a1)
				: "r"(a0), "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(a6), "r"(a7)
				: "memory");
	return (struct sbiret){.error = a0, .value = a1};
}

void putchar(char ch) {
	sbi_call(ch,0,0,0,0,0,0,1);
}

void kernel_main(void) {
	/* bss領域をブートローダが認識してゼロクリアしてくれることもあるが、その確証がないため自らの手で初期化*/
	memset(__bss, 0, (size_t) __bss_end - (size_t) __bss);

	printf("Hello World\n");
	printf("Hello %s\n","mahiro");
	printf("1 + 2 = %d\n",1+2);
	printf("hoge" "hoge" "hoge");

	PANIC("panic!!! %s","arg test");
	printf("ok?");
	while (1){};
}


/* .text.bootという属性をつけて専用のセクションに配置 */
/* __attribute__((naked)): 余計なアセンブリを生成しない */
__attribute__((section(".text.boot")))
__attribute__((naked))
void boot(void) {
	__asm__ __volatile__ (
		"mv sp, %[stack_top]\n"
		"j kernel_main"
		:
		: [stack_top] "r" (__stack_top)
	);
}

