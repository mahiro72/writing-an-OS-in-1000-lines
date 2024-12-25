#pragma once /* 多重インクルードの防止 */

struct sbiret {
    long error;
    long value;
};

#define PANIC(fmt, ...) \
    do { \
        printf("PANIC: %s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
        while (1) {} \
    } while (0) /* 複数の分をマクロで書きたいときに頻出する書き方 */
