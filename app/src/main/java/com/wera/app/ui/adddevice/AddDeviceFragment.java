package com.wera.app.ui.adddevice;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.graphics.Bitmap;
import android.net.Uri;
import android.os.Bundle;
import android.provider.MediaStore;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.lifecycle.ViewModelProvider;

import com.google.zxing.BinaryBitmap;
import com.google.zxing.Result;
import com.google.zxing.common.HybridBinarizer;
import com.google.zxing.LuminanceSource;
import com.google.zxing.RGBLuminanceSource;
import com.google.zxing.Reader;
import com.google.zxing.qrcode.QRCodeReader;
import com.journeyapps.barcodescanner.CaptureActivity;
import com.wera.app.R;
import com.wera.app.data.Device;
import com.wera.app.ui.home.HomeActivity;
import com.wera.app.viewmodel.DeviceViewModel;

import java.io.IOException;

public class AddDeviceFragment extends Fragment {

    private static final int CAMERA_SCAN_REQUEST = 102;
    private static final int PICK_IMAGE_REQUEST = 1;

    private ImageView imgScannerArea;
    private DeviceViewModel deviceViewModel;

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater,
                             @Nullable ViewGroup container,
                             @Nullable Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_add_device, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view,
                              @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        deviceViewModel = new ViewModelProvider(requireActivity()).get(DeviceViewModel.class);

        ImageButton btnBack = view.findViewById(R.id.btnBack);
        ImageButton btnGallery = view.findViewById(R.id.btnGallery);
        ImageButton btnFlash = view.findViewById(R.id.btnFlash);
        imgScannerArea = view.findViewById(R.id.imgScannerArea);

        // 📸 Kamera QR Scan
        imgScannerArea.setOnClickListener(v -> {
            Intent intent = new Intent(requireContext(), CaptureActivity.class);
            startActivityForResult(intent, CAMERA_SCAN_REQUEST);
        });

        // 🔙 Kembali ke Home
        btnBack.setOnClickListener(v -> {
            ((HomeActivity) requireActivity()).showHomeUI();
            requireActivity().getSupportFragmentManager().popBackStack();
        });

        // 🖼️ Galeri
        btnGallery.setOnClickListener(v -> openGallery());

        // 💡 Flash (placeholder, tergantung implementasi kamera)
        final boolean[] isFlashOn = {false};
        btnFlash.setOnClickListener(v -> {
            isFlashOn[0] = !isFlashOn[0];
            String msg = isFlashOn[0] ? "Flashlight ON" : "Flashlight OFF";
            Toast.makeText(requireContext(), msg, Toast.LENGTH_SHORT).show();
            // Custom implementasi flash bisa kamu tambahkan di sini
        });
    }

    private void openGallery() {
        Intent intent = new Intent(Intent.ACTION_PICK, MediaStore.Images.Media.EXTERNAL_CONTENT_URI);
        startActivityForResult(intent, PICK_IMAGE_REQUEST);
    }

    private void scanQRFromBitmap(Bitmap bitmap) {
        int[] intArray = new int[bitmap.getWidth() * bitmap.getHeight()];
        bitmap.getPixels(intArray, 0, bitmap.getWidth(), 0, 0, bitmap.getWidth(), bitmap.getHeight());
        LuminanceSource source = new RGBLuminanceSource(bitmap.getWidth(), bitmap.getHeight(), intArray);
        BinaryBitmap binaryBitmap = new BinaryBitmap(new HybridBinarizer(source));

        Reader reader = new QRCodeReader();
        try {
            Result result = reader.decode(binaryBitmap);
            String qrData = result.getText();
            showDeviceDialog(qrData);
        } catch (Exception e) {
            Toast.makeText(getContext(), "Gagal membaca QR code 😢", Toast.LENGTH_SHORT).show();
        }
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, @Nullable Intent data) {
        super.onActivityResult(requestCode, resultCode, data);

        if (requestCode == PICK_IMAGE_REQUEST && resultCode == Activity.RESULT_OK && data != null) {
            Uri selectedImage = data.getData();
            try {
                Bitmap bitmap = MediaStore.Images.Media.getBitmap(requireActivity().getContentResolver(), selectedImage);
                imgScannerArea.setImageBitmap(bitmap);
                scanQRFromBitmap(bitmap);
            } catch (IOException e) {
                e.printStackTrace();
            }
        } else if (requestCode == CAMERA_SCAN_REQUEST && resultCode == Activity.RESULT_OK && data != null) {
            String qrResult = data.getStringExtra("SCAN_RESULT");
            showDeviceDialog(qrResult);
        }
    }

    private void showDeviceDialog(String qrData) {
        String[] parts = qrData.split("\\|");

        if (parts.length == 4) {
            String deviceId = parts[0];
            String status = parts[1];
            int battery = Integer.parseInt(parts[2]);
            String ip = parts[3];

            View dialogView = LayoutInflater.from(requireContext()).inflate(R.layout.dialog_add_device, null);
            TextView tvId = dialogView.findViewById(R.id.tvDeviceId);
            EditText editName = dialogView.findViewById(R.id.editDeviceName);

            tvId.setText("ID Perangkat\n" + deviceId);

            AlertDialog dialog = new AlertDialog.Builder(requireContext())
                    .setView(dialogView)
                    .setCancelable(false)
                    .create();

            dialog.setButton(AlertDialog.BUTTON_POSITIVE, "Simpan", (d, which) -> {
                String name = editName.getText().toString().trim();
                if (name.isEmpty()) {
                    Toast.makeText(getContext(), "Nama perangkat tidak boleh kosong", Toast.LENGTH_SHORT).show();
                    return;
                }

                Device device = new Device(deviceId, name, status, battery, ip);

                // Simpan ke SharedPreferences
                SharedPreferences prefs = requireContext().getSharedPreferences("wera_devices", Context.MODE_PRIVATE);
                String existing = prefs.getString("device_list", "");
                String newData = existing.isEmpty() ? deviceId + "|" + name + "|" + status + "|" + battery + "|" + ip
                        : existing + ";" + deviceId + "|" + name + "|" + status + "|" + battery + "|" + ip;
                prefs.edit().putString("device_list", newData).apply();

                // Update ViewModel
                deviceViewModel.addDevice(device);
                Toast.makeText(getContext(), "Perangkat berhasil ditambahkan 🎉", Toast.LENGTH_SHORT).show();

                dialog.dismiss();
                ((HomeActivity) requireActivity()).showHomeUI();
                requireActivity().getSupportFragmentManager().popBackStack();
            });

            dialog.setButton(AlertDialog.BUTTON_NEGATIVE, "Batal", (d, which) -> dialog.dismiss());

            dialog.show();

            // Custom icon di tombol
            dialog.getButton(AlertDialog.BUTTON_POSITIVE).setCompoundDrawablesWithIntrinsicBounds(R.drawable.ic_check, 0, 0, 0);
            dialog.getButton(AlertDialog.BUTTON_NEGATIVE).setCompoundDrawablesWithIntrinsicBounds(R.drawable.ic_close, 0, 0, 0);
        } else {
            Toast.makeText(getContext(), "Format QR tidak valid ❌", Toast.LENGTH_SHORT).show();
        }
    }
}

