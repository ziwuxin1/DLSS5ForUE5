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
