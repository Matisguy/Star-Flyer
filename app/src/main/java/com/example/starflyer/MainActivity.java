package com.example.starflyer;

import android.app.AlertDialog;
import android.content.pm.ActivityInfo;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.BitmapDrawable;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.os.Bundle;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowInsets;
import android.view.WindowInsetsController;
import android.widget.Button;
import android.widget.GridLayout;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.SeekBar;
import android.widget.TextView;

import com.google.androidgamesdk.GameActivity;

import java.io.IOException;
import java.io.InputStream;
import java.util.Locale;

public class MainActivity extends GameActivity implements SensorEventListener {
    static {
        System.loadLibrary("starflyer");
    }

    public static final int STATE_MAIN_MENU = 0;
    public static final int STATE_LEVEL_SELECT = 1;
    public static final int STATE_PLAYING = 2;
    public static final int STATE_PAUSED = 3;
    public static final int STATE_WORKSHOP = 4;

    private SensorManager sensorManager;
    private Sensor gravity;
    private Sensor gyroscope;
    private Sensor rotationVector;

    private final float[] gravValues = new float[3];
    private final float[] gyroValues = new float[3];
    private final float[] rotationMatrix = new float[9];
    private final float[] orientationQuaternion = new float[4];

    // UI View Containers
    private RelativeLayout rootOverlay;
    private LinearLayout mainMenuContainer;
    private LinearLayout levelSelectContainer;
    private LinearLayout workshopContainer;
    private LinearLayout pauseContainer;
    private RelativeLayout inGameHudContainer;

    // Audio & Settings State
    private float musicVolume = 0.8f;
    private float sfxVolume = 1.0f;
    float turnSensitivity = 3.0f;

    // Native JNI functions
    public native void updateNativeSensors(
            float gravX, float gravY, float gravZ,
            float gyroX, float gyroY, float gyroZ,
            float rotX, float rotY, float rotZ, float rotW
    );

    public native void nativeRequestCalibration();
    public native void nativeTogglePause();
    public native void nativeSetGameState(int state);
    public native void nativeSetAudioSettings(float musicVol, float sfxVol);
    public native void nativeSetSensitivity(float turnSens);

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Enforce Sensor Landscape orientation for flight simulator
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE);

        // Setup Sensor Manager
        sensorManager = (SensorManager) getSystemService(SENSOR_SERVICE);
        if (sensorManager != null) {
            gravity = sensorManager.getDefaultSensor(Sensor.TYPE_GRAVITY);
            gyroscope = sensorManager.getDefaultSensor(Sensor.TYPE_GYROSCOPE);
            rotationVector = sensorManager.getDefaultSensor(Sensor.TYPE_ROTATION_VECTOR);
            if (rotationVector == null) {
                rotationVector = sensorManager.getDefaultSensor(Sensor.TYPE_GAME_ROTATION_VECTOR);
            }
        }

        // Create Root UI Overlay Container
        rootOverlay = new RelativeLayout(this);
        rootOverlay.setLayoutParams(new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT
        ));

        // Create Landscape UI Screens
        createMainMenuScreen();
        createLevelSelectScreen();
        createWorkshopScreen();
        createPauseScreen();
        createInGameHudScreen();

        // Add root container on top of GameActivity native surface
        addContentView(rootOverlay, new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT
        ));

        // Show Main Menu by default
        setGameState(STATE_MAIN_MENU);
    }

    private void setGameState(int state) {
        nativeSetGameState(state);

        mainMenuContainer.setVisibility(state == STATE_MAIN_MENU ? View.VISIBLE : View.GONE);
        levelSelectContainer.setVisibility(state == STATE_LEVEL_SELECT ? View.VISIBLE : View.GONE);
        workshopContainer.setVisibility(state == STATE_WORKSHOP ? View.VISIBLE : View.GONE);
        pauseContainer.setVisibility(state == STATE_PAUSED ? View.VISIBLE : View.GONE);
        inGameHudContainer.setVisibility(state == STATE_PLAYING ? View.VISIBLE : View.GONE);
    }

    // Asset Fallback Helper: Loads custom asset from assets/ui/ if present, otherwise falls back to system styling
    private void applyAssetOrFallback(Button button, String assetFileName, String defaultText, int defaultTextColor) {
        boolean loaded = false;
        try {
            InputStream is = getAssets().open("ui/" + assetFileName);
            Bitmap bitmap = BitmapFactory.decodeStream(is);
            if (bitmap != null) {
                BitmapDrawable drawable = new BitmapDrawable(getResources(), bitmap);
                button.setBackground(drawable);
                button.setText(""); // Use graphical button asset
                loaded = true;
            }
            is.close();
        } catch (IOException ignored) {
            // Asset not found; fallback to procedural default styling
        }

        if (!loaded) {
            button.setText(defaultText);
            button.setTextColor(defaultTextColor);
            button.setBackgroundColor(Color.argb(180, 20, 25, 40));
            button.setTypeface(Typeface.MONOSPACE, Typeface.BOLD);
            button.setTextSize(TypedValue.COMPLEX_UNIT_SP, 13);
        }
    }

    // 1. MAIN MENU SCREEN (LANDSCAPE GRID)
    private void createMainMenuScreen() {
        mainMenuContainer = new LinearLayout(this);
        mainMenuContainer.setOrientation(LinearLayout.VERTICAL);
        mainMenuContainer.setGravity(Gravity.CENTER);
        mainMenuContainer.setBackgroundColor(Color.argb(160, 5, 10, 20));
        mainMenuContainer.setPadding(32, 24, 32, 24);

        RelativeLayout.LayoutParams params = new RelativeLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT
        );
        mainMenuContainer.setLayoutParams(params);

        // Title
        TextView title = new TextView(this);
        title.setText("STAR FLYER");
        title.setTextColor(Color.CYAN);
        title.setTextSize(TypedValue.COMPLEX_UNIT_SP, 32);
        title.setTypeface(Typeface.MONOSPACE, Typeface.BOLD);
        title.setGravity(Gravity.CENTER);

        TextView subtitle = new TextView(this);
        subtitle.setText("SCI-FI FLIGHT SIMULATOR");
        subtitle.setTextColor(Color.LTGRAY);
        subtitle.setTextSize(TypedValue.COMPLEX_UNIT_SP, 12);
        subtitle.setTypeface(Typeface.MONOSPACE);
        subtitle.setGravity(Gravity.CENTER);
        subtitle.setPadding(0, 0, 0, 20);

        // Landscape 2-Column Grid Layout for buttons
        GridLayout grid = new GridLayout(this);
        grid.setColumnCount(2);
        grid.setRowCount(2);
        grid.setAlignmentMode(GridLayout.ALIGN_BOUNDS);

        Button playBtn = new Button(this);
        applyAssetOrFallback(playBtn, "btn_play.png", "🚀 PLAY GAME", Color.CYAN);
        playBtn.setOnClickListener(v -> setGameState(STATE_LEVEL_SELECT));

        Button workshopBtn = new Button(this);
        applyAssetOrFallback(workshopBtn, "btn_workshop.png", "🛠️ SHIP WORKSHOP", Color.WHITE);
        workshopBtn.setOnClickListener(v -> setGameState(STATE_WORKSHOP));

        Button settingsBtn = new Button(this);
        applyAssetOrFallback(settingsBtn, "btn_settings.png", "⚙️ SETTINGS", Color.WHITE);
        settingsBtn.setOnClickListener(v -> showSettingsDialog());

        Button exitBtn = new Button(this);
        applyAssetOrFallback(exitBtn, "btn_exit.png", "🚪 EXIT GAME", Color.argb(255, 255, 90, 90));
        exitBtn.setOnClickListener(v -> showExitConfirmationDialog());

        // Grid Specs for 2x2 Landscape Layout
        int buttonMargin = 12;
        GridLayout.LayoutParams p1 = new GridLayout.LayoutParams(GridLayout.spec(0), GridLayout.spec(0));
        p1.setMargins(buttonMargin, buttonMargin, buttonMargin, buttonMargin);
        p1.width = 340; p1.height = 110;
        playBtn.setLayoutParams(p1);

        GridLayout.LayoutParams p2 = new GridLayout.LayoutParams(GridLayout.spec(0), GridLayout.spec(1));
        p2.setMargins(buttonMargin, buttonMargin, buttonMargin, buttonMargin);
        p2.width = 340; p2.height = 110;
        workshopBtn.setLayoutParams(p2);

        GridLayout.LayoutParams p3 = new GridLayout.LayoutParams(GridLayout.spec(1), GridLayout.spec(0));
        p3.setMargins(buttonMargin, buttonMargin, buttonMargin, buttonMargin);
        p3.width = 340; p3.height = 110;
        settingsBtn.setLayoutParams(p3);

        GridLayout.LayoutParams p4 = new GridLayout.LayoutParams(GridLayout.spec(1), GridLayout.spec(1));
        p4.setMargins(buttonMargin, buttonMargin, buttonMargin, buttonMargin);
        p4.width = 340; p4.height = 110;
        exitBtn.setLayoutParams(p4);

        grid.addView(playBtn);
        grid.addView(workshopBtn);
        grid.addView(settingsBtn);
        grid.addView(exitBtn);

        mainMenuContainer.addView(title);
        mainMenuContainer.addView(subtitle);
        mainMenuContainer.addView(grid);

        rootOverlay.addView(mainMenuContainer);
    }

    // 2. LEVEL SELECT SCREEN (LANDSCAPE HORIZONTAL)
    private void createLevelSelectScreen() {
        levelSelectContainer = new LinearLayout(this);
        levelSelectContainer.setOrientation(LinearLayout.VERTICAL);
        levelSelectContainer.setGravity(Gravity.CENTER);
        levelSelectContainer.setBackgroundColor(Color.argb(180, 5, 10, 20));
        levelSelectContainer.setPadding(32, 20, 32, 20);

        RelativeLayout.LayoutParams params = new RelativeLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT
        );
        levelSelectContainer.setLayoutParams(params);

        TextView header = new TextView(this);
        header.setText("SELECT MISSION");
        header.setTextColor(Color.CYAN);
        header.setTextSize(TypedValue.COMPLEX_UNIT_SP, 22);
        header.setTypeface(Typeface.MONOSPACE, Typeface.BOLD);
        header.setGravity(Gravity.CENTER);
        header.setPadding(0, 0, 0, 16);

        // Horizontal Row for Level Cards
        LinearLayout levelRow = new LinearLayout(this);
        levelRow.setOrientation(LinearLayout.HORIZONTAL);
        levelRow.setGravity(Gravity.CENTER);

        Button level0Btn = new Button(this);
        applyAssetOrFallback(level0Btn, "level_0.png", "MISSION 0:\nDEBUG DEBRIS\n(UNLOCKED)", Color.GREEN);
        level0Btn.setOnClickListener(v -> {
            nativeRequestCalibration();
            setGameState(STATE_PLAYING);
        });

        Button level1Btn = new Button(this);
        applyAssetOrFallback(level1Btn, "level_1.png", "MISSION 1:\nLUNAR BASE\n(LOCKED 🔒)", Color.GRAY);
        level1Btn.setEnabled(false);

        Button level2Btn = new Button(this);
        applyAssetOrFallback(level2Btn, "level_2.png", "MISSION 2:\nAD CAELOS\n(LOCKED 🔒)", Color.GRAY);
        level2Btn.setEnabled(false);

        LinearLayout.LayoutParams cardParams = new LinearLayout.LayoutParams(220, 150);
        cardParams.setMargins(12, 0, 12, 0);
        level0Btn.setLayoutParams(cardParams);
        level1Btn.setLayoutParams(cardParams);
        level2Btn.setLayoutParams(cardParams);

        levelRow.addView(level0Btn);
        levelRow.addView(level1Btn);
        levelRow.addView(level2Btn);

        Button backBtn = new Button(this);
        applyAssetOrFallback(backBtn, "btn_back.png", "⬅️ BACK TO MAIN MENU", Color.WHITE);
        LinearLayout.LayoutParams backParams = new LinearLayout.LayoutParams(400, 90);
        backParams.setMargins(0, 16, 0, 0);
        backBtn.setLayoutParams(backParams);
        backBtn.setOnClickListener(v -> setGameState(STATE_MAIN_MENU));

        levelSelectContainer.addView(header);
        levelSelectContainer.addView(levelRow);
        levelSelectContainer.addView(backBtn);

        rootOverlay.addView(levelSelectContainer);
    }

    // 3. SHIP WORKSHOP SCREEN (LANDSCAPE SIDE-BY-SIDE)
    private void createWorkshopScreen() {
        workshopContainer = new LinearLayout(this);
        workshopContainer.setOrientation(LinearLayout.VERTICAL);
        workshopContainer.setGravity(Gravity.CENTER);
        workshopContainer.setBackgroundColor(Color.argb(190, 5, 12, 25));
        workshopContainer.setPadding(32, 20, 32, 20);

        RelativeLayout.LayoutParams params = new RelativeLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT
        );
        workshopContainer.setLayoutParams(params);

        TextView header = new TextView(this);
        header.setText("HANGAR & SHIP WORKSHOP");
        header.setTextColor(Color.CYAN);
        header.setTextSize(TypedValue.COMPLEX_UNIT_SP, 22);
        header.setTypeface(Typeface.MONOSPACE, Typeface.BOLD);
        header.setGravity(Gravity.CENTER);
        header.setPadding(0, 0, 0, 16);

        // Side-by-Side Panel Layout
        LinearLayout sideBySide = new LinearLayout(this);
        sideBySide.setOrientation(LinearLayout.HORIZONTAL);
        sideBySide.setGravity(Gravity.CENTER);

        TextView statsCard = new TextView(this);
        statsCard.setText(
                "--- SHIP STATS & CONFIG ---\n" +
                "HULL SHIELDS   : 100 HP [MAX]\n" +
                "THRUST POWER   : 6.0 u/s\n" +
                "TURNING AGILITY: 100%\n" +
                "STATUS         : COMBAT READY"
        );
        statsCard.setTextColor(Color.GREEN);
        statsCard.setTextSize(TypedValue.COMPLEX_UNIT_SP, 12);
        statsCard.setTypeface(Typeface.MONOSPACE);
        statsCard.setBackgroundColor(Color.argb(160, 10, 20, 35));
        statsCard.setPadding(24, 20, 24, 20);

        LinearLayout.LayoutParams statsParams = new LinearLayout.LayoutParams(380, 160);
        statsParams.setMargins(16, 0, 16, 0);
        statsCard.setLayoutParams(statsParams);

        Button backBtn = new Button(this);
        applyAssetOrFallback(backBtn, "btn_back.png", "⬅️ BACK TO MAIN MENU", Color.WHITE);
        LinearLayout.LayoutParams backParams = new LinearLayout.LayoutParams(260, 110);
        backParams.setMargins(16, 0, 16, 0);
        backBtn.setLayoutParams(backParams);
        backBtn.setOnClickListener(v -> setGameState(STATE_MAIN_MENU));

        sideBySide.addView(statsCard);
        sideBySide.addView(backBtn);

        workshopContainer.addView(header);
        workshopContainer.addView(sideBySide);

        rootOverlay.addView(workshopContainer);
    }

    // 4. PAUSE MENU SCREEN (LANDSCAPE COMPACT GRID)
    private void createPauseScreen() {
        pauseContainer = new LinearLayout(this);
        pauseContainer.setOrientation(LinearLayout.VERTICAL);
        pauseContainer.setGravity(Gravity.CENTER);
        pauseContainer.setBackgroundColor(Color.argb(180, 5, 10, 20));
        pauseContainer.setPadding(32, 20, 32, 20);

        RelativeLayout.LayoutParams params = new RelativeLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT
        );
        pauseContainer.setLayoutParams(params);

        TextView header = new TextView(this);
        header.setText("GAME PAUSED");
        header.setTextColor(Color.YELLOW);
        header.setTextSize(TypedValue.COMPLEX_UNIT_SP, 24);
        header.setTypeface(Typeface.MONOSPACE, Typeface.BOLD);
        header.setGravity(Gravity.CENTER);
        header.setPadding(0, 0, 0, 16);

        GridLayout grid = new GridLayout(this);
        grid.setColumnCount(2);
        grid.setRowCount(2);

        Button resumeBtn = new Button(this);
        applyAssetOrFallback(resumeBtn, "btn_resume.png", "▶️ RESUME GAME", Color.GREEN);
        resumeBtn.setOnClickListener(v -> setGameState(STATE_PLAYING));

        Button calibBtn = new Button(this);
        applyAssetOrFallback(calibBtn, "btn_calibrate.png", "🎯 CALIBRATE", Color.CYAN);
        calibBtn.setOnClickListener(v -> nativeRequestCalibration());

        Button settingsBtn = new Button(this);
        applyAssetOrFallback(settingsBtn, "btn_settings.png", "⚙️ SETTINGS", Color.WHITE);
        settingsBtn.setOnClickListener(v -> showSettingsDialog());

        Button quitMenuBtn = new Button(this);
        applyAssetOrFallback(quitMenuBtn, "btn_quit.png", "🏠 QUIT TO MAIN MENU", Color.argb(255, 255, 100, 100));
        quitMenuBtn.setOnClickListener(v -> setGameState(STATE_MAIN_MENU));

        int buttonMargin = 10;
        GridLayout.LayoutParams p1 = new GridLayout.LayoutParams(GridLayout.spec(0), GridLayout.spec(0));
        p1.setMargins(buttonMargin, buttonMargin, buttonMargin, buttonMargin);
        p1.width = 300; p1.height = 95;
        resumeBtn.setLayoutParams(p1);

        GridLayout.LayoutParams p2 = new GridLayout.LayoutParams(GridLayout.spec(0), GridLayout.spec(1));
        p2.setMargins(buttonMargin, buttonMargin, buttonMargin, buttonMargin);
        p2.width = 300; p2.height = 95;
        calibBtn.setLayoutParams(p2);

        GridLayout.LayoutParams p3 = new GridLayout.LayoutParams(GridLayout.spec(1), GridLayout.spec(0));
        p3.setMargins(buttonMargin, buttonMargin, buttonMargin, buttonMargin);
        p3.width = 300; p3.height = 95;
        settingsBtn.setLayoutParams(p3);

        GridLayout.LayoutParams p4 = new GridLayout.LayoutParams(GridLayout.spec(1), GridLayout.spec(1));
        p4.setMargins(buttonMargin, buttonMargin, buttonMargin, buttonMargin);
        p4.width = 300; p4.height = 95;
        quitMenuBtn.setLayoutParams(p4);

        grid.addView(resumeBtn);
        grid.addView(calibBtn);
        grid.addView(settingsBtn);
        grid.addView(quitMenuBtn);

        pauseContainer.addView(header);
        pauseContainer.addView(grid);

        rootOverlay.addView(pauseContainer);
    }

    // 5. IN-GAME HUD OVERLAY
    private void createInGameHudScreen() {
        inGameHudContainer = new RelativeLayout(this);
        inGameHudContainer.setLayoutParams(new RelativeLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT
        ));

        // Pause Button (Top Left)
        Button pauseBtn = new Button(this);
        applyAssetOrFallback(pauseBtn, "hud_pause.png", "⏸️ PAUSE", Color.WHITE);
        RelativeLayout.LayoutParams pauseParams = new RelativeLayout.LayoutParams(180, 80);
        pauseParams.addRule(RelativeLayout.ALIGN_PARENT_LEFT);
        pauseParams.addRule(RelativeLayout.ALIGN_PARENT_TOP);
        pauseParams.setMargins(32, 24, 0, 0);
        pauseBtn.setLayoutParams(pauseParams);
        pauseBtn.setOnClickListener(v -> setGameState(STATE_PAUSED));

        // Calibrate Button (Top Right)
        Button calibBtn = new Button(this);
        applyAssetOrFallback(calibBtn, "hud_calibrate.png", "🎯 CALIBRATE", Color.CYAN);
        RelativeLayout.LayoutParams calParams = new RelativeLayout.LayoutParams(220, 80);
        calParams.addRule(RelativeLayout.ALIGN_PARENT_RIGHT);
        calParams.addRule(RelativeLayout.ALIGN_PARENT_TOP);
        calParams.setMargins(0, 24, 32, 0);
        calibBtn.setLayoutParams(calParams);
        calibBtn.setOnClickListener(v -> nativeRequestCalibration());

        inGameHudContainer.addView(pauseBtn);
        inGameHudContainer.addView(calibBtn);

        rootOverlay.addView(inGameHudContainer);
    }

    // SETTINGS OVERLAY DIALOG
    private void showSettingsDialog() {
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle("AUDIO & CONTROL SETTINGS");

        LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.VERTICAL);
        layout.setPadding(48, 24, 48, 24);

        // Music Volume
        TextView musicLabel = new TextView(this);
        musicLabel.setText(String.format(Locale.US, "Music Volume: %d%%", (int)(musicVolume * 100)));
        musicLabel.setTypeface(Typeface.MONOSPACE);

        SeekBar musicSeekBar = new SeekBar(this);
        musicSeekBar.setMax(100);
        musicSeekBar.setProgress((int)(musicVolume * 100));
        musicSeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                musicVolume = progress / 100.0f;
                musicLabel.setText(String.format(Locale.US, "Music Volume: %d%%", progress));
                nativeSetAudioSettings(musicVolume, sfxVolume);
            }
            @Override public void onStartTrackingTouch(SeekBar seekBar) {}
            @Override public void onStopTrackingTouch(SeekBar seekBar) {}
        });

        // SFX Volume
        TextView sfxLabel = new TextView(this);
        sfxLabel.setText(String.format(Locale.US, "SFX Volume: %d%%", (int)(sfxVolume * 100)));
        sfxLabel.setTypeface(Typeface.MONOSPACE);
        sfxLabel.setPadding(0, 16, 0, 0);

        SeekBar sfxSeekBar = new SeekBar(this);
        sfxSeekBar.setMax(100);
        sfxSeekBar.setProgress((int)(sfxVolume * 100));
        sfxSeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                sfxVolume = progress / 100.0f;
                sfxLabel.setText(String.format(Locale.US, "SFX Volume: %d%%", progress));
                nativeSetAudioSettings(musicVolume, sfxVolume);
            }
            @Override public void onStartTrackingTouch(SeekBar seekBar) {}
            @Override public void onStopTrackingTouch(SeekBar seekBar) {}
        });

        // Steering Sensitivity
        TextView sensLabel = new TextView(this);
        sensLabel.setText(String.format(Locale.US, "Steering Sensitivity: %.1fx", turnSensitivity));
        sensLabel.setTypeface(Typeface.MONOSPACE);
        sensLabel.setPadding(0, 16, 0, 0);

        SeekBar sensSeekBar = new SeekBar(this);
        sensSeekBar.setMax(100);
        sensSeekBar.setProgress((int)((turnSensitivity - 1.0f) * 10.0f));
        sensSeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                turnSensitivity = 1.0f + (progress / 10.0f);
                sensLabel.setText(String.format(Locale.US, "Steering Sensitivity: %.1fx", turnSensitivity));
                nativeSetSensitivity(turnSensitivity);
            }
            @Override public void onStartTrackingTouch(SeekBar seekBar) {}
            @Override public void onStopTrackingTouch(SeekBar seekBar) {}
        });

        layout.addView(musicLabel);
        layout.addView(musicSeekBar);
        layout.addView(sfxLabel);
        layout.addView(sfxSeekBar);
        layout.addView(sensLabel);
        layout.addView(sensSeekBar);

        builder.setView(layout);
        builder.setPositiveButton("SAVE & CLOSE", (dialog, which) -> dialog.dismiss());
        builder.show();
    }

    // EXIT CONFIRMATION DIALOG
    private void showExitConfirmationDialog() {
        new AlertDialog.Builder(this)
                .setTitle("EXIT STAR FLYER")
                .setMessage("Are you sure you want to exit the application?")
                .setPositiveButton("YES, EXIT", (dialog, which) -> finish())
                .setNegativeButton("CANCEL", (dialog, which) -> dialog.dismiss())
                .show();
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (sensorManager != null) {
            if (gravity != null) {
                sensorManager.registerListener(this, gravity, SensorManager.SENSOR_DELAY_GAME);
            }
            if (gyroscope != null) {
                sensorManager.registerListener(this, gyroscope, SensorManager.SENSOR_DELAY_GAME);
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

        if (sensorType == Sensor.TYPE_GRAVITY) {
            System.arraycopy(event.values, 0, gravValues, 0, 3);
        } else if (sensorType == Sensor.TYPE_GYROSCOPE) {
            System.arraycopy(event.values, 0, gyroValues, 0, 3);
        } else if (sensorType == Sensor.TYPE_ROTATION_VECTOR || sensorType == Sensor.TYPE_GAME_ROTATION_VECTOR) {
            System.arraycopy(event.values, 0, orientationQuaternion, 0, 4);
            SensorManager.getRotationMatrixFromVector(rotationMatrix, event.values);
            // Determine active landscape display orientation (90° vs 270°)
            int displayRot = android.view.Surface.ROTATION_90;
            if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.R && getDisplay() != null) {
                displayRot = getDisplay().getRotation();
            } else if (getWindowManager() != null && getWindowManager().getDefaultDisplay() != null) {
                displayRot = getWindowManager().getDefaultDisplay().getRotation();
            }
            if (displayRot == android.view.Surface.ROTATION_270) {
                orientationQuaternion[0] *= -1.0f;
                orientationQuaternion[1] *= -1.0f;
            }
        }

        try {
            updateNativeSensors(
                    gravValues[0], gravValues[1], gravValues[2],
                    gyroValues[0], gyroValues[1], gyroValues[2],
                    orientationQuaternion[0], orientationQuaternion[1], orientationQuaternion[2], orientationQuaternion[3]
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
