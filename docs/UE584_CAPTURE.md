# UE 5.8.4 capture additions (ziwuxin1 fork)

This fork extends the v0.6.1 source baseline with a GPU watermark and an explicit Movie Render Queue setting. Use **Unreal Editor 5.8.4, Win64, DirectX 12**. Keep NVIDIA's matching DLSS plugin enabled and supply your existing `nvngx_dlssnr.dll` in `DLSS5ForUE5/Binaries/ThirdParty/Win64/`. The runtime is not included in this fork's source or build packages.

## High Resolution Screenshot

1. Click **D5** in the main editor toolbar (or **Tools > DLSS 5 for UE5**) and enable Neural Rendering.
2. Under **Capture**, optionally enable **Show DLSS5 ON / OFF Watermark**.
3. Use UE's normal **High Resolution Screenshot** viewport tool, or run `HighResShot 3840x2160` in the viewport console.

The watermark is part of scene color and is saved with the screenshot. Its corner and scale are configurable. For a selected capture region, it is placed inside the region. Disable the watermark to export a clean image. This path is for the final perspective Lit image; buffer-visualization dumps, scene-capture textures and asset thumbnails are not neural-rendered outputs.

`DLSS 5 On` means at least one native NR pass returned success for that view/sample. `DLSS 5 Off` also covers disabled NR, missing runtime and failed initialization/evaluation. With multiple NR passes, ON does not promise that every pass succeeded; inspect `DLSS5.Status` for errors. The label reports runtime success, not an independent visual-quality measurement.

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

The panel retains **Style**, **NR Preset**, **Intensity**, **Local Tone**, NR enable and Pass Count. It also exposes **Effect Strength**, **Color Preservation**, **Before / After**, and watermark controls. Effect Strength mixes the original UE image with the final NR output: `0` bypasses NR, `1` uses the full result. This is a UE GPU composite, independent of the runtime's opaque Intensity hint. Comparison shows original on the left and processed on the right; Split Position moves the boundary. Both controls also affect HighResShot and MRQ. Turn comparison off before exporting a full processed frame. MRQ uses these project settings unless its Console Variables setting explicitly overrides them.

**Reset Defaults** restores and saves all panel-owned values, including hidden legacy settings. NR, comparison, watermark and experimental guides return to off; strength returns to 1 and pass count to 1. It does not change the scene or settings owned by other plugins.

Each of the 31 visible controls, including the custom **Passes** slider, also has an always-visible reset arrow on its right. It restores just that control to its factory value, applies it immediately and saves it for the next launch; other controls stay unchanged. Dependent controls follow UE's edit conditions: enable their parent option before editing or resetting them. The reset uses the registered CVar default, because saved settings mutate the settings CDO and cannot serve as factory defaults.

Local Structure, Skin Structure, Automatic Mask, UI Correction, and experimental Scene Guides/Advanced controls are hidden from the panel. The requested Style, NR Preset, Intensity and Local Tone remain visible and retain their original runtime mappings; their visible effect depends on the supplied closed runtime and scene. Their existing CVars/config keys remain available for compatibility and investigation. The diagnostics/About/social/version footer is removed; use `DLSS5.Status` for diagnostics. Earlier documentation images show the upstream UI rather than this compact fork panel.

Changes made in the plugin window, including the custom Pass Count slider, are automatically saved under the project's `Saved/Config/WindowsEditor/EditorPerProjectUserSettings.ini`. Shared `Config/DefaultEngine.ini` files do not need to be checked out in Perforce. The next editor launch restores these settings before rendering starts.

Explicit console/startup overrides retain their priority. Crash recovery may still temporarily disable NR after an unclean NR-active session; that recovery does not erase saved settings. The UI Safe Mode button saves its safe choices, while the `DLSS5.SafeMode` console command remains session-only. To persist changes made through CVars, run `DLSS5.SaveSettings`.

## Editor build packaging

Use `python Scripts/package_editor_build.py --engine <UE_5.8> --output <build.zip>` after compiling the plugin. The script checks the compiled module BuildId against the selected engine and packages only the plugin modules, never the NVIDIA runtime.

`EngineVersion` in the ZIP descriptor describes Unreal's compatibility baseline, not the hotfix version shown in About Unreal Editor. For Epic UE 5.8.4 this is `5.8.0`; the DLLs are still built and tested with 5.8.4. The original locally delivered ZIP incorrectly declared `5.8.4`, which triggered a false incompatibility prompt. The corrected r2 package uses the compatible baseline and retains the module BuildId checks.

## Source-guided color, tone and detail (r4)

**Original Color Preserve** reduces broad color shifts using the original UE image as a guide. **Original Tone Preserve** restores its lightness base; **Neural Detail** scales the remaining neural detail. These are GPU image-processing controls, separate from the runtime's Intensity/Local Tone hints.

- Color Preserve: 0..1, default **0** (no refinement textures or dispatches when the structure/tone weights are also neutral).
- Tone Preserve: 0..1, default **0** (retain neural lightness).
- Neural Detail: 0..2, default **1**. Higher values can emphasize noise or halos. Both subordinate controls are scaled by Color Preserve.
- The filter uses an Oklab 9x9 source-guided bilateral base correction, adapted from MIT-licensed DLSS5-Feeder. It runs after native NR and before strength/comparison/watermark. At 4K it needs about 253 MiB for two float32 Lab buffers plus a 63 MiB FP16 result, before any RDG memory reuse. Enable it only when needed; it adds GPU work.
- Supported input contracts: SDR sRGB, and tone-mapped linear sRGB-primary MRQ. Other output devices/gamuts bypass this filter; out-of-range HDR pixels are preserved rather than clipped. Leave Disable Tone Curve and OCIO off for validated MRQ. This does not establish general HDR or scene-referred EXR support.
- Failed native evaluation bypasses refinement, including detail gain. Existing UE alpha is restored by the final composite.

## Save and load effect presets (r4)

Use **Save Preset...** to write a JSON file and **Load Preset...** to restore the complete configuration. The initial folder is `<Project>/Saved/DLSS5Presets/`. The file includes Style, NR Preset, Passes, Intensity, Local Tone, new color controls, comparison, watermark and hidden legacy settings. Imported settings apply immediately and are saved for the next editor launch.

Import validates the complete versioned schema before changing anything. Missing fields, wrong types, non-integral counts, out-of-range values, invalid styles and unsupported versions are rejected without changing current settings. Preset files are configuration only; they do not include runtime binaries or project assets.


## Watermark and toolbar appearance (r6)

The compact gray **D5** square opens the same settings window. The On watermark uses black bold lettering on an opaque white panel and a solid **#76B900** strip below a transparent gap. Off uses white lettering on a translucent black panel and an opaque black strip. Both read **DLSS 5 On / DLSS 5 Off**, with the same geometry, adjustable corner and scale, and GPU evaluation-based status. The accent is encoded appropriately for SDR screenshots and tone-mapped linear MRQ so their PNG colors agree. The watermark is drawn at final image resolution after UE spatial scaling, so the strip stays solid and lettering remains crisp.

## Global structure/tone and object groups (r6)

**Global Controls (UE)** adds **Structure Intensity** and **Tone Intensity**, both 0..1, default 1. Structure mixes source/neural fine detail; Tone mixes their broad lighting/color base. Both at 0 reproduce the original UE color; both at 1 retain the native result before optional color preservation. This is UE-side source-guided image processing, not access to NVIDIA's internal model controls. It uses the same Oklab filter and supported color contracts described above.

**Developer Masking** provides three independently enabled groups. Each has its own Structure and Tone weights, overriding the global weights on visible assigned objects. Unassigned objects use the global weights. To protect an object, set both group weights to 0.

1. Select actors in the level, then click **Assign Selected Objects to Group 1/2/3**. This enables object masking and assigns Custom Depth Stencil IDs (defaults 201, 202, 203) to their primitive components.
2. Use UE Undo to restore the previous component stencil/depth settings. Save the level to retain actor assignments. Parameter settings themselves save automatically per project/user.
3. **Show Masks** overlays red/green/blue for the three groups. It works with NR disabled, and it appears in screenshots/MRQ; turn it off for final output.

Assignment replaces an object's previous stencil ID, which may also be used by other project effects. Choose unused IDs. If two enabled groups share an ID, the first group wins. Enabling masks enables UE Custom Depth with Stencil for the session; disabling them leaves that project-wide render feature available to other effects. Translucent materials need Custom Depth writes enabled. Masking uses nearest-pixel stencil lookup with scene-depth visibility rejection; it is not semantic segmentation, and complex transparency/anti-aliased edges are not claimed as production-validated. The compact group texture costs roughly 8 MiB at 4K, in addition to UE Custom Depth and any active refinement buffers.

All new settings have individual reset arrows, participate in Reset Defaults, and are included in preset **version 2**. Complete version-1 presets still load: the new global weights default to 1 and object masking/preview default to off. Native **Model A/B/C** switching and NVIDIA's semantic **Model Automask** are not exposed because the supplied closed runtime's model/mask interface has not been verified. The [NVIDIA developer-control article](https://www.nvidia.com/en-us/geforce/news/where-winds-meet-dlss-5-3d-guided-neural-rendering-out-now/) describes those SDK capabilities, not capabilities established for this unofficial integration.
