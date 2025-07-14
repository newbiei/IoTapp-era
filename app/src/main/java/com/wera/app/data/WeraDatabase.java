package com.wera.app.data;

import android.content.Context;

import androidx.room.Database;
import androidx.room.Room;
import androidx.room.RoomDatabase;

@Database(entities = {Device.class}, version = 1, exportSchema = false)
public abstract class WeraDatabase extends RoomDatabase {

    private static volatile WeraDatabase INSTANCE;

    public abstract DeviceDao deviceDao();

    public static synchronized WeraDatabase getInstance(Context context) {
        if (INSTANCE == null) {
            INSTANCE = Room.databaseBuilder(
                            context.getApplicationContext(),
                            WeraDatabase.class,
                            "wera_db"
                    )
                    .fallbackToDestructiveMigration()
                    .build();
        }
        return INSTANCE;
    }
}
