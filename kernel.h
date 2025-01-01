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
#define PROC_EXITED 2

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

#define USER_BASE 0x1000000
#define SSTATUS_SPIE (1 << 5) // sstatusの5ビット目(SPIE)として定義
#define SCAUSE_ECALL 8


// virtio
// 基本設定
#define SECTOR_SIZE       512    // ディスクの1セクタのサイズ（バイト）
#define VIRTQ_ENTRY_NUM   16     // 仮想キューのエントリ数
#define VIRTIO_DEVICE_BLK 2      // ブロックデバイスの種類識別子
#define VIRTIO_BLK_PADDR  0x10001000  // デバイスの物理アドレス

// VirtIOレジスタのオフセット
#define VIRTIO_REG_MAGIC         0x00  // マジック値のレジスタ
#define VIRTIO_REG_VERSION       0x04  // バージョン情報
#define VIRTIO_REG_DEVICE_ID     0x08  // デバイスID
#define VIRTIO_REG_QUEUE_SEL     0x30  // キュー選択
#define VIRTIO_REG_QUEUE_NUM_MAX 0x34  // キューの最大サイズ
#define VIRTIO_REG_QUEUE_NUM     0x38  // 現在のキューサイズ
#define VIRTIO_REG_QUEUE_ALIGN   0x3c  // キューのアライメント要件
#define VIRTIO_REG_QUEUE_PFN     0x40  // キューのページフレーム番号
#define VIRTIO_REG_QUEUE_READY   0x44  // キューの準備状態
#define VIRTIO_REG_QUEUE_NOTIFY  0x50  // キュー通知
#define VIRTIO_REG_DEVICE_STATUS 0x70  // デバイスのステータス
#define VIRTIO_REG_DEVICE_CONFIG 0x100 // デバイス固有の設定

// デバイスステータスフラグ
#define VIRTIO_STATUS_ACK       1  // デバイスを認識
#define VIRTIO_STATUS_DRIVER    2  // ドライバがロードされた
#define VIRTIO_STATUS_DRIVER_OK 4  // ドライバの準備完了
#define VIRTIO_STATUS_FEAT_OK   8  // 機能ネゴシエーション完了

// デスクリプタとキューのフラグ
#define VIRTQ_DESC_F_NEXT          1  // 次のデスクリプタが存在
#define VIRTQ_DESC_F_WRITE         2  // デバイスがメモリに書き込み可能
#define VIRTQ_AVAIL_F_NO_INTERRUPT 1  // 割り込み無効

// ブロックデバイスの操作タイプ
#define VIRTIO_BLK_T_IN  0   // 読み込み操作
#define VIRTIO_BLK_T_OUT 1   // 書き込み操作



// デスクリプタキューのエントリを表す構造体
struct virtq_desc {
    uint64_t addr;    // バッファの物理アドレス
    uint32_t len;     // バッファの長さ（バイト）
    uint16_t flags;   // 各種フラグ（VIRTQ_DESC_F_NEXTなど）
    uint16_t next;    // チェーン時の次のデスクリプタのインデックス
} __attribute__((packed));  // パディングなしでパック

// デバイスが使用可能なデスクリプタを管理する構造体
struct virtq_avail {
    uint16_t flags;   // フラグ（割り込み制御など）
    uint16_t index;   // 次に使用するリングのインデックス
    uint16_t ring[VIRTQ_ENTRY_NUM];  // 使用可能なデスクリプタのインデックス配列
} __attribute__((packed));

// デバイスが使用済みのデスクリプタの情報を保持する要素
struct virtq_used_elem {
    uint32_t id;      // 使用済みデスクリプタのインデックス
    uint32_t len;     // 処理されたバイト数
} __attribute__((packed));

// デバイスが処理完了したデスクリプタを管理する構造体
struct virtq_used {
    uint16_t flags;   // フラグ
    uint16_t index;   // 次に書き込むリングのインデックス
    struct virtq_used_elem ring[VIRTQ_ENTRY_NUM];  // 使用済みデスクリプタの情報配列
} __attribute__((packed));

// VirtIOの仮想キュー全体を管理する構造体
struct virtio_virtq {
    struct virtq_desc descs[VIRTQ_ENTRY_NUM];  // デスクリプタキュー
    struct virtq_avail avail;                  // 使用可能キュー
    struct virtq_used used __attribute__((aligned(PAGE_SIZE)));  // 使用済みキュー（ページ境界にアライン）
    int queue_index;                           // このキューのインデックス
    volatile uint16_t *used_index;             // 現在の使用済みインデックスへのポインタ
    uint16_t last_used_index;                  // 最後に確認した使用済みインデックス
} __attribute__((packed));

struct virtio_blk_req {
    // 1つ目のディスクリプタ: デバイスからは読み込み専用
    uint32_t type; // リクエストタイプ（IN/OUT）
    uint32_t reserved; // 予約領域（未使用）
    uint64_t sector; // アクセス対象のセクタ番号

    // 2つ目のディスクリプタ: 読み込み処理の場合は、デバイスから書き込み可 (VIRTQ_DESC_F_WRITE)
    uint8_t data[512]; // データバッファ（1セクタ分）

    // 3つ目のディスクリプタ: デバイスから書き込み可 (VIRTQ_DESC_F_WRITE)
    uint8_t status; // リクエストの完了ステータス
} __attribute__((packed));
