#include "kit.h"
#include <stdio.h>

kit_memory kit_read_file(kit_allocator *alloc, const char *path, bool null_terminate, kit_file_error *err) {
    if (!alloc || !path || !err) return (kit_memory){0};

    FILE *file = fopen(path, "rb");
    if (!file) {
        if (err) *err = KIT_FILE_ERROR_NOT_FOUND;
        kit_log_error("Failed to open file: %s", path);
        return (kit_memory){0};
    }
    fseek(file, 0, SEEK_END);
    long filesize = ftell(file);
    fseek(file, 0, SEEK_SET);

    kit_memory result = {0};
    result.ptr = kit_alloc(alloc, filesize + 1);
    if (!result.ptr) {
        fclose(file);
        if (err) *err = KIT_FILE_ERROR_NOMEM;
        kit_log_error("Failed to allocate memory for file: %s", path);
        return (kit_memory){0};
    }

    fread(result.ptr, 1, filesize, file);
    fclose(file);

    if (null_terminate) {
        result.ptr[filesize] = '\0';
    }

    result.size = filesize;
    kit_log_trace("Loaded file: %s (%ld bytes)", path, filesize);
    return result;
}