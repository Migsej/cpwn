#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
	char *line = NULL;
	size_t size = 0;
	getline(&line, &size, stdin);
	printf("what %s", line);
	printf("fr\n");
	return 0;
}
