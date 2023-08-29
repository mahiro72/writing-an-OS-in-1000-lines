#include "kernel.h"
#include "common.h"

extern char __bss[], __bss_end[], __stack_top[];
extern char __free_ram[],__free_ram_end[];
extern char __kernel_base[];
extern char _binary_shell_bin_start[], _binary_shell_bin_size[];


/* mallocのようにバイト単位で割り当てるのではなく、ページ単位で動的に割り当てる関数
1ページは一般的に4KB 
個別のメモリページを解放できないデメリットを持つ*/
paddr_t alloc_pages(uint32_t n){
	static paddr_t next_addr = (paddr_t) __free_ram; //staticをつけることで関数呼び出し間で値が保持される(グローバル変数のような動き)
	paddr_t paddr = next_addr;
	next_addr += n * PAGE_SIZE;

	if (next_addr >	(paddr_t)__free_ram_end)
		PANIC("out of memory");

	memset((void *)paddr, 0, n * PAGE_SIZE);
	return paddr;
}

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


long getchar(void) {
	struct sbiret ret = sbi_call(0, 0, 0, 0, 0, 0, 0, 2);
	return ret.error;
}


/* 
s0~s11  セーブレジスタ(save register) 関数内で使用される一時的なデータを格納するためのレジスタグループ
a0~a7   引数レジスタ(argument register) 関数呼び出しの際に、関数に渡される引数を格納するために使用される
ra      リターンアドレス(return address) レジスタ. 関数呼び出しの時に、呼び出し元のプログラムカウンタ(次に実行される命令のメモリアドレス) の値を保持するために使用される
gp      グローバルポインタ(global pointer) レジスタ. グローバルなデータを指すために使用される
tp      スレッドポインタ(thread pointer) レジスタ. スレッドローカル(各スレッドごとに異なる値を持つ必要があるデータ) なデータへのアクセスをサポートするために使用される 
t0~t6   テンポラリレジスタ(temporary register). 一時的なデータの保存に使用される。特に短期間の一時的なデータ
*/
__attribute__((naked))
__attribute__((aligned(4)))
void kernel_entry(void) {
    __asm__ __volatile__(
    	// 実行中プロセスのカーネルスタックをsscratch(一時的な汎用レジスタ) から取り出す 
	// tmp = sp; sp = sscratch; sscratch = tmp;
        "csrrw sp, sscratch, sp\n"
        
	"addi sp, sp, -4 * 31\n"
        "sw ra,  4 * 0(sp)\n" // sw(store word) raの内容をsp + 4*0バイトに格納
        "sw gp,  4 * 1(sp)\n"
        "sw tp,  4 * 2(sp)\n"
        "sw t0,  4 * 3(sp)\n"
        "sw t1,  4 * 4(sp)\n"
        "sw t2,  4 * 5(sp)\n"
        "sw t3,  4 * 6(sp)\n"
        "sw t4,  4 * 7(sp)\n"
        "sw t5,  4 * 8(sp)\n"
        "sw t6,  4 * 9(sp)\n"
        "sw a0,  4 * 10(sp)\n"
        "sw a1,  4 * 11(sp)\n"
        "sw a2,  4 * 12(sp)\n"
        "sw a3,  4 * 13(sp)\n"
        "sw a4,  4 * 14(sp)\n"
        "sw a5,  4 * 15(sp)\n"
        "sw a6,  4 * 16(sp)\n"
        "sw a7,  4 * 17(sp)\n"
        "sw s0,  4 * 18(sp)\n"
        "sw s1,  4 * 19(sp)\n"
        "sw s2,  4 * 20(sp)\n"
        "sw s3,  4 * 21(sp)\n"
        "sw s4,  4 * 22(sp)\n"
        "sw s5,  4 * 23(sp)\n"
        "sw s6,  4 * 24(sp)\n"
        "sw s7,  4 * 25(sp)\n"
        "sw s8,  4 * 26(sp)\n"
        "sw s9,  4 * 27(sp)\n"
        "sw s10, 4 * 28(sp)\n"
        "sw s11, 4 * 29(sp)\n"

	// 例外発生時のspを取り出して保存
        "csrr a0, sscratch\n" // sscratchから値を読み取ってa0に保存
        "sw a0, 4 * 30(sp)\n" // a0に保存したものをsp + 4*30バイトに格納

	// カーネルスタックを再設定
	"addi a0, sp, 4 * 31\n" //spレジスタに4*31を加えてそれをa0に格納
	"csrw sscratch, a0\n" // a0の値がsscratchに書き込まれる. csrw, csrrはcsrに対して書き込んだり読み込んだりする操作

        "mv a0, sp\n" // a0レジスタにスタックポイントをセット. 指し示すアドレスにはtrap_frame構造体と同じ構造でレジスタの値が保存されている
        "call handle_trap\n"

        "lw ra,  4 * 0(sp)\n" // lw(load word) データをロードしてraに格納
        "lw gp,  4 * 1(sp)\n"
        "lw tp,  4 * 2(sp)\n"
        "lw t0,  4 * 3(sp)\n"
        "lw t1,  4 * 4(sp)\n"
        "lw t2,  4 * 5(sp)\n"
        "lw t3,  4 * 6(sp)\n"
        "lw t4,  4 * 7(sp)\n"
        "lw t5,  4 * 8(sp)\n"
        "lw t6,  4 * 9(sp)\n"
        "lw a0,  4 * 10(sp)\n"
        "lw a1,  4 * 11(sp)\n"
        "lw a2,  4 * 12(sp)\n"
        "lw a3,  4 * 13(sp)\n"
        "lw a4,  4 * 14(sp)\n"
        "lw a5,  4 * 15(sp)\n"
        "lw a6,  4 * 16(sp)\n"
        "lw a7,  4 * 17(sp)\n"
        "lw s0,  4 * 18(sp)\n"
        "lw s1,  4 * 19(sp)\n"
        "lw s2,  4 * 20(sp)\n"
        "lw s3,  4 * 21(sp)\n"
        "lw s4,  4 * 22(sp)\n"
        "lw s5,  4 * 23(sp)\n"
        "lw s6,  4 * 24(sp)\n"
        "lw s7,  4 * 25(sp)\n"
        "lw s8,  4 * 26(sp)\n"
        "lw s9,  4 * 27(sp)\n"
        "lw s10, 4 * 28(sp)\n"
        "lw s11, 4 * 29(sp)\n"
        "lw sp,  4 * 30(sp)\n"
        "sret\n" // スーパーバイザーレベルからユーザーレベルへ復帰
    );
}

struct process procs[PROCS_MAX];

// スタックに呼び出し先保存レジスタを保存し、スタックポインタの保存復元、そして呼び出し先保存レジスタの復元
__attribute__((naked)) void switch_context(uint32_t *prev_sp,
                                           uint32_t *next_sp) {
    __asm__ __volatile__(
        "addi sp, sp, -13 * 4\n" //addr(add immediate) レジスタに即値を加える. sp(2つめ) に-13 *4を加えて、その結果をsp(1つめ)に格納する
        "sw ra,  0  * 4(sp)\n" // raの値が sp + 4*0(バイト) アドレスに格納される
        "sw s0,  1  * 4(sp)\n"
        "sw s1,  2  * 4(sp)\n"
        "sw s2,  3  * 4(sp)\n"
        "sw s3,  4  * 4(sp)\n"
        "sw s4,  5  * 4(sp)\n"
        "sw s5,  6  * 4(sp)\n"
        "sw s6,  7  * 4(sp)\n"
        "sw s7,  8  * 4(sp)\n"
        "sw s8,  9  * 4(sp)\n"
        "sw s9,  10 * 4(sp)\n"
        "sw s10, 11 * 4(sp)\n"
        "sw s11, 12 * 4(sp)\n"
        "sw sp, (a0)\n" // スタックポインタをa0に格納. ()がついているとレジスタのアドレスに対してメモリアクセスが行われる
        "lw sp, (a1)\n" // a1からデータをロードしてspに格納
        "lw ra,  0  * 4(sp)\n"
        "lw s0,  1  * 4(sp)\n"
        "lw s1,  2  * 4(sp)\n"
        "lw s2,  3  * 4(sp)\n"
        "lw s3,  4  * 4(sp)\n"
        "lw s4,  5  * 4(sp)\n"
        "lw s5,  6  * 4(sp)\n"
        "lw s6,  7  * 4(sp)\n"
        "lw s7,  8  * 4(sp)\n"
        "lw s8,  9  * 4(sp)\n"
        "lw s9,  10 * 4(sp)\n"
        "lw s10, 11 * 4(sp)\n"
        "lw s11, 12 * 4(sp)\n"
        "addi sp, sp, 13 * 4\n"
        "ret\n"
    );
}

/* ページテーブルを構築する関数
1段目のページテーブル(table1), マップしたい仮想アドレス(vaddr), 
マップ先の物理アドレス(paddr), ページテーブルエントリに設定するフラグ(flags)を受け取る*/
void map_page(uint32_t *table1, uint32_t vaddr, paddr_t paddr, uint32_t flags) {
	if (!is_aligned(vaddr,PAGE_SIZE))
		PANIC("unaligned vaddr %x",vaddr);
	if (!is_aligned(paddr,PAGE_SIZE))
		PANIC("unaligned paddr %x",paddr);
	
	uint32_t vpn1 = (vaddr >> 22) & 0x3ff; //仮想アドレスの上位から11ビット目から20ビット目までの部分を取り出している
	if ((table1[vpn1] & PAGE_V) == 0){
		// 2段目のページテーブルが存在しないので作成する
		uint32_t pt_paddr = alloc_pages(1);
		table1[vpn1] = ((pt_paddr / PAGE_SIZE) << 10) | PAGE_V;
	}
	
	// 2段目のページテーブルにエントリを追加する
	/* ビット演算
	
	*/
	/* 2段目のページテーブルを用意して、2段目の設定したいページテーブルエントリへマップ先の
	物理ページ番号とフラグを設定*/
	uint32_t vpn0 = (vaddr >> 12) & 0x3ff;
	uint32_t *table0 = (uint32_t *) ((table1[vpn1] >> 10) * PAGE_SIZE);
	table0[vpn0] = ((paddr / PAGE_SIZE) << 10) | flags | PAGE_V;
}

// naked: 関数のプロローグ(エントリ処理) とエピローグ(終了処理) を自動生成せず、ユーザーがアセンブリを直接記述できるようにする
/* S-Mode(カーネルの特権モード) からU-Mode(ユーザープログラムの非特権モード) に、CPU動作モードを切り替える
sretで切り替えを行う
事前準備
- sepcレジスタにU-Modeに移行した際のプログラムカウンタを設定する
- sstatusレジスタのSPIEビットをたてる. これによりU-Modeに入った際に割り込みが有効化され、例外と同じようにstvecレジスタに設定しているハンドラが呼ばれる */
__attribute__((naked)) void user_entry(void) {
	__asm__ __volatile__(
		"csrw sepc, %[sepc]\n"
		"csrw sstatus, %[sstatus]\n"
		"sret\n"
		:
		: [sepc] "r" (USER_BASE),
			[sstatus] "r" (SSTATUS_SPIE)
	);
}


struct process *create_process(const void *image, size_t image_size) {
	// 空いているプロセス管理構造体を探す
	struct process *proc = NULL;
	int i = 0;
	while (i < PROCS_MAX) {
		if (procs[i].state == PROC_UNUSED){
			proc = &procs[i];
			break;
		}
		i++;
	}

	if (!proc)
		PANIC("no free process slots");
	
	// switch_contextで復帰できるように、スタックに呼び出し先保存レジスタを積む	
	// s0~s11は呼び出し先保存レジスタ
	// 呼び出されたサブルーチン内でのみ変更され、呼び出し元のコードに戻る際に元の値を復元する必要がある
	uint32_t *sp = (uint32_t *) &proc->stack[sizeof(proc->stack)];
	*--sp = 0; // s11
	*--sp = 0; // s10
	*--sp = 0; // s9
	*--sp = 0; // s8
	*--sp = 0; // s7
	*--sp = 0; // s6
	*--sp = 0; // s5
	*--sp = 0; // s4
	*--sp = 0; // s3
	*--sp = 0; // s2
	*--sp = 0; // s1
	*--sp = 0; // s0
	*--sp = (uint32_t)user_entry; // ra

	uint32_t *page_table = (uint32_t *) alloc_pages(1);
	/* カーネルのページをマッピングする
	カーネルページは__kernel_base から __free_ram_endまで.
	これにより、静的に配置される.text領域や、alloc_pagesなどで動的に配置される領域の
	両方にカーネルがアクセスできるようになる.*/
	for (paddr_t paddr = (paddr_t) __kernel_base; paddr < (paddr_t) __free_ram_end; paddr += PAGE_SIZE)
		map_page(page_table, paddr, paddr, PAGE_R | PAGE_W | PAGE_X);
	
	/* ユーザーのページをマッピングする */
	for (uint32_t off = 0; off < image_size; off += PAGE_SIZE) {
		paddr_t page = alloc_pages(1);
		memcpy((void *) page, image + off, PAGE_SIZE);
		map_page(page_table, USER_BASE + off, page, PAGE_U | PAGE_R | PAGE_W | PAGE_X);
	}

	// 各フィールドの初期化
	proc->pid = i + 1;
	proc->state = PROC_RUNNABLE;
	proc->sp = (uint32_t) sp;
	proc->page_table = page_table;
	return proc;
}


struct process *current_proc;  // 現在実行中のプロセス
struct process *idle_proc;     // アイドルプロセス 実行可能なタスクが存在しない時に実行される特殊なプロセス

/* yieldは譲るという英単語
CPU時間という資源を他のプロセスに譲る*/
void yield(void) {
	// 実行可能なプロセスを探す
	struct process *next = idle_proc;
	int i = 0;
	while (i < PROCS_MAX) {
		struct process *proc = &procs[(current_proc->pid + i) % PROCS_MAX];
		if (proc->state == PROC_RUNNABLE && proc->pid > 0) { //idle processはpidが-1
			next = proc;
			break;
		}
		i++;
	}

	// 現在実行中のプロセス以外に実行可能なプロセスがない場合戻って処理を実行する
	if (next == current_proc)
		return;

	// 実行中のプロセスのカーネルスタックを初期値を設定
	// ?: なぜswitch_contextではなくここでレジスタを設定するのか
	__asm__ __volatile__(
		"sfence.vma\n" // 仮想メモリアクセスの同期. 仮想メモリアドレスの変更が正しく反映されることを保証
		"csrw satp, %[satp]\n" // satpレジスタ: 仮想メモリのアドレス変換と、保護機構に関連する設定を保持
		"sfence.vma\n" // 新たなページング設定を反映させるために同期させる
		"csrw sscratch, %[sscratch]\n"
		:
		: [satp] "r" (SATP_SV32 | ((uint32_t) next->page_table / PAGE_SIZE)),
			[sscratch] "r" ((uint32_t) &next->stack[sizeof(next->stack)]) //スタックは下位のアドレスに向かって伸びるため、末尾を設定しておく
	);

	// コンテキストスイッチする
	struct process *prev = current_proc;
	current_proc = next;
	switch_context(&prev->sp, &next->sp);
}


void handle_syscall(struct trap_frame *f) {
	switch (f->a3) {
		case SYS_GETCHAR:
			while (1) {
				long ch = getchar();
				if (ch >= 0) {
					f->a0 = ch;
					break;
				}
				
				// 文字が入力されるまでSBiを繰り返し呼び出す
				// ただし単純に繰り返すとCPUを占有してしまうのでyieldでCPUを他のプロセスに譲るようにする
				yield();
			}
			break;
		case SYS_PUTCHAR:
			putchar(f->a0);
			break;
		case SYS_EXIT:
			printf("process %d exited\n", current_proc->pid);
			current_proc->state = PROC_EXITED;
			yield();
			PANIC("unreachable");
		default:
			PANIC("unexpected syscall a3=%x\n", f->a3);
	}
}

void handle_trap(struct trap_frame *f) {
	uint32_t scause = READ_CSR(scause); // 例外の種類
	uint32_t stval = READ_CSR(stval); // 例外の負荷情報(ex: 例外の原因となったメモリ情報など)
	uint32_t user_pc = READ_CSR(sepc); // 例外発生箇所のプログラムカウンタ
	if (scause == SCAUSE_ECALL) {
		handle_syscall(f);
		/* sepcは例外を起こしたプログラムカウンタ、ecallを指している
		このままだとecallを無限に繰り返してしまうので、命令のサイズ分(4バイト) 加算し、
		ユーザーモードに戻る際に次の命令から実行を再開するようにしている */
		user_pc += 4;
	} else {
		PANIC("unexpected trap scause=%x,stval=%x,sepc=%x", scause, stval, user_pc);
	}

	WRITE_CSR(sepc, user_pc);
}

void kernel_main(void) {
	/* bss領域をブートローダが認識してゼロクリアしてくれることもあるが、その確証がないため自らの手で初期化*/
	memset(__bss, 0, (size_t) __bss_end - (size_t) __bss);
	
	// 例外ハンドラのアドレスをstvecレジスタに書き込む
	WRITE_CSR(stvec, (uint32_t) kernel_entry);

	printf("\nHello World from kernel!\n");

	idle_proc = create_process(NULL,0);
	idle_proc->pid = -1; //idle
	current_proc = idle_proc;

	create_process(_binary_shell_bin_start, (size_t) _binary_shell_bin_size);

	yield();
	PANIC("switched to idle process");

	while (1){};
}

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
