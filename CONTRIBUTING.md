# Contributing to Phasemap

Thanks for helping! Phasemap should stay small, accurate and easy to read. Some guidelines:

## Before you start

- **Bugs:** open an issue with your DAW, OS, plugin format and what you expected. Screenshots help a lot.
- **Bigger changes:** open an issue first so we can agree on the approach before you spend time on it.
- **Small fixes** (typos, a new loudness target, a clearer hover text): just send a pull request.

## Code style

- Read [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) first.
- Keep the three layers: `dsp` (no GUI), `plugin`, `ui`. The audio thread must never allocate, lock or wait.
- Names say what things are (`truePeakLimitDb`, not `tp`). Units go into the name when it is not obvious.
- Comments explain *why*, not *what*. Every class gets a short comment at the top.
- JUCE formatting style (see `.clang-format`): 4 spaces, braces on new lines, a space before function-call parentheses.
- No new dependencies without discussing it first.

## Accuracy

Anything that changes a measurement needs a test in `tests/` with a signal whose answer is known
(for loudness: the EBU test cases). Run all tests before sending a pull request:

```bash
cmake --build build --target phasemap_tests && ctest --test-dir build
```

## Design

The UI is drawn entirely in code on a 1600 × 900 canvas (`src/ui`). Colours, fonts and positions live in `src/ui/Theme.h`.
If you change something visible, add before/after screenshots to the pull request
(`-DPHASEMAP_BUILD_TOOLS=ON`, then `PhasemapScreenshots out/`).

## License

By contributing you agree that your contribution is licensed under GPL-3.0-or-later, like the rest of the project.
