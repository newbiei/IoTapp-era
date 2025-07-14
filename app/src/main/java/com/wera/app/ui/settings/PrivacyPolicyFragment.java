package com.wera.app.ui.settings;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.webkit.WebView;
import android.widget.ImageButton;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;

import com.wera.app.R;
import com.wera.app.ui.home.HomeActivity;

public class PrivacyPolicyFragment extends Fragment {

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater,
                             @Nullable ViewGroup container,
                             @Nullable Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_privacy_policy, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view,
                              @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        ImageButton btnBack = view.findViewById(R.id.btnBackPrivacy);
        btnBack.setOnClickListener(v -> {
            ((HomeActivity) requireActivity()).showHomeUI();
            requireActivity().getSupportFragmentManager().popBackStack();
        });

        WebView webView = view.findViewById(R.id.webViewPrivacy);
        webView.getSettings().setJavaScriptEnabled(true); // kalau butuh
        webView.loadUrl("file:///android_asset/privacy_policy.html");


    }
}
