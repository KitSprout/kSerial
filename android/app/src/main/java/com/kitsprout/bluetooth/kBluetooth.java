package com.kitsprout.bluetooth;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.util.Log;
import android.view.Menu;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Set;
import java.util.UUID;

public class kBluetooth {

    private static final String TAG = "KSBLE";
    private static final UUID btUUID = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB");

    public static BluetoothDevice device;

    private static boolean connectStatus;
    private static BluetoothAdapter adapter;
    private static BluetoothDevice[] deviceList;
    private static BluetoothSocket socket;
    private static InputStream inputStream;
    private static OutputStream outputStream;

    public static boolean startup(Context context) {
        connectStatus = false;

        Log.d(TAG, "kBluetooth startup ...");

        // get the bluetooth adapter
        adapter = getAdapter();
        if (adapter == null) {
            Log.e(TAG, "    getAdapter() ... error");
            return false;
        }
        Log.d(TAG, "    getAdapter() ... ok");

        // get bluetooth paired devices
        deviceList = getPairedDevice(adapter);
        Log.d(TAG, "    getPairedDevice() ... ok");
        // select print device list
        for (int i = 0; i < deviceList.length; i++) {
            Log.d(TAG, "        [" + (i+1) + "] "
                    + deviceList[i].getName() +
                    " (" + deviceList[i].getAddress() + ")");
        }

        // listening bluetooth broadcasts
        BroadcastReceiver btReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                String action = intent.getAction();
                if (BluetoothDevice.ACTION_ACL_CONNECTED.equals(action)) {
                    connectStatus = true;
                    Log.d(TAG, "onReceive() ... connected");
                } else if (BluetoothDevice.ACTION_ACL_DISCONNECTED.equals(action)) {
                    if (connectStatus) {
                        connectStatus = false;
                        disconnect();
                    }
                    Log.d(TAG, "onReceive() ... disconnected");
                }
            }
        };
        IntentFilter btConnectFilter = new IntentFilter(BluetoothDevice.ACTION_ACL_CONNECTED);
        IntentFilter btDisconnectFilter = new IntentFilter(BluetoothDevice.ACTION_ACL_DISCONNECTED);
        context.registerReceiver(btReceiver, btConnectFilter);
        context.registerReceiver(btReceiver, btDisconnectFilter);

        return true;
    }

    private static BluetoothAdapter getAdapter() {
//        Log.d(TAG, "getAdapter()");
        BluetoothAdapter btAdapter = BluetoothAdapter.getDefaultAdapter();
        if (btAdapter == null) {
            Log.e(TAG, "device doesn't support bluetooth");
            return null;
        }
        return btAdapter;
    }

    private static BluetoothDevice[] getPairedDevice(BluetoothAdapter btAdapter) {
//        Log.d(TAG, "getPairedDevice()");
        BluetoothDevice[] btDeviceList = null;
        Set<BluetoothDevice> btDevicesList = btAdapter.getBondedDevices();
        if (btDevicesList.size() > 0) {
            int cnt = 0;
            btDeviceList = new BluetoothDevice[btDevicesList.size()];
            for (BluetoothDevice device : btDevicesList) {
                btDeviceList[cnt++] = device;
            }
        }
        return btDeviceList;
    }

    private static BluetoothSocket getSocket(BluetoothDevice device) {
//        Log.d(TAG, "getSocket()");
        BluetoothSocket btSocket = null;
        try {
            btSocket = device.createRfcommSocketToServiceRecord(btUUID);
        } catch (IOException e) {
            Log.e(TAG, "getSocket() method failed", e);
        }
        return btSocket;
    }

    private static BluetoothDevice getDevice(BluetoothAdapter btAdapter, String address) {
//        Log.d(TAG, "getDevice()");
        return btAdapter.getRemoteDevice(address);
    }

    public static String[] getDeviceName() {
//        Log.d(TAG, "getDeviceName()");
        String[] deviceName = new String[deviceList.length];
        for (int i = 0; i < deviceList.length; i++) {
            String name = deviceList[i].getName();
            if (deviceList[i].getName().length() > 13) {
                name = name.substring(0, 10) + "...";
            }
            deviceName[i] = String.format("%s (%s)", name, deviceList[i].getAddress());
        }
        return deviceName;
    }

    public static void updatePairedDeviceList() {
//        Log.d(TAG, "updatePairedDevice()");
        deviceList = getPairedDevice(adapter);
    }

    public static boolean connect(String address) {
        Log.d(TAG, String.format("connect() ... mac: %s", address));
        socket = getSocket(getDevice(adapter, address));
        if (socket == null) {
            return false;
        }
        adapter.cancelDiscovery();
        try {
            // Connect to the remote device through the socket
            socket.connect();
            inputStream = socket.getInputStream();
            outputStream = socket.getOutputStream();
        } catch (IOException connectException) {
            // Unable to connect; close the socket and return
            try {
                socket.close();
                Log.d(TAG, "connect() ... failed");
            } catch (IOException e) {
                Log.e(TAG, "connect() ... failed ... could not open the client socket", e);
            }
            return false;
        }
        return true;
    }

    public static boolean isConnected() {
        return connectStatus;
    }

    public static boolean disconnect() {
        Log.d(TAG, String.format("disconnect() ... mac: %s", device.getAddress()));
        try {
            socket.close();
        } catch (IOException e) {
            Log.e(TAG, "disconnect() ... failed ... could not close the client socket", e);
            return false;
        }
        return true;
    }

    public static final int DISCONNECT_FAILED   = -2;   // disconnect
    public static final int DISCONNECTED        = -1;   // disconnect
    public static final int UNCHANE             =  0;   // do nothing
    public static final int CONNECTED           =  1;   // connect
    public static final int CONNECT_FAILED      =  2;   // disconnect
    public static int select(int index) {
        int state = UNCHANE;
        if ((index > 0) && (index <= deviceList.length)) {
            // connect
            device = deviceList[index - 1];
            if (!connectStatus) {
                connectStatus = connect(device.getAddress());
                if (connectStatus) {
                    state = CONNECTED;
                } else {
                    state = CONNECT_FAILED;
                }
            } else {
                state = UNCHANE;
            }
        } else {
            // disconnect
            if (connectStatus) {
                connectStatus = false;
                if (disconnect()) {
                    state = DISCONNECTED;
                } else {
                    state = DISCONNECT_FAILED;
                }
            }
        }
        return state;
    }

    public static int send(byte[] data) {
        Log.d(TAG, "send()");
        try {
            outputStream.write(data);
        } catch (IOException e) {
            Log.e(TAG, "OutputStream write() method failed", e);
        }
        return data.length;
    }

    public static byte[] receive() {
        try {
            int bytesAvailable = inputStream.available();
            if (bytesAvailable > 0) {
                byte[] receiveBytes = new byte[bytesAvailable];
                if (inputStream.read(receiveBytes) != -1) {
                    return receiveBytes;
                }
            }
        } catch (IOException e) {
            Log.e(TAG, "InputStream read() method failed", e);
            return null;
        }
        return new byte[0];
    }

    public static Menu updateDeviceMenu(Menu menu) {
        updatePairedDeviceList();
        menu.clear();
        menu.add(0, 0, 0, "Disconnect");
        String[] deviceName = getDeviceName();
        for (int i = 1; i <= deviceName.length; i++) {
            menu.add(0, i, i, deviceName[i - 1]);
        }
        return menu;
    }

}
