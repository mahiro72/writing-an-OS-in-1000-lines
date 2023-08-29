/* 同じヘッダファイルが複数includeされるのを防ぐ */
#pragma once

#include "common.h"

struct sbiret {
	long error;
	long value;
};

// スタックに積まれている元の実行状態の構造体
struct trap_frame {
    uint32_t ra;
    uint32_t gp;
    uint32_t tp;
    uint32_t t0;
    uint32_t t1;
    uint32_t t2;
    uint32_t t3;
    uint32_t t4;
    uint32_t t5;
    uint32_t t6;
    uint32_t a0;
    uint32_t a1;
    uint32_t a2;
    uint32_t a3;
    uint32_t a4;
    uint32_t a5;
    uint32_t a6;
    uint32_t a7;
    uint32_t s0;
    uint32_t s1;
    uint32_t s2;
    uint32_t s3;
    uint32_t s4;
    uint32_t s5;
    uint32_t s6;
    uint32_t s7;
    uint32_t s8;
    uint32_t s9;
    uint32_t s10;
    uint32_t s11;
    uint32_t sp;
} __attribute__((packed)); /* アラインメントを無視してメンバー変数をパックできる
これにより、データが最小限のメモリ空間を使用するようになる。
メモリ使用量やデータの送受信の効率性重視*/


#define PROCS_MAX 8 // 最大プロセス数
#define PROC_UNUSED 0 // 未使用のプロセス管理構造体
#define PROC_RUNNABLE 1 // 実行可能なプロセス

struct process {
	int pid;     // プロセスID
	int state;  // プロセスの状態
	vaddr_t sp;  // コンテキストスイッチ時のスタックポインタ
	uint32_t *page_table; // 1段目のページテーブルを指すポインタ
	
	/* カーネルスタック: コンテキストスイッチ時のCPUレジスタ、どこから呼ばれたのか(関数の戻り先)、各関数のローカル変数などを保持
	カーネルスタックをプロセスごとに用意することにより、異なる実行コンテキストを持ち、状態の保存や復元が可能に*/
	uint8_t stack[8192];
};



// page table
#define SATP_SV32 (1u << 31)  // 仮想アドレスを物理アドレスに変換する時のページングモードを指定するビット. 0x80000000
#define PAGE_V    (1 << 0)    // 有効化ビット. 1を左シフト演算子(<<)で0ビット移動
#define PAGE_R    (1 << 1)    // 読み込み可能
#define PAGE_W    (1 << 2)    // 書き込み可能
#define PAGE_X    (1 << 3)    // 実行可能
#define PAGE_U    (1 << 4)    // ユーザーモードでアクセス可能

#define PANIC(fmt,...) \
	do { \
		printf("PANIC: %s:%d " fmt "\n",__FILE__,__LINE__,##__VA_ARGS__); \
		while (1){} \
	} while (0)


#define READ_CSR(reg) \
	({ \
		unsigned long __tmp; \
		__asm__ __volatile__("csrr %0, " #reg: "=r"(__tmp)); \
		__tmp; \
	})


#define WRITE_CSR(reg, value) \
	do { \
		uint32_t __tmp = (value); \
		__asm__ __volatile__("csrw " #reg ", %0" ::"r"(__tmp)); \
	} while (0)


/* user.ldで定義されている開始アドレスと合致する必要あり
ELFのような一般的な実行可能ファイルであれば、そのファイルのヘッダにロード先のアドレスが書かれている.
しかし、今回の実行イメージは生バイナリなので、決め内で指定する必要あり */
#define USER_BASE 0x1000000

#define SSTATUS_SPIE (1 << 5)

#define SCAUSE_ECALL 8

#define PROC_EXITED 2
