#include "kernel.h"
#include "common.h"

extern char __bss[], __bss_end[], __stack_top[];
extern char __free_ram[], __free_ram_end[];

struct process procs[PROCS_MAX];
struct process *current_proc; // 現在実行中のプロセス
struct process *idle_proc; // 実行可能なプロセスがない時に実行するプロセス。プロセスIDは-1で、起動時に作成される

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

paddr_t alloc_page(uint32_t n) {
    static paddr_t next_paddr = (paddr_t) __free_ram; /* static変数なので関数呼び出し間で値が保持される(グローバル変数のようなイメージ) */
    paddr_t paddr = next_paddr;
    next_paddr += n * PAGE_SIZE;

    if (next_paddr > (paddr_t) __free_ram_end) {
        PANIC("out of memory");
    }
    memset((void *) paddr, 0, n * PAGE_SIZE);
    return paddr;
}

__attribute__((naked))
void switch_context(uint32_t *prev_sp, uint32_t *next_sp) {
    __asm__ __volatile__ (
        "addi sp, sp, -13 * 4\n" //最初のspはプロセス実行時点を指している。つまりprev_sp=sp
        "sw ra,  0  * 4(sp)\n"
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
        "sw sp, (a0)\n" // a0(prev_sp) に最新のsp(-13*4したもの)を設定する
        "lw sp, (a1)\n" // a1(next_sp) から次のspを読み込む
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
        "addi sp, sp, 13 * 4\n" // レジスタの復元が終わったので spを復元情報の一番下まで移動する。sp利用時に復元時の情報を上書きしていくことになるが、パフォーマンス重視のため問題なし。0埋めするのもありだが、セキュリティレベルが上がる一方、パフォーマンス低下のトレードオフがある
        "ret\n" // raの位置にジャンプ
    );
}

struct process *create_process(uint32_t pc) {
    // 空いてるプロセス管理構造体を探す
    struct process *proc = NULL;
    int i;
    for (i = 0; i< PROCS_MAX; i++) {
        if (procs[i].state == PROC_UNUSED) {
            proc = &procs[i];
            break;
        }
    }

    if (!proc)
        PANIC("no free process slots");

    // switch_contextで復帰できるように、スタックに呼び出先保存レジスタを積む
    uint32_t *sp = (uint32_t *) &proc->stack[sizeof(proc->stack)];
    *--sp = 0;                      // s11
    *--sp = 0;                      // s10
    *--sp = 0;                      // s9
    *--sp = 0;                      // s8
    *--sp = 0;                      // s7
    *--sp = 0;                      // s6
    *--sp = 0;                      // s5
    *--sp = 0;                      // s4
    *--sp = 0;                      // s3
    *--sp = 0;                      // s2
    *--sp = 0;                      // s1
    *--sp = 0;                      // s0
    *--sp = (uint32_t) pc;          // ra

    // 各フィールドの初期化
    proc->pid = i + 1;
    proc->state = PROC_RUNNABLE;
    proc->sp = (uint32_t) sp;
    return proc;
}

void yield(void) {
    // 実行可能なプロセスを探す
    struct process *next = idle_proc;
    for (int i = 0; i < PROCS_MAX; i++) {
        struct process *proc = &procs[(current_proc->pid+i) % PROCS_MAX];
        if (proc->state == PROC_RUNNABLE && proc->pid > 0) {
            next = proc;
            break;
        }
    }

    // current process以外に実行可能なプロセスがない場合、戻って処理を再開する
    if (next == current_proc) {
        return;
    }

    __asm__ __volatile__(
        "csrw sscratch, %[sscratch]\n"
        :
        : [sscratch] "r" ((uint32_t) &next->stack[sizeof(next->stack)])
    ); //sscratchレジスタにカーネルスタックのアドレスを保存する。これはswitch_context時に参照される

    // コンテキストスイッチ
    struct process *prev = current_proc;
    current_proc = next;
    switch_context(&prev->sp, &current_proc->sp);
}

/*テスト*/
void proc_a_entry(void) {
    printf("starting process A\n");
    while (1) {
        putchar('A');
        yield();

        for (int i = 0; i < 30000000; i++)
            __asm__ __volatile__("nop");
    }
}

void proc_b_entry(void) {
    printf("starting process B\n");
    while (1) {
        putchar('B');
        yield();

        for (int i = 0; i < 30000000; i++)
            __asm__ __volatile__("nop");
    }
}
/*テスト*/

__attribute__((naked))
__attribute__((aligned(4)))
void kernel_entry(void) {
    __asm__ __volatile__(
        // 以降の例ではユーザモードで例外が発生されたていでのコメントが記載されているが、カーネルや例外ハンドリング中の例外が発生するパターンもある
        // 例外発生時にはspに例外が発生したプロセスのユーザスタックポインタが入っている
        "csrrw sp, sscratch, sp\n" // spとsscratchの交換。ここでカーネルスタックのアドレスがspに保存され、sscratchにはユーザスタックポインタが保存される

        "addi sp, sp, -4 * 31\n" /* スタックポインタ(上に伸びていく) を減算し、枠を確保してから例外発生時の状態を保存する */
        "sw ra,  4 * 0(sp)\n" /* store word: raレジスタをメモリに保存 */
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

        "csrr a0, sscratch\n" /* a0にsscratch(ユーザのスタックポインタ)の先頭をいれる。CSRから直接メモリ保存はできないので一度汎用レジスタに読み込む必要がある。a0である必要はなく汎用レジスタであればおk */
        "sw a0, 4 * 30(sp)\n"

        // カーネルスタックを設定し直す
        "addi a0, sp, 4 * 31\n" /* sp(カーネルのスタックポインタ)に4 * 31足したものをa0に設定する */
        "csrw sscratch, a0\n"

        "mv a0, sp\n" /* 現在のsp(カーネルのスタックポインタ) をa0にコピーし、関数呼び出時の引数とする */
        "call handle_trap\n"

        "lw ra,  4 * 0(sp)\n" /* load word: spから読み込んでレジスタに入れる */
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
        "sret\n" /* 例外発生時にsepcに一時的に保存していた例外発生時のプログラムカウンタを元に戻し同じ場所から再実行できるようにしたり、特権レベルの変更、割り込みフラグを元に戻したりする、*/
    );
}

void handle_trap(struct trap_frame *f) {
    uint32_t scause = READ_CSR(scause);
    uint32_t stval = READ_CSR(stval);
    uint32_t user_pc = READ_CSR(sepc);

    PANIC("unexpected trap scause=%x stval=%x sepc=%x", scause, stval, user_pc);
}

void kernel_main(void) {
    memset(__bss, 0, (size_t) __bss_end - (size_t) __bss);

    WRITE_CSR(stvec, (uint32_t) kernel_entry); /* stvecレジスタにkernel_entryのアドレスを書き込み、例外ハンドラの場所をCPUに伝える */

    idle_proc = create_process((uint32_t) NULL);
    idle_proc->pid = -1; // idle
    current_proc = idle_proc;

    struct process *proc_a, *proc_b;
    proc_a = create_process((uint32_t) proc_a_entry);
    proc_b = create_process((uint32_t) proc_b_entry);
    yield();
    PANIC("switched to idle process");

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
