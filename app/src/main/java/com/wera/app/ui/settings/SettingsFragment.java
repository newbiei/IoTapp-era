package com.wera.app.ui.settings;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.res.Configuration;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CheckBox;
import android.widget.ImageButton;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatDelegate;
import androidx.fragment.app.Fragment;

import com.wera.app.R;
import com.wera.app.ui.home.HomeActivity;

import java.util.Locale;

public class SettingsFragment extends Fragment {

    private static final String PREF_NAME = "wera_settings";
    private static final String KEY_NOTIF = "notif_enabled";
    private static final String KEY_PRIVACY = "privacy_agreed";
    private static final String KEY_THEME = "theme_mode";
    private static final String KEY_LANG = "language";

    private CheckBox cbNotif, cbPrivacy;
    private SharedPreferences preferences;

    private void updateThemeText(TextView tv, int mode) {
        String text = "Tidak diketahui";
        if (mode == AppCompatDelegate.MODE_NIGHT_NO) text = getString(R.string.light);
        else if (mode == AppCompatDelegate.MODE_NIGHT_YES) text = getString(R.string.dark);
        else if (mode == AppCompatDelegate.MODE_NIGHT_FOLLOW_SYSTEM) text = "Otomatis";
        tv.setText(text);
    }

    private void setLocale(String langCode) {
        Locale locale = new Locale(langCode);
        Locale.setDefault(locale);
        Configuration config = new Configuration();
        config.setLocale(locale);
        requireContext().getResources().updateConfiguration(config, requireContext().getResources().getDisplayMetrics());
    }

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater,
                             @Nullable ViewGroup container,
                             @Nullable Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_settings, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view,
                              @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        preferences = requireContext().getSharedPreferences(PREF_NAME, Context.MODE_PRIVATE);

        ImageButton btnBack = view.findViewById(R.id.btnBack);
        btnBack.setOnClickListener(v -> {
            ((HomeActivity) requireActivity()).showHomeUI();
            requireActivity().getSupportFragmentManager().popBackStack();
        });

        // Tema
        TextView tvCurrentTheme = view.findViewById(R.id.tvCurrentTheme);
        View layoutTheme = view.findViewById(R.id.layoutTheme);
        final int[] themePref = {preferences.getInt(KEY_THEME, AppCompatDelegate.MODE_NIGHT_NO)};
        updateThemeText(tvCurrentTheme, themePref[0]);

        layoutTheme.setOnClickListener(v -> {
            String[] options = {"System (default)",
            getString(R.string.light), getString(R.string.dark)};
            int checked = (themePref[0] == AppCompatDelegate.MODE_NIGHT_FOLLOW_SYSTEM) ? 0 :
                    (themePref[0] == AppCompatDelegate.MODE_NIGHT_NO) ? 1 : 2;

            new AlertDialog.Builder(requireContext())
                    .setTitle(R.string.Pilih_Bahasa)
                    .setSingleChoiceItems(options, checked, (dialog, which) -> {
                        int selectedMode;
                        if (which == 0) selectedMode = AppCompatDelegate.MODE_NIGHT_FOLLOW_SYSTEM;
                        else if (which == 1) selectedMode = AppCompatDelegate.MODE_NIGHT_NO;
                        else selectedMode = AppCompatDelegate.MODE_NIGHT_YES;

                        preferences.edit()
                                .putInt(KEY_THEME, selectedMode)
                                .putBoolean("theme_changed", true)
                                .apply();

                        AppCompatDelegate.setDefaultNightMode(selectedMode);
                        updateThemeText(tvCurrentTheme, selectedMode);
                        themePref[0] = selectedMode;
                        dialog.dismiss();
                    })
                    .show();
        });

        // Checkbox
        cbNotif = view.findViewById(R.id.cbEnableNotification);
        cbPrivacy = view.findViewById(R.id.cbPrivacy);

        boolean notifStatus = preferences.getBoolean(KEY_NOTIF, true);
        boolean privacyStatus = preferences.getBoolean(KEY_PRIVACY, true);
        cbNotif.setChecked(notifStatus);
        cbPrivacy.setChecked(privacyStatus);

        cbNotif.setOnCheckedChangeListener((buttonView, isChecked) -> {
            preferences.edit().putBoolean(KEY_NOTIF, isChecked).apply();
        });

        cbPrivacy.setOnCheckedChangeListener((buttonView, isChecked) -> {
            preferences.edit().putBoolean(KEY_PRIVACY, isChecked).apply();

            if (isChecked) {
                // User setuju: aktifkan pengumpulan data
                Log.d("Privacy", "User agreed to privacy policy - data collection allowed");
                // Kamu bisa set flag di SharedPreferences, misal:
                preferences.edit().putBoolean("allow_data_collection", true).apply();
            } else {
                // User tidak setuju: nonaktifkan pengumpulan data
                Log.d("Privacy", "User disagreed - data collection disabled");
                preferences.edit().putBoolean("allow_data_collection", false).apply();
            }
        });

        View layoutPrivacy = view.findViewById(R.id.tvPrivacyPolicy);
        layoutPrivacy.setOnClickListener(v -> {
            requireActivity().getSupportFragmentManager().beginTransaction()
                    .replace(R.id.fragment_container, new PrivacyPolicyFragment())
                    .addToBackStack(null)
                    .commit();
            ((HomeActivity) requireActivity()).hideHomeUI();
        });

        // Bahasa
        TextView tvCurrentLanguage = view.findViewById(R.id.tvCurrentLanguage);
        View layoutLanguage = view.findViewById(R.id.tvLanguage);

        String currentLang = preferences.getString(KEY_LANG, "id");
        tvCurrentLanguage.setText(currentLang.equals("en") ? "English" : "Indonesia");

        layoutLanguage.setOnClickListener(v -> {
            String[] options = {
                    getString(R.string.lang_id),
                    getString(R.string.lang_en)
            };
            int checked = currentLang.equals("en") ? 1 : 0;

            new AlertDialog.Builder(requireContext())
                    .setTitle(R.string.Pilih_Bahasa)
                    .setSingleChoiceItems(options, checked, (dialog, which) -> {
                        String selectedLang = (which == 1) ? "en" : "id";

                        // simpan ke SharedPreferences
                        preferences.edit()
                                .putString(KEY_LANG, selectedLang)
                                .putBoolean("language_changed", true)
                                .apply();

                        // update locale
                        setLocale(selectedLang);

                        // recreate activity tanpa balik ke home
                        requireActivity().recreate();

                        dialog.dismiss();
                    })
                    .show();
        });
    }
}
