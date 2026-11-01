/* Android phone/Cardboard presentation and tracker bridge. The legacy MallVR
   renderer runs in a shared EGL pbuffer context; GLSurfaceView owns the phone
   display surface and applies Cardboard's per-viewer distortion. */
#include "vpix_cardboard.h"

#include <GLES3/gl3.h>
#include <android/log.h>
#include <cardboard.h>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <time.h>

#define CLOG(...) __android_log_print(ANDROID_LOG_INFO, "MallVRCardboard", __VA_ARGS__)

namespace {
std::mutex state_mutex;
std::condition_variable state_changed;
std::mutex frame_mutex;
EGLDisplay share_display = EGL_NO_DISPLAY;
EGLContext share_context = EGL_NO_CONTEXT;
EGLint share_config_id = 0;
int surface_width = 0, surface_height = 0;
int eye_width = 0, eye_height = 0;
bool surface_ready = false, paused = false;
bool frame_ready = false;
uint32_t frame_textures[2] = {0, 0};
CardboardHeadTracker *head_tracker = nullptr;
CardboardLensDistortion *lens = nullptr;
CardboardDistortionRenderer *distortion = nullptr;
float eye_projection[2][16] = {};
float eye_offset_mm[2] = {-32.f, 32.f};
jobject initialized_context = nullptr;

int64_t boot_time_ns() {
    struct timespec ts = {};
    clock_gettime(CLOCK_BOOTTIME, &ts);
    return static_cast<int64_t>(ts.tv_sec) * 1000000000LL + ts.tv_nsec;
}

void init_sdk(JNIEnv *env, jobject context) {
    JavaVM *vm = nullptr;
    if (!env || !context || env->GetJavaVM(&vm) != JNI_OK || !vm) return;
    std::lock_guard<std::mutex> lock(state_mutex);
    if (initialized_context) return;
    initialized_context = env->NewGlobalRef(context);
    Cardboard_initializeAndroid(vm, initialized_context);
    head_tracker = CardboardHeadTracker_create();
    if (head_tracker) {
        CardboardHeadTracker_setLowPassFilter(head_tracker, 30);
        if (!paused) CardboardHeadTracker_resume(head_tracker);
    }
    CLOG("Google Cardboard SDK initialized; head tracker %s", head_tracker ? "ready" : "unavailable");
}

void destroy_gl_objects() {
    if (distortion) CardboardDistortionRenderer_destroy(distortion);
    if (lens) CardboardLensDistortion_destroy(lens);
    distortion = nullptr;
    lens = nullptr;
}
} // namespace

EGLContext VPIX_CardboardWaitForShareContext(EGLDisplay *display, EGLint *config_id) {
    std::unique_lock<std::mutex> lock(state_mutex);
    const bool ready = state_changed.wait_for(lock, std::chrono::seconds(30), [] {
        return share_context != EGL_NO_CONTEXT;
    });
    if (!ready) {
        CLOG("timed out waiting for the Cardboard display EGL context");
        return EGL_NO_CONTEXT;
    }
    if (display) *display = share_display;
    if (config_id) *config_id = share_config_id;
    return share_context;
}

bool VPIX_CardboardWaitForSurface(int *width, int *height) {
    std::unique_lock<std::mutex> lock(state_mutex);
    const bool ready = state_changed.wait_for(lock, std::chrono::seconds(30), [] {
        return surface_ready && eye_width > 0 && eye_height > 0;
    });
    if (!ready) {
        CLOG("timed out waiting for the Cardboard display size");
        return false;
    }
    if (width) *width = eye_width;
    if (height) *height = eye_height;
    return true;
}

void VPIX_CardboardOnSurfaceCreated(JNIEnv *env, jobject activity) {
    init_sdk(env, activity);
    EGLDisplay dpy = eglGetCurrentDisplay();
    EGLContext ctx = eglGetCurrentContext();
    EGLint config_id = 0;
    if (dpy == EGL_NO_DISPLAY || ctx == EGL_NO_CONTEXT ||
        !eglQueryContext(dpy, ctx, EGL_CONFIG_ID, &config_id)) {
        CLOG("could not query GLSurfaceView EGL context/config (0x%X)", eglGetError());
        return;
    }
    {
        std::lock_guard<std::mutex> lock(state_mutex);
        share_display = dpy;
        share_context = ctx;
        share_config_id = config_id;
        surface_ready = false;
    }
    state_changed.notify_all();
    CLOG("Cardboard display context ready (config %d)", config_id);
}

void VPIX_CardboardOnSurfaceChanged(int width, int height) {
    if (width < 2 || height < 1) return;
    std::lock_guard<std::mutex> frame_lock(frame_mutex);
    destroy_gl_objects();

    uint8_t *params = nullptr;
    int params_size = 0;
    CardboardQrCode_getSavedDeviceParams(&params, &params_size);
    bool release_params = params != nullptr;
    if (!params_size) {
        if (release_params) CardboardQrCode_destroy(params);
        release_params = false;
        params = nullptr;
        CardboardQrCode_getCardboardV1DeviceParams(&params, &params_size);
        CLOG("using Cardboard V1 viewer profile fallback");
    }

    surface_width = width;
    surface_height = height;
    eye_width = width / 2;
    eye_height = height;
    if (params && params_size > 0)
        lens = CardboardLensDistortion_create(params, params_size, width, height);
    // Saved parameters are returned as an owned copy. The built-in Cardboard
    // V1 fallback is static SDK storage and must not be freed here.
    if (release_params) CardboardQrCode_destroy(params);

    if (lens) {
        const CardboardOpenGlEsDistortionRendererConfig config = {kGlTexture2D};
        distortion = CardboardOpenGlEs2DistortionRenderer_create(&config);
        CardboardMesh mesh = {};
        for (int eye = 0; eye < 2; ++eye) {
            const CardboardEye which = eye == 0 ? kLeft : kRight;
            CardboardLensDistortion_getProjectionMatrix(lens, which, 60.f, 400000.f, eye_projection[eye]);
            float eye_from_head[16] = {};
            CardboardLensDistortion_getEyeFromHeadMatrix(lens, which, eye_from_head);
            eye_offset_mm[eye] = -eye_from_head[12] * 1000.f;
            CardboardLensDistortion_getDistortionMesh(lens, which, &mesh);
            if (distortion) CardboardDistortionRenderer_setMesh(distortion, &mesh, which);
        }
    }

    {
        std::lock_guard<std::mutex> lock(state_mutex);
        surface_ready = lens && distortion;
    }
    state_changed.notify_all();
    CLOG("Cardboard surface %dx%d, eyes %dx%d (%s)", width, height, eye_width, eye_height,
         lens && distortion ? "ready" : "profile/render setup failed");
}

void VPIX_CardboardDrawToDisplay() {
    std::lock_guard<std::mutex> frame_lock(frame_mutex);
    if (!distortion || !frame_ready || !frame_textures[0] || !frame_textures[1]) {
        glClearColor(0.f, 0.f, 0.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        return;
    }
    CardboardEyeTextureDescription left = {frame_textures[0], 0.f, 1.f, 1.f, 0.f};
    CardboardEyeTextureDescription right = {frame_textures[1], 0.f, 1.f, 1.f, 0.f};
    CardboardDistortionRenderer_renderEyeToDisplay(distortion, 0, 0, 0, surface_width, surface_height, &left, &right);
    // The engine may overwrite the eye textures as soon as the frame lock is
    // released, so finish sampling them before that can happen.
    glFinish();
}

void VPIX_CardboardPause() {
    std::lock_guard<std::mutex> lock(state_mutex);
    paused = true;
    if (head_tracker) CardboardHeadTracker_pause(head_tracker);
}

void VPIX_CardboardResume() {
    std::lock_guard<std::mutex> lock(state_mutex);
    paused = false;
    if (head_tracker) CardboardHeadTracker_resume(head_tracker);
}

void VPIX_CardboardRecenter() {
    std::lock_guard<std::mutex> lock(state_mutex);
    if (head_tracker) CardboardHeadTracker_recenter(head_tracker);
}

bool VPIX_CardboardBeginEngineFrame() {
    frame_mutex.lock();
    {
        std::lock_guard<std::mutex> lock(state_mutex);
        if (paused || !surface_ready || !head_tracker) {
            frame_mutex.unlock();
            return false;
        }
    }
    frame_ready = false;
    frame_textures[0] = frame_textures[1] = 0;
    return true;
}

void VPIX_CardboardSetEyeTexture(int eye, uint32_t texture) {
    if (eye >= 0 && eye < 2) frame_textures[eye] = texture;
}

void VPIX_CardboardEndEngineFrame() {
    glFinish();
    frame_ready = frame_textures[0] != 0 && frame_textures[1] != 0;
    frame_mutex.unlock();
}

void VPIX_CardboardGetHeadPose(float position[3], float orientation_xyzw[4]) {
    std::lock_guard<std::mutex> lock(state_mutex);
    if (!head_tracker || paused) return;
    CardboardHeadTracker_getPose(head_tracker, boot_time_ns() + 11000000LL, kLandscapeLeft,
                                 position, orientation_xyzw);
}

void VPIX_CardboardGetEyeProjection(int eye, float projection[16]) {
    if (eye < 0 || eye > 1) return;
    std::lock_guard<std::mutex> lock(state_mutex);
    for (int i = 0; i < 16; ++i) projection[i] = eye_projection[eye][i];
}

float VPIX_CardboardGetEyeOffsetMm(int eye) {
    if (eye < 0 || eye > 1) return 0.f;
    std::lock_guard<std::mutex> lock(state_mutex);
    return eye_offset_mm[eye];
}

extern "C" JNIEXPORT void JNICALL
Java_com_gainesphere_mallvr_PhoneMallVRActivity_nativeInitCardboard(JNIEnv *env, jclass, jobject activity) {
    init_sdk(env, activity);
}
extern "C" JNIEXPORT void JNICALL
Java_com_gainesphere_mallvr_PhoneMallVRActivity_nativeCardboardSurfaceCreated(JNIEnv *env, jclass, jobject activity) {
    VPIX_CardboardOnSurfaceCreated(env, activity);
}
extern "C" JNIEXPORT void JNICALL
Java_com_gainesphere_mallvr_PhoneMallVRActivity_nativeCardboardSurfaceChanged(JNIEnv *, jclass, jint width, jint height) {
    VPIX_CardboardOnSurfaceChanged(width, height);
}
extern "C" JNIEXPORT void JNICALL
Java_com_gainesphere_mallvr_PhoneMallVRActivity_nativeCardboardDraw(JNIEnv *, jclass) {
    VPIX_CardboardDrawToDisplay();
}
extern "C" JNIEXPORT void JNICALL
Java_com_gainesphere_mallvr_PhoneMallVRActivity_nativeCardboardPause(JNIEnv *, jclass) { VPIX_CardboardPause(); }
extern "C" JNIEXPORT void JNICALL
Java_com_gainesphere_mallvr_PhoneMallVRActivity_nativeCardboardResume(JNIEnv *, jclass) { VPIX_CardboardResume(); }
extern "C" JNIEXPORT void JNICALL
Java_com_gainesphere_mallvr_CardboardSetupActivity_nativeInitCardboard(JNIEnv *env, jclass, jobject context) {
    init_sdk(env, context);
}
extern "C" JNIEXPORT void JNICALL
Java_com_gainesphere_mallvr_CardboardSetupActivity_nativeScanViewerQrCode(JNIEnv *, jclass) {
    CardboardQrCode_scanQrCodeAndSaveDeviceParams();
}

