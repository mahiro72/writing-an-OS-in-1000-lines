#include "user.h"

extern char _binary_shell_bin_start[];
extern char _binary_shell_bin_size[];

void main(void){
	/*
	0x80200000はページテーブル上でマップされているカーネルが利用するメモリ領域
	しかし、ページテーブルエントリのUビットが立っていないカーネル用ページであるため、例外(ページフォルト) が発生する`
	*((volatile int *) 0x80200000) = 0x1234;
 	*/
	
	printf("Hello World from shell!\n\n");

	while (1) {
prompt:
		printf("> ");
		char cmdline[128];
		int i = 0;
		while (1) {
			char ch = getchar();
			putchar(ch);
			if (i == sizeof(cmdline) - 1) {
				printf("command line too long\n");
				goto prompt;
			} else if (ch == '\r') {
				printf("\n");
				cmdline[i] = '\0';
				break;
			} else {
				cmdline[i] = ch;
			}
			i++;
		}

		if (strcmp(cmdline, "hello") == 0)
			printf("Hello world from shell!\n");
		else if (strcmp(cmdline, "exit") == 0)
			exit();
		else if (strcmp(cmdline, "\0") == 0)
			continue;
		else 
			printf("unknown command: %s\n", cmdline);
	}
}

