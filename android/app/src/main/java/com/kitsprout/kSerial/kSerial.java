package com.kitsprout.kSerial;

import android.util.Log;

import java.util.ArrayList;
//import com.kitsprout.kSerial.kPacket;

public class kSerial {

    // form JNI
    static {
        System.loadLibrary("libkserial");
    }
    private static native int unpackBuffer(byte[] buf, int lens);
    private static native int getPacketCount();
    private static native int getPacketType(int index);
    private static native int getPacketBytes(int index);
    private static native int[] getPacketParam(int index);
    private static native double[] getPacketData(int index);

    // log tag
    private static final String TAG = "KSERIAL";

    // default maximum receive buffer size
    private static final int maxPacketBufferSize = 8 * 1024;     // 8 kB

    // receive packet buffer and index
    private int packetBufferIndex;
    private byte[] packetBuffer;

    // maximum record buffer size
    private int saveMaxBufferSize;
    private ArrayList<kPacket> pklist;

    // receive packet
    private kPacket[] pk;
    private double packetTimeUnit;
    private long packetTotalCount;
    private int parameter16;

    // timestamp and packet frequency
    private boolean timestampInited;
    private boolean enablePacketTime;
    private long[] firstTimestamp;
    private long[] lastTimestamp;
    private long systemTimestampCount;
    private long systemLastTimestamp;
    private double frequency;
    private double frequencyFiltered;

    // lost rate
    private boolean enableLostRate;
    private long lostCount;

    public static class kPacket {
        public int type;
        public int nbyte;
        public int[] param;
        public double[] data;
    }

    public kSerial(int saveMaxBufferSize, double timeUnit) {
        this.saveMaxBufferSize = saveMaxBufferSize;
        this.pklist = new ArrayList<kPacket>();
        this.packetTotalCount = 0;
        this.timestampInited = false;
        this.firstTimestamp = new long[2];
        this.lastTimestamp = new long[2];
        this.resetPacketBuffer();
        this.setTimeUnit(timeUnit);
    }

    private void resetPacketBuffer() {
        packetBufferIndex = 0;
        packetBuffer = new byte[maxPacketBufferSize];
    }

    private void setTimeUnit(double timeUnit) {
        if (timeUnit > 0) {
            // use packet time
            enablePacketTime = true;
            packetTimeUnit = timeUnit;
        } else {
            // use system time
            enablePacketTime = false;
            packetTimeUnit = 0;
        }
    }

    private kPacket runPacketProcess(int index) {
        // get packet info and data form jni
        kPacket packet = new kPacket();
        packet.type  = kSerial.getPacketType(index);
        packet.nbyte = kSerial.getPacketBytes(index);
        packet.param = kSerial.getPacketParam(index);
        packet.data  = kSerial.getPacketData(index);
        // record packet
        if (saveMaxBufferSize > 0) {
            if (pklist.size() >= saveMaxBufferSize) {
                pklist.remove(0); // remove oldest packet
            }
            pklist.add(packet);
        }
        return packet;
    }

    private long getPacketTimestamp(kPacket packet) {
        return (long)(packet.data[0] * 1000 + packet.data[1]);
    }

    public int getPacketParameterU16(kPacket packet) {
        parameter16 = (packet.param[1] & 0x0000FFFF) * 256 | packet.param[0];
        return parameter16;
    }

    private long getPacketLost(kPacket[] packets) {
        long count = 0;
        for (kPacket packet : packets) {
            int lastCount = parameter16;
            int differenceCount = getPacketParameterU16(packet) - lastCount;
            if ((differenceCount != 1) && (differenceCount != -65535)) {
                count++;
            }
        }
        return count;
    }

    private double getPacketFrequency(kPacket lastPacket, int count) {
        if (!timestampInited) {
            timestampInited = true;
            firstTimestamp[0] = System.currentTimeMillis();
            lastTimestamp[0] = firstTimestamp[0];
            if (enablePacketTime) {
                firstTimestamp[1] = getPacketTimestamp(lastPacket);
                lastTimestamp[1] = firstTimestamp[1];
            }
            frequency = 0;
        } else {
            packetTotalCount = packetTotalCount + count;
            long systemTimestamp = System.currentTimeMillis();
            if (enablePacketTime) {
                long packetTimestamp = getPacketTimestamp(lastPacket);
                double dt = (packetTimestamp - lastTimestamp[1]) * packetTimeUnit;
                lastTimestamp[1] = packetTimestamp;
                frequency = count / dt;
            } else {
                double dt = (systemTimestamp - systemLastTimestamp) * 0.001;
                systemTimestampCount = systemTimestampCount + count;
                if (dt > 1.2) {
                    frequency = systemTimestampCount / dt;
                    systemTimestampCount = 0;
                    systemLastTimestamp = systemTimestamp;
                }
            }
            lastTimestamp[0] = systemTimestamp;
        }
        return frequency;
    }

    public kPacket[] getPacket(byte[] receiveBytes) {
        // update packet buffer
        System.arraycopy(receiveBytes, 0, packetBuffer, packetBufferIndex, receiveBytes.length);
        packetBufferIndex += receiveBytes.length;
        // unpack receive buffer
        int newPacketBufferIndex = kSerial.unpackBuffer(packetBuffer, packetBufferIndex);
        int packetCount = kSerial.getPacketCount();
        pk = new kPacket[packetCount];
        if (packetCount > 0) {
            for (int i = 0; i < packetCount; i++) {
                pk[i] = runPacketProcess(i);
            }
            // get frequency
            frequency = getPacketFrequency(pk[packetCount-1], packetCount);
            // get lostcount
            if (enableLostRate) {
                lostCount += getPacketLost(pk);
            }
            // remove unpack data from receive buffer
            packetBufferIndex -= newPacketBufferIndex;
            System.arraycopy(packetBuffer, newPacketBufferIndex, packetBuffer, 0, packetBufferIndex);
//            Log.d(TAG, String.format("time=%.2f, freq=%.2f, packetTotalCount=%d, pklist.size()=%d", getTimes(), getFrequency(0), packetTotalCount, pklist.size()));
        }

        return pk;
    }

    public double getFrequency(double weighting) {
        if (weighting > 0) {
            frequencyFiltered = (1 - weighting) * frequencyFiltered + weighting * frequency;
        } else {
            frequencyFiltered = frequency;
        }
        return frequencyFiltered;
    }

    public double getTimes() {
        if (enablePacketTime) {
            return (lastTimestamp[1] - firstTimestamp[1]) * packetTimeUnit;
        } else {
            return (lastTimestamp[0] - firstTimestamp[0]) * 0.001;
        }
    }

    // need to set parameter byte to counter (16-bit)
    public void enableLostRateDetection(boolean cmd) {
        enableLostRate = cmd;
    }

    public long getLostRate() {
        return lostCount;
//        if (enablePacketTime) {
//            return (lastTimestamp[1] - firstTimestamp[1]) * packetTimeUnit;
//        } else {
//            return (lastTimestamp[0] - firstTimestamp[0]) * 0.001;
//        }
    }

    public long getPacketTotalCount() {
        return packetTotalCount;
    }

    public long getSavePacketCount() {
        return pklist.size();
    }

    public ArrayList<kPacket> getSavePacketBuffer() {
        return pklist;
    }

}
