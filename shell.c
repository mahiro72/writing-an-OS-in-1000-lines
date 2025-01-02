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
        } else if (strcmp(cmdline, "readfile") == 0) {
            char buf[128];
            int len = readfile("./hello.txt", buf, sizeof(buf));
            buf[len] = '\0';
            printf("%s\n", buf);
        } else if (strcmp(cmdline, "writefile") == 0) {\
            char *msg = "hello from shell by writefile command";
            writefile("./hello.txt", (const char *) msg, len(msg));
        } else {
            printf("sh: %s: command not found\n", cmdline);
        }
    }
}
