# Console Commands and CVars

## Commands

```text
DLSS5.Open      Editor only: open the editor control window
DLSS5.Status    Print runtime / guide diagnostics
DLSS5.SafeMode  Disable NR and experimental guides
```

`DLSS5.Open` is registered by the Editor module and is unavailable in packaged or `-game` runtime targets. `DLSS5.Status` is the runtime diagnostic command; its availability and logging in Shipping builds depend on the host's console and build settings.

## Main CVars

```text
r.DLSS5.Enable
r.DLSS5.Preset
r.DLSS5.PassCount
r.DLSS5.Intensity
r.DLSS5.LocalTone
r.DLSS5.LocalStructure
r.DLSS5.SkinStructure
r.DLSS5.AutoMask
r.DLSS5.Style
r.DLSS5.UICorrection
r.DLSS5.EnableSceneGuides
r.DLSS5.FeedSceneGuides
r.DLSS5.UseDepth
r.DLSS5.UseMotionVectors
r.DLSS5.AutoResetHistory
r.DLSS5.DepthInverted
r.DLSS5.MVecScaleX
r.DLSS5.MVecScaleY
r.DLSS5.ExposureResetRatio
r.DLSS5.Debug
r.DLSS5.HighlightPreserve
r.DLSS5.HighlightPreserveStart
r.DLSS5.HighlightPreserveEnd
```

## Pass Count

```text
r.DLSS5.PassCount 1
r.DLSS5.PassCount 2
r.DLSS5.PassCount 3
r.DLSS5.PassCount 4
```

## Optional highlight preservation

`r.DLSS5.HighlightPreserve` toggles restoration of bright highlights over the Neural Rendering output (`0` off, `1` on). The `HighlightPreserveStart` and `HighlightPreserveEnd` controls set the blend thresholds in the **current post-tonemap color-buffer units**, not absolute scene luminance or HDR nits. The end threshold must exceed the start threshold for recovery to apply.

## Capture watermark (UE 5.8.4 fork)

| Final-image CVar | Default | Meaning |
| --- | --- | --- |
| `r.DLSS5.Blend` | 1 | UE composite strength, 0 original / 1 full NR; zero bypasses evaluation. |
| `r.DLSS5.Compare` | 0 | Original left / processed right, including captures. |
| `r.DLSS5.Compare.Split` | 0.5 | Split fraction, 0 all processed / 1 all original. |

These final-image controls work independently of opaque runtime hints such as `r.DLSS5.Intensity`. The editor panel retains Style, NR Preset, Intensity and Local Tone. Structure/mask and experimental guide/debug controls remain hidden; their CVars remain available. `Reset Defaults` restores all panel-owned controls and persists the result.

| CVar | Default | Meaning |
| --- | --- | --- |
| `r.DLSS5.Watermark` | 0 | Burn per-sample native NR ON/OFF status into the image. |
| `r.DLSS5.Watermark.Corner` | 3 | 0 top left, 1 top right, 2 bottom left, 3 bottom right. |
| `r.DLSS5.Watermark.Scale` | 1 | Size relative to image height, clamped to 0.5–4. |

See [UE 5.8.4 capture](UE584_CAPTURE.md) for HighResShot and 4K MRQ sequence setup.

`DLSS5.SaveSettings` saves current control CVars for this project/user. UI changes save automatically. MRQ job overrides are not automatically persisted.

## Source-guided color preservation

| CVar | Default | Meaning |
| --- | --- | --- |
| `r.DLSS5.ColorPreserve` | 0 | 0..1 source-color base restoration; zero skips the filter entirely. |
| `r.DLSS5.TonePreserve` | 0 | 0..1 source-lightness base restoration, scaled by ColorPreserve. |
| `r.DLSS5.NeuralDetail` | 1 | 0..2 residual detail gain, scaled by ColorPreserve. |

See [color preservation and presets](UE584_CAPTURE.md) for color-space limits and JSON preset exchange. These controls affect viewport, HighResShot and MRQ through the same GPU pass.

## UE art direction and object masks

- `r.DLSS5.Art.Structure` (default `1.0`): UE frequency blend: 0 keeps source detail, 1 keeps neural detail. Independent of the runtime hint.
- `r.DLSS5.Art.Tone` (default `1.0`): UE frequency blend: 0 keeps source lighting and color base, 1 keeps the neural base.
- `r.DLSS5.Mask.Enable` (default `false`): Use UE Custom Depth Stencil groups. Enables stencil rendering for this session while active.
- `r.DLSS5.Mask.Preview` (default `false`): Preview group 1 red, 2 green, 3 blue. Included in captures: turn off before final output.
- `r.DLSS5.Mask.Group1.Enable` (default `true`): Disabled groups follow the global intensities.
- `r.DLSS5.Mask.Group1.Stencil` (default `201`): Choose a distinct ID from 1 to 255. Shared with other project effects; the first enabled matching group wins.
- `r.DLSS5.Mask.Group1.Structure` (default `1.0`): Override global structure for visible objects in this group.
- `r.DLSS5.Mask.Group1.Tone` (default `1.0`): Override global tone for visible objects in this group. Set both intensities to zero to protect the original image.
- `r.DLSS5.Mask.Group2.Enable` (default `true`): Disabled groups follow the global intensities.
- `r.DLSS5.Mask.Group2.Stencil` (default `202`): Choose a distinct ID from 1 to 255. Shared with other project effects; the first enabled matching group wins.
- `r.DLSS5.Mask.Group2.Structure` (default `1.0`): Override global structure for visible objects in this group.
- `r.DLSS5.Mask.Group2.Tone` (default `1.0`): Override global tone for visible objects in this group. Set both intensities to zero to protect the original image.
- `r.DLSS5.Mask.Group3.Enable` (default `true`): Disabled groups follow the global intensities.
- `r.DLSS5.Mask.Group3.Stencil` (default `203`): Choose a distinct ID from 1 to 255. Shared with other project effects; the first enabled matching group wins.
- `r.DLSS5.Mask.Group3.Structure` (default `1.0`): Override global structure for visible objects in this group.
- `r.DLSS5.Mask.Group3.Tone` (default `1.0`): Override global tone for visible objects in this group. Set both intensities to zero to protect the original image.
