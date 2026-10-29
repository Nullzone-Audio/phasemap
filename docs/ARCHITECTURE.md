# How Phasemap works

A ten-minute tour for contributors. The code is split into three layers that only depend downwards:

```
src/ui/      the window: views, settings, hover tips        (message thread)
   │ reads
src/plugin/  PhasemapProcessor: parameters, state, routing  (audio thread)
   │ uses
src/dsp/     measurement, no JUCE GUI, fully unit-tested
```

## Threads and data flow

```
host audio ──► PhasemapProcessor::processBlock
                 ├─ Mono Check (optional, 30 ms crossfade)
                 ├─ LoudnessMeter::process        → atomics (momentary, integrated, ...)
                 └─ StereoFifo::push              → lock-free ring buffer
                                                       │
editor timer (60 Hz) ◄─────────────────────────────────┘
                 ├─ StereoFifo::pull → SpectralCorrelation::process (FFT, bands, goniometer)
                 ├─ reads the loudness atomics
                 └─ repaints the views
```

Rules that keep it safe:

- The audio thread never allocates, locks or waits. If the editor is closed, the FIFO simply overflows and drops samples.
- Loudness runs on the audio thread on purpose: *Integrated* must keep measuring while the window is closed.
- The spectral analysis runs in the editor: it is only needed while someone looks at it.
- `PhasemapProcessor::getTarget / setTarget` touch the ValueTree and are message-thread only.

## src/dsp

| File | What it does |
|---|---|
| `StereoFifo.h` | Single-producer/single-consumer ring buffer (juce::AbstractFifo). |
| `SpectralCorrelation` | 8192-point FFT every 2048 samples, Hann window. Per band (1/3, 1/6 or 1/12 octave) it averages power L, power R and the cross spectrum Re(L·R\*) over time and computes `cross / sqrt(pL·pR)`. Averaging the sums (not the ratio) keeps quiet bands stable. Also overall correlation, balance and the goniometer buffer. |
| `LoudnessMeter` | BS.1770-4: K-weighting (coefficients derived for any sample rate), 100 ms energy blocks, Momentary = 4 blocks, Short-term = 30 blocks. Integrated and LRA use histograms with 0.1 LU bins that also store the exact energy, so the gates cost O(800) per block and memory is constant for any programme length. |
| `TruePeakDetector` | Polyphase windowed-sinc interpolation, 4x up to 48 kHz, 2x up to 96 kHz. |
| `LoudnessTargets.h` | The built-in platform targets. Update here when a platform changes its normalisation. |

## src/plugin

`Parameters.h` holds the IDs (never rename them, hosts store them) and the layout. `PhasemapProcessor` wires everything together and stores the loudness target in the project state with all its values.

## src/ui

The editor draws on a fixed **1600 × 900 canvas** that is scaled to the window, so the layout never reflows. All positions are in `Theme.h → layout`.

| File | What it does |
|---|---|
| `Theme` | Colours, fonts, layout rectangles, easing. Start here when changing the look. |
| `Background` | Everything static (panels, scales, labels), drawn in code and cached as an image per window size. The style: grey hardware lit from the top right. |
| `MeterViews` | Spectrum (energy/phase), correlation LEDs, goniometer, balance. |
| `LoudnessPanel` | The loudness column. |
| `Controls` | Look-and-feel, hardware buttons and their icons, drag-value field. |
| `HoverTips` | Explanations after 3 s hover, 180 ms fade in / 300 ms out. |
| `SettingsMenu` | Opened by clicking *PHASEMAP*: resolution, window size, loudness targets. |
| `UserSettings` | Per-computer settings file (window size, resolution, saved targets). |
| `PhasemapEditor` | Creates and connects everything, runs the 60 Hz timer. |

## Tests and tools

- `tests/LoudnessTests.cpp` – EBU Tech 3341/3342 cases at 44.1–192 kHz.
- `tests/CorrelationTests.cpp` – known-answer signals for the phase analysis.
- `tests/ProcessorTests.cpp` – pass-through at every rate and block size, layouts, Mono Check, state, editor.
- `tools/Screenshots.cpp` – renders the UI with a test song (`-DPHASEMAP_BUILD_TOOLS=ON`), used for `docs/images`.

CI builds and tests on macOS, Windows and Linux and runs [pluginval](https://github.com/Tracktion/pluginval) on every push.
