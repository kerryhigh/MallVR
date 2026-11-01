package com.gainesphere.mallvr;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.provider.Settings;
import android.view.Gravity;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;
import java.util.List;

/** One-game launcher for the Google Cardboard phone build. */
public class PhoneLauncherActivity extends Activity {
    private static final int BG = 0xff15111b, PANEL = 0xff261f31, GOLD = 0xffffc94d, CREAM = 0xfff5efdf;
    private TextView status;
    private Button startButton, fileButton;
    private MallVR.Game pacman;
    private boolean refreshing;

    @Override protected void onCreate(Bundle state) {
        super.onCreate(state);
        MallVR.PLATFORM = "cardboard";
        getWindow().setStatusBarColor(BG);
        getWindow().setNavigationBarColor(BG);

        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setBackgroundColor(BG);
        root.setPadding(dp(22), dp(18), dp(22), dp(18));
        root.addView(label("MALL VR  /  CARDBOARD", 16, GOLD, true));
        root.addView(label("PAC-MAN VR", 34, CREAM, true));
        root.addView(label("Single player · Android phone · Google Cardboard", 17, CREAM, false));

        LinearLayout actions = new LinearLayout(this);
        actions.setOrientation(LinearLayout.HORIZONTAL);
        actions.addView(button("SET UP VIEWER QR", v -> startActivity(new Intent(this, CardboardSetupActivity.class))));
        actions.addView(button("PAIR XBOX PAD", v -> openBluetoothSettings()));
        root.addView(actions);

        root.addView(label("Controls: Left stick strafe/back · Right stick turn · RT forward · LT back\n"
            + "Xbox Menu: Start game · View: Recenter headset\n"
            + "Hold B + Menu for one second to return to this screen.", 16, CREAM, false));

        startButton = button("START PAC-MAN VR", v -> startPacman());
        startButton.setEnabled(false);
        root.addView(startButton);
        fileButton = button("ALLOW GAME FILE ACCESS", v -> requestFileAccess());
        fileButton.setVisibility(View.GONE);
        root.addView(fileButton);
        status = label("Checking Pac-Man files…", 16, CREAM, false);
        status.setPadding(0, dp(12), 0, dp(0));
        root.addView(status);

        ScrollView scroll = new ScrollView(this);
        LinearLayout detail = new LinearLayout(this);
        detail.setOrientation(LinearLayout.VERTICAL);
        detail.setGravity(Gravity.CENTER_VERTICAL);
        detail.setBackgroundColor(PANEL);
        detail.setPadding(dp(16), dp(12), dp(16), dp(12));
        detail.addView(label("Viewer profile", 17, GOLD, true));
        detail.addView(label("Scan the QR code printed on your Cardboard viewer for the best lens fit. "
            + "If you skip setup, the built-in Cardboard V1 profile is used.", 15, CREAM, false));
        scroll.addView(detail);
        LinearLayout.LayoutParams scrollParams = new LinearLayout.LayoutParams(-1, 0, 1f);
        scrollParams.topMargin = dp(12);
        root.addView(scroll, scrollParams);
        setContentView(root);

        try {
            List<MallVR.Game> games = MallVR.load(this);
            for (MallVR.Game game : games) if ("PAC".equals(game.code)) pacman = game;
        } catch (Exception e) {
            status.setText("Pac-Man setup data is missing from the APK: " + e.getMessage());
        }
    }

    @Override protected void onResume() { super.onResume(); refresh(); }

    private void refresh() {
        if (refreshing || pacman == null) return;
        if (!filesOk()) {
            fileButton.setVisibility(View.VISIBLE);
            startButton.setEnabled(false);
            status.setText("Allow file access so MallVR can install the Pac-Man game files and write its save data.");
            return;
        }
        fileButton.setVisibility(View.GONE);
        refreshing = true;
        status.setText("Installing engine files and checking Pac-Man…");
        new Thread(() -> {
            MallVR.installEngine(this);
            MallVR.find(java.util.Collections.singletonList(pacman));
            runOnUiThread(() -> {
                refreshing = false;
                boolean ready = pacman.state == 1;
                startButton.setEnabled(ready);
                status.setText(ready
                    ? "Pac-Man VR is ready. Pair the Xbox controller before putting the phone in the viewer."
                    : pacman.stateText() + (pacman.why.isEmpty() ? "" : " — " + pacman.why));
            });
        }, "MallVR Pac-Man setup").start();
    }

    private boolean filesOk() {
        if (android.os.Build.VERSION.SDK_INT >= 30) return Environment.isExternalStorageManager();
        return checkSelfPermission(android.Manifest.permission.WRITE_EXTERNAL_STORAGE)
            == android.content.pm.PackageManager.PERMISSION_GRANTED;
    }

    private void requestFileAccess() {
        if (android.os.Build.VERSION.SDK_INT < 30) {
            requestPermissions(new String[] { android.Manifest.permission.READ_EXTERNAL_STORAGE,
                android.Manifest.permission.WRITE_EXTERNAL_STORAGE }, 1);
            return;
        }
        try {
            startActivity(new Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION,
                Uri.parse("package:" + getPackageName())));
        } catch (Exception e) {
            startActivity(new Intent(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION));
        }
    }

    @Override public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] results) {
        super.onRequestPermissionsResult(requestCode, permissions, results);
        refresh();
    }

    private void openBluetoothSettings() {
        try { startActivity(new Intent(Settings.ACTION_BLUETOOTH_SETTINGS)); }
        catch (Exception e) { status.setText("Open Android Settings → Bluetooth and pair your Xbox controller."); }
    }

    private void startPacman() {
        if (pacman == null || pacman.state != 1) return;
        try {
            MallVR.Plan plan = MallVR.plan(this, pacman);
            Intent game = new Intent(this, PhoneMallVRActivity.class);
            game.putExtra("conf", plan.conf);
            game.putExtra("env", plan.env);
            game.putExtra("game", pacman.code);
            startActivity(game);
        } catch (Exception e) {
            MallVR.log("Cardboard Pac-Man launch failed: " + e);
            status.setText("Couldn't start Pac-Man VR: " + e.getMessage());
        }
    }

    private TextView label(String text, int size, int color, boolean bold) {
        TextView v = new TextView(this);
        v.setText(text);
        v.setTextSize(size);
        v.setTextColor(color);
        if (bold) v.setTypeface(android.graphics.Typeface.DEFAULT, android.graphics.Typeface.BOLD);
        return v;
    }

    private Button button(String text, View.OnClickListener click) {
        Button b = new Button(this);
        b.setText(text);
        b.setTextSize(14);
        b.setAllCaps(false);
        b.setOnClickListener(click);
        LinearLayout.LayoutParams p = new LinearLayout.LayoutParams(-2, -2);
        p.setMargins(0, dp(8), dp(10), dp(8));
        b.setLayoutParams(p);
        return b;
    }

    private int dp(int value) { return (int)(value * getResources().getDisplayMetrics().density + 0.5f); }
}
