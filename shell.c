#include "user.h"

void print_welcome(void) {
    printf("\n");
    printf("*********************************\n");
    printf("*   Welcome to RISC-V Shell!    *\n");
    printf("*   Type 'hello' to start       *\n");
    printf("*********************************\n");
    printf("\n");
}

void main(void) {
    print_welcome();

    while (1) {
prompt:
        printf("$ ");
        char cmdline[128];
        for (int i = 0; ; i++) {
            char ch = getchar();
            putchar(ch);
            if (i == sizeof(cmdline) - 1) {
                printf("sh: command too long\n");
                goto prompt;
            } else if (ch == '\r') {
                cmdline[i] = '\0';
                break;
            } else {
                cmdline[i] = ch;
            }
        }

        printf("$ %s\n", cmdline);
        if (strcmp(cmdline, "hello") == 0) {
            printf("Hello, world from shell!\n");
        } else if (cmdline[0] == '\0') {
            // do nothing
        } else if (strcmp(cmdline, "exit") == 0) {
            exit();
        } else {
            printf("sh: %s: command not found\n", cmdline);
        }
    }
}
