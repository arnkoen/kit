#include "kit_allocators.c"
#include "kit_log.c"
#include "kit_file.c"
#include "kit_shader.c"
#include "kit_image.c"
#include "kit_mesh.c"
#include "kit_anim.c"
#include "kit_camera.c"

bool kit_init(const kit_desc* desc) {
	kit_log_set_level(desc->log_level);

	bgfx_render_frame(0);

	bgfx_init_t init = { 0 };
	bgfx_init_ctor(&init);

	init.platformData.nwh = desc->window;
#if !defined _WIN32
    init.platformData.ndt = (void*)desc->display;
 #endif
	init.type = KIT_DEF(desc->renderer, BGFX_RENDERER_TYPE_COUNT);
	init.resolution.width = (uint32_t)desc->width;
	init.resolution.height = (uint32_t)desc->height;
	init.resolution.reset = KIT_DEF(desc->reset, BGFX_RESET_VSYNC);
	if(!bgfx_init(&init)) {
		kit_log_error("Failed to init bgfx!");
		return false;
	}
    return true;
}

void kit_shutdown(void) {
	bgfx_shutdown();
}
