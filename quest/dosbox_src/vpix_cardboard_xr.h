/* Cardboard's phone tracker, projection, eye submission and Bluetooth
   controller input. This header is included in vpix_render.cpp's anonymous
   namespace beside the OpenVR helpers it reuses. */
#pragma once

#include "vpix_cardboard.h"
#include <SDL_gamecontroller.h>
#include <SDL_events.h>
#include <algorithm>

extern "C" int SDL_InitSubSystem(Uint32 flags);
static constexpr Uint32 VPIX_SDL_INIT_GAMECONTROLLER = 0x00002000u;

struct XrPoses {
    bool head_ok = false;
    bool hand_ok[2] = {false, false};
    VPIX_Pose head, eye[2], hand[2];
};

static struct CardboardVrState {
    bool on = false, frame_open = false;
    SDL_GameController *pad = nullptr;
} xr;

static SDL_GameController *xr_controller() {
    if (xr.pad && !SDL_GameControllerGetAttached(xr.pad)) {
        SDL_GameControllerClose(xr.pad);
        xr.pad = nullptr;
    }
    if (!xr.pad) {
        for (int i = 0; i < SDL_NumJoysticks(); ++i) {
            if (SDL_IsGameController(i)) {
                xr.pad = SDL_GameControllerOpen(i);
                if (xr.pad) {
                    rl("Cardboard Xbox/game controller connected: %s", SDL_GameControllerName(xr.pad));
                    break;
                }
            }
        }
    }
    return xr.pad;
}

static void xr_controller_init() {
    SDL_InitSubSystem(VPIX_SDL_INIT_GAMECONTROLLER);
    SDL_GameControllerEventState(SDL_ENABLE);
    xr_controller();
}

static float xr_axis(SDL_GameController *pad, SDL_GameControllerAxis axis, bool invert = false) {
    if (!pad) return 0.f;
    float v = static_cast<float>(SDL_GameControllerGetAxis(pad, axis)) / 32767.f;
    v = (std::max)(-1.f, (std::min)(1.f, v));
    return invert ? -v : v;
}

static bool xr_button(SDL_GameController *pad, SDL_GameControllerButton button) {
    return pad && SDL_GameControllerGetButton(pad, button) != 0;
}

static bool xr_init(HDC, HGLRC, uint32_t &ew, uint32_t &eh) {
    const char *enabled = getenv("VPIX_XR");
    if (!(enabled && atoi(enabled))) return false;
    int w = 0, h = 0;
    if (!VPIX_CardboardWaitForSurface(&w, &h)) return false;
    ew = static_cast<uint32_t>(w);
    eh = static_cast<uint32_t>(h);
    xr_controller_init();
    xr.on = true;
    rl("Google Cardboard ready: eye buffers %ux%u; Xbox Menu starts, View recenters", ew, eh);
    return true;
}

static bool xr_active() { return xr.on; }

static void xr_rotate_quaternion(const float q[4], const float v[3], float out[3]) {
    // Phone testing showed Cardboard's yaw and pitch signs opposite the VPIX
    // pose convention; conjugate the unit quaternion to restore the axes.
    const float x = -q[0], y = -q[1], z = -q[2], w = q[3];
    const float xx = x * x, yy = y * y, zz = z * z;
    const float xy = x * y, xz = x * z, yz = y * z;
    const float wx = w * x, wy = w * y, wz = w * z;
    out[0] = (1.f - 2.f * (yy + zz)) * v[0] + 2.f * (xy - wz) * v[1] + 2.f * (xz + wy) * v[2];
    out[1] = 2.f * (xy + wz) * v[0] + (1.f - 2.f * (xx + zz)) * v[1] + 2.f * (yz - wx) * v[2];
    out[2] = 2.f * (xz - wy) * v[0] + 2.f * (yz + wx) * v[1] + (1.f - 2.f * (xx + yy)) * v[2];
}

static void xr_cardboard_to_vpix(const float c[3], float v[3]) {
    // Cardboard uses +X right, +Y up and -Z forward; MallVR uses +X right,
    // +Y forward and +Z up, in millimetres.
    v[0] = c[0];
    v[1] = -c[2];
    v[2] = c[1];
}

static bool xr_begin(XrPoses &o) {
    if (!xr.on || !VPIX_CardboardBeginEngineFrame()) return false;
    xr.frame_open = true;
    float pos[3] = {0.f, 0.f, 0.f}, q[4] = {0.f, 0.f, 0.f, 1.f};
    VPIX_CardboardGetHeadPose(pos, q);
    const float right[3] = {1.f, 0.f, 0.f}, up[3] = {0.f, 1.f, 0.f}, forward[3] = {0.f, 0.f, -1.f};
    float cr[3], cu[3], cf[3];
    xr_rotate_quaternion(q, right, cr);
    xr_rotate_quaternion(q, up, cu);
    xr_rotate_quaternion(q, forward, cf);
    o.head.valid = true;
    xr_cardboard_to_vpix(cr, o.head.R[0]);
    xr_cardboard_to_vpix(cf, o.head.R[1]);
    xr_cardboard_to_vpix(cu, o.head.R[2]);
    float cp[3]; xr_cardboard_to_vpix(pos, cp);
    for (int i = 0; i < 3; ++i) o.head.p[i] = cp[i] * 1000.f;
    for (int row = 0; row < 3; ++row) rotz(yaw_off, o.head.R[row]);
    rotz(yaw_off, o.head.p);
    o.head_ok = true;
    for (int eye = 0; eye < 2; ++eye) {
        o.eye[eye] = o.head;
        const float offset = VPIX_CardboardGetEyeOffsetMm(eye);
        for (int i = 0; i < 3; ++i) o.eye[eye].p[i] += o.head.R[0][i] * offset;
    }
    return true;
}

static void xr_proj(int eye, float projection[16]) {
    VPIX_CardboardGetEyeProjection(eye, projection);
    rev_proj(projection);
}

static void xr_submit(int eye, GLuint eye_texture) {
    if (xr.frame_open) VPIX_CardboardSetEyeTexture(eye, eye_texture);
}

static void xr_end() {
    if (!xr.frame_open) return;
    VPIX_CardboardEndEngineFrame();
    xr.frame_open = false;
}

static void xr_acts_read(VPIX_Acts &a) {
    SDL_GameController *pad = xr_controller();
    if (!pad) return;
    SDL_GameControllerUpdate();
    a.lmove[0] = xr_axis(pad, SDL_CONTROLLER_AXIS_LEFTX);
    a.lmove[1] = xr_axis(pad, SDL_CONTROLLER_AXIS_LEFTY, true);
    a.rmove[0] = xr_axis(pad, SDL_CONTROLLER_AXIS_RIGHTX);
    a.rmove[1] = xr_axis(pad, SDL_CONTROLLER_AXIS_RIGHTY, true);
    const float rt = (std::max)(0.f, xr_axis(pad, SDL_CONTROLLER_AXIS_TRIGGERRIGHT));
    const float lt = (std::max)(0.f, xr_axis(pad, SDL_CONTROLLER_AXIS_TRIGGERLEFT));
    a.thr = rt < 0.08f ? 0.f : rt;
    a.brk = lt < 0.08f ? 0.f : lt;
    a.fire = a.thr > 0.15f;        // Pac-Man: right trigger walks forward.
    a.walk = xr_button(pad, SDL_CONTROLLER_BUTTON_A);
    a.crouch = xr_button(pad, SDL_CONTROLLER_BUTTON_B);
    a.menu = xr_button(pad, SDL_CONTROLLER_BUTTON_START); // Xbox Menu: coin + start game.
    a.top = xr_button(pad, SDL_CONTROLLER_BUTTON_Y);
    static bool view_was_down = false;
    const bool view_down = xr_button(pad, SDL_CONTROLLER_BUTTON_BACK);
    if (view_down && !view_was_down) VPIX_CardboardRecenter();
    view_was_down = view_down;
}

static void xr_haptic(int, float seconds, float amplitude) {
    SDL_GameController *pad = xr_controller();
    if (!pad) return;
    const Uint32 ms = static_cast<Uint32>((std::max)(0.f, seconds) * 1000.f);
    const Uint16 strength = static_cast<Uint16>((std::max)(0.f, (std::min)(1.f, amplitude)) * 65535.f);
    SDL_GameControllerRumble(pad, strength, strength, ms);
}

static void xr_shutdown() {
    xr.on = false;
    xr.frame_open = false;
    if (xr.pad) SDL_GameControllerClose(xr.pad);
    xr.pad = nullptr;
}

