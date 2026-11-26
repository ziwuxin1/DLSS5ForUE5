# UE 5.8.4 capture validation

Tested locally on 2026-11-25. These results apply to the fork additions, not every upstream configuration.

## Configuration

- Installed Unreal Editor 5.8.4, changelist 58902052, compatible changelist / BuildId 55116800.
- Windows Win64 Development Editor; DirectX 12, Shader Model 6 (SM5 was also exercised during development).
- NVIDIA GeForce RTX 5090, driver 621.24.
- NVIDIA DLSS plugin 8.9.0-NGX310.9.16; existing native NR runtime supplied locally, excluded from source and build deliverables.
- NR preset 1, style Natural, pass count 1, intensity 1; scene-guide feeding and motion vectors disabled.
- Isolated test project with basic meshes, a cine camera and a three-frame Level Sequence. The user's production scene was not rendered or modified.

## Results

| Check | Observed result |
| --- | --- |
| Win64 Development Editor build against 5.8.4 | Succeeded; runtime, editor and MoviePipeline modules linked |
| Unreal automation suite `DLSS5` on DX12/SM6 | 5 succeeded, 0 failed, 0 warnings |
| Built-in high-resolution screenshot path | OFF and ON PNG files, each 3840 x 2160; labels and image content inspected |
| Native inference evidence | Native feature 18 created at 3840 x 2160; 5/5 successes after the ON screenshot, 52/52 successes after MRQ, 0 failures; final NGX result 0x00000001 |
| MRQ in-editor deferred render | Three consecutive PNG files and three EXR files, all 3840 x 2160; executor reported success |
| Watermark in saved MRQ frames | PNG and decoded EXR glyph pixels verified opaque; ON glyph green and text white in all three frames |
| Settings persistence | Edited settings through Unreal property-change notifications, exited editor, started a new process; intensity 0.65, watermark enabled, corner 2 and scale 1.5 restored; intensity CVar matched |
| Static repository checks | Public text check, release-contract check, Python preset syntax and git whitespace check passed |

The total native counters include viewport and warm-up renders, not 52 delivered 4K frames. Returning to the viewport changes the final diagnostic feature size; the log separately records the 3840 x 2160 feature creation. ON/OFF scene pixels differed outside the watermark, but this is pipeline evidence rather than a model-quality benchmark.

The automation suite covers guide conversion, a real 4K watermark GPU readback including MRQ alpha convention, MRQ override/restore and invalid tiled jobs, settings disk serialization/startup priority, recovery and feature retirement. Mock-runtime tests are separate from the native counters above.

## Reproduction and limits

Run the `DLSS5` automation group in Session Frontend or use:

```text
UnrealEditor-Cmd.exe <Project.uproject> -d3d12 -RenderOffscreen -unattended -ExecCmds="Automation RunTests DLSS5" -TestExit="Automation Test Queue Empty"
```

For manual capture, follow [capture setup](UE584_CAPTURE.md), compare `HighResShot 3840x2160` with NR disabled/enabled, then render a short MRQ sequence using the included 4K preset. Inspect both the output images and `DLSS5.Status`; DLL loading alone is not inference proof.

This validation does not establish production-scene quality, long-render stability, multi-GPU behavior, multi-sample ghosting, tiled MRQ, linear/OCIO EXR, Movie Render Graph, path tracing, packaged native NR, or runtime compatibility with other UE versions. Use Tile Count 1 and the default tone curve. The runtime does not infer meaningful depth/motion-guide quality from this scene-guide-disabled capture test.

## Packaging correction (r2)

The initial source-based validation did not test the final ZIP's added `EngineVersion` field. That field used the hotfix version `5.8.4`, while PluginManager compares against `FEngineVersion::CompatibleWith()` (`5.8.0` for this Epic build). This caused a prompt even though both module BuildIds were 55116800. The packaging script now derives the compatible baseline from the engine build and rejects mismatched module manifests. No rendering binaries changed in this correction.

The corrected ZIP was extracted into a fresh project and launched with UE 5.8.4. The runtime settings class and MRQ setting class loaded, the process exited with code 0, and the log contained no plugin engine-version warning. This was a package-loading smoke test without the NVIDIA runtime; it does not add new native inference claims.

## r3 compact controls and production-scene acceptance

The earlier basic-mesh results above are historical. The r3 acceptance additionally used an isolated project reading the production Tuxi map, with its original map/content left unsaved. Existing local NVIDIA runtime was retained during the plugin update.

- Win64 Development Editor build succeeded on UE 5.8.4.
- All six `DLSS5` automation tests passed. The new `CompositeControls4K` test reads back actual GPU pixels for strength 0/0.5/1 and split 0/0.5/1, with a non-zero 4K view origin and original alpha retained. Configuration tests check reset values on disk as well as legacy hidden controls.
- Four real-scene 3840x2160 HighResShot PNGs: NR off, full effect, 50% effect, and original-left / processed-right comparison. These screenshots used a 1920x1080 NR feature before UE's final 4K output; they do not prove full-resolution NR inference for HighResShot.
- MRQ produced three PNG and three EXR frames at 3840x2160 with comparison active. Its native NGX feature was created at 3840x2160. All six files were decoded and checked for dimensions, finite pixel values, white label text, green ON glyphs and opaque glyph alpha.
- Native counters: 15/15 successes after screenshots, 61/61 after MRQ, zero failures. These totals include viewport and warm-up evaluations, not 61 delivered frames. Configuration remained DX12/SM6, RTX 5090, driver 621.24, one pass, guides off.
- The D5 button opened the compact panel in the editor. Hidden controls and the entire footer were absent. Clicking Reset Defaults changed strength 0.35 to 1, passes 3 to 1, comparison/watermark to off, and hidden motion scale 2 to 1; the saved user INI contained those defaults.

The final-image blend/comparison code is independently implemented. No implementation from the proprietary Visual Enhancer license was copied. At the r3 checkpoint, color-guided restoration, preset exchange, and automatic multi-variant MRQ queues were still research proposals. The r4 results below supersede that status for color refinement and preset exchange. These short checks do not establish long-render stability, all scenes, linear/OCIO EXR, tiled output, Movie Render Graph, path tracing, packaged NR or other engine versions.

After a clean editor shutdown and new process launch, ten control CVars (including hidden motion scale) matched the reset values. The rebuilt toolbar was visually checked with P4 loaded: D5 appeared immediately to its right and opened the compact panel. No engine-version prompt occurred.

## r4 retained controls, source-guided color and preset exchange

The panel again exposes Style, NR Preset, Intensity and Local Tone, alongside NR enable and Pass Count. The new color pass adapts the MIT-licensed Feeder shader with attribution; preset exchange is implemented locally, without Visual Enhancer code.

- UE 5.8.4 Win64 Development Editor build succeeded. All eight DX12 `DLSS5` automation tests passed, including GPU readback for source reconstruction in encoded and linear inputs, unchanged identical images, zero-cost disabled path, HDR-value bypass, detail response, non-zero view origin and failed-native-evaluation bypass. Preset tests cover all config fields and rejection without partial application.
- Tuxi acceptance used the installed r4 runtime module, reading the original map through an isolated project. Four 3840x2160 screenshots cover NR off, NR on, color preservation and comparison. Screenshot NR input remained 1920x1080 before UE's output scaling.
- MRQ produced three PNG and three EXR frames at 3840x2160, with a native 3840x2160 NGX feature, one pass, guides off, Color Preserve 0.9, Tone Preserve 0.6, Neural Detail 1.3, comparison off. Native counters ended at 61/61 successful evaluations, zero failed (includes viewport/warm-up).
- Runtime logs confirmed the filter active with encoded sRGB (device 0/gamut 0) in screenshots and tone-mapped linear (device 9/gamut 0) in MRQ. All ten output images were decoded and checked for dimensions, finite pixels and opaque white/ON glyphs. Separate screenshot differences are not an isolated quality benchmark; synthetic GPU tests isolate filter behavior.
- UI acceptance: saved a complete JSON preset, clicked Reset Defaults, loaded the saved file, and observed all retained/new values restored. The file dialogs are owned by the settings window and guard repeated activation.
- A clean shutdown and fresh process verified 15 CVars, including Style=1, Preset=2, PassCount=3, Intensity=0.65, LocalTone=1.7, and color/tone/detail=0.9/0.6/1.3. All matched the imported preset.

The earlier limitations still apply. Color refinement is an optional additional GPU cost, not a new neural model; it does not establish HDR/OCIO/untone-mapped EXR support or improve every image. Original closed-runtime hints remain runtime-dependent. All tests that changed settings used the isolated project's personal INI, not the production project's settings.

## r5 per-control factory reset

- UE 5.8.4 Win64 Development Editor build succeeded. All 15 visible rows, including Passes, show the UE reset arrow at both changed and default values.
- `DLSS5.Runtime.ConfigurationAndRecovery` passed under NullRHI. For every visible control, it checks the factory value after reset and verifies that all 28 other panel-owned CVars remain unchanged, even after SaveConfig has mutated the CDO. It also checks single-field reset persistence while retaining an unrelated custom value.
- In the isolated editor UI, clicked Local Tone (1.7 to 1), Passes (3 to 1), Style (Cinematic to Natural), watermark enable (on to off) and watermark scale (1.5 to 1, after re-enabling the watermark). Other customized values remained visible. Disabled dependent fields retain UE's normal edit-condition behavior.
- A clean editor restart verified all 15 visible-control CVars against the resulting configuration; all matched, including the reset values and retained Intensity 0.65, Preset 2 and Effect Strength 0.35.
- This revision changes settings/reset UI only. Rendering shaders and capture paths are unchanged; the earlier r4 GPU/MRQ results were not repeated or claimed as fresh r5 rendering tests.
