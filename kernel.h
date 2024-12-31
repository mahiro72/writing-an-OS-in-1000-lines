#pragma once /* 多重インクルードの防止 */

#include "common.h"

struct sbiret {
    long error;
    long value;
};

#define PANIC(fmt, ...) \
    do { \
        printf("PANIC: %s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
        while (1) {} \
    } while (0) /* 複数の分をマクロで書きたいときに頻出する書き方 */

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
} __attribute__((packed));

/* #regで受け取ったマクロの引数を文字列リテラルに変換する READ_CSR(123)だとreg="123"という文字になる */
/* 値の返却する場合({})の書き方をする */
#define READ_CSR(reg) \
    ({ \
        unsigned long __v; \
        __asm__ __volatile__("csrr %0, " #reg : "=r"(__v)); \
        __v; \
    })

#define WRITE_CSR(reg, value) \
    do { \
        uint32_t __v = (value); \
        __asm__ __volatile__("csrw " #reg ", %0" :: "r"(__v)); \
    } while (0)

#define PROCS_MAX 8 // 最大プロセス数
#define PROC_UNUSED 0 // プロセスが未使用状態
#define PROC_RUNNABLE 1 // プロセスが実行可能状態

struct process {
    int pid; // プロセスID
    int state; // プロセスの状態
    vaddr_t sp; // コンテキストスイッチ時のスタックポインタ。実際はカーネルスタックのいずれかのポインタを保持する
    uint32_t *page_table; // ページテーブル
    uint8_t stack[8192]; // カーネルスタック
};


#define SATP_SV32 (1u << 31)
/* ビット演算: 1<<0 1を左に0ビットシフト */
#define PAGE_V (1 << 0) //有効化ビット
#define PAGE_R (1 << 1) //読み取り可能ビット
#define PAGE_W (1 << 2) //書き込み可能ビット
#define PAGE_X (1 << 3) //実行可能ビット
#define PAGE_U (1 << 4) //ユーザモードで実行可能ビット
