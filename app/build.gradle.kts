import java.net.URI
import java.security.MessageDigest
import java.util.zip.ZipFile

plugins {
    alias(libs.plugins.android.application)
}

// SDL3 isn't published to a Maven repository, so fetch the official Android archive once.
// Keep the version in step with ios/CMakeLists.txt.
val sdlVersion = "3.4.20"
val sdlZipSha256 = "535320e58f3bd7235b3e15f940595cdeeb9b8c0aa6b3810ff127345e3567d4da"
val sdlAar = file("libs/SDL3-$sdlVersion.aar")

if (!sdlAar.exists()) {
    val url = "https://github.com/libsdl-org/SDL/releases/download/" +
        "release-$sdlVersion/SDL3-devel-$sdlVersion-android.zip"
    val zipFile = File.createTempFile("sdl3-android", ".zip")
    try {
        URI(url).toURL().openStream().use { input ->
            zipFile.outputStream().use { input.copyTo(it) }
        }
        val actualSha256 = MessageDigest.getInstance("SHA-256")
            .digest(zipFile.readBytes())
            .joinToString("") { "%02x".format(it) }
        check(actualSha256 == sdlZipSha256) { "SDL3 download failed its checksum: $url" }

        sdlAar.parentFile.mkdirs()
        ZipFile(zipFile).use { zip ->
            zip.getInputStream(zip.getEntry(sdlAar.name)).use { input ->
                sdlAar.outputStream().use { input.copyTo(it) }
            }
        }
    } finally {
        zipFile.delete()
    }
}

android {
    namespace = "com.example.starflyer"
    compileSdk {
        version = release(37)
    }

    defaultConfig {
        applicationId = "com.example.starflyer"
        minSdk = 31
        targetSdk = 37
        versionCode = 1
        versionName = "1.0"

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
    }

    buildTypes {
        release {
            optimization {
                enable = true
                packageScope = setOf("androidx.**", "kotlin.**", "kotlinx.**")
            }
        }
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_11
        targetCompatibility = JavaVersion.VERSION_11
    }
    buildFeatures {
        prefab = true
    }
    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }
}

dependencies {
    implementation(libs.appcompat)
    implementation(files(sdlAar))
    implementation(libs.material)
    testImplementation(libs.junit)
    androidTestImplementation(libs.espresso.core)
    androidTestImplementation(libs.ext.junit)
}