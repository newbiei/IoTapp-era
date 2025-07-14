package com.wera.app.adapter;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;
import com.wera.app.R;
import com.wera.app.model.NotificationItem;

import java.util.List;

public class NotificationAdapter extends RecyclerView.Adapter<NotificationAdapter.NotifViewHolder> {

    private final List<NotificationItem> notificationList;

    public NotificationAdapter(List<NotificationItem> notificationList) {
        this.notificationList = notificationList;
    }

    @NonNull
    @Override
    public NotifViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
        View view = LayoutInflater.from(parent.getContext())
                .inflate(R.layout.item_notification_card, parent, false);
        return new NotifViewHolder(view);
    }

    @Override
    public void onBindViewHolder(@NonNull NotifViewHolder holder, int position) {
        NotificationItem item = notificationList.get(position);

        holder.tvType.setText(item.getTypeText());
        holder.tvMessage.setText(item.getMessage());

        // Set icon sesuai tipe notifikasi
        switch (item.getType()) {
            case ALERT:
                holder.icon.setImageResource(R.drawable.ic_alert);
                break;
            case ERROR:
                holder.icon.setImageResource(R.drawable.ic_error);
                break;
            case DEVICE:
                holder.icon.setImageResource(R.drawable.ic_check);
                break;
        }
    }

    @Override
    public int getItemCount() {
        return notificationList.size();
    }

    public static class NotifViewHolder extends RecyclerView.ViewHolder {
        ImageView icon;
        TextView tvType, tvMessage;

        public NotifViewHolder(@NonNull View itemView) {
            super(itemView);
            icon = itemView.findViewById(R.id.imgNotifType);
            tvType = itemView.findViewById(R.id.tvNotifType);
            tvMessage = itemView.findViewById(R.id.tvNotifMessage);
        }
    }
}
