package com.kitsprout.ks;

import android.util.Log;

import java.util.ArrayList;

public class KSerial {

    // form JNI
    static {
        System.loadLibrary("libkserial");
    }
    private static native byte[] pack(int[] param, int type, int lens, double[] data);
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
    private ArrayList<KPacket> pklist;

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

    public static class KPacket {
        public int type;
        public int nbyte;
        public int[] param;
        public double[] data;
    }

    public KSerial(int saveMaxBufferSize, double timeUnit) {
        this.saveMaxBufferSize = saveMaxBufferSize;
        this.pklist = new ArrayList<KPacket>();
        this.packetTotalCount = 0;
        this.timestampInited = false;
        this.firstTimestamp = new long[2];
        this.lastTimestamp = new long[2];
        this.resetPacketBuffer();
        this.setTimeUnit(timeUnit);
    }

    private static final String[] typeNum2str = {
            "uint8",  "uint16", "uint32", "uint64",
            "int8",   "int16",  "int32",  "int64",
            "R0",     "half",   "float",  "double",
            "R1",     "R2",     "R3",     "R4",
    };
    public static String typeConvert(int type) {
        if ((type < 0) || (type > 15)) {
            return "";
        }
        return typeNum2str[type];
    }
    public static int typeConvert(String type) {
        return 0;
    }
    private static int typeSize(int type) {
        return 0;
    }
    private static int typeSize(String type) {
        return 0;
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

    private long getPacketTimestamp(KPacket packet) {
        return (long)(packet.data[0] * 1000 + packet.data[1]);
    }

    public int getPacketParameterU16(KPacket packet) {
        parameter16 = (packet.param[1] & 0x0000FFFF) * 256 | packet.param[0];
        return parameter16;
    }

    private long getPacketLost(KPacket[] packets) {
        long count = 0;
        for (KPacket packet : packets) {
            int lastCount = parameter16;
            int differenceCount = getPacketParameterU16(packet) - lastCount;
            if ((differenceCount != 1) && (differenceCount != -65535)) {
                count++;
            }
        }
        return count;
    }

    private double getPacketFrequency(KPacket lastPacket, int count) {
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

    public KPacket[] getPacket(byte[] receiveBytes) {
        // update packet buffer
        System.arraycopy(receiveBytes, 0, packetBuffer, packetBufferIndex, receiveBytes.length);
        packetBufferIndex += receiveBytes.length;
        // unpack receive buffer
        int newPacketBufferIndex = unpackBuffer(packetBuffer, packetBufferIndex);
        int packetCount = getPacketCount();
        // receive packet
        KPacket[] pk = new KPacket[packetCount];
        if (packetCount > 0) {
            for (int i = 0; i < packetCount; i++) {
//                pk[i] = runPacketProcess(i);
                // get packet info and data form jni
                KPacket packet = new KPacket();
                packet.type  = getPacketType(i);
                packet.nbyte = getPacketBytes(i);
                packet.param = getPacketParam(i);
                packet.data  = getPacketData(i);
                // record packet
                if (saveMaxBufferSize > 0) {
                    if (pklist.size() >= saveMaxBufferSize) {
                        pklist.remove(0); // remove oldest packet
                    }
                    pklist.add(packet);
                }
                pk[i] = packet;
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
    }

    public long getPacketTotalCount() {
        return packetTotalCount;
    }

    public long getSavePacketCount() {
        return pklist.size();
    }

    public ArrayList<KPacket> getSavePacketBuffer() {
        return pklist;
    }

}
