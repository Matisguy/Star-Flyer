# Star Flyer

A tilt-steered space flight game for Android and iOS, written in C++17 on SDL3. Rendering uses SDL's GPU API, which runs on Vulkan on Android and Metal on iOS.

## Layout

- `app/src/main/cpp/`: game code and renderer shared by both platforms. `GameSources.cmake` lists the shared sources.
- `app/src/main/cpp/AndroidBridge.cpp`: JNI entry points, Android only.
- `app/src/main/cpp/shaders/`: Vulkan GLSL, compiled to SPIR-V during the Android build by the NDK's `glslc`.
- `app/src/main/java/.../MainActivity.java`: Android menus, HUD and sensors, built on `SDLActivity`.
- `ios/`: UIKit menus and Core Motion input (`IosPlatform.mm`), plus the CMake definition of the Xcode project.
- `.github/workflows/ios-build.yml`: iOS compile and TestFlight upload.
- `.github/workflows/android-build.yml`: Android debug compile.

`main.cpp` uses SDL's main callbacks (`SDL_AppInit`, `SDL_AppIterate`, and so on). Do not add a blocking game loop, because iOS needs control returned each frame.

## Building and testing

- **Android:** open the project in Android Studio, or run `bash gradlew assembleDebug`. This requires the Android SDK and NDK. Gradle downloads SDL3 into `app/libs/` on first sync.
- **iOS:** requires macOS and Xcode, so it can't be built from Linux or WSL. Use GitHub Actions, or `cmake -S ios -B build/ios -G Xcode` on a Mac.
- There are no meaningful automated tests. `app/src/test` and `app/src/androidTest` hold only the Android Studio templates. Steering and rendering need checking on a physical device.

## CI and releases

- Every push to `master` or an `ios-*` branch archives the app and uploads a TestFlight build. Pull requests only compile. Treat a push to those branches as a release to testers.
- The iOS build number comes from `github.run_number`, so it must keep increasing and skipped numbers are normal.
- Upload signing uses the `ASC_KEY_ID`, `ASC_ISSUER_ID` and `ASC_KEY_P8` repository secrets. Never commit or print them.
- App Store processing can reject a build after a successful upload, and reports this by email. A green workflow doesn't guarantee acceptance.
- Keep CI changes in commits separate from code changes.

## Keeping the platforms in step

- **Shaders:** `shaders/scene.vert` and `scene.frag` must match the Metal source embedded in `Shader.cpp`. Change both together. The SPIR-V resource sets and Metal slots follow the layout documented for `SDL_CreateGPUShader`.
- **SDL version:** it's pinned with a checksum in both `app/build.gradle.kts` and `ios/CMakeLists.txt`. The two checksums differ because they cover different release files.
- **Menus:** Android (`MainActivity.java`) and iOS (`IosPlatform.mm`) implement the same screens separately. Apply menu changes to both. The Android state constants must match the `GameState` enum in `GameSettings.h`.
- **Sensors:** `OrientationTracker` expects Android's rotation-vector convention. `IosPlatform.mm` converts Core Motion data to match it.
- **Bundle IDs:** Android uses `com.example.starflyer` and iOS uses `com.woodedisland.starflyer`.

## Constraints

- Don't use OpenGL or OpenGL ES. Apple has deprecated them, and avoiding that risk is why the game uses SDL's GPU API.
- Projection matrices must produce clip-space depth in [0, 1], not OpenGL's [-1, 1].
- Don't submit GPU work while the app is in the background, because iOS terminates apps that do. `SDL_AppIterate` already guards against this.
- iOS builds compile out SDL's camera and HIDAPI code, because Apple rejects builds that reference those APIs without permission strings. Re-enabling them requires the matching `Info.plist` usage descriptions.
- iOS has no Exit button, because the system closes apps.
- Android requires Vulkan and declares this in `AndroidManifest.xml`. The optional GPU features are turned off in `Renderer::init` to reach more devices.
- Respect `.aiexclude`.
