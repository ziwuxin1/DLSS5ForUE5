#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "DLSS5NRSettings.generated.h"

UENUM()
enum class EDLSS5NRStyle : uint8
{
    Natural UMETA(DisplayName = "Natural"),
    Cinematic UMETA(DisplayName = "Cinematic")
};

/**
 * Artist-facing settings model for DLSS 5 for UE5.
 *
 * The render thread still consumes console variables internally because they are cheap and
 * thread-safe. The dedicated editor window edits this config object and synchronizes the render-thread CVars.
 */
UCLASS(Config = Engine, DefaultConfig, meta = (DisplayName = "DLSS 5 for UE5"))
class DLSS5FORUE5_NVNGX_API UDLSS5NRSettings : public UObject
{
    GENERATED_BODY()

public:
    UDLSS5NRSettings();

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

    // Startup restores saved values below explicit launch/console CVar priorities.
    void ApplyToCVars(bool bStartup = false) const;
    void LoadUserSettings();
    void SaveUserSettings();
    void LoadFromCVars();
    // Restore constructor defaults, not the mutable CDO or saved project values.
    void ResetToDefaults();
    // Reset only this control using the registered CVar's immutable factory value.
    // Does not save; the editor calls SaveUserSettings after a successful reset.
    bool ResetPropertyToDefault(FName PropertyName);
    void RefreshDiagnostics();
    bool ExportPreset(const FString& Filename, FText& Error) const;
    // Validates the complete file before changing this object; caller applies/saves on success.
    bool ImportPreset(const FString& Filename, FText& Error);

    // ---------------- General ----------------
    UPROPERTY(EditAnywhere, Config, Category = "General", meta = (DisplayName = "Enable Neural Rendering"))
    bool bEnableNeuralRendering = false;

    UPROPERTY(EditAnywhere, Config, Category = "General")
    EDLSS5NRStyle Style = EDLSS5NRStyle::Natural;

    UPROPERTY(EditAnywhere, Config, Category = "General", meta = (ClampMin = "0", ClampMax = "3", UIMin = "0", UIMax = "3", DisplayName = "NR Preset"))
    int32 Preset = 1;

    // Edited by the dedicated Slate slider in the DLSS 5 window rather than the generic DetailsView.
    UPROPERTY(Config)
    int32 PassCount = 1;

    // UE-owned final-image controls: independent of opaque NGX parameter hints.
    UPROPERTY(EditAnywhere, Config, Category = "General", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", DisplayName = "Effect Strength", ToolTip = "0 = original UE image, 1 = full NR output. Blends the final images in UE; applies to screenshots and MRQ too."))
    float EffectStrength = 1.0f;

    UPROPERTY(EditAnywhere, Config, Category = "Comparison", meta = (DisplayName = "Compare Original / DLSS5", ToolTip = "Left: original UE image. Right: processed image. Comparison is included in screenshots and MRQ; turn off for a full processed render."))
    bool bCompare = false;

    UPROPERTY(EditAnywhere, Config, Category = "Comparison", meta = (EditCondition = "bCompare", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", DisplayName = "Split Position", ToolTip = "0 = all processed, 0.5 = equal halves, 1 = all original."))
    float SplitPosition = 0.5f;

    // Composited into scene color, so screenshots and MRQ image sequences retain it.
    UPROPERTY(EditAnywhere, Config, Category = "Capture", meta = (DisplayName = "Show DLSS5 ON / OFF Watermark"))
    bool bShowWatermark = false;

    UPROPERTY(EditAnywhere, Config, Category = "Capture", meta = (EditCondition = "bShowWatermark", ClampMin = "0", ClampMax = "3", ToolTip = "0: top left, 1: top right, 2: bottom left, 3: bottom right"))
    int32 WatermarkCorner = 3;

    UPROPERTY(EditAnywhere, Config, Category = "Capture", meta = (EditCondition = "bShowWatermark", ClampMin = "0.5", ClampMax = "4.0"))
    float WatermarkScale = 1.0f;

    // ---------------- Neural Controls ----------------
    UPROPERTY(EditAnywhere, Config, Category = "Neural Controls", meta = (ClampMin = "0.0", ClampMax = "2.0", UIMin = "0.0", UIMax = "2.0"))
    float Intensity = 1.0f;

    UPROPERTY(EditAnywhere, Config, Category = "Neural Controls", meta = (ClampMin = "0.0", ClampMax = "2.0", UIMin = "0.0", UIMax = "2.0", DisplayName = "Local Tone"))
    float LocalTone = 1.0f;

    UPROPERTY(EditAnywhere, Config, Category = "Neural Controls", meta = (ClampMin = "0.0", ClampMax = "2.0", UIMin = "0.0", UIMax = "2.0", DisplayName = "Local Structure"))
    float LocalStructure = 1.0f;

    UPROPERTY(EditAnywhere, Config, Category = "Neural Controls", meta = (ClampMin = "0.0", ClampMax = "2.0", UIMin = "0.0", UIMax = "2.0", DisplayName = "Skin Structure"))
    float SkinStructure = 1.0f;

    UPROPERTY(EditAnywhere, Config, Category = "Neural Controls", meta = (DisplayName = "Automatic Mask"))
    bool bAutoMask = true;

    UPROPERTY(EditAnywhere, Config, Category = "Neural Controls", meta = (DisplayName = "UI Correction"))
    bool bUICorrection = false;

    UPROPERTY(EditAnywhere, Config, Category = "Color Preservation", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", DisplayName = "Original Color Preserve", ToolTip = "0 disables the filter. Higher values restore the source color base while retaining neural detail. Supports SDR sRGB and tone-mapped linear MRQ; Other output color spaces are bypassed."))
    float ColorPreserve = 0.0f;

    UPROPERTY(EditAnywhere, Config, Category = "Color Preservation", meta = (EditCondition = "ColorPreserve > 0", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", DisplayName = "Original Tone Preserve", ToolTip = "0 retains neural lightness; 1 restores the source lightness base. Scaled by Original Color Preserve."))
    float TonePreserve = 0.0f;

    UPROPERTY(EditAnywhere, Config, Category = "Color Preservation", meta = (EditCondition = "ColorPreserve > 0", ClampMin = "0.0", ClampMax = "2.0", UIMin = "0.0", UIMax = "2.0", DisplayName = "Neural Detail", ToolTip = "1 retains neural detail; lower values soften it, higher values emphasize it and may amplify noise. Scaled by Original Color Preserve."))
    float NeuralDetail = 1.0f;

    // ---------------- Scene Guides ----------------
    UPROPERTY(EditAnywhere, Config, Category = "Scene Guides", meta = (DisplayName = "Enable 3D Scene Guides (Experimental)", ToolTip = "Master safety switch for depth and motion-vector guides. Off keeps the proven v0.3 color-only path. Defaults off. Configuration is respected; recovery can disable NR after an unclean NR-active session."))
    bool bEnableSceneGuides = false;

    UPROPERTY(EditAnywhere, Config, Category = "Scene Guides", meta = (EditCondition = "bEnableSceneGuides", DisplayName = "Feed Scene Guides Into NR (Experimental)", ToolTip = "Safe staging switch. Off generates/diagnoses guide textures without handing them to NGX. Turn this on only after guide generation is stable."))
    bool bFeedSceneGuides = false;

    UPROPERTY(EditAnywhere, Config, Category = "Scene Guides", meta = (EditCondition = "bEnableSceneGuides", DisplayName = "Use DLSS Input Depth (UE Scene Depth)", ToolTip = "Uses Unreal Engine scene depth, which is the host depth source that DLSS/Streamline consumes. DLSS does not generate a separate depth buffer. Falls back to color-only if unavailable."))
    bool bUseDepth = true;

    UPROPERTY(EditAnywhere, Config, Category = "Scene Guides", meta = (EditCondition = "bEnableSceneGuides", DisplayName = "Use DLSS-Compatible Motion Vectors", ToolTip = "Builds a dense pixel-space motion-vector guide from UE depth + GBuffer velocity. Defaults off; enable after guide validation."))
    bool bUseMotionVectors = false;

    UPROPERTY(EditAnywhere, Config, Category = "Scene Guides", meta = (DisplayName = "Automatic History Reset", ToolTip = "Resets NR temporal history on camera cuts, forced camera resets, first frame, viewport-size changes, and major exposure discontinuities."))
    bool bAutoResetHistory = true;

    // ---------------- Advanced ----------------
    UPROPERTY(EditAnywhere, Config, Category = "Advanced", meta = (DisplayName = "Depth Is Reversed-Z", ToolTip = "UE D3D12 uses reversed-Z by default."))
    bool bDepthInverted = true;

    UPROPERTY(EditAnywhere, Config, Category = "Advanced", meta = (ClampMin = "-4.0", ClampMax = "4.0", UIMin = "-2.0", UIMax = "2.0", DisplayName = "Motion Scale X Multiplier"))
    float MotionScaleXMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, Config, Category = "Advanced", meta = (ClampMin = "-4.0", ClampMax = "4.0", UIMin = "-2.0", UIMax = "2.0", DisplayName = "Motion Scale Y Multiplier"))
    float MotionScaleYMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, Config, Category = "Advanced", meta = (ClampMin = "1.1", ClampMax = "8.0", UIMin = "1.1", UIMax = "4.0", DisplayName = "Exposure Reset Ratio", ToolTip = "If pre-exposure changes by more than this ratio between frames, reset NR history."))
    float ExposureResetRatio = 2.0f;

    UPROPERTY(EditAnywhere, Config, Category = "Advanced")
    bool bVerboseDiagnostics = true;

    // ---------------- Live Diagnostics ----------------
    // Legacy diagnostic strings retained for internal callers; omitted from the artist panel.
    FString DiagnosticRuntime;
    FString DiagnosticNGX;
    FString DiagnosticGuides;
    FString DiagnosticSubrects;
    FString DiagnosticTemporal;
    FString DiagnosticCounters;
    FString DiagnosticLastResult;
};
