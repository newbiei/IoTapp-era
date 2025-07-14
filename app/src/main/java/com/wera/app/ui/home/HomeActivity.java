package com.wera.app.ui.home;

import android.content.SharedPreferences;
import android.content.res.Configuration;
import android.os.Bundle;
import android.view.View;
import android.widget.ImageButton;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.app.AppCompatDelegate;
import android.Manifest;
import android.content.pm.PackageManager;
import android.os.Build;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import androidx.lifecycle.ViewModelProvider;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import androidx.fragment.app.FragmentTransaction;

import com.google.android.material.floatingactionbutton.FloatingActionButton;
import com.wera.app.R;
import com.wera.app.adapter.DeviceAdapter;
import com.wera.app.data.Device;
import com.wera.app.ui.adddevice.AddDeviceFragment;
import com.wera.app.ui.notifications.NotificationFragment;
import com.wera.app.ui.settings.SettingsFragment;
import com.wera.app.viewmodel.DeviceViewModel;

import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

public class HomeActivity extends AppCompatActivity {
    private DeviceViewModel deviceViewModel;
    private RecyclerView rvDeviceList;
    private TextView tvGreeting;
    private FloatingActionButton fabStartAll;
    private DeviceAdapter deviceAdapter;
    private List<Device> dummyDevices;

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions,
                                           @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == 100) {
            if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                Toast.makeText(this, "Notifikasi diizinkan 🎉", Toast.LENGTH_SHORT).show();
            } else {
                Toast.makeText(this, "Akses notifikasi ditolak ❌", Toast.LENGTH_SHORT).show();
            }
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        SharedPreferences prefs = getSharedPreferences("wera_settings", MODE_PRIVATE);
        String lang = prefs.getString("language", "id");
        Locale locale = new Locale(lang);
        Locale.setDefault(locale);
        Configuration config = new Configuration();
        config.setLocale(locale);
        getResources().updateConfiguration(config, getResources().getDisplayMetrics());
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_home);

        deviceViewModel = new ViewModelProvider(this).get(DeviceViewModel.class);
        SharedPreferences devPrefs = getSharedPreferences("wera_devices", MODE_PRIVATE);
        String deviceRaw = devPrefs.getString("device_list", "");
        if (!deviceRaw.isEmpty()) {
            String[] devices = deviceRaw.split(";");
            for (String data : devices) {
                String[] parts = data.split("\\|");
                if (parts.length == 5) {
                    Device d = new Device(parts[0], parts[1], parts[2], Integer.parseInt(parts[3]), parts[4]);
                    deviceViewModel.addDevice(d);
                }
            }
        }

// Observe perubahan data dari ViewModel
        deviceViewModel.getDevices().observe(this, updatedList -> {
            deviceAdapter.updateData(updatedList);
        });

        int themePref = prefs.getInt("theme_mode", AppCompatDelegate.MODE_NIGHT_NO);
        AppCompatDelegate.setDefaultNightMode(themePref);
        boolean themeChanged = prefs.getBoolean("theme_changed", false);
        if (themeChanged) {
            prefs.edit().putBoolean("theme_changed", false).apply();

            getSupportFragmentManager().beginTransaction()
                    .replace(R.id.fragment_container, new SettingsFragment())
                    .commit();

            // Sembunyikan tampilan Home
            findViewById(R.id.rvDeviceList).setVisibility(View.GONE);
            findViewById(R.id.tvGreeting).setVisibility(View.GONE);
            findViewById(R.id.fabStartAll).setVisibility(View.GONE);
            findViewById(R.id.fragment_container).setVisibility(View.VISIBLE);
        }

        boolean langChanged = prefs.getBoolean("language_changed", false);
        if (langChanged) {
            prefs.edit().putBoolean("language_changed", false).apply();

            getSupportFragmentManager().beginTransaction()
                    .replace(R.id.fragment_container, new SettingsFragment())
                    .commit();

            findViewById(R.id.rvDeviceList).setVisibility(View.GONE);
            findViewById(R.id.tvGreeting).setVisibility(View.GONE);
            findViewById(R.id.fabStartAll).setVisibility(View.GONE);
            findViewById(R.id.fragment_container).setVisibility(View.VISIBLE);
        }

        // Inisialisasi View
        rvDeviceList = findViewById(R.id.rvDeviceList);
        tvGreeting = findViewById(R.id.tvGreeting);
        fabStartAll = findViewById(R.id.fabStartAll);
        ImageButton btnNotif = findViewById(R.id.btnNotif); // tombol notifikasi
        ImageButton btnSettings = findViewById(R.id.btnSettings);
        ImageButton btnAdd = findViewById(R.id.btnAdd);

        // RecyclerView setup
        rvDeviceList.setLayoutManager(new LinearLayoutManager(this));

        // Sembunyikan ActionBar default
        if (getSupportActionBar() != null) {
            getSupportActionBar().hide();}

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            if (ContextCompat.checkSelfPermission(this, Manifest.permission.POST_NOTIFICATIONS)
                    != PackageManager.PERMISSION_GRANTED) {
                ActivityCompat.requestPermissions(this,
                        new String[]{Manifest.permission.POST_NOTIFICATIONS}, 100);
            }
        }

        // Adapter
        deviceAdapter = new DeviceAdapter(this, dummyDevices, new DeviceAdapter.OnDeviceActionListener() {
            @Override
            public void onControlClick(Device device) {
                Toast.makeText(HomeActivity.this, "▶️ " + device.name, Toast.LENGTH_SHORT).show();
            }

            @Override
            public void onSettingsClick(Device device) {
                Toast.makeText(HomeActivity.this, "⚙️ " + device.name, Toast.LENGTH_SHORT).show();
            }
        });

        rvDeviceList.setAdapter(deviceAdapter);

        btnAdd.setOnClickListener(v -> {
            FragmentTransaction ft = getSupportFragmentManager().beginTransaction();
            ft.replace(R.id.fragment_container, new AddDeviceFragment());
            ft.addToBackStack(null);
            ft.commit();

            // Sembunyikan tampilan Home
            rvDeviceList.setVisibility(View.GONE);
            tvGreeting.setVisibility(View.GONE);
            fabStartAll.setVisibility(View.GONE);
            findViewById(R.id.fragment_container).setVisibility(View.VISIBLE);
        });

        btnSettings.setOnClickListener(v -> {
            FragmentTransaction ft = getSupportFragmentManager().beginTransaction();
            ft.replace(R.id.fragment_container, new SettingsFragment());
            ft.addToBackStack(null);
            ft.commit();

            // Sembunyikan tampilan Home
            rvDeviceList.setVisibility(View.GONE);
            tvGreeting.setVisibility(View.GONE);
            fabStartAll.setVisibility(View.GONE);
            findViewById(R.id.fragment_container).setVisibility(View.VISIBLE);        });

        // 🔔 Pindah ke Fragment Notifikasi saat tombol notifikasi diklik
        btnNotif.setOnClickListener(v -> {
            FragmentTransaction ft = getSupportFragmentManager().beginTransaction();
            ft.replace(R.id.fragment_container, new NotificationFragment());
            ft.addToBackStack(null);
            ft.commit();

            // Sembunyikan tampilan Home
            rvDeviceList.setVisibility(View.GONE);
            tvGreeting.setVisibility(View.GONE);
            fabStartAll.setVisibility(View.GONE);
            findViewById(R.id.fragment_container).setVisibility(View.VISIBLE);
        });
    }
    public void hideHomeUI() {
        rvDeviceList.setVisibility(View.GONE);
        tvGreeting.setVisibility(View.GONE);
        fabStartAll.setVisibility(View.GONE);
        findViewById(R.id.fragment_container).setVisibility(View.VISIBLE);
    }
    public void showHomeUI() {
        rvDeviceList.setVisibility(View.VISIBLE);
        tvGreeting.setVisibility(View.VISIBLE);
        fabStartAll.setVisibility(View.VISIBLE);
        findViewById(R.id.fragment_container).setVisibility(View.GONE);
    }

    @Override
    public void onBackPressed() {
        View fragment = findViewById(R.id.fragment_container);
        if (fragment.getVisibility() == View.VISIBLE) {
            fragment.setVisibility(View.GONE);
            rvDeviceList.setVisibility(View.VISIBLE);
            tvGreeting.setVisibility(View.VISIBLE);
            fabStartAll.setVisibility(View.VISIBLE);
            getSupportFragmentManager().popBackStack();
        } else {
            super.onBackPressed();
        }
    }

}
