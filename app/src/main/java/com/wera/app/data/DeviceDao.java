package com.wera.app.data;

import androidx.lifecycle.LiveData;
import androidx.room.Dao;
import androidx.room.Delete;
import androidx.room.Insert;
import androidx.room.OnConflictStrategy;
import androidx.room.Query;

import java.util.List;

@Dao
public interface DeviceDao {
    @Query("SELECT * FROM devices")
    LiveData<List<Device>> getAll();

    @Insert(onConflict = OnConflictStrategy.REPLACE)
    void insert(Device device);

    @Delete
    void delete(Device device);
}
