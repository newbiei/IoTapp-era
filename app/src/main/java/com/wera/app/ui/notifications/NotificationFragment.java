package com.wera.app.ui.notifications;

import android.os.Bundle;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageButton;

import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.wera.app.R;
import com.wera.app.adapter.NotificationAdapter;
import com.wera.app.model.NotificationItem;
import com.wera.app.ui.home.HomeActivity;

import java.util.ArrayList;
import java.util.List;

public class NotificationFragment extends Fragment {

    private RecyclerView rvNotifList;
    private NotificationAdapter adapter;
    private final List<NotificationItem> notifList = new ArrayList<>();

    public NotificationFragment() {
        // Required empty public constructor
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_notification, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        ImageButton btnBack = view.findViewById(R.id.btnBack);
        btnBack.setOnClickListener(v -> {
            ((HomeActivity) requireActivity()).showHomeUI();
            requireActivity().getSupportFragmentManager().popBackStack();
        });


        rvNotifList = view.findViewById(R.id.rvNotifList);
        rvNotifList.setLayoutManager(new LinearLayoutManager(getContext()));

        // Dummy data
        notifList.add(new NotificationItem(NotificationItem.Type.ALERT, "(WERA 1) Baterai lemah! Harap isi daya."));
        notifList.add(new NotificationItem(NotificationItem.Type.DEVICE, "Perangkat WERA 2 berhasil ditambahkan."));
        notifList.add(new NotificationItem(NotificationItem.Type.ERROR, "Sensor gagal membaca sudut tinta."));
        notifList.add(new NotificationItem(NotificationItem.Type.ALERT, "(WERA 2) Penghapusan papan selesai."));

        adapter = new NotificationAdapter(notifList);
        rvNotifList.setAdapter(adapter);
    }
}
