# Developing Phasemap

## Open the project in Xcode (macOS)

```bash
cd phasemap
cmake -B build-xcode -G Xcode -DPHASEMAP_INSPECTOR=ON
open build-xcode/Phasemap.xcodeproj
```

1. In the scheme menu at the top, choose **Phasemap_Standalone** and **My Mac**.
2. Press **⌘R**. Phasemap opens as its own app. In its window, choose your audio input under *Options* (e.g. BlackHole, or your interface) to feed it music.
3. Click into the Phasemap window and press **⌘⇧I**: the inspector opens.

The project navigator on the left shows `src/dsp`, `src/plugin`, `src/ui`; the design is in `src/ui/Theme.h` (colours, positions) and `src/ui/Background.cpp` (panels, scales, labels).

On Windows, use `cmake -B build-vs -G "Visual Studio 17 2022" -DPHASEMAP_INSPECTOR=ON` and open the `.sln`.

## The live inspector

With `-DPHASEMAP_INSPECTOR=ON`, **⌘⇧I** (Ctrl+Shift+I on Windows/Linux) opens [melatonin_inspector](https://github.com/sudara/melatonin_inspector) - the browser developer tools, for JUCE:

- hover or click any part of the UI to see its name, position and size (in canvas pixels),
- change bounds live and see the result,
- pick colours with the eyedropper,
- measure how long each part takes to paint.

It is only compiled into developer builds, never into releases.

## Debugging inside a DAW

Plugins in Logic run in a separate process that Xcode cannot attach to easily. For most work the standalone app is enough. For host-specific problems, use JUCE's *AudioPluginHost* (in the JUCE folder under `extras/AudioPluginHost`), or Reaper / Ableton: choose the **Phasemap_VST3** scheme, *Edit Scheme → Run → Executable*, and pick the host app.

## Changing the design

Everything you see is drawn in code on a 1600 × 900 canvas:

| What | Where |
|---|---|
| Colours, fonts, positions of all elements | `src/ui/Theme.h` |
| Panels, bevels, scales, static labels | `src/ui/Background.cpp` |
| Moving parts (bars, LEDs, goniometer) | `src/ui/MeterViews.cpp`, `src/ui/LoudnessPanel.cpp` |
| Buttons and their icons | `src/ui/Controls.cpp` |

A layered SVG of the current design (for Affinity, Illustrator, Figma) is in `docs/design/phasemap-layers.svg`.
It is generated from the running code: `cmake -B build -DPHASEMAP_BUILD_TOOLS=ON`, build `PhasemapScreenshots`, run it, then
`python3 tools/export_design_svg.py out/energy.json docs/design/phasemap-layers.svg`.

## Screenshots

```bash
cmake -B build -DPHASEMAP_BUILD_TOOLS=ON && cmake --build build --target PhasemapScreenshots
./build/PhasemapScreenshots_artefacts/Release/PhasemapScreenshots out 1.5
```
