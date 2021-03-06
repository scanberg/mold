#ifndef _MD_POOL_ALLOCATOR_H_
#define _MD_POOL_ALLOCATOR_H_

#include <stdint.h>

struct md_allocator;

#ifdef __cplusplus
extern "C" {
#endif

// The pool allocator is ideal for allocating and freeing objects of the same size (or less than the defined object_size)
// It internally allocates larger pages to restrict the strain on the backing allocator.
// NOTE: YOU CAN ONLY ALLOCATE OBJECT_SIZE OR LESS FROM THIS ALLOCATOR, OTHERWISE ASSERTIONS WILL FIRE!

struct md_allocator* md_pool_allocator_create(struct md_allocator* backing, uint32_t slot_size);
void md_pool_allocator_destroy(struct md_allocator* a);

#ifdef __cplusplus
}
#endif

#endif