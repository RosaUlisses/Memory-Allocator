#pragma once
#ifndef MEMORY_ALLOCATOR_H
#define MEMORY_ALLOCATOR_H

#include <math.h>
#include <stdio.h>
#include <limits.h>
#include <stdint.h>
#include "boolean.h"


void initHeap();
void* malloc(size_t size);
void* calloc(size_t number_of_elements, size_t element_size);
void free(void* pointer);
void* realloc(void* pointer, size_t size);


#endif
