// JNI entry points called by MainActivity. Android only.
#include <jni.h>

#include "GameSettings.h"
#include "SensorData.h"

static JavaVM* gJavaVM = nullptr;
static jobject gMainActivityObj = nullptr;

extern "C" {

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    gJavaVM = vm;
    return JNI_VERSION_1_6;
}

void triggerAndroidBanner(const char* message) {
    if (!gJavaVM || !gMainActivityObj) return;
    JNIEnv* env = nullptr;
    jint getEnvRes = gJavaVM->GetEnv((void**)&env, JNI_VERSION_1_6);
    bool attached = false;
    if (getEnvRes == JNI_EDETACHED) {
        if (gJavaVM->AttachCurrentThread(&env, nullptr) == JNI_OK) {
            attached = true;
        } else {
            return;
        }
    }
    if (env) {
        jclass clazz = env->GetObjectClass(gMainActivityObj);
        jmethodID method = env->GetMethodID(clazz, "showFloatingBanner", "(Ljava/lang/String;F)V");
        if (method) {
            jstring jmsg = env->NewStringUTF(message);
            env->CallVoidMethod(gMainActivityObj, method, jmsg, 2.0f);
            env->DeleteLocalRef(jmsg);
        }
    }
    if (attached) {
        gJavaVM->DetachCurrentThread();
    }
}

JNIEXPORT void JNICALL
Java_com_example_starflyer_MainActivity_nativeRequestCalibration(
        JNIEnv *env, jobject thiz) {
    gGameSettings.calibrationRequested = true;
}

JNIEXPORT void JNICALL
Java_com_example_starflyer_MainActivity_nativeTogglePause(
        JNIEnv *env, jobject thiz) {
    applyGameState(gGameSettings.isPaused ? GameState::PLAYING : GameState::PAUSED);
}

JNIEXPORT void JNICALL
Java_com_example_starflyer_MainActivity_nativeSetGameState(
        JNIEnv *env, jobject thiz, jint state) {
    if (gMainActivityObj) {
        env->DeleteGlobalRef(gMainActivityObj);
    }
    gMainActivityObj = env->NewGlobalRef(thiz);
    applyGameState(static_cast<GameState>(state));
}

JNIEXPORT void JNICALL
Java_com_example_starflyer_MainActivity_nativeSetAudioSettings(
        JNIEnv *env, jobject thiz, jfloat musicVol, jfloat sfxVol) {
    gGameSettings.musicVolume = musicVol;
    gGameSettings.sfxVolume = sfxVol;
}

JNIEXPORT void JNICALL
Java_com_example_starflyer_MainActivity_nativeSetSensitivity(
        JNIEnv *env, jobject thiz, jfloat turnSens) {
    gGameSettings.turnSensitivity = turnSens;
}

JNIEXPORT void JNICALL
Java_com_example_starflyer_MainActivity_updateNativeSensors(
        JNIEnv *env, jobject thiz,
        jfloat gravX, jfloat gravY, jfloat gravZ,
        jfloat gyroX, jfloat gyroY, jfloat gyroZ,
        jfloat rotX, jfloat rotY, jfloat rotZ, jfloat rotW) {
    std::lock_guard<std::mutex> lock(gSensorMutex);
    gSensorData.grav[0] = gravX;
    gSensorData.grav[1] = gravY;
    gSensorData.grav[2] = gravZ;

    gSensorData.gyro[0] = gyroX;
    gSensorData.gyro[1] = gyroY;
    gSensorData.gyro[2] = gyroZ;

    gSensorData.orientation[0] = rotX;
    gSensorData.orientation[1] = rotY;
    gSensorData.orientation[2] = rotZ;
    gSensorData.orientation[3] = rotW;
}

}
