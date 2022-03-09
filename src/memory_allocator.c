#include "memory_allocator.h"

#define HEAP_SIZE   (640000)
unsigned char heap[HEAP_SIZE] = { 0 };
boolean is_heap_inited = false;

typedef struct
{
	uint64_t chunk_size;
	unsigned char* previous_free_chunk;
	unsigned char* next_free_chunk;
}chunk_data;


static void set_bitMask(uint64_t* size)
{
	*size |= 1;
}

static void reset_bitMask(uint64_t* size)
{
	*size &= ~1;
}

static uint64_t readSize(uint64_t size)
{
	return size &= ~1;
}

static boolean is_block_free(const uint64_t size)
{
	if ((size & 1) == 1) return false;
	return true;
}

static void reset_chunk_data(chunk_data* pointer)
{
	chunk_data data = { 0, NULL, NULL };
	*pointer = data;
}

static void save_chunk_data(unsigned char* pointer, const chunk_data data)
{
	chunk_data* pointer_to_data = pointer - sizeof(chunk_data);
	*pointer_to_data = data;
	*((uint64_t*)(pointer - sizeof(chunk_data) + readSize(data.chunk_size) - sizeof(uint64_t))) = data.chunk_size;
}

void initHeap()
{
	chunk_data data = { HEAP_SIZE, NULL, NULL };
	save_chunk_data(&heap[0] + sizeof(chunk_data), data);
}

static void update_sizes(chunk_data* chunk_begin, uint64_t new_size)
{
	uint64_t old_size = readSize(chunk_begin->chunk_size);
	chunk_begin->chunk_size = new_size;
	uint64_t* chunk_end = (unsigned char*)chunk_begin + old_size - sizeof(uint64_t);
	*chunk_end = new_size;
	chunk_data* ptr = ((unsigned char*)chunk_begin + old_size - new_size);
	*ptr = *chunk_begin;
}


static void split_free_chunk(const uint64_t size, chunk_data* chunk)
{
	chunk_data* first_chunk = chunk;
	chunk_data* second_chunk = ((unsigned char*)chunk + size);
	update_sizes(chunk, readSize(chunk->chunk_size) - size);
	chunk_data data = { size, NULL, NULL };
	*first_chunk = data;
	set_bitMask(&(first_chunk->chunk_size));
	*((uint64_t*)((unsigned char*)second_chunk + second_chunk->chunk_size - sizeof(uint64_t))) = second_chunk->chunk_size;
	*((uint64_t*)((unsigned char*)first_chunk + size - sizeof(uint64_t))) = first_chunk->chunk_size;

}


static void merge_chunks(chunk_data* first, chunk_data* second)
{
	uint64_t final_size = first->chunk_size + second->chunk_size;
	first->chunk_size = final_size;
	*((uint64_t*)((unsigned char*)second + second->chunk_size - sizeof(uint64_t))) = final_size;

	chunk_data* next_free_chunk = second->next_free_chunk;
	if (next_free_chunk != NULL) next_free_chunk->previous_free_chunk = first;

	chunk_data* previous_free_chunk = first->previous_free_chunk;
	if (previous_free_chunk != NULL) previous_free_chunk->next_free_chunk = first;

	first->next_free_chunk = second->next_free_chunk;
	reset_chunk_data(second);
}

static unsigned char* get_next_free_block(chunk_data* block)
{
	block = (chunk_data*)((unsigned char*)block + readSize(block->chunk_size));
	while ((unsigned char*)block < &heap[HEAP_SIZE - 1])
	{
		if (is_block_free(block->chunk_size)) return (unsigned char*)block;
		block = (chunk_data*)((unsigned char*)block + readSize(block->chunk_size));
	}

	return NULL;
}

static unsigned char* get_previous_free_block(chunk_data* block)
{
	while (true)
	{
		uint64_t previous_chunk_size = *((uint64_t*)((unsigned char*)block - sizeof(uint64_t)));
		if ((unsigned char*)block - sizeof(uint64_t) < &heap[0]) break;
		block = (chunk_data*)((unsigned char*)block - readSize(previous_chunk_size));
		if (is_block_free(block->chunk_size)) return block;
	}

	return NULL;
}


void* malloc(size_t size)
{
	if (!is_heap_inited) initHeap();

	uint64_t chunk_size = ceil((size + sizeof(chunk_data) + sizeof(uint64_t)) / 8.0f) * 8;
	chunk_data* current = (chunk_data*)&heap[0];

	long int best_fit = INT64_MAX;
	chunk_data* best_chunk = NULL;

	while ((unsigned char*)current < &heap[HEAP_SIZE - 1])
	{
		if (is_block_free(current->chunk_size)) {
			if (current->chunk_size < best_fit)
			{
				best_chunk = current;
				best_fit = current->chunk_size;
			}
			if (current->next_free_chunk == NULL) break;
			current = current->next_free_chunk;
		}
		current = (unsigned char*)current + readSize(current->chunk_size);
	}

	if (best_chunk->chunk_size - sizeof(chunk_data) - sizeof(uint64_t) >= chunk_size)
	{
		split_free_chunk(chunk_size, best_chunk);
		uint64_t* ptr = (unsigned char*)best_chunk + chunk_size - sizeof(uint64_t);
		return ((unsigned char*)best_chunk + sizeof(chunk_data));
	}

	if (best_chunk->chunk_size == chunk_size)
	{
		chunk_data data = { chunk_size, NULL, NULL };
		save_chunk_data((unsigned char*)best_chunk, data);
		return ((unsigned char*)best_chunk + sizeof(chunk_data));
	}

	return NULL;
}


void* calloc(size_t number_of_elements, size_t element_size)
{
	if (!is_heap_inited) initHeap();

	size_t total_size = number_of_elements * element_size;

	unsigned char* pointer = (unsigned char*)malloc(total_size);
	if (pointer == NULL) return NULL;

	unsigned char* block_begin = pointer;

	for (; pointer < block_begin + total_size; pointer++)
	{
		*pointer = 0;
	}

	return block_begin;
}


void free(void* pointer)
{
	chunk_data* ptr = &heap[0];
	chunk_data* chunk_begin = ((unsigned char*)pointer - sizeof(chunk_data));
	if (is_block_free(chunk_begin->chunk_size)) return;
	reset_bitMask(&(chunk_begin->chunk_size));

	chunk_data* next_free_chunk = get_next_free_block(chunk_begin);
	chunk_data* previous_free_chunk = get_previous_free_block(chunk_begin);
	chunk_begin->next_free_chunk = next_free_chunk;
	chunk_begin->previous_free_chunk = previous_free_chunk;

	if (next_free_chunk != NULL) next_free_chunk->previous_free_chunk = chunk_begin;
	if (previous_free_chunk != NULL) previous_free_chunk->next_free_chunk = chunk_begin;

	if (next_free_chunk != NULL && (unsigned char*)chunk_begin + chunk_begin->chunk_size == (unsigned char*)next_free_chunk)
	{
		merge_chunks(chunk_begin, next_free_chunk);
	}

	if (previous_free_chunk != NULL && (unsigned char*)previous_free_chunk + previous_free_chunk->chunk_size == (unsigned char*)chunk_begin)
	{
		merge_chunks(previous_free_chunk, chunk_begin);
	}
}

void* realloc(void* pointer, size_t size)
{
	if (!is_heap_inited) initHeap();

	chunk_data* data = (unsigned char*)pointer - sizeof(chunk_data);
	uint64_t old_size = data->chunk_size - sizeof(chunk_data) - sizeof(uint64_t);

	if (old_size == size) return pointer;

	unsigned char* new_chunk = (unsigned char*)malloc(size);

	if (pointer == NULL) return new_chunk;
	if (new_chunk == NULL) return pointer;
	if (pointer == NULL && new_chunk == NULL) return NULL;

	if (size > old_size)
	{
		for (uint64_t i = 0; i < old_size; i++)
		{
			new_chunk[i] = ((unsigned char*)pointer)[i];
		}
	}

	else
	{
		for (uint64_t i = 0; i < size; i++)
		{
			new_chunk[i] = ((unsigned char*)pointer)[i];
		}
	}

	free(pointer);
	return new_chunk;
}
