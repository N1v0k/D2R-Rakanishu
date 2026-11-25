package com.xiaoboluo.d2r;

import android.app.Activity;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.view.Display;
import android.view.Surface;












final class CamSensor implements SensorEventListener {
    private final Activity activity;
    private final SensorManager sensors;
    private int mode;
    private boolean listening;


    private final float[] up = new float[3];
    private boolean haveUp;
    private long lastGyro, lastTilt;

    CamSensor(Activity activity) { this.activity = activity; sensors = activity.getSystemService(SensorManager.class); }

    void start(int m) {
        stop();
        mode = m;
        if (m <= 0 || sensors == null) return;
        Sensor g = sensors.getDefaultSensor(Sensor.TYPE_GRAVITY);
        if (g == null) g = sensors.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);
        if (g != null) listening |= sensors.registerListener(this, g, SensorManager.SENSOR_DELAY_GAME);
        if (m == 1) {
            Sensor gyro = sensors.getDefaultSensor(Sensor.TYPE_GYROSCOPE);
            if (gyro != null) listening |= sensors.registerListener(this, gyro, SensorManager.SENSOR_DELAY_GAME);
        }
        android.util.Log.i("dm", "camera sensor: mode " + m + ", listening " + listening);
    }

    void stop() {
        if (listening && sensors != null) sensors.unregisterListener(this);
        listening = false;
        haveUp = false;
        lastGyro = lastTilt = 0;
    }

    @Override public void onAccuracyChanged(Sensor sensor, int accuracy) {}

    @Override public void onSensorChanged(SensorEvent e) {
        int type = e.sensor.getType();
        if (type == Sensor.TYPE_GRAVITY || type == Sensor.TYPE_ACCELEROMETER) {
            float k = type == Sensor.TYPE_GRAVITY || !haveUp ? 1f : 0.1f;
            for (int i = 0; i < 3; i++) up[i] += (e.values[i] - up[i]) * k;
            haveUp = true;
            if (mode == 2) tilt(e.timestamp);
        } else if (type == Sensor.TYPE_GYROSCOPE && mode == 1 && haveUp) {
            float dt = lastGyro == 0 ? 0 : Math.min(0.1f, (e.timestamp - lastGyro) * 1e-9f);
            lastGyro = e.timestamp;
            float n = (float) Math.sqrt(up[0] * up[0] + up[1] * up[1] + up[2] * up[2]);
            if (n < 1 || dt <= 0) return;



            float yaw = (e.values[0] * up[0] + e.values[1] * up[1] + e.values[2] * up[2]) / n;
            if (Math.abs(yaw) < 0.05f) return;
            DmActivity.nativeCamTurn(-yaw * dt);
        }
    }

    private void tilt(long timestamp) {
        float dt = lastTilt == 0 ? 0 : Math.min(0.1f, (timestamp - lastTilt) * 1e-9f);
        lastTilt = timestamp;
        if (dt <= 0) return;


        Display display = activity.getDisplay();
        int rotation = display == null ? Surface.ROTATION_90 : display.getRotation();
        float sx, sy;
        switch (rotation) {
            case Surface.ROTATION_90: sx = -up[1]; sy = up[0]; break;
            case Surface.ROTATION_180: sx = -up[0]; sy = -up[1]; break;
            case Surface.ROTATION_270: sx = up[1]; sy = -up[0]; break;
            default: sx = up[0]; sy = up[1]; break;
        }
        if (Math.hypot(sx, sy) < 3) return;


        float roll = (float) Math.atan2(-sx, sy), over = Math.abs(roll) - 0.14f;
        if (over <= 0) return;
        DmActivity.nativeCamTurn(Math.min(2.5f, over * 2.5f) * Math.signum(roll) * dt);
    }
}
