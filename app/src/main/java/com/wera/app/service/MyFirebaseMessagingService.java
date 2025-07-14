package com.wera.app.service;

import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.content.Context;
import android.content.SharedPreferences;
import android.os.Build;

import androidx.annotation.NonNull;
import androidx.core.app.NotificationCompat;
import androidx.core.content.ContextCompat;

import com.google.firebase.messaging.FirebaseMessagingService;
import com.google.firebase.messaging.RemoteMessage;
import com.wera.app.R;

public class MyFirebaseMessagingService extends FirebaseMessagingService {

    @Override
    public void onMessageReceived(@NonNull RemoteMessage remoteMessage) {
        // Cek preferensi notifikasi (dari checkbox)
        SharedPreferences prefs = getSharedPreferences("wera_settings", MODE_PRIVATE);
        boolean notifAllowed = prefs.getBoolean("notif_enabled", true);

        if (!notifAllowed) return; // Kalau notifikasi dimatikan, skip

        String title = "WERA Notification";
        String message = "Ada notifikasi baru dari perangkat";

        if (remoteMessage.getNotification() != null) {
            title = remoteMessage.getNotification().getTitle();
            message = remoteMessage.getNotification().getBody();
        }

        sendNotification(title, message);
    }

    private void sendNotification(String title, String message) {
        String channelId = "wera_channel";
        NotificationManager manager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);

        // Buat channel kalau belum ada (Android 8+)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            NotificationChannel channel = new NotificationChannel(
                    channelId,
                    "WERA Notification Channel",
                    NotificationManager.IMPORTANCE_HIGH
            );
            manager.createNotificationChannel(channel);
        }

        NotificationCompat.Builder builder = new NotificationCompat.Builder(this, channelId)
                .setSmallIcon(R.drawable.ic_notification) // icon kecil (wajib)
                .setContentTitle(title)
                .setContentText(message)
                .setColor(ContextCompat.getColor(this, R.color.yellow)) // warna aksen
                .setPriority(NotificationCompat.PRIORITY_HIGH)
                .setAutoCancel(true);

        manager.notify((int) System.currentTimeMillis(), builder.build());
    }
}
