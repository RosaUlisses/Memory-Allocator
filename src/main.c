#include <stdio.h>
#include "memory_allocator.h"


void print_vector(int* ptr, int size)
{
	printf("Array = { ");
	for(int i = 0; i < size; i++)
	{
		if (i == size - 1) {
			printf("%d }", ptr[i]);
			break;
		}

		printf("%d, ", ptr[i]);
	}
	printf("\n\n");
}

int main(int argc, char** argv)
{
	int* vector = malloc(10 * sizeof(int));

	for(int i = 0; i < 10; i++)
	{
		vector[i] = i;
	}
	print_vector(vector, 10);



	vector = (int*)realloc(vector, 20 * sizeof(int));
	for(int i = 10; i < 20; i++)
	{
		vector[i] = i;
	}
	printf("Reallocated ");
	print_vector(vector, 20);

	free(vector);

	return 0;
}
