package com.hgs.biapp;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.NotificationCompat;

import android.annotation.SuppressLint;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.os.Build;
import android.os.Bundle;
import android.view.View;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.widget.TextView;

import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.net.URLConnection;
import java.util.Timer;
import java.util.TimerTask;

public class MainActivity extends AppCompatActivity {

    WebView webView;
    WebView orderView;
    TextView tvState;
    HttpURLConnection conn = null;
    String channelId = "detected";
    NotificationManager notificationManager = null;
    boolean notified = false;
    String[] stateMsg = new String[] {"열림", "잠김", "도난"};

    TimerTask task = new TimerTask() {
        @SuppressLint("SetTextI18n")
        @Override
        public void run() {
            String str = "";
            try {
                URL url = new URL("http://<DEVICE_IP>/");
                URLConnection urlConnection = url.openConnection();
                InputStream stream = urlConnection.getInputStream();

                int i;
                while ((i = stream.read()) != -1) {
                    str += (char)i;
                }

                String[] comps = str.split(",");
                int state = Integer.parseInt(comps[2].trim());
                tvState.setText("현재 상태: " + stateMsg[state]);
                if (state == 2) { //detected
                    if (!notified) sendPush();
                    notified = true;
                }
                else {
                    notified = false;
                }
            } catch(Exception ex) {
                System.out.println(ex.getMessage());
            }
        }
    };
    Timer timer;

    @SuppressLint("SetJavaScriptEnabled")
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        timer = new Timer("Timer");
        timer.schedule(task, 5000, 5000);

        createNotificationChannel();

        webView = findViewById(R.id.webView);
        orderView = findViewById(R.id.wbOrder);
        tvState = findViewById(R.id.tvState);

        WebSettings settings = webView.getSettings();
        settings.setJavaScriptEnabled(true);
        webView.loadUrl("http://<Web_Server_IP>");
    }

    private void sendPush() {
        NotificationCompat.Builder builder = new NotificationCompat.Builder(this, channelId)
                .setSmallIcon(R.drawable.ic_launcher_foreground)
                .setContentTitle("도난 감지")
                .setContentText("도난이 감지되었습니다.")
                .setPriority(NotificationCompat.PRIORITY_MAX);

        notificationManager.notify(45, builder.build());
    }

    private void createNotificationChannel() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            CharSequence name = getString(R.string.channel_name);
            String description = getString(R.string.channel_description);
            int importance = NotificationManager.IMPORTANCE_DEFAULT;
            NotificationChannel channel = new NotificationChannel(channelId, name, importance);
            channel.setDescription(description);
            notificationManager = getSystemService(NotificationManager.class);
            notificationManager.createNotificationChannel(channel);
        }
    }

    public void btnOpen_Click(View view) {
        orderView.loadUrl("http://<DEVICE_IP>/O");
        tvState.setText("현재 상태: 열림");
    }

    public void btnClose_Click(View view) {
        orderView.loadUrl("http://<DEVICE_IP>/C");
        tvState.setText("현재 상태: 잠김");
    }
}