#include "kit/kit.h"
#define RGFW_IMPLEMENTATION
#include "RGFW.h"

#define SHADER_PATH(x) "shd/vk/" x

int main() {
	RGFW_window* win = RGFW_createWindow("KIT", 0, 0, 800, 600, RGFW_windowCenter | RGFW_windowNoResize);
	RGFW_window_setExitKey(win, RGFW_escape);

	bool ok = kit_init(&(kit_desc) {
		.window = win->src.window,
		.width = (uint32_t)win->w,
		.height = (uint32_t)win->h,
		.renderer = BGFX_RENDERER_TYPE_VULKAN,
		.reset = BGFX_RESET_VSYNC | BGFX_RESET_MSAA_X4,
		.log_level = KIT_LOG_TRACE,
	});

	uint64_t state = 0|
	BGFX_STATE_WRITE_RGB|
	BGFX_STATE_WRITE_Z|
	BGFX_STATE_DEPTH_TEST_LEQUAL|
	BGFX_STATE_MSAA;

	bgfx_set_state(state, 0);

	bgfx_set_view_rect(0, 0, 0, (uint16_t)win->w, (uint16_t)win->h);

	bgfx_set_view_clear(
		0,
		BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
		0x200020ff,
		1.0f,
		0
    );

	bgfx_set_debug(BGFX_DEBUG_TEXT);

	kit_allocator alloc = kit_default_allocator();
	kit_allocator arena = kit_arena_allocator(&alloc, 10 * 1024 * 1024, 16);

	kit_file_error err = KIT_FILE_ERROR_NONE;

	bgfx_shader_handle_t vsh = kit_load_shader(&arena, SHADER_PATH("skinned.vs.bin"), &err);
	kit_log_debug("Loaded vertex shader, err: %d", err);

	bgfx_shader_handle_t fsh = kit_load_shader(&arena, SHADER_PATH("skinned.fs.bin"), &err);
	kit_log_debug("Loaded fragment shader, err: %d", err);

	bgfx_program_handle_t program = bgfx_create_program(vsh, fsh, false);
	bgfx_uniform_handle_t u_color = bgfx_create_uniform("u_color", BGFX_UNIFORM_TYPE_VEC4, 1);
	bgfx_uniform_handle_t u_bones = bgfx_create_uniform("u_bones", BGFX_UNIFORM_TYPE_MAT4, 32);

	kit_m3d_data* m3d = kit_load_m3d_data(&arena, "assets/cesium_man.m3d", &err);
	kit_log_debug("Loaded fragment shader, err: %d", err);

	kit_mesh mesh = kit_make_mesh_from_m3d(&arena, m3d);

	kit_skeleton skel = {0};
	kit_load_skeleton(&arena, &skel, m3d);

	int anim_count = 0;
	kit_bone_anim_data* anims = kit_load_bone_anims(&arena, m3d, &anim_count);
	kit_log_debug("Loaded animations, count: %d", anim_count);
	assert(anims);

	kit_bone_anim_state anim_state = {
		.anim = &anims[0],
		.time = 0,
		.frame = 0,
		.loop = true
	};

	kit_release_m3d_data(m3d);

	kit_cam cam = {0};
	kit_init_cam(&cam, &(kit_cam_desc) {
		.distance = 2.0f,
		.latitude = 20.f,
		.center = HMM_V3(0.0f, 0.5f, 0.0f)
	});

	while(!RGFW_window_shouldClose(win)) {
		RGFW_event event;
		while(RGFW_window_checkEvent(win, &event)) {
			switch(event.type) {
				case RGFW_quit: break;
				default: break;
			}
		}
		bgfx_touch(0);

		kit_orbit_cam(&cam, 1.f, 0.0f);
		kit_update_cam(&cam, win->w, win->h);

		bgfx_dbg_text_printf(1, 1, 0x0f, "Hello, Kit!");

		bgfx_set_view_transform(0, &cam.view.Elements[0], &cam.proj.Elements[0]);

		HMM_Mat4 bones[KIT_MAX_BONES] = {0};
		kit_play_bone_anim(bones, &skel, &anim_state, 1.0f / 60.0f);

		bgfx_set_uniform(u_bones, &bones[0].Elements[0], KIT_MAX_BONES);

		HMM_Vec4 color = HMM_V4(0.1f, 0.1f, 0.5f, 1.0f);
		bgfx_set_uniform(u_color, &color, 1);

		kit_set_mesh(&mesh);
		bgfx_submit(0, program, 0, BGFX_DISCARD_NONE);
		bgfx_frame(false);
	}

	kit_shutdown();
	RGFW_window_close(win);
	return 0;
}

