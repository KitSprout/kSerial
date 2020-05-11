package com.kitsprout.main;

import androidx.appcompat.app.AppCompatActivity;

import android.graphics.Color;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import com.kitsprout.ks.KBluetooth;
import com.kitsprout.ks.KSerial;
import com.kitsprout.ks.KSerial.KPacket;

public class MainActivity extends AppCompatActivity {

    private Menu bluetoothDeviceMenu = null;

    private TextView bluetoothRecvText;
    private TextView bluetoothRecvBufferText;
    private EditText bluetoothSendText;
    private Button bluetoothSendButton;
    private Button bluetoothConnectStatus;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // layout
        bluetoothRecvText = findViewById(R.id.textViewBluetoothRecv);
        bluetoothRecvBufferText = findViewById(R.id.textViewBluetoothRecvBuffer);
        bluetoothSendText = findViewById(R.id.editTextBluetoothSend);
        bluetoothSendButton = findViewById(R.id.buttonBluetoothSend);
        bluetoothConnectStatus = findViewById(R.id.buttonBluetoothConnectStatus);
        changeConnectStatusColor(false);

        // bluetooth startup
        if (!KBluetooth.startup(this)) {
            Log.e("KS_DBG", "bluetoothStartup ... error");
        }

        // keep screen on
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        bluetoothDeviceMenu = menu;
        return super.onCreateOptionsMenu(bluetoothDeviceMenu);
    }

    @Override
    public boolean onPrepareOptionsMenu(Menu menu) {
        bluetoothDeviceMenu = KBluetooth.updateDeviceMenu(menu);
        return super.onPrepareOptionsMenu(bluetoothDeviceMenu);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        int id = item.getItemId();
        int state = KBluetooth.select(id);
        if (state == KBluetooth.DISCONNECTED) {
            bluetoothRecvBufferText.setText("");
            changeConnectStatusColor(false);
            Toast.makeText(this, "DISCONNECT SUCCESS", Toast.LENGTH_SHORT).show();
        } else if (state == KBluetooth.DISCONNECT_FAILED) {
            Toast.makeText(this, "DISCONNECT FAILED", Toast.LENGTH_SHORT).show();
        } else if (state == KBluetooth.CONNECT_FAILED) {
            Toast.makeText(this, "CONNECT FAILED", Toast.LENGTH_SHORT).show();
        } else if (state == KBluetooth.CONNECTED) {
            bluetoothBeginListening();
            Toast.makeText(this, "CONNECT SUCCESS", Toast.LENGTH_SHORT).show();
        }
        return super.onOptionsItemSelected(item);
    }

    private void changeConnectStatusColor(boolean enable) {
        if (enable) {
            bluetoothSendButton.setTextColor(Color.BLACK);
            bluetoothConnectStatus.setBackgroundColor(Color.RED);
        } else {
            bluetoothSendButton.setTextColor(Color.GRAY);
            bluetoothConnectStatus.setBackgroundColor(Color.parseColor("#DCDCDC"));
        }
    }

    public void OnClickBluetoothSendData(View view) {
        if (KBluetooth.isConnected()) {
            int lens = KBluetooth.send(bluetoothSendText.getText().toString().getBytes());
            Log.d("KS_DBG", String.format("OnClickBluetoothSendData() ... lens = %d", lens));
        } else {
            Log.d("KS_DBG", "OnClickBluetoothSendData() ... without connect");
        }
    }

    KSerial ks;
    Thread bluetoothRecvThread;
    void bluetoothBeginListening() {
        Log.d("KS_DBG", "bluetoothBeginListening()");
        final Handler handler = new Handler();
        ks = new KSerial(32*1024, 0.001);
        bluetoothRecvThread = new Thread(new Runnable() {
            public void run() {
                changeConnectStatusColor(true);
                while (!Thread.currentThread().isInterrupted() && KBluetooth.isConnected()) {
                    byte[] receiveBytes = KBluetooth.receive();
                    if (receiveBytes != null) {
                        int bytesAvailable = receiveBytes.length;
                        if (bytesAvailable > 0) {
                            KPacket[] pk = ks.getPacket(receiveBytes);
                            for (KSerial.KPacket KPacket : pk) {
                                Log.d("KSERIAL", String.format("%6d,%.0f,%d,%d",
                                        ks.getPacketParameterU16(KPacket), ks.getFrequency(0), bytesAvailable, pk.length));
                            }
                            // show information
                            final String recvBufferString = String.format("%.0fHz\n%.1fs\n%3d,%d", ks.getFrequency(0), ks.getTimes(), bytesAvailable, pk.length);
                            final String recvByteString = String.format("freq = %.0f Hz, bytes = %3d, packet = %d (%d), run time = %.3f s", ks.getFrequency(0), bytesAvailable, ks.getPacketTotalCount(), pk.length, ks.getTimes());
                            handler.post(new Runnable() {
                                public void run() {
                                    bluetoothRecvText.setText(recvBufferString);
                                    bluetoothRecvBufferText.setText(recvByteString);
                                }
                            });
                        }
                    } else {
                        KBluetooth.disconnect();
                        break;
                    }
                }
                changeConnectStatusColor(false);
            }
        });
        bluetoothRecvThread.start();
    }

}
