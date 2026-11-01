package com.gainesphere.mallvr;

import android.app.Activity;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;

/** Scans and saves this viewer's Google Cardboard QR profile. */
public class CardboardSetupActivity extends Activity {
    static { System.loadLibrary("main"); }

    @Override protected void onCreate(Bundle state) {
        super.onCreate(state);
        nativeInitCardboard(this);
        getWindow().setStatusBarColor(0xff15111b);
        getWindow().setNavigationBarColor(0xff15111b);
        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setPadding(dp(24), dp(24), dp(24), dp(24));
        root.setBackgroundColor(0xff15111b);
        TextView title = new TextView(this);
        title.setText("CARDBOARD VIEWER SETUP");
        title.setTextSize(24);
        title.setTextColor(0xffffc94d);
        root.addView(title);
        TextView info = new TextView(this);
        info.setText("Scan the QR code printed on your viewer. The profile is saved by Google Cardboard and used for lens correction.");
        info.setTextSize(17);
        info.setTextColor(0xfff5efdf);
        info.setPadding(0, dp(16), 0, dp(16));
        root.addView(info);
        Button scan = new Button(this);
        scan.setText("SCAN VIEWER QR CODE");
        scan.setOnClickListener(v -> nativeScanViewerQrCode());
        root.addView(scan);
        Button done = new Button(this);
        done.setText("DONE");
        done.setOnClickListener(v -> finish());
        root.addView(done);
        setContentView(root);
    }

    private int dp(int v) { return (int)(v * getResources().getDisplayMetrics().density + 0.5f); }
    private static native void nativeInitCardboard(Object context);
    private static native void nativeScanViewerQrCode();
}
