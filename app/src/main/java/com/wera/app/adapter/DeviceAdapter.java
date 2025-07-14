package com.wera.app.adapter;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import com.wera.app.R;
import com.wera.app.data.Device;

import java.util.List;

public class DeviceAdapter extends RecyclerView.Adapter<DeviceAdapter.DeviceViewHolder> {

    private List<Device> deviceList;
    private final Context context;
    private final OnDeviceActionListener listener;

    public interface OnDeviceActionListener {
        void onControlClick(Device device);
        void onSettingsClick(Device device);
    }

    public DeviceAdapter(Context context, List<Device> deviceList, OnDeviceActionListener listener) {
        this.context = context;
        this.deviceList = deviceList;
        this.listener = listener;
    }

    public void setDeviceList(List<Device> deviceList) {
        this.deviceList = deviceList;
        notifyDataSetChanged();
    }

    @NonNull
    @Override
    public DeviceViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
        View view = LayoutInflater.from(context).inflate(R.layout.item_device_card, parent, false);
        return new DeviceViewHolder(view);
    }

    public void updateData(List<Device> newList) {
        this.deviceList = newList;
        notifyDataSetChanged();
    }

    @Override
    public void onBindViewHolder(@NonNull DeviceViewHolder holder, int position) {
        Device device = deviceList.get(position);
        holder.tvDeviceName.setText(device.name);
        holder.tvDeviceId.setText("ID: " + device.id);
        holder.tvStatus.setText("Status: " + device.status);
        holder.tvBattery.setText(device.battery + "%");

        // Ubah ikon baterai sesuai persentase
        holder.imgBattery.setImageResource(getBatteryIcon(device.battery));

        holder.btnControl.setOnClickListener(v -> listener.onControlClick(device));
        holder.btnSettings.setOnClickListener(v -> listener.onSettingsClick(device));
    }

    @Override
    public int getItemCount() {
        return deviceList != null ? deviceList.size() : 0;
    }

    private int getBatteryIcon(int battery) {
        if (battery >= 90) return R.drawable.ic_battery_full;
        else if (battery >= 75) return R.drawable.ic_battery_75;
        else if (battery >= 50) return R.drawable.ic_battery_50;
        else if (battery >= 25) return R.drawable.ic_battery_25;
        else return R.drawable.ic_battery_low;
    }

    public static class DeviceViewHolder extends RecyclerView.ViewHolder {
        TextView tvDeviceName, tvDeviceId, tvStatus, tvBattery;
        ImageView imgBattery;
        ImageButton btnControl, btnSettings;

        public DeviceViewHolder(@NonNull View itemView) {
            super(itemView);
            tvDeviceName = itemView.findViewById(R.id.tvDeviceName);
            tvDeviceId = itemView.findViewById(R.id.tvDeviceId);
            tvStatus = itemView.findViewById(R.id.tvDeviceStatus);
            tvBattery = itemView.findViewById(R.id.tvDeviceBattery);
            imgBattery = itemView.findViewById(R.id.imgBatteryIcon);
            btnControl = itemView.findViewById(R.id.btnControl);
            btnSettings = itemView.findViewById(R.id.btnSettings);
        }
    }
}
