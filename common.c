#include "common.h"

void putchar(char ch);

int atoi(const char *str) {
	int sign;
	int num;

	while ((9 <= *str && *str <= 13) || *str == 32)
		str++;
	sign = 1;
	if (*str == '+' || *str == '-')
		if (*str++ == '-')
			sign = -1;
	num = 0;
	while ('0' <= *str && *str <= '9'){
		num = 10 * num + (*str - '0');
		str++;
	}
	return ((int)(num * sign));
}

size_t get_len(int n) {
	size_t len;

	len = 0;
	if (n == 0)
		return (1);
	if (n < 0) {
		len++;
		n *= -1;
	}
	while (n) {
		len++;
		n /= 10;
	}
	return len;
}


void printf(const char *fmt, ...) {
	va_list vargs;
	va_start(vargs,fmt);

	while (*fmt) {
		if (*fmt == '%') {
			fmt++;
			switch (*fmt){
				case '\0':
				case '%':
					putchar('%');
					break;
				case 's' : {
					const char *s = va_arg(vargs,const char *);
					while (*s)
						putchar(*s++);
					break;
				}
				case 'd' : {
					int v = va_arg(vargs,int);
       					 if (v < 0){
               					 putchar('-');
						 v *= -1;
					 }
       					 
					int divisor = 1;
					while (v / divisor > 9)
						divisor *= 10;

					 while (divisor) {
						 putchar(v / divisor + '0');
						 divisor /= 10;
						 v %= 10;
       					 }
					break;
				}
				case 'x': {
					int v = va_arg(vargs,int);
					int i;
					i = 7;
					while (i >= 0) {
						int nibble = (v >> (i * 4)) & 0xf;
						putchar("0123456789abcdef"[nibble]);
						i--;
					}
				}
			}
		} else {
			putchar(*fmt);
		}
		fmt++;
	}

	va_end(vargs);
}



