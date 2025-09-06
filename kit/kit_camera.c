#include "kit.h"
#include <assert.h>

//--CAMERA----------------------------------------------------------

#define CAMERA_DEFAULT_MIN_DIST (2.0f)
#define CAMERA_DEFAULT_MAX_DIST (50.0f)
#define CAMERA_DEFAULT_MIN_LAT (-85.0f)
#define CAMERA_DEFAULT_MAX_LAT (85.0f)
#define CAMERA_DEFAULT_DIST (10.0f)
#define CAMERA_DEFAULT_ASPECT (60.0f)
#define CAMERA_DEFAULT_NEARZ (1.0f)
#define CAMERA_DEFAULT_FARZ (1000.0f)

static float _cam_def(float val, float def) { return ((val == 0.0f) ? def : val); }

static HMM_Vec3 _cam_euclidean(float latitude, float longitude) {
    const float lat = latitude * HMM_DegToRad;
    const float lng = longitude * HMM_DegToRad;
    return HMM_V3(cosf(lat)* sinf(lng), sinf(lat), cosf(lat)* cosf(lng));
}

void kit_init_cam(kit_cam* cam, const kit_cam_desc* desc) {
    assert(desc && cam);
    cam->mindist = _cam_def(desc->mindist, CAMERA_DEFAULT_MIN_DIST);
    cam->maxdist = _cam_def(desc->maxdist, CAMERA_DEFAULT_MAX_DIST);
    cam->minlat = _cam_def(desc->minlat, CAMERA_DEFAULT_MIN_LAT);
    cam->maxlat = _cam_def(desc->maxlat, CAMERA_DEFAULT_MAX_LAT);
    cam->distance = _cam_def(desc->distance, CAMERA_DEFAULT_DIST);
    cam->center = desc->center;
    cam->latitude = desc->latitude;
    cam->longitude = desc->longitude;
    cam->aspect = _cam_def(desc->aspect, CAMERA_DEFAULT_ASPECT);
    cam->nearz = _cam_def(desc->nearz, CAMERA_DEFAULT_NEARZ);
    cam->farz = _cam_def(desc->farz, CAMERA_DEFAULT_FARZ);
}

void kit_orbit_cam(kit_cam* cam, float dx, float dy) {
    assert(cam);
    cam->longitude -= dx;
    if (cam->longitude < 0.0f) {
        cam->longitude += 360.0f;
    }
    if (cam->longitude > 360.0f) {
        cam->longitude -= 360.0f;
    }
    cam->latitude = HMM_Clamp(cam->minlat, cam->latitude + dy, cam->maxlat);
}

void kit_zoom_cam(kit_cam* cam, float d) {
    assert(cam);
    cam->distance = HMM_Clamp(cam->mindist, cam->distance + d, cam->maxdist);
}

void kit_update_cam(kit_cam* cam, int fb_width, int fb_height) {
    assert(cam);
    const float w = (float)fb_width;
    const float h = (float)fb_height;
    
    cam->eyepos =  HMM_AddV3(cam->center, HMM_MulV3F(_cam_euclidean(cam->latitude, cam->longitude), cam->distance));
    cam->view = HMM_LookAt_RH(cam->eyepos, cam->center, HMM_V3(0.0f, 1.0f, 0.0f));
    cam->proj = HMM_Perspective_RH_NO(cam->aspect * HMM_DegToRad, w / h, 0.1f, 1000.f);
}

/*
void kit_cam_input(kit_cam* cam) {
    assert(cam);

    if (glfwGetMouseButton(kit.window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        int width, height;
        glfwGetWindowSize(kit.window, &width, &height);

		if (cam->first_click) {
			glfwSetCursorPos(kit.window, (width * 0.5f), (height * 0.5f));
			cam->first_click = false;
		}

        glfwSetInputMode(kit.window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

        double mx, my;
        glfwGetCursorPos(kit.window, &mx, &my);
        float rotx = (float)mx - (width * 0.5f);
        float roty = (float)my - (height * 0.5f);

        kit_orbit_cam(cam, (float)rotx*0.25f, (float)roty*0.25f);

        glfwSetCursorPos(kit.window, (double)width * 0.5, (double)height * 0.5);
    } else {
        glfwSetInputMode(kit.window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        cam->first_click = true;
    }
}
*/