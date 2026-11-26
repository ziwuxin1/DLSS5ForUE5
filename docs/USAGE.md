# Usage

Click **D5** in the main editor toolbar, or open **Tools > MAXs Education DLSS 5 For UE5**.

1. Enable Neural Rendering and start with one pass.
2. Adjust **Effect Strength**: 0 is the original UE image, 1 is full NR output. UE blends the final images directly.
3. Enable **Compare Original / DLSS5** for an original-left / processed-right comparison. Move **Split Position** to inspect different parts of the image.
4. Optionally enable the ON/OFF watermark. Turn comparison off for a full processed capture.
5. Use UE High Resolution Screenshot or [MRQ 4K image sequence output](UE584_CAPTURE.md).

All panel changes save automatically for this project/user. **Reset Defaults** restores and saves every panel-owned setting, including hidden legacy parameters. It turns off NR, comparison, watermark and experimental guides, and restores one pass/full strength.

## Pass Count

Pass Count ranges from 1 to 4. Each pass consumes the previous pass output. Each additional pass increases GPU cost. More passes are not automatically higher quality.

## Advanced investigation

Style, NR Preset, Intensity and Local Tone remain visible, together with UE-side structure/tone and object-mask controls. Unverified legacy hints and experimental Scene Guides/Advanced controls are hidden from the artist panel. Their existing config keys and [CVars](COMMANDS.md) remain available. Their presence does not guarantee the installed runtime honors every parameter or improves quality.

The footer diagnostics, About, developer/social links and version row are omitted. Run `DLSS5.Status` in the UE console to inspect actual initialization, feature size and evaluation success/failure counters. A loaded DLL alone does not prove native inference.

## Safe Mode

The panel no longer includes a Safe Mode button. For diagnostics, the `DLSS5.SafeMode` console command disables NR and experimental guides for the current session; use `DLSS5.SaveSettings` to persist console changes.
