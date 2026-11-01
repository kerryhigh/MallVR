/* Google Cardboard bridge used by the Android phone build. The DOSBox-X
   renderer still draws into its own left/right eye FBOs; this bridge tracks
   the phone, shares those textures with the display context, and applies the
   viewer's lens distortion. */
#pragma once

#include <EGL/egl.h>
#include <jni.h>
#include <stdint.h>

EGLContext VPIX_CardboardWaitForShareContext(EGLDisplay *display, EGLint *config_id);
bool VPIX_CardboardWaitForSurface(int *eye_width, int *eye_height);
void VPIX_CardboardOnSurfaceCreated(JNIEnv *env, jobject activity);
void VPIX_CardboardOnSurfaceChanged(int width, int height);
void VPIX_CardboardDrawToDisplay();
void VPIX_CardboardPause();
void VPIX_CardboardResume();
void VPIX_CardboardRecenter();
bool VPIX_CardboardBeginEngineFrame();
void VPIX_CardboardSetEyeTexture(int eye, uint32_t texture);
void VPIX_CardboardEndEngineFrame();
void VPIX_CardboardGetHeadPose(float position[3], float orientation_xyzw[4]);
void VPIX_CardboardGetEyeProjection(int eye, float projection[16]);
float VPIX_CardboardGetEyeOffsetMm(int eye);

