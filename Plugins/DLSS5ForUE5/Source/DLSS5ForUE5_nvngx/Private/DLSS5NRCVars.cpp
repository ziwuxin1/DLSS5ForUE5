#include "DLSS5NRCVars.h"
#include "CoreMinimal.h"
#include "HAL/IConsoleManager.h"

static TAutoConsoleVariable<int32> CVarDLSS5NREnable(
    TEXT("r.DLSS5.Enable"), 0,
    TEXT("Enable experimental DLSS Neural Rendering post-pass. 0=Off 1=On"),
    ECVF_RenderThreadSafe);

static TAutoConsoleVariable<float> CVarGlobalStructure(TEXT("r.DLSS5.Art.Structure"), 1.0f,
    TEXT("UE frequency blend: 0 keeps source detail, 1 keeps neural detail. Independent of the runtime hint."), ECVF_RenderThreadSafe);
static TAutoConsoleVariable<float> CVarGlobalTone(TEXT("r.DLSS5.Art.Tone"), 1.0f,
    TEXT("UE frequency blend: 0 keeps source lighting and color base, 1 keeps the neural base."), ECVF_RenderThreadSafe);
static TAutoConsoleVariable<int32> CVarbDeveloperMasking(TEXT("r.DLSS5.Mask.Enable"), 0,
    TEXT("Use UE Custom Depth Stencil groups. Enables stencil rendering for this session while active."), ECVF_RenderThreadSafe);
static TAutoConsoleVariable<int32> CVarbShowDeveloperMasks(TEXT("r.DLSS5.Mask.Preview"), 0,
    TEXT("Preview group 1 red, 2 green, 3 blue. Included in captures: turn off before final output."), ECVF_RenderThreadSafe);
static TAutoConsoleVariable<int32> CVarbMaskGroup1Enabled(TEXT("r.DLSS5.Mask.Group1.Enable"), 1,
    TEXT("Disabled groups follow the global intensities."), ECVF_RenderThreadSafe);
static TAutoConsoleVariable<int32> CVarMaskGroup1Stencil(TEXT("r.DLSS5.Mask.Group1.Stencil"), 201,
    TEXT("Choose a distinct ID from 1 to 255. Shared with other project effects; the first enabled matching group wins."), ECVF_RenderThreadSafe);
static TAutoConsoleVariable<float> CVarMaskGroup1Structure(TEXT("r.DLSS5.Mask.Group1.Structure"), 1.0f,
    TEXT("Override global structure for visible objects in this group."), ECVF_RenderThreadSafe);
static TAutoConsoleVariable<float> CVarMaskGroup1Tone(TEXT("r.DLSS5.Mask.Group1.Tone"), 1.0f,
    TEXT("Override global tone for visible objects in this group. Set both intensities to zero to protect the original image."), ECVF_RenderThreadSafe);
static TAutoConsoleVariable<int32> CVarbMaskGroup2Enabled(TEXT("r.DLSS5.Mask.Group2.Enable"), 1,
    TEXT("Disabled groups follow the global intensities."), ECVF_RenderThreadSafe);
static TAutoConsoleVariable<int32> CVarMaskGroup2Stencil(TEXT("r.DLSS5.Mask.Group2.Stencil"), 202,
    TEXT("Choose a distinct ID from 1 to 255. Shared with other project effects; the first enabled matching group wins."), ECVF_RenderThreadSafe);
static TAutoConsoleVariable<float> CVarMaskGroup2Structure(TEXT("r.DLSS5.Mask.Group2.Structure"), 1.0f,
    TEXT("Override global structure for visible objects in this group."), ECVF_RenderThreadSafe);
static TAutoConsoleVariable<float> CVarMaskGroup2Tone(TEXT("r.DLSS5.Mask.Group2.Tone"), 1.0f,
    TEXT("Override global tone for visible objects in this group. Set both intensities to zero to protect the original image."), ECVF_RenderThreadSafe);
static TAutoConsoleVariable<int32> CVarbMaskGroup3Enabled(TEXT("r.DLSS5.Mask.Group3.Enable"), 1,
    TEXT("Disabled groups follow the global intensities."), ECVF_RenderThreadSafe);
static TAutoConsoleVariable<int32> CVarMaskGroup3Stencil(TEXT("r.DLSS5.Mask.Group3.Stencil"), 203,
    TEXT("Choose a distinct ID from 1 to 255. Shared with other project effects; the first enabled matching group wins."), ECVF_RenderThreadSafe);
static TAutoConsoleVariable<float> CVarMaskGroup3Structure(TEXT("r.DLSS5.Mask.Group3.Structure"), 1.0f,
    TEXT("Override global structure for visible objects in this group."), ECVF_RenderThreadSafe);
static TAutoConsoleVariable<float> CVarMaskGroup3Tone(TEXT("r.DLSS5.Mask.Group3.Tone"), 1.0f,
    TEXT("Override global tone for visible objects in this group. Set both intensities to zero to protect the original image."), ECVF_RenderThreadSafe);

static TAutoConsoleVariable<float> CVarColorPreserve(TEXT("r.DLSS5.ColorPreserve"), 0.0f,
    TEXT("Source-guided color preservation strength, 0 disables. SDR sRGB/tone-mapped linear MRQ only."), ECVF_RenderThreadSafe);
static TAutoConsoleVariable<float> CVarTonePreserve(TEXT("r.DLSS5.TonePreserve"), 0.0f,
    TEXT("Source lightness-base preservation, scaled by ColorPreserve. 0..1."), ECVF_RenderThreadSafe);
static TAutoConsoleVariable<float> CVarNeuralDetail(TEXT("r.DLSS5.NeuralDetail"), 1.0f,
    TEXT("Neural residual detail gain, 0..2. Effective only with ColorPreserve > 0."), ECVF_RenderThreadSafe);

static TAutoConsoleVariable<float> CVarBlend(
    TEXT("r.DLSS5.Blend"), 1.0f,
    TEXT("UE final-image blend: 0=original, 1=full NR. Independent of runtime intensity hints."), ECVF_RenderThreadSafe);
static TAutoConsoleVariable<int32> CVarCompare(
    TEXT("r.DLSS5.Compare"), 0,
    TEXT("Compare original (left) and processed (right). Also applies to screenshots and MRQ."), ECVF_RenderThreadSafe);
static TAutoConsoleVariable<float> CVarCompareSplit(
    TEXT("r.DLSS5.Compare.Split"), 0.5f,
    TEXT("Normalized comparison split position. 0=all processed, 1=all original."), ECVF_RenderThreadSafe);

static TAutoConsoleVariable<int32> CVarWatermark(
    TEXT("r.DLSS5.Watermark"), 0,
    TEXT("Burn DLSS5 ON/OFF into the rendered image. ON means at least one native NR pass succeeded for this view/sample."), ECVF_RenderThreadSafe);
static TAutoConsoleVariable<int32> CVarWatermarkCorner(
    TEXT("r.DLSS5.Watermark.Corner"), 3,
    TEXT("Watermark corner: 0=top left, 1=top right, 2=bottom left, 3=bottom right."), ECVF_RenderThreadSafe);
static TAutoConsoleVariable<float> CVarWatermarkScale(
    TEXT("r.DLSS5.Watermark.Scale"), 1.0f,
    TEXT("Watermark size relative to image height. Clamped to 0.5..4."), ECVF_RenderThreadSafe);

static TAutoConsoleVariable<int32> CVarDLSS5NRPreset(
    TEXT("r.DLSS5.Preset"), 1,
    TEXT("Opaque runtime preset hint 0..3. Different networks or visual effects are not guaranteed."),
    ECVF_RenderThreadSafe);

static TAutoConsoleVariable<int32> CVarDLSS5NRPassCount(
    TEXT("r.DLSS5.PassCount"), 1,
    TEXT("Sequential DLSS 5 Neural Rendering pass count. 1..4. Higher values increase GPU cost."),
    ECVF_RenderThreadSafe);

static TAutoConsoleVariable<float> CVarDLSS5NRIntensity(
    TEXT("r.DLSS5.Intensity"), 1.0f,
    TEXT("Neural rendering intensity."), ECVF_RenderThreadSafe);

static TAutoConsoleVariable<float> CVarDLSS5NRLocalTone(
    TEXT("r.DLSS5.LocalTone"), 1.0f,
    TEXT("Local tone strength."), ECVF_RenderThreadSafe);

static TAutoConsoleVariable<float> CVarDLSS5NRLocalStructure(
    TEXT("r.DLSS5.LocalStructure"), 1.0f,
    TEXT("Local structure strength."), ECVF_RenderThreadSafe);

static TAutoConsoleVariable<float> CVarDLSS5NRSkinStructure(
    TEXT("r.DLSS5.SkinStructure"), 1.0f,
    TEXT("Skin structure strength."), ECVF_RenderThreadSafe);

static TAutoConsoleVariable<int32> CVarDLSS5NRAutoMask(
    TEXT("r.DLSS5.AutoMask"), 1,
    TEXT("Use automatic mask."), ECVF_RenderThreadSafe);

static TAutoConsoleVariable<int32> CVarDLSS5NRStyle(
    TEXT("r.DLSS5.Style"), 0,
    TEXT("Style. 0=Natural, 1=Cinematic/alternate."), ECVF_RenderThreadSafe);

static TAutoConsoleVariable<int32> CVarDLSS5NRUICorrection(
    TEXT("r.DLSS5.UICorrection"), 0,
    TEXT("Enable DLSS-NR UI correction hint. UE Slate is downstream of this pass."), ECVF_RenderThreadSafe);

static TAutoConsoleVariable<int32> CVarDLSS5NREnableSceneGuides(
    TEXT("r.DLSS5.EnableSceneGuides"), 0,
    TEXT("Master safety switch for experimental depth/motion scene guides. 0=Color-only fallback 1=Allow guides."),
    ECVF_RenderThreadSafe);

static TAutoConsoleVariable<int32> CVarDLSS5NRFeedSceneGuides(
    TEXT("r.DLSS5.FeedSceneGuides"), 0,
    TEXT("Feed generated scene guides into the NR runtime. 0=Generate/diagnose only 1=Feed guides. Safe default is 0."),
    ECVF_RenderThreadSafe);

static TAutoConsoleVariable<int32> CVarDLSS5NRUseDepth(
    TEXT("r.DLSS5.UseDepth"), 1,
    TEXT("Feed UE scene depth to DLSS-NR when available. 0=Off 1=On."), ECVF_RenderThreadSafe);

static TAutoConsoleVariable<int32> CVarDLSS5NRUseMotionVectors(
    TEXT("r.DLSS5.UseMotionVectors"), 0,
    TEXT("Build and feed dense DLSS-compatible UE motion vectors when available. 0=Off 1=On."), ECVF_RenderThreadSafe);

static TAutoConsoleVariable<int32> CVarDLSS5NRAutoResetHistory(
    TEXT("r.DLSS5.AutoResetHistory"), 1,
    TEXT("Reset NR history on camera cuts, forced camera reset, first frame, rect changes, and large exposure discontinuities."), ECVF_RenderThreadSafe);

static TAutoConsoleVariable<int32> CVarDLSS5NRDepthInverted(
    TEXT("r.DLSS5.DepthInverted"), 1,
    TEXT("UE uses reversed-Z on D3D12, so default is 1."), ECVF_RenderThreadSafe);

static TAutoConsoleVariable<float> CVarDLSS5NRMVecScaleX(
    TEXT("r.DLSS5.MVecScaleX"), 1.0f,
    TEXT("Debug X multiplier on display-pixel motion; 1 is the NGX pixel-space convention."), ECVF_RenderThreadSafe);

static TAutoConsoleVariable<float> CVarDLSS5NRMVecScaleY(
    TEXT("r.DLSS5.MVecScaleY"), 1.0f,
    TEXT("Debug Y multiplier on display-pixel motion; 1 is the NGX pixel-space convention."), ECVF_RenderThreadSafe);

static TAutoConsoleVariable<float> CVarDLSS5NRExposureResetRatio(
    TEXT("r.DLSS5.ExposureResetRatio"), 2.0f,
    TEXT("Reset history when pre-exposure changes by more than this ratio. Minimum 1.1."), ECVF_RenderThreadSafe);

static TAutoConsoleVariable<int32> CVarDLSS5NRDebug(
    TEXT("r.DLSS5.Debug"), 1,
    TEXT("Verbose DLSS5 logging."), ECVF_RenderThreadSafe);

static TAutoConsoleVariable<int32> CVarHighlight(TEXT("r.DLSS5.HighlightPreserve"),0,
    TEXT("Optional post-tonemap highlight recovery. Disabled by default; validate thresholds for the output encoding."),ECVF_RenderThreadSafe);
static TAutoConsoleVariable<float> CVarHighlightStart(TEXT("r.DLSS5.HighlightPreserveStart"),0.7f,
    TEXT("Recovery start in current post-tonemap buffer units."),ECVF_RenderThreadSafe);
static TAutoConsoleVariable<float> CVarHighlightEnd(TEXT("r.DLSS5.HighlightPreserveEnd"),0.9f,
    TEXT("Recovery end. Start >= End disables RGB recovery."),ECVF_RenderThreadSafe);

namespace DLSS5NR
{
    float GetColorPreserve() { return FMath::Clamp(CVarColorPreserve.GetValueOnRenderThread(), 0.0f, 1.0f); }
    float GetTonePreserve() { return FMath::Clamp(CVarTonePreserve.GetValueOnRenderThread(), 0.0f, 1.0f); }
    float GetNeuralDetail() { return FMath::Clamp(CVarNeuralDetail.GetValueOnRenderThread(), 0.0f, 2.0f); }
    float GetBlend() { return FMath::Clamp(CVarBlend.GetValueOnRenderThread(), 0.0f, 1.0f); }
    int32 GetCompare() { return CVarCompare.GetValueOnRenderThread(); }
    float GetCompareSplit() { return FMath::Clamp(CVarCompareSplit.GetValueOnRenderThread(), 0.0f, 1.0f); }
    int32 GetHighlightPreserve() { return CVarHighlight.GetValueOnRenderThread(); }
    float GetHighlightStart() { return CVarHighlightStart.GetValueOnRenderThread(); }
    float GetHighlightEnd() { return CVarHighlightEnd.GetValueOnRenderThread(); }
    int32 GetWatermark() { return CVarWatermark.GetValueOnRenderThread(); }
    int32 GetWatermarkCorner() { return FMath::Clamp(CVarWatermarkCorner.GetValueOnRenderThread(), 0, 3); }
    float GetWatermarkScale() { return FMath::Clamp(CVarWatermarkScale.GetValueOnRenderThread(), 0.5f, 4.0f); }
    int32 GetEnable() { return CVarDLSS5NREnable.GetValueOnRenderThread(); }
    int32 GetPreset() { return CVarDLSS5NRPreset.GetValueOnRenderThread(); }
    int32 GetPassCount() { return CVarDLSS5NRPassCount.GetValueOnRenderThread(); }
    float GetIntensity() { return CVarDLSS5NRIntensity.GetValueOnRenderThread(); }
    float GetLocalTone() { return CVarDLSS5NRLocalTone.GetValueOnRenderThread(); }
    float GetLocalStructure() { return CVarDLSS5NRLocalStructure.GetValueOnRenderThread(); }
    float GetSkinStructure() { return CVarDLSS5NRSkinStructure.GetValueOnRenderThread(); }
    int32 GetAutoMask() { return CVarDLSS5NRAutoMask.GetValueOnRenderThread(); }
    int32 GetStyle() { return CVarDLSS5NRStyle.GetValueOnRenderThread(); }
    int32 GetUICorrection() { return CVarDLSS5NRUICorrection.GetValueOnRenderThread(); }
    int32 GetEnableSceneGuides() { return CVarDLSS5NREnableSceneGuides.GetValueOnRenderThread(); }
    int32 GetFeedSceneGuides() { return CVarDLSS5NRFeedSceneGuides.GetValueOnRenderThread(); }
    int32 GetUseDepth() { return CVarDLSS5NRUseDepth.GetValueOnRenderThread(); }
    int32 GetUseMotionVectors() { return CVarDLSS5NRUseMotionVectors.GetValueOnRenderThread(); }
    int32 GetAutoResetHistory() { return CVarDLSS5NRAutoResetHistory.GetValueOnRenderThread(); }
    int32 GetDepthInverted() { return CVarDLSS5NRDepthInverted.GetValueOnRenderThread(); }
    float GetMVecScaleX() { return CVarDLSS5NRMVecScaleX.GetValueOnRenderThread(); }
    float GetMVecScaleY() { return CVarDLSS5NRMVecScaleY.GetValueOnRenderThread(); }
    float GetExposureResetRatio() { return CVarDLSS5NRExposureResetRatio.GetValueOnRenderThread(); }
    int32 GetDebug() { return CVarDLSS5NRDebug.GetValueOnRenderThread(); }
}

float DLSS5NR::GetArtStructure() { return FMath::Clamp(CVarGlobalStructure.GetValueOnAnyThread(), 0.f, 1.f); }
float DLSS5NR::GetArtTone() { return FMath::Clamp(CVarGlobalTone.GetValueOnAnyThread(), 0.f, 1.f); }
int32 DLSS5NR::GetDeveloperMasking() { return CVarbDeveloperMasking.GetValueOnAnyThread(); }
int32 DLSS5NR::GetShowDeveloperMasks() { return CVarbShowDeveloperMasks.GetValueOnAnyThread(); }
FVector4f DLSS5NR::GetMaskGroup(int32 Index)
{
    if (Index == 0) return FVector4f(CVarbMaskGroup1Enabled.GetValueOnAnyThread() ? FMath::Clamp(CVarMaskGroup1Stencil.GetValueOnAnyThread(), 1, 255) : -1,
        FMath::Clamp(CVarMaskGroup1Structure.GetValueOnAnyThread(), 0.f, 1.f), FMath::Clamp(CVarMaskGroup1Tone.GetValueOnAnyThread(), 0.f, 1.f), 0);
    if (Index == 1) return FVector4f(CVarbMaskGroup2Enabled.GetValueOnAnyThread() ? FMath::Clamp(CVarMaskGroup2Stencil.GetValueOnAnyThread(), 1, 255) : -1,
        FMath::Clamp(CVarMaskGroup2Structure.GetValueOnAnyThread(), 0.f, 1.f), FMath::Clamp(CVarMaskGroup2Tone.GetValueOnAnyThread(), 0.f, 1.f), 0);
    if (Index == 2) return FVector4f(CVarbMaskGroup3Enabled.GetValueOnAnyThread() ? FMath::Clamp(CVarMaskGroup3Stencil.GetValueOnAnyThread(), 1, 255) : -1,
        FMath::Clamp(CVarMaskGroup3Structure.GetValueOnAnyThread(), 0.f, 1.f), FMath::Clamp(CVarMaskGroup3Tone.GetValueOnAnyThread(), 0.f, 1.f), 0);
    return FVector4f(-1, 1, 1, 0);
}
