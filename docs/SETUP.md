# Toolchain setup

| Tool | Version | Status on dev PC |
|---|---|---|
| Unreal Engine | 5.8 (`C:\Program Files\Epic Games\UE_5.8`) | installed |
| Visual Studio | 2022 Community with "Game development with C++" | installed |
| Android target platform for UE 5.8 | Epic Launcher → UE 5.8 → Options → **Android** | installed |
| Android Studio | latest (SDK manager; its bundled Java 25 is too new for UE 5.8 Gradle 8.13) | installed via `winget install Google.AndroidStudio` |
| JDK 21 | Microsoft OpenJDK 21; `package_android.ps1` picks it up (`Use-AndroidToolchain` in `common.ps1`) | installed via `winget install Microsoft.OpenJDK.21` |
| Android SDK cmdline-tools | `%LOCALAPPDATA%\Android\Sdk\cmdline-tools\latest` | installed |
| SDK packages (UE 5.8 requirements) | platforms;android-36, build-tools;36.1.0, cmake;3.22.1, ndk;27.2.12479018 | via `SetupAndroid.bat` |

## One-time Android setup

```powershell
# accepts licences, then installs the SDK/NDK packages UE 5.8 expects and sets ANDROID_HOME / JAVA_HOME / NDKROOT
./Scripts/setup_android.ps1
```

Restart the terminal (and the editor) afterwards so the new environment variables apply.

## Signing (before the first Play upload)

1. Generate an upload keystore: `keytool -genkeypair -v -keystore doubleit-upload.jks -keyalg RSA -keysize 2048 -validity 10000 -alias upload`
2. Put it in `Build/Android/` (**git-ignored**) and fill in *Project Settings → Android → Distribution Signing*.
3. Enroll in Play App Signing (Google holds the app-signing key).
