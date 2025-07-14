package com.wera.app.viewmodel;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.ViewModel;

import com.wera.app.data.Device;

import java.util.ArrayList;
import java.util.List;

public class DeviceViewModel extends ViewModel {
    private final MutableLiveData<List<Device>> deviceList = new MutableLiveData<>(new ArrayList<>());

    public LiveData<List<Device>> getDevices() {
        return deviceList;
    }

    public void addDevice(Device newDevice) {
        List<Device> currentList = new ArrayList<>(deviceList.getValue());
        currentList.add(newDevice);
        deviceList.setValue(currentList);
    }
}
