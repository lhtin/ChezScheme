/* memory.c -- ChezSchemeOS freestanding libc: malloc/free/realloc/calloc
 *
 * Simple bump allocator with free-list recycling.
 * Heap region defined by linker symbols _heap_start and _heap_end.
 */

#include <stddef.h>
#include <stdint.h>

extern void *memcpy(void *, const void *, size_t);
extern void *memset(void *, int, size_t);

/* Linker-provided symbols */
extern char _heap_start[];
extern char _heap_end[];

#define ALLOC_MAGIC 0xDEADBEEFCAFEBABEULL
#define ALIGN16(x) (((x) + 15) & ~(size_t)15)

/* Allocation header: 16 bytes, keeps alignment */
typedef struct alloc_hdr {
    size_t size;      /* usable size (not including header) */
    size_t magic;     /* ALLOC_MAGIC for corruption detection */
} alloc_hdr_t;

/* Free list node: stored inside freed blocks */
typedef struct free_node {
    struct free_node *next;
    size_t size;  /* usable size */
} free_node_t;

static char *heap_pos = NULL;
static free_node_t *free_list = NULL;

static void heap_init(void) {
    if (heap_pos == NULL) {
        heap_pos = _heap_start;
    }
}

void *malloc(size_t size) {
    if (size == 0) size = 1;
    heap_init();

    size = ALIGN16(size);

    /* Search free list for first fit */
    free_node_t **pp = &free_list;
    while (*pp) {
        free_node_t *node = *pp;
        if (node->size >= size) {
            /* Remove from free list and reuse */
            *pp = node->next;
            alloc_hdr_t *hdr = (alloc_hdr_t *)node;
            hdr->size = node->size;
            hdr->magic = ALLOC_MAGIC;
            return (void *)(hdr + 1);
        }
        pp = &node->next;
    }

    /* Bump allocator */
    size_t total = sizeof(alloc_hdr_t) + size;
    if (heap_pos + total > _heap_end) {
        /* Out of memory */
        return NULL;
    }

    alloc_hdr_t *hdr = (alloc_hdr_t *)heap_pos;
    heap_pos += total;
    hdr->size = size;
    hdr->magic = ALLOC_MAGIC;
    return (void *)(hdr + 1);
}

void free(void *ptr) {
    if (ptr == NULL) return;

    alloc_hdr_t *hdr = ((alloc_hdr_t *)ptr) - 1;

    /* Validate magic */
    if (hdr->magic != ALLOC_MAGIC) {
        /* Corrupted or double-free -- silently ignore */
        return;
    }

    /* Clear magic to detect double-free */
    hdr->magic = 0;

    /* Only recycle blocks >= 32 bytes (smaller ones not worth tracking) */
    if (hdr->size >= sizeof(free_node_t)) {
        free_node_t *node = (free_node_t *)hdr;
        node->size = hdr->size;
        node->next = free_list;
        free_list = node;
    }
    /* else: leak small blocks (they're tiny and rare) */
}

void *realloc(void *ptr, size_t size) {
    if (ptr == NULL) return malloc(size);
    if (size == 0) { free(ptr); return NULL; }

    alloc_hdr_t *hdr = ((alloc_hdr_t *)ptr) - 1;
    if (hdr->magic != ALLOC_MAGIC) return NULL;

    size_t old_size = hdr->size;
    size = ALIGN16(size);

    /* If the block is already big enough, just return it */
    if (old_size >= size) return ptr;

    /* Allocate new block, copy, free old */
    void *new_ptr = malloc(size);
    if (new_ptr == NULL) return NULL;
    memcpy(new_ptr, ptr, old_size < size ? old_size : size);
    free(ptr);
    return new_ptr;
}

void *calloc(size_t nmemb, size_t size) {
    size_t total = nmemb * size;
    /* Check for overflow */
    if (nmemb != 0 && total / nmemb != size) return NULL;
    void *ptr = malloc(total);
    if (ptr) memset(ptr, 0, total);
    return ptr;
}

/* posix_memalign: allocate aligned memory */
int posix_memalign(void **memptr, size_t alignment, size_t size) {
    if (alignment < sizeof(void *) || (alignment & (alignment - 1)) != 0)
        return 22; /* EINVAL */
    /* Simple approach: over-allocate and align */
    size_t total = size + alignment + sizeof(alloc_hdr_t);
    void *raw = malloc(total);
    if (!raw) return 12; /* ENOMEM */
    uintptr_t addr = (uintptr_t)raw;
    uintptr_t aligned = (addr + alignment - 1) & ~(alignment - 1);
    if (aligned == addr) aligned += alignment; /* ensure space for header */
    /* We can't easily free this properly, but for Chez Scheme's use
       (rare aligned allocations), this is acceptable */
    *memptr = (void *)aligned;
    return 0;
}
