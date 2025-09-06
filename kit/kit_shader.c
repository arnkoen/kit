#include "deps/bgfx/bgfx.h"
#include "kit.h"

bgfx_shader_handle_t kit_load_shader(kit_allocator *alloc, const char *path, kit_file_error *err) {
    if (!alloc || !path) return (bgfx_shader_handle_t)BGFX_INVALID_HANDLE;
    kit_memory mem = kit_read_file(alloc, path, true, err);
    bgfx_shader_handle_t shader = kit_load_shader_mem(alloc, &mem);
    kit_free(alloc, mem.ptr);
    return shader;
}

bgfx_shader_handle_t kit_load_shader_mem(kit_allocator *alloc, const kit_memory *mem) {
    if (!alloc || !mem || !mem->ptr || mem->size == 0) return (bgfx_shader_handle_t)BGFX_INVALID_HANDLE;
    bgfx_shader_handle_t shader = bgfx_create_shader(bgfx_copy(mem->ptr, (uint32_t)mem->size));
    return shader;
}