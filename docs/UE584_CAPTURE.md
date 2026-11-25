# UE 5.8.4 capture additions (ziwuxin1 fork)

This fork extends the v0.6.1 source baseline with a GPU watermark and an explicit Movie Render Queue setting. Use **Unreal Editor 5.8.4, Win64, DirectX 12**. Keep NVIDIA's matching DLSS plugin enabled and supply your existing `nvngx_dlssnr.dll` in `DLSS5ForUE5/Binaries/ThirdParty/Win64/`. The runtime is not included in this fork's source or build packages.

## High Resolution Screenshot

1. Open **Tools > DLSS 5 for UE5** and enable Neural Rendering.
2. Under **Capture**, optionally enable **Show DLSS5 ON / OFF Watermark**.
3. Use UE's normal **High Resolution Screenshot** viewport tool, or run `HighResShot 3840x2160` in the viewport console.

The watermark is part of scene color and is saved with the screenshot. Its corner and scale are configurable. For a selected capture region, it is placed inside the region. Disable the watermark to export a clean image. This path is for the final perspective Lit image; buffer-visualization dumps, scene-capture textures and asset thumbnails are not neural-rendered outputs.

`DLSS5 ON` means at least one native NR pass returned success for that view/sample. `DLSS5 OFF` also covers disabled NR, missing runtime and failed initialization/evaluation. With multiple NR passes, ON does not promise that every pass succeeded; inspect `DLSS5.Status` for errors. The label reports runtime success, not an independent visual-quality measurement.

## MRQ 4K image sequences

1. Enable **Movie Render Queue** and restart the editor.
2. Add your Level Sequence to MRQ and open its job settings.
3. Add **DLSS5 Neural Rendering**. Choose enable/disable, pass count and watermark settings for this job.
4. Use **Deferred Rendering**, **Output: 3840 × 2160**, and **PNG Sequence** or **EXR Sequence**.
5. Start with one spatial sample, one temporal sample, 32 engine warm-up frames and 8 render warm-up frames. Keep **High Resolution > Tile Count = 1**.
6. Render locally in the editor. Original NR/watermark console values are restored on completion or cancellation.

For a reusable preset, enable **Python Editor Script Plugin**, then use **Tools > Execute Python Script** to run `Scripts/create_mrq_4k_preset.py`. It creates a new preset under `/Game/DLSS5`, without overwriting existing assets or starting a render. Set `OUTPUT_FORMAT = "exr"` in the script if required. The preset enables the watermark; uncheck it for clean delivery.

Use distinct output folders for ON/OFF comparisons. `{dlss5}` is also available as a filename token, but describes the **requested job setting**, while the watermark describes the current sample's actual result.

## Scope

- Full-frame 4K output uses the existing native NR render path; it does not require MRQ tile rendering.
- Tile Count greater than 1 is rejected by setting validation because independent tile histories and repeated labels would not represent a full-frame result.
- Keep **Disable Tone Curve** unchecked and OCIO disabled for NR. Linear scene-referred EXR/OCIO transforms are outside the validated post-tonemap input contract. EXR can still store the tone-mapped result.
- NR executes before MRQ sample accumulation. Increasing sample counts requires scene-specific ghosting checks; mixed successful/failed samples can also blend the label.
- This setting targets classic MRQ. Movie Render Graph, path tracing, stereo capture and packaged-game NR are not claimed as supported by these additions.
- UI changes are saved automatically per project/user and restored on editor restart. MRQ overrides are temporary; console changes are temporary unless saved with `DLSS5.SaveSettings`. Explicit launch/INI overrides take priority over saved UI settings.

## Verification

The repository includes `DLSS5.GPU.Watermark4K`, `DLSS5.MRQ.SettingLifecycle`, and the existing runtime/guide tests. Build success, test status and native evaluation evidence are separate; consult the [UE 5.8.4 validation report](VALIDATION_UE584.md) for the exact locally tested configuration and any limitations.

## Automatic settings persistence

Changes made in the plugin window, including the custom Pass Count slider, are automatically saved under the project's `Saved/Config/WindowsEditor/EditorPerProjectUserSettings.ini`. Shared `Config/DefaultEngine.ini` files do not need to be checked out in Perforce. The next editor launch restores these settings before rendering starts.

Explicit console/startup overrides retain their priority. Crash recovery may still temporarily disable NR after an unclean NR-active session; that recovery does not erase saved settings. The UI Safe Mode button saves its safe choices, while the `DLSS5.SafeMode` console command remains session-only. To persist changes made through CVars, run `DLSS5.SaveSettings`.

## Editor build packaging

Use `python Scripts/package_editor_build.py --engine <UE_5.8> --output <build.zip>` after compiling the plugin. The script checks the compiled module BuildId against the selected engine and packages only the plugin modules, never the NVIDIA runtime.

`EngineVersion` in the ZIP descriptor describes Unreal's compatibility baseline, not the hotfix version shown in About Unreal Editor. For Epic UE 5.8.4 this is `5.8.0`; the DLLs are still built and tested with 5.8.4. The original locally delivered ZIP incorrectly declared `5.8.4`, which triggered a false incompatibility prompt. The corrected r2 package uses the compatible baseline and retains the module BuildId checks.
