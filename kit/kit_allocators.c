#include "kit.h"
#include <stdio.h>
#include <assert.h>

//--ALLOCATORS-------------------------------------------------------

void* kit_alloc(kit_allocator* alloc, size_t size) {
    if (alloc && alloc->alloc) {
        return alloc->alloc(size, alloc->udata);
    }
    return NULL;
}

void* kit_realloc(kit_allocator* alloc, void* ptr, size_t size) {
    if (alloc && alloc->realloc) {
        return alloc->realloc(ptr, size, alloc->udata);
    }
    return NULL;
}

void kit_free(kit_allocator* alloc, void* ptr) {
    if (alloc && alloc->free) {
        alloc->free(ptr, alloc->udata);
    }
}

//DEFAULTS

static void* _default_alloc(size_t size, void* udata) {
    (void)udata;
    return malloc(size);
}

static void* _default_realloc(void* ptr, size_t size, void* udata) {
    (void)udata;
    return realloc(ptr, size);
}

static void _default_free(void* udata, void* ptr) {
    (void)udata;
    free(ptr);
}

kit_allocator kit_default_allocator(void) {
    kit_allocator alloc = {0};
    alloc.alloc = _default_alloc;
    alloc.realloc = _default_realloc;
    alloc.free = _default_free;
    return alloc;
}

//ARENA

typedef struct {
    kit_allocator* alloc;
    char* buffer;
    size_t capacity;
    size_t offset;
    size_t align;
} arena_t;

static size_t _align_forward(size_t ptr, size_t align) {
    size_t mod = ptr % align;
    return mod == 0 ? ptr : ptr + (align - mod);
}

static void* _arena_alloc(size_t size, void* udata) {
    arena_t* arena = (arena_t*)udata;
    size_t current = (size_t)arena->buffer + arena->offset;
    size_t aligned = _align_forward(current, arena->align);
    size_t new_offset = aligned - (size_t)arena->buffer + size;

    if(new_offset > arena->capacity) return NULL;

    void* ret = (void*)(arena->buffer + (aligned - (size_t)arena->buffer));
    arena->offset = new_offset;
    return ret;
}

static void* _arena_realloc(void* ptr, size_t size, void* udata) {
    (void)ptr;
    return _arena_alloc(size, udata);
}

static void _arena_free(kit_allocator* alloc, void* ptr) {
    (void)alloc; (void)ptr;
}

void kit_release_arena(kit_allocator* alloc) {
    arena_t* arena = (arena_t*)alloc->udata;
    kit_free(arena->alloc, arena->buffer);
    kit_free(alloc, arena);
    arena->buffer = NULL;
    arena->capacity = 0;
    arena->offset = 0;
}

void kit_arena_reset(kit_allocator* alloc) {
    arena_t* arena = (arena_t*)alloc->udata;
    arena->offset = 0;
}

kit_allocator kit_arena_allocator(kit_allocator* alloc, size_t capacity, size_t align) {
    arena_t* arena = (arena_t*)kit_alloc(alloc, sizeof(arena_t));
    if (!arena) return (kit_allocator){0};
    arena->buffer = (char*)kit_alloc(alloc, capacity);
    if(!arena->buffer) return (kit_allocator){0};
    arena->capacity = capacity;
    arena->align = align;
    arena->offset = 0;
    kit_allocator ret = {
        .alloc = (kit_alloc_fn)_arena_alloc,
        .realloc = (kit_realloc_fn)_arena_realloc,
        .free = (kit_free_fn)_arena_free,
        .udata = arena
    };
    return ret;
}
