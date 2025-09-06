//Copyright (c) 2025 Arne Koenig
//Redistribution and use in source and binary forms, with or without modification, are permitted.
//THIS SOFTWARE IS PROVIDED 'AS-IS', WITHOUT ANY EXPRESS OR IMPLIED WARRANTY. IN NO EVENT WILL THE AUTHORS BE HELD LIABLE FOR ANY DAMAGES ARISING FROM THE USE OF THIS SOFTWARE. 

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "deps/hmm.h"
#include "deps/bgfx/bgfx.h"
#include <stdbool.h>

#ifndef KIT_ASSERT
#include <assert.h>
#define KIT_ASSERT(x) assert(x)
#endif

#if BX_PLATFORM_WINDOWS
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "opengl32.lib")
#endif

#define KIT_DEF(val, def) (val == 0 ? def : val)

typedef struct kit_memory {
	uint8_t* ptr;
	size_t size;
} kit_memory;

#ifndef __cplusplus
#define KIT_MEMORY(x) ((kit_memory){&x, sizeof(x)})
#else 
#define KIT_MEMORY(x) (kit_memory{&x, sizeof(x)})
#endif

//--ALLOCATORS-------------------------------------------------------

typedef void* (*kit_alloc_fn)(size_t size, void* udata);
typedef void* (*kit_realloc_fn)(void* ptr, size_t size, void* udata);
typedef void  (*kit_free_fn)(void* ptr, void* udata);

typedef struct kit_allocator {
	void* udata;
	kit_alloc_fn alloc;
	kit_realloc_fn realloc;
	kit_free_fn free;
} kit_allocator;

void* kit_alloc(kit_allocator* alloc, size_t size);
void* kit_realloc(kit_allocator* alloc, void* ptr, size_t size);
void kit_free(kit_allocator* alloc, void* ptr);

kit_allocator kit_default_allocator(void);

kit_allocator kit_arena_allocator(kit_allocator* alloc, size_t capacity, size_t align);
void kit_arena_reset(kit_allocator* arena);
void kit_release_arena(kit_allocator* arena);

//--LOGGING---------------------------------------------------------
// from https://github.com/rxi/log.c

typedef struct kit_log_event {
	va_list ap;
	const char *fmt;
	const char *file;
	struct tm *time;
	void *udata;
	int line;
	int level;
} kit_log_event;

typedef void (*kit_log_fn)(kit_log_event* ev);
typedef void (*kit_log_lock_fn)(bool lock, void *udata);

typedef enum { 
	KIT_LOG_TRACE,
	KIT_LOG_DEBUG,
	KIT_LOG_INFO,
	KIT_LOG_WARN,
	KIT_LOG_ERROR,
	KIT_LOG_FATAL
} kit_log_level;

#define kit_log_trace(...) kit_log(KIT_LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define kit_log_debug(...) kit_log(KIT_LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define kit_log_info(...)  kit_log(KIT_LOG_INFO,  __FILE__, __LINE__, __VA_ARGS__)
#define kit_log_warn(...)  kit_log(KIT_LOG_WARN,  __FILE__, __LINE__, __VA_ARGS__)
#define kit_log_error(...) kit_log(KIT_LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define kit_log_fatal(...) kit_log(KIT_LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__)

const char* kit_log_level_string(int level);
void kit_log_set_lock(kit_log_lock_fn fn, void *udata);
void kit_log_set_level(kit_log_level level);
void kit_log_set_quiet(bool enable);
bool kit_log_add_callback(kit_log_fn fn, void *udata, kit_log_level level);
bool kit_log_add_file(const char *path, kit_log_level level);

void kit_log(kit_log_level level, const char* file, int line, const char *fmt, ...);

//--INIT&SHUTDOWN--------------------------------------------------------------

typedef struct kit_desc {
	void* window;
	void* display;
    uint32_t width, height;
    bgfx_renderer_type_t renderer;
	uint16_t vendor_id;
    uint32_t reset;
	kit_log_level log_level;
} kit_desc;

bool kit_init(const kit_desc* desc);
void kit_shutdown(void);

//--FILE--------------------------------------------

typedef enum kit_file_error {
	KIT_FILE_ERROR_NONE = 0,
	KIT_FILE_ERROR_NOT_FOUND,
	KIT_FILE_ERROR_IO,
	KIT_FILE_ERROR_NOMEM,
	KIT_FILE_ERROR_INVALID_ARGS,
	KIT_FILE_ERROR_UNKNOWN,
} kit_file_error;

kit_memory kit_read_file(kit_allocator* alloc, const char* path, bool null_terminate, kit_file_error* err);

//--SHADER----------------------------------------------

bgfx_shader_handle_t kit_load_shader(kit_allocator* alloc, const char* path, kit_file_error* err);
bgfx_shader_handle_t kit_load_shader_mem(kit_allocator* alloc, const kit_memory* mem);

//--IMAGE--------------------------------------------

typedef struct kit_image_data {
	void* data;
	uint16_t width;
	uint16_t height;
	uint16_t channel_count;
} kit_image_data;

//loads a 2D image in the qoi image format
kit_image_data kit_load_image_data(kit_allocator* alloc, const char* path, uint16_t channel_count, kit_file_error* err);
kit_image_data kit_load_image_data_mem(kit_allocator* allocator, const kit_memory* mem, uint16_t channel_count);
void kit_release_image_data(kit_allocator* alloc, kit_image_data* img);

//--MESH--------------------------------------------

typedef struct m3d_t kit_m3d_data;

kit_m3d_data* kit_load_m3d_data(kit_allocator* alloc, const char* path, kit_file_error* err);
kit_m3d_data* kit_load_m3d_data_mem(kit_memory* mem);
void kit_release_m3d_data(kit_m3d_data* m3d);

typedef struct kit_vertex_pnt {
	HMM_Vec3 pos;
	HMM_Vec3 nrm;
	HMM_Vec2 uv;
} kit_vertex_pnt;

bgfx_vertex_layout_t kit_vertex_layout_pnt();

typedef struct kit_vertex_skin {
	HMM_Vec3 pos;
	HMM_Vec3 nrm;
	HMM_Vec2 uv;
	uint8_t indices[4];
	float weights[4];
} kit_vertex_skin;

bgfx_vertex_layout_t kit_vertex_layout_skin();

typedef struct kit_mesh_desc {
	bgfx_vertex_layout_t layout;
	kit_memory vertices;
	kit_memory indices;
	uint16_t element_count;
} kit_mesh_desc;

typedef struct kit_mesh {
	bgfx_vertex_buffer_handle_t vbuf;
	bgfx_index_buffer_handle_t ibuf;
	uint16_t element_count;
} kit_mesh;

kit_mesh kit_make_mesh(const kit_mesh_desc* desc);
kit_mesh kit_make_mesh_from_m3d(kit_allocator* alloc, kit_m3d_data* m3d);
void kit_set_mesh(kit_mesh* mesh);
void kit_release_mesh(kit_mesh* mesh);


//--ANIMATIONS---------------------------------------------

#define KIT_MAX_NAME_LEN 32
#define KIT_MAX_BONES 32

typedef struct kit_bone {
    char name[KIT_MAX_NAME_LEN];
    int parent;
} kit_bone;

typedef struct kit_transform {
    HMM_Vec3 pos;
    HMM_Quat rot;
    HMM_Vec3 scale;
} kit_transform;

typedef struct kit_skeleton {
    kit_transform* bind_poses;
    kit_bone* bones;
    int bone_count;
} kit_skeleton;

bool kit_load_skeleton(kit_allocator* alloc, kit_skeleton* skeleton, kit_m3d_data* m3d);
void kit_release_skeleton(kit_allocator* alloc, kit_skeleton* skeleton);

typedef struct kit_bone_keyframe {
    float time;
    kit_transform* pose;
} kit_bone_keyframe;

typedef struct kit_bone_anim_data {
    kit_bone* bones;
    kit_bone_keyframe* keyframes;
    int keyframe_count;
    int bone_count;
} kit_bone_anim_data;

typedef struct kit_bone_anim_state {
    kit_bone_anim_data* anim;
    float time;
    int frame;
    bool loop;
} kit_bone_anim_state;

kit_bone_anim_data* kit_load_bone_anims(kit_allocator* alloc, kit_m3d_data* m3d, int* count);
void kit_play_bone_anim(HMM_Mat4* trs, kit_skeleton* skeleton, kit_bone_anim_state* state, float dt);
void kit_release_bone_anim(kit_allocator* alloc, kit_bone_anim_data* anim);


//--CAM---------------------------------------------

typedef struct kit_cam_desc {
	float mindist;
	float maxdist;
	float minlat;
	float maxlat;
	float distance;
	float latitude;
	float longitude;
	float aspect;
	float nearz;
	float farz;
	HMM_Vec3 center;
} kit_cam_desc;

typedef struct kit_cam {
	float mindist;
	float maxdist;
	float minlat;
	float maxlat;
	float distance;
	float latitude;
	float longitude;
	float aspect;
	float nearz;
	float farz;
	HMM_Vec3 center;
	HMM_Vec3 eyepos;
	HMM_Mat4 view;
	HMM_Mat4 proj;
} kit_cam;

void kit_init_cam(kit_cam* cam, const kit_cam_desc* desc);
void kit_orbit_cam(kit_cam* cam, float dx, float dy);
void kit_zoom_cam(kit_cam* cam, float d);
void kit_update_cam(kit_cam* cam, int fb_width, int fb_height);
//void kit_cam_input(kit_cam* cam);

#ifdef __cplusplus
}
#endif
