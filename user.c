/* ユーザーランド用ライブラリ */

#include "user.h"

extern char __stack_top[];

int syscall(int sysno, int arg0, int arg1, int arg2) {
	register int a0 __asm__("a0") = arg0;
	register int a1 __asm__("a1") = arg1;
	register int a2 __asm__("a2") = arg2;
	register int a3 __asm__("a3") = sysno;

	/* ecall はカーネルに処理を委譲する特殊な命令
	ecallを実行すると、例外ハンドラが呼び出され、カーネルに処理が移る
	戻り値はa0に設定される */
	__asm__ __volatile__("ecall"
				: "=r"(a0)
				: "r"(a0), "r"(a1), "r"(a2), "r"(a3)
				: "memory");
	return a0;
}

void putchar(char ch) {
	// 第2引数以降は未使用なので0を渡す
	syscall(SYS_PUTCHAR, ch, 0, 0);
}

int getchar(void) {
	return syscall(SYS_GETCHAR, 0, 0, 0);
}

// noreturn: 呼び出し元に戻ることがないことを示す属性
// 一般的に終了しない関数(エラー処理や以上処理) のために使用される
__attribute__((noreturn)) void exit(void){
	syscall(SYS_EXIT, 0, 0, 0);
	while (1); // 念の為
}

// .text.startセクションにこの関数を配置
// .bssセクションをカーネルがゼロクリア(ゼロうめ) していることを保証しているため(alloc_pages) 特にゼロクリアしない
__attribute__((section(".text.start")))
__attribute__((naked))
void start(void) {
	// __volatile__: 最適化を抑制し、意図しない振る舞いを防ぐ
	__asm__ __volatile__(
		"mv sp, %[stack_top]\n" //stack_topをspにコピー
		"call main\n"
		"call exit\n"
		:
		: [stack_top] "r" (__stack_top));
}
