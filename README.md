# Screen Translator

**The project is almost abandoned. I don't have time for it and I can only fix minor issues**

## Introduction

This software allows you to translate any text on screen.
Basically it is a combination of screen capture, OCR and translation tools.
Translation can be done via online services (Google, DeepL, Bing) or via local/remote AI (Ollama, OpenAI-compatible APIs).

## Installation

**Windows**: download archive from [github releases](https://github.com/OneMoreGres/ScreenTranslator/releases) page, extract it and run `.exe` file.

If the app fails to start complaining about missing dlls or there are any update errors related to SSL/TLS then install or repair `vs_redist*.exe` from the release archive.

**Linux**: download `.AppImage` file from [github releases](https://github.com/OneMoreGres/ScreenTranslator/releases), make executable (`chmod +x <file>`) and run it.

**OS X**: currently not supported.

## Setup

The app doesn't have a main window.
After start it shows only the tray icon.

If the app detects invalid settings, it will show the error message via system tray.
It will also highlight the section name in red on the left panel of the settings window.
Clicking on that section name will show a more detailed error message in the right panel (also in red).

The packages downloaded from this site do not include resources, such as recognition language packs or scripts to interact with online translation services.

To download them, open the settings window and go to the `Update` section.
In the right panel, expand the `recognizers` and `translators` sections.
Select preferred items, then right click and choose `Install/Update`.
After the progress bar reaches `100%`, the resource's state will change to `Up to Date`.

You must download at least one `recognizers` resource and one `translators` resource.

After finishing downloads, go to the `Recognition` section and update the default recognition language setting (the source to be translated).
Then go to the `Translation` section, update the default translation language setting (the language to be translated into) and enable some or all translation sevices (you may also change their order by dragging).

After that all sections in the left panel should be black.
Then click `Ok` to close settings.

### Third party enhancements

**Not tested or reviewed by me**

* to translate with online AI services use scripts from [here](https://github.com/Suki8898/Translator)

* to install Hebrew translation of the app itself (thanks to [Y-PLONI](https://github.com/Y-PLONI)),
download [this](https://github.com/OneMoreGres/ScreenTranslator/releases/download/3.3.0/screentranslator_he.qm)
file and place it into the `translations` folder next to `screen-translator.exe`.

## Usage

1. Run program (note that it doesn't have main window).
2. Press capture hotkey.
3. Select region on screen. Customize it if needed.
4. Get translation of recognized text.
5. Check for updates if something is not working.

## FAQ

By default resources are downloaded to the one of the user's folders.
If `Portable` setting in `General` section is checked, then resources will be downloaded to the app's folder.

Set `QTWEBENGINE_DISABLE_SANDBOX=1` environment variable when fail to start due to crash.

Answers to some frequently asked questions can be found in issues or
[wiki](https://github.com/OneMoreGres/ScreenTranslator/wiki/FAQ)

## Limitations

* Can not capture DRM-protected content (Netflix, etc.) — Widevine/HDCP block capture path
* On Wayland, fullscreen hardware-accelerated video may not be capturable on all compositors; use `xdg-desktop-portal` based screen capture for best results
* JavaScript-based translators (Google/DeepL/Bing) depend on the current DOM structure of the service's web UI; if the page changes the selectors must be updated in `translators/*.js`
* AI translators (Ollama / OpenAI compatible) require a running service and produce latency proportional to model size

## Translators

### Web scraping (no API key required)

| Service | File | Notes |
|---|---|---|
| Google  | `translators/google.js`  | Auto-detect source, modern DOM selector chain |
| DeepL   | `translators/deepl.js`   | Free tier via web UI |
| Bing    | `translators/bing.js`    | Microsoft Translator web UI |
| Google Translate API | `translators/google_api.js` | Public endpoint, may break |

### AI (HTTP)

Configured in `Settings → Translation → AI Translation`:

* **Ollama** (local): `POST http://localhost:11434/api/generate`. The model list is auto-discovered via `GET /api/tags` when you click *Refresh*. Set `ollamaUrl` to a remote endpoint if needed.
* **OpenAI compatible**: `POST {endpoint}/v1/chat/completions`. Works with OpenAI, LM Studio, vLLM, Groq, OpenRouter. API key stored with the same obfuscation as the proxy password.

To add an AI translator to the enabled list: pick the model, then click `Add`. The entry becomes `ollama:<model>` or `openai:<model>` in the translators list.

## Subtitle mode (live video translation)

A second capture mode for translating hardcoded subtitles from running video:

1. Press the *Subtitle mode* hotkey (`Ctrl+Alt+V` by default) or pick *Toggle subtitle mode* in the tray menu.
2. Draw a rectangle around the subtitle area of the running video.
3. The app captures the region every 500ms (configurable), runs OCR and translation, and shows the result.
4. Press the hotkey again (or `Esc` while the selector is up) to stop.

Best on X11 with compositing. Hardware-accelerated fullscreen video on Wayland may not be captured.

## Dependencies

* see [Qt 5](https://qt-project.org/)
* see [Tesseract](https://github.com/tesseract-ocr/tesseract/)
* see [Leptonica](https://leptonica.com/)
* several online translation services

## Build from source

Look at the scripts (python3) in the `share/ci` folder.
Normally, you should only edit the `config.py` file.

Build dependencies at first, then build the app.

### Build on Linux

```bash
sudo apt install qtbase5-dev qttools5-dev-tools qtwebengine5-dev \
                 libtesseract-dev libleptonica-dev libhunspell-dev \
                 libxcb-cursor-dev libxcb-keysyms1-dev libxcb-icccm4-dev

cd ScreenTranslateOCR
qmake screen-translator.pro
make -j$(nproc)
./screen-translator
```

### Build on Windows

**Recommended:** download the prebuilt `.exe` from the project's
GitHub Actions artifacts:

1. Open <https://github.com/OneMoreGres/ScreenTranslator/actions>
2. Pick a successful `Build` run
3. Scroll to **Artifacts** → download `screen-translator-windows.zip`
4. Extract and run `screen-translator.exe`

**Manual build on Windows** (requires MSVC or MinGW + Qt 5.15):

```cmd
:: Open "x64 Native Tools Command Prompt for VS 2022"
set QTDIR=C:\Qt\5.15.2\msvc2022_64
set PATH=%QTDIR%\bin;%PATH%

:: Install deps: tesseract, leptonica, hunspell (e.g. via vcpkg)
git clone https://github.com/microsoft/vcpkg
cd vcpkg && .\bootstrap-vcpkg.bat
.\vcpkg install tesseract:x64-windows leptonica:x64-windows hunspell:x64-windows

cd ..\ScreenTranslateOCR
python share\ci\get_qt_ssl.py
python share\ci\get_leptonica.py
python share\ci\get_tesseract.py
python share\ci\get_hunspell.py
python share\ci\build.py
python share\ci\windeploy.py
```

### Local cross-compile (Linux → Windows)

Requires `mingw-w64-x86-64-dev` and the Qt 5 Windows binary (~1 GB).
See `share/ci/win-cross.sh` for the recipe; much faster is to download
the GitHub Actions artifact above.

## Attributions

* icons made by
[Smashicons](https://www.flaticon.com/authors/smashicons),
[Freepik](https://www.flaticon.com/authors/freepik),
from [Flaticon](https://www.flaticon.com/)

## Alternative solutions

* [Translumo](https://github.com/ramjke/Translumo) - Advanced real-time screen translator for games, hardcoded subtitles in videos, static text and etc.
