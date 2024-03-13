#include <stdio.h>
#define CPWN_IMPLEMENTATION
#include "ccon.h"

int main() {
	//proc p = remote("127.0.0.1", 6969);
	proc p = process("./test");
	sendline(p, "poopy");
	printf("damn %s", recvuntil(p, "poopy\n"));
	printf("bussin %s", recvline(p));
	closep(p);
	return 0;
}
