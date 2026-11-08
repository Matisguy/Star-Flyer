package com.example.starflyer;

import android.graphics.Color;
import android.graphics.Typeface;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.os.Bundle;
import android.util.TypedValue;
import android.view.ViewGroup;
import android.view.WindowInsets;
import android.view.WindowInsetsController;
import android.widget.Button;
import android.widget.RelativeLayout;

import com.google.androidgamesdk.GameActivity;

public class MainActivity extends GameActivity implements SensorEventListener {
    static {
        System.loadLibrary("starflyer");
    }

    private SensorManager sensorManager;
    private Sensor accelerometer;
    private Sensor gyroscope;
    private Sensor magnetometer;
    private Sensor rotationVector;

    private final float[] accelValues = new float[3];
    private final float[] gyroValues = new float[3];
    private final float[] magValues = new float[3];
    private final float[] rotationMatrix = new float[9];
    private final float[] orientationAngles = new float[3];

    private float pitch = 0f;
    private float roll = 0f;
    private float yaw = 0f;

    private boolean isPaused = false;

    // Native JNI calls to bridge sensor data and UI actions to C++ game logic
    public native void updateNativeSensors(
            float accelX, float accelY, float accelZ,
            float gyroX, float gyroY, float gyroZ,
            float magX, float magY, float magZ,
            float pitch, float roll, float yaw
    );

    public native void nativeRequestCalibration();
    public native void nativeTogglePause();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Setup Sensor Manager & retrieve sensors
        sensorManager = (SensorManager) getSystemService(SENSOR_SERVICE);
        if (sensorManager != null) {
            accelerometer = sensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);
            gyroscope = sensorManager.getDefaultSensor(Sensor.TYPE_GYROSCOPE);
            magnetometer = sensorManager.getDefaultSensor(Sensor.TYPE_MAGNETIC_FIELD);
            rotationVector = sensorManager.getDefaultSensor(Sensor.TYPE_ROTATION_VECTOR);
            if (rotationVector == null) {
                rotationVector = sensorManager.getDefaultSensor(Sensor.TYPE_GAME_ROTATION_VECTOR);
            }
        }

        // Setup HUD UI Overlay
        RelativeLayout uiOverlay = new RelativeLayout(this);

        // Calibrate Button
        Button calibrateBtn = new Button(this);
        calibrateBtn.setText("🎯 CALIBRATE");
        calibrateBtn.setTextColor(Color.CYAN);
        calibrateBtn.setBackgroundColor(Color.argb(180, 20, 25, 40));
        calibrateBtn.setTypeface(Typeface.MONOSPACE, Typeface.BOLD);
        calibrateBtn.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);

        RelativeLayout.LayoutParams calParams = new RelativeLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
        );
        calParams.addRule(RelativeLayout.ALIGN_PARENT_RIGHT);
        calParams.addRule(RelativeLayout.ALIGN_PARENT_TOP);
        calParams.setMargins(0, 48, 48, 0);
        calibrateBtn.setLayoutParams(calParams);
        calibrateBtn.setOnClickListener(v -> nativeRequestCalibration());

        // Pause / Resume Button
        Button pauseBtn = new Button(this);
        pauseBtn.setText("⏸️ PAUSE");
        pauseBtn.setTextColor(Color.WHITE);
        pauseBtn.setBackgroundColor(Color.argb(180, 20, 25, 40));
        pauseBtn.setTypeface(Typeface.MONOSPACE, Typeface.BOLD);
        pauseBtn.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);

        RelativeLayout.LayoutParams pauseParams = new RelativeLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
        );
        pauseParams.addRule(RelativeLayout.ALIGN_PARENT_LEFT);
        pauseParams.addRule(RelativeLayout.ALIGN_PARENT_TOP);
        pauseParams.setMargins(48, 48, 0, 0);
        pauseBtn.setLayoutParams(pauseParams);

        pauseBtn.setOnClickListener(v -> {
            isPaused = !isPaused;
            pauseBtn.setText(isPaused ? "▶️ RESUME" : "⏸️ PAUSE");
            nativeTogglePause();
        });

        uiOverlay.addView(calibrateBtn);
        uiOverlay.addView(pauseBtn);

        // Add UI Overlay on top of GameActivity View
        addContentView(uiOverlay, new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT
        ));
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (sensorManager != null) {
            if (accelerometer != null) {
                sensorManager.registerListener(this, accelerometer, SensorManager.SENSOR_DELAY_GAME);
            }
            if (gyroscope != null) {
                sensorManager.registerListener(this, gyroscope, SensorManager.SENSOR_DELAY_GAME);
            }
            if (magnetometer != null) {
                sensorManager.registerListener(this, magnetometer, SensorManager.SENSOR_DELAY_GAME);
            }
            if (rotationVector != null) {
                sensorManager.registerListener(this, rotationVector, SensorManager.SENSOR_DELAY_GAME);
            }
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
        if (sensorManager != null) {
            sensorManager.unregisterListener(this);
        }
    }

    @Override
    public void onSensorChanged(SensorEvent event) {
        int sensorType = event.sensor.getType();

        if (sensorType == Sensor.TYPE_ACCELEROMETER) {
            System.arraycopy(event.values, 0, accelValues, 0, 3);
        } else if (sensorType == Sensor.TYPE_GYROSCOPE) {
            System.arraycopy(event.values, 0, gyroValues, 0, 3);
        } else if (sensorType == Sensor.TYPE_MAGNETIC_FIELD) {
            System.arraycopy(event.values, 0, magValues, 0, 3);
        } else if (sensorType == Sensor.TYPE_ROTATION_VECTOR || sensorType == Sensor.TYPE_GAME_ROTATION_VECTOR) {
            SensorManager.getRotationMatrixFromVector(rotationMatrix, event.values);
            SensorManager.getOrientation(rotationMatrix, orientationAngles);

            yaw = (float) Math.toDegrees(orientationAngles[0]);
            pitch = (float) Math.toDegrees(orientationAngles[1]);
            roll = (float) Math.toDegrees(orientationAngles[2]);
        }

        try {
            updateNativeSensors(
                    accelValues[0], accelValues[1], accelValues[2],
                    gyroValues[0], gyroValues[1], gyroValues[2],
                    magValues[0], magValues[1], magValues[2],
                    pitch, roll, yaw
            );
        } catch (UnsatisfiedLinkError ignored) {
            // Native function not linked yet or optional
        }
    }

    @Override
    public void onAccuracyChanged(Sensor sensor, int accuracy) {
        // Not needed
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);

        if (hasFocus) {
            hideSystemUi();
        }
    }

    private void hideSystemUi() {
        WindowInsetsController insetsController = getWindow().getInsetsController();
        if (insetsController != null) {
            insetsController.hide(WindowInsets.Type.systemBars());
            insetsController.setSystemBarsBehavior(WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
        }
    }
}
