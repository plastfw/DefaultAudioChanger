# Default Audio Changer

Default Audio Changer is a small Windows utility for switching the default audio output device.

It is useful when you frequently switch between speakers, headphones, headsets, HDMI audio, or other playback devices.

## Features

- Switch between selected playback devices.
- Optional global hotkey for quick switching.
- Tray application workflow.
- Modern Dear ImGui + Direct3D 11 interface.
- Dark compact UI.
- Keeps the original audio switching behavior while updating the interface.

## Requirements

- Windows 10/11 x64.
- DirectX 11.
- Visual Studio with C++ desktop development tools.

The app uses Windows audio APIs, including policy configuration interfaces that are not part of the stable public Win32 API surface. They work on supported Windows versions, but Microsoft may change this behavior in future Windows updates.

## Usage

1. Start `DefaultAudioChanger.exe`.
2. Select the playback devices you want to cycle through.
3. Optionally configure a global hotkey.
4. Hide the window or keep it open.
5. Run the app again or use the hotkey to switch to the next selected device.

## Build

Open the solution in Visual Studio:

```text
DefaultAudioChanger.sln
```

Build the desired configuration from Visual Studio.

The current UI uses Dear ImGui with a Direct3D 11 renderer. See `IMGUI_SETUP.md` for notes about the UI modernization.

## Repository contents

- `DefaultAudioChanger/` — application source code.
- `DefaultAudioChanger/imgui/` — bundled Dear ImGui sources.
- `make_releases.bat` — helper script for release builds.
- `changelog.txt` — project changelog.

## License

MIT — see [LICENSE](LICENSE).
