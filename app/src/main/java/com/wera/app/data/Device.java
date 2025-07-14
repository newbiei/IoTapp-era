package com.wera.app.data;

import androidx.room.Entity;
import androidx.room.PrimaryKey;
import androidx.annotation.NonNull;

@Entity(tableName = "devices")
public class Device {
    @PrimaryKey
    @NonNull

    public String id;
    public String name;
    public String status;
    public int battery;
    public String ip;

    public Device(String id, String name, String status, int battery, String ip) {
        this.id = id;
        this.name = name;
        this.status = status;
        this.battery = battery;
        this.ip = ip;
    }
}
