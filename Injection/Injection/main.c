#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include "create_thread_inject.h"

int main(int argc, char *argv[]) {
	FILE *f;
	unsigned char *buffer;
	long size;

	if (argc < 2) {
		printf("Usage: %s <filepath>\n", argv[0]);
		return 1;
	}

	const char *filepath = argv[1];

	f = fopen(filepath, "rb");
	if (!f) {
		printf("Cannot open file: %s\n", filepath);
		return 1;
	}

	fseek(f, 0, SEEK_END);
	size = ftell(f);
	rewind(f);

	buffer = (unsigned char*)malloc(size);
	if (!buffer) {
		printf("Memory alloc failed\n");
		fclose(f);
		return 1;
	}

	size_t read = fread(buffer, 1, size, f);
	fclose(f);

	if (read != size) {
		printf("Read error\n");
		free(buffer);
		return 1;
	}

	printf("Read %ld bytes from %s\n", size, filepath);

	create_thread_self_inject(buffer, size);

	free(buffer);
	return 0;
}
