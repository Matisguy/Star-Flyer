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
import android.widget.TextView;

import com.google.androidgamesdk.GameActivity;

import java.util.Locale;

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

    private TextView debugTextView;
    private long lastUiUpdateTimestamp = 0;
    private static final long UI_UPDATE_INTERVAL_MS = 50; // Update UI ~20 times per second

    // Native JNI call to bridge sensor data to C++ game logic
    public native void updateNativeSensors(
            float accelX, float accelY, float accelZ,
            float gyroX, float gyroY, float gyroZ,
            float magX, float magY, float magZ,
            float pitch, float roll, float yaw
    );

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
                // Fallback to game rotation vector if standard rotation vector is unavailable
                rotationVector = sensorManager.getDefaultSensor(Sensor.TYPE_GAME_ROTATION_VECTOR);
            }
        }

        // Create overlay TextView for debug menu
        debugTextView = new TextView(this);
        debugTextView.setTextColor(Color.GREEN);
        debugTextView.setBackgroundColor(Color.argb(180, 0, 0, 0)); // Semi-transparent black background
        debugTextView.setTextSize(TypedValue.COMPLEX_UNIT_SP, 13);
        debugTextView.setTypeface(Typeface.MONOSPACE);
        debugTextView.setPadding(32, 48, 32, 32);
        debugTextView.setText("Initializing Sensor Debug Menu...");

        // Add overlay on top of GameActivity native view
        addContentView(debugTextView, new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
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

            // Azimuth (yaw), Pitch, Roll in radians -> converted to degrees
            yaw = (float) Math.toDegrees(orientationAngles[0]);
            pitch = (float) Math.toDegrees(orientationAngles[1]);
            roll = (float) Math.toDegrees(orientationAngles[2]);
        }

        // Send sensor values to C++ native layer
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

        // Throttle UI update to ~20 FPS for optimal rendering performance
        long now = System.currentTimeMillis();
        if (now - lastUiUpdateTimestamp >= UI_UPDATE_INTERVAL_MS) {
            lastUiUpdateTimestamp = now;
            updateDebugDisplay();
        }
    }

    private void updateDebugDisplay() {
        if (debugTextView == null) return;

        String accelStatus = (accelerometer != null)
                ? String.format(Locale.US, "X:%+6.2f  Y:%+6.2f  Z:%+6.2f m/s²", accelValues[0], accelValues[1], accelValues[2])
                : "NOT AVAILABLE";

        String gyroStatus = (gyroscope != null)
                ? String.format(Locale.US, "X:%+6.2f  Y:%+6.2f  Z:%+6.2f rad/s", gyroValues[0], gyroValues[1], gyroValues[2])
                : "NOT AVAILABLE";

        String magStatus = (magnetometer != null)
                ? String.format(Locale.US, "X:%+6.1f  Y:%+6.1f  Z:%+6.1f μT", magValues[0], magValues[1], magValues[2])
                : "NOT AVAILABLE";

        String tiltStatus = (rotationVector != null)
                ? String.format(Locale.US, "Pitch:%+6.1f°  Roll:%+6.1f°  Yaw:%+6.1f°", pitch, roll, yaw)
                : "NOT AVAILABLE";

        String debugText = String.format(Locale.US,
                "\n \n === STAR FLYER SENSOR DEBUG ===\n" +
                "ACCEL: %s\n" +
                "GYRO : %s\n" +
                "MAG  : %s\n" +
                "TILT : %s",
                accelStatus, gyroStatus, magStatus, tiltStatus
        );

        debugTextView.setText(debugText);
    }

    @Override
    public void onAccuracyChanged(Sensor sensor, int accuracy) {
        // Not needed for basic readings
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