package com.gainesphere.mallvr;

import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.os.Bundle;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.opengl.GLSurfaceView;
import org.libsdl.app.SDLActivity;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.util.HashMap;
import java.util.Map;
import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.opengles.GL10;

/** SDL/DOSBox game activity with a Google Cardboard distortion surface above it. */
public class PhoneMallVRActivity extends SDLActivity {
    private String conf = "";
    private DatagramSocket menuSocket;
    private volatile DatagramPacket engine;
    private GLSurfaceView cardboardView;
    private boolean leaving;

    @Override protected void onCreate(Bundle state) {
        Intent launch = getIntent();
        conf = launch.getStringExtra("conf");
        @SuppressWarnings("unchecked") HashMap<String, String> env = (HashMap<String, String>)launch.getSerializableExtra("env");
        try {
            if (env != null) for (Map.Entry<String, String> e : env.entrySet())
                android.system.Os.setenv(e.getKey(), e.getValue(), true);
            menuSocket = new DatagramSocket(0, InetAddress.getByName("127.0.0.1"));
            android.system.Os.setenv("VPIX_MENU_PORT", String.valueOf(menuSocket.getLocalPort()), true);
            Thread listener = new Thread(this::listenForMenu, "MallVR Cardboard menu");
            listener.setDaemon(true);
            listener.start();
        } catch (Exception e) {
            MallVR.log("Cardboard game setup failed: " + e);
        }

        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        getWindow().getDecorView().setSystemUiVisibility(
            View.SYSTEM_UI_FLAG_FULLSCREEN | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                | View.SYSTEM_UI_FLAG_LAYOUT_STABLE | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN);
        super.onCreate(state);
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
        nativeInitCardboard(this);
        installCardboardSurface();
    }

    /** SDL's default FULL_USER orientation would rotate the Cardboard view. */
    @Override public void setOrientationBis(int width, int height, boolean resizable, String hint) {
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
    }

    private void installCardboardSurface() {
        cardboardView = new GLSurfaceView(this);
        cardboardView.setEGLContextClientVersion(3);
        cardboardView.setEGLConfigChooser(new CardboardConfigChooser());
        cardboardView.setPreserveEGLContextOnPause(true);
        cardboardView.setZOrderMediaOverlay(true);
        cardboardView.setRenderer(new GLSurfaceView.Renderer() {
            @Override public void onSurfaceCreated(GL10 gl, EGLConfig config) {
                nativeCardboardSurfaceCreated(PhoneMallVRActivity.this);
            }
            @Override public void onSurfaceChanged(GL10 gl, int width, int height) {
                nativeCardboardSurfaceChanged(width, height);
            }
            @Override public void onDrawFrame(GL10 gl) { nativeCardboardDraw(); }
        });
        cardboardView.setRenderMode(GLSurfaceView.RENDERMODE_CONTINUOUSLY);
        if (mLayout != null) {
            mLayout.addView(cardboardView, new ViewGroup.LayoutParams(-1, -1));
            mLayout.bringChildToFront(cardboardView);
        } else {
            MallVR.log("Cardboard display surface could not attach to SDL layout");
        }
    }

    private static final class CardboardConfigChooser implements GLSurfaceView.EGLConfigChooser {
        @Override public EGLConfig chooseConfig(EGL10 egl, EGLDisplay display) {
            int[] attrs = {
                EGL10.EGL_RED_SIZE, 8, EGL10.EGL_GREEN_SIZE, 8, EGL10.EGL_BLUE_SIZE, 8, EGL10.EGL_ALPHA_SIZE, 8,
                EGL10.EGL_DEPTH_SIZE, 24,
                EGL10.EGL_RENDERABLE_TYPE, 0x40, // EGL_OPENGL_ES3_BIT_KHR
                EGL10.EGL_SURFACE_TYPE, EGL10.EGL_WINDOW_BIT | EGL10.EGL_PBUFFER_BIT,
                EGL10.EGL_NONE
            };
            EGLConfig[] configs = new EGLConfig[16];
            int[] count = new int[1];
            if (!egl.eglChooseConfig(display, attrs, configs, configs.length, count) || count[0] == 0)
                throw new IllegalArgumentException("No shared Cardboard EGL window/pbuffer config");
            return configs[0];
        }
    }

    private void listenForMenu() {
        byte[] bytes = new byte[128];
        while (menuSocket != null && !menuSocket.isClosed()) {
            try {
                DatagramPacket packet = new DatagramPacket(bytes, bytes.length);
                menuSocket.receive(packet);
                engine = new DatagramPacket(new byte[0], 0, packet.getAddress(), packet.getPort());
                String line = new String(packet.getData(), 0, packet.getLength(), "US-ASCII");
                if (line.startsWith("MENU")) runOnUiThread(this::returnToLauncher);
            } catch (Exception e) { return; }
        }
    }

    private void returnToLauncher() {
        if (leaving) return;
        leaving = true;
        DatagramPacket peer = engine;
        if (peer != null && menuSocket != null) {
            try {
                byte[] quit = "QUIT".getBytes("US-ASCII");
                menuSocket.send(new DatagramPacket(quit, quit.length, peer.getAddress(), peer.getPort()));
            } catch (Exception ignored) { }
        }
        Intent home = new Intent(this, PhoneLauncherActivity.class);
        home.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_REORDER_TO_FRONT);
        startActivity(home);
        finish();
    }

    @Override public void onBackPressed() { returnToLauncher(); }
    @Override protected String[] getLibraries() { return new String[] { "c++_shared", "GfxPluginCardboard", "SDL2", "main" }; }
    @Override protected String[] getArguments() { return new String[] { "-conf", conf, "-nopromptfolder" }; }

    @Override protected void onPause() {
        nativeCardboardPause();
        if (cardboardView != null) cardboardView.onPause();
        super.onPause();
    }

    @Override protected void onResume() {
        super.onResume();
        getWindow().getDecorView().setSystemUiVisibility(
            View.SYSTEM_UI_FLAG_FULLSCREEN | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                | View.SYSTEM_UI_FLAG_LAYOUT_STABLE | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN);
        nativeCardboardResume();
        if (cardboardView != null) cardboardView.onResume();
    }

    @Override protected void onDestroy() {
        if (menuSocket != null) menuSocket.close();
        MallVR.log("Cardboard Pac-Man activity closed");
        super.onDestroy();
        if (!isChangingConfigurations()) android.os.Process.killProcess(android.os.Process.myPid());
    }

    private static native void nativeInitCardboard(Object activity);
    private static native void nativeCardboardSurfaceCreated(Object activity);
    private static native void nativeCardboardSurfaceChanged(int width, int height);
    private static native void nativeCardboardDraw();
    private static native void nativeCardboardPause();
    private static native void nativeCardboardResume();
}
