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
	
	/* カーネルスタック: コンテキストスイッチ時のCPUレジスタ、どこから呼ばれたのか(関数の戻り先)、各関数のローカル変数などを保持
	カーネルスタックをプロセスごとに用意することにより、異なる実行コンテキストを持ち、状態の保存や復元が可能に*/
	uint8_t stack[8192];
};



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


