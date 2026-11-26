#include "DLSS5NRSettings.h"

#include "DLSS5NRRuntime.h"
#include "HAL/IConsoleManager.h"
#include "Misc/ConfigCacheIni.h"
#include "UObject/UnrealType.h"

#define LOCTEXT_NAMESPACE "DLSS5NRSettings"

namespace
{
    void SetIntCVar(const TCHAR* Name, int32 Value, EConsoleVariableFlags Priority)
    {
        if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(Name))
        {
            if (uint32(CVar->GetFlags() & ECVF_SetByMask) <= uint32(Priority))
                CVar->Set(Value, Priority);
        }
    }

    void SetFloatCVar(const TCHAR* Name, float Value, EConsoleVariableFlags Priority)
    {
        if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(Name))
        {
            if (uint32(CVar->GetFlags() & ECVF_SetByMask) <= uint32(Priority))
                CVar->Set(Value, Priority);
        }
    }

    FString RectToString(const FIntRect& Rect)
    {
        return FString::Printf(TEXT("(%d,%d) %dx%d"), Rect.Min.X, Rect.Min.Y, Rect.Width(), Rect.Height());
    }
}

UDLSS5NRSettings::UDLSS5NRSettings()
{
    DiagnosticRuntime = TEXT("Waiting for renderer...");
    DiagnosticNGX = TEXT("Waiting for renderer...");
    DiagnosticGuides = TEXT("Waiting for renderer...");
    DiagnosticSubrects = TEXT("Waiting for renderer...");
    DiagnosticTemporal = TEXT("Waiting for renderer...");
    DiagnosticCounters = TEXT("0 attempts / 0 successful / 0 failed");
    DiagnosticLastResult = TEXT("No evaluation yet");
}

void UDLSS5NRSettings::LoadFromCVars()
{
    if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DLSS5.Art.Structure"))) GlobalStructure = V->GetFloat();
    if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DLSS5.Art.Tone"))) GlobalTone = V->GetFloat();
    if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DLSS5.Mask.Enable"))) bDeveloperMasking = V->GetInt() != 0;
    if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DLSS5.Mask.Preview"))) bShowDeveloperMasks = V->GetInt() != 0;
    if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DLSS5.Mask.Group1.Enable"))) bMaskGroup1Enabled = V->GetInt() != 0;
    if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DLSS5.Mask.Group1.Stencil"))) MaskGroup1Stencil = V->GetInt();
    if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DLSS5.Mask.Group1.Structure"))) MaskGroup1Structure = V->GetFloat();
    if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DLSS5.Mask.Group1.Tone"))) MaskGroup1Tone = V->GetFloat();
    if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DLSS5.Mask.Group2.Enable"))) bMaskGroup2Enabled = V->GetInt() != 0;
    if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DLSS5.Mask.Group2.Stencil"))) MaskGroup2Stencil = V->GetInt();
    if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DLSS5.Mask.Group2.Structure"))) MaskGroup2Structure = V->GetFloat();
    if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DLSS5.Mask.Group2.Tone"))) MaskGroup2Tone = V->GetFloat();
    if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DLSS5.Mask.Group3.Enable"))) bMaskGroup3Enabled = V->GetInt() != 0;
    if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DLSS5.Mask.Group3.Stencil"))) MaskGroup3Stencil = V->GetInt();
    if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DLSS5.Mask.Group3.Structure"))) MaskGroup3Structure = V->GetFloat();
    if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DLSS5.Mask.Group3.Tone"))) MaskGroup3Tone = V->GetFloat();
    if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DLSS5.ColorPreserve"))) ColorPreserve = V->GetFloat();
    if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DLSS5.TonePreserve"))) TonePreserve = V->GetFloat();
    if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DLSS5.NeuralDetail"))) NeuralDetail = V->GetFloat();
    if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DLSS5.Blend"))) EffectStrength = V->GetFloat();
    if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DLSS5.Compare"))) bCompare = V->GetInt() != 0;
    if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DLSS5.Compare.Split"))) SplitPosition = V->GetFloat();
    if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DLSS5.Watermark"))) bShowWatermark = V->GetInt() != 0;
    if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DLSS5.Watermark.Corner"))) WatermarkCorner = V->GetInt();
    if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DLSS5.Watermark.Scale"))) WatermarkScale = V->GetFloat();
    if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DLSS5.Enable"))) bEnableNeuralRendering = V->GetInt() != 0;
    if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DLSS5.Style"))) Style = static_cast<EDLSS5NRStyle>(FMath::Clamp(V->GetInt(), 0, 1));
    if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DLSS5.Preset"))) Preset = V->GetInt();
    if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DLSS5.PassCount"))) PassCount = V->GetInt();
    if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DLSS5.Intensity"))) Intensity = V->GetFloat();
    if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DLSS5.LocalTone"))) LocalTone = V->GetFloat();
    if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DLSS5.LocalStructure"))) LocalStructure = V->GetFloat();
    if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DLSS5.SkinStructure"))) SkinStructure = V->GetFloat();
    if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DLSS5.AutoMask"))) bAutoMask = V->GetInt() != 0;
    if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DLSS5.UICorrection"))) bUICorrection = V->GetInt() != 0;
    if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DLSS5.EnableSceneGuides"))) bEnableSceneGuides = V->GetInt() != 0;
    if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DLSS5.FeedSceneGuides"))) bFeedSceneGuides = V->GetInt() != 0;
    if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DLSS5.UseDepth"))) bUseDepth = V->GetInt() != 0;
    if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DLSS5.UseMotionVectors"))) bUseMotionVectors = V->GetInt() != 0;
    if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DLSS5.AutoResetHistory"))) bAutoResetHistory = V->GetInt() != 0;
    if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DLSS5.DepthInverted"))) bDepthInverted = V->GetInt() != 0;
    if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DLSS5.MVecScaleX"))) MotionScaleXMultiplier = V->GetFloat();
    if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DLSS5.MVecScaleY"))) MotionScaleYMultiplier = V->GetFloat();
    if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DLSS5.ExposureResetRatio"))) ExposureResetRatio = V->GetFloat();
    if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DLSS5.Debug"))) bVerboseDiagnostics = V->GetInt() != 0;
}

void UDLSS5NRSettings::ApplyToCVars(bool bStartup) const
{
    const EConsoleVariableFlags Priority = bStartup ? ECVF_SetByProjectSetting : ECVF_SetByConsole;
    SetFloatCVar(TEXT("r.DLSS5.Art.Structure"), FMath::Clamp(GlobalStructure, 0.0f, 1.0f), Priority);
    SetFloatCVar(TEXT("r.DLSS5.Art.Tone"), FMath::Clamp(GlobalTone, 0.0f, 1.0f), Priority);
    SetIntCVar(TEXT("r.DLSS5.Mask.Enable"), bDeveloperMasking ? 1 : 0, Priority);
    SetIntCVar(TEXT("r.DLSS5.Mask.Preview"), bShowDeveloperMasks ? 1 : 0, Priority);
    SetIntCVar(TEXT("r.DLSS5.Mask.Group1.Enable"), bMaskGroup1Enabled ? 1 : 0, Priority);
    SetIntCVar(TEXT("r.DLSS5.Mask.Group1.Stencil"), FMath::Clamp(MaskGroup1Stencil, 1, 255), Priority);
    SetFloatCVar(TEXT("r.DLSS5.Mask.Group1.Structure"), FMath::Clamp(MaskGroup1Structure, 0.0f, 1.0f), Priority);
    SetFloatCVar(TEXT("r.DLSS5.Mask.Group1.Tone"), FMath::Clamp(MaskGroup1Tone, 0.0f, 1.0f), Priority);
    SetIntCVar(TEXT("r.DLSS5.Mask.Group2.Enable"), bMaskGroup2Enabled ? 1 : 0, Priority);
    SetIntCVar(TEXT("r.DLSS5.Mask.Group2.Stencil"), FMath::Clamp(MaskGroup2Stencil, 1, 255), Priority);
    SetFloatCVar(TEXT("r.DLSS5.Mask.Group2.Structure"), FMath::Clamp(MaskGroup2Structure, 0.0f, 1.0f), Priority);
    SetFloatCVar(TEXT("r.DLSS5.Mask.Group2.Tone"), FMath::Clamp(MaskGroup2Tone, 0.0f, 1.0f), Priority);
    SetIntCVar(TEXT("r.DLSS5.Mask.Group3.Enable"), bMaskGroup3Enabled ? 1 : 0, Priority);
    SetIntCVar(TEXT("r.DLSS5.Mask.Group3.Stencil"), FMath::Clamp(MaskGroup3Stencil, 1, 255), Priority);
    SetFloatCVar(TEXT("r.DLSS5.Mask.Group3.Structure"), FMath::Clamp(MaskGroup3Structure, 0.0f, 1.0f), Priority);
    SetFloatCVar(TEXT("r.DLSS5.Mask.Group3.Tone"), FMath::Clamp(MaskGroup3Tone, 0.0f, 1.0f), Priority);
    if (bDeveloperMasking) SetIntCVar(TEXT("r.CustomDepth"), 3, Priority);
    SetFloatCVar(TEXT("r.DLSS5.ColorPreserve"), FMath::Clamp(ColorPreserve, 0.0f, 1.0f), Priority);
    SetFloatCVar(TEXT("r.DLSS5.TonePreserve"), FMath::Clamp(TonePreserve, 0.0f, 1.0f), Priority);
    SetFloatCVar(TEXT("r.DLSS5.NeuralDetail"), FMath::Clamp(NeuralDetail, 0.0f, 2.0f), Priority);
    SetFloatCVar(TEXT("r.DLSS5.Blend"), FMath::Clamp(EffectStrength, 0.0f, 1.0f), Priority);
    SetIntCVar(TEXT("r.DLSS5.Compare"), bCompare ? 1 : 0, Priority);
    SetFloatCVar(TEXT("r.DLSS5.Compare.Split"), FMath::Clamp(SplitPosition, 0.0f, 1.0f), Priority);
    SetIntCVar(TEXT("r.DLSS5.Watermark"), bShowWatermark ? 1 : 0, Priority);
    SetIntCVar(TEXT("r.DLSS5.Watermark.Corner"), FMath::Clamp(WatermarkCorner, 0, 3), Priority);
    SetFloatCVar(TEXT("r.DLSS5.Watermark.Scale"), FMath::Clamp(WatermarkScale, 0.5f, 4.0f), Priority);
    SetIntCVar(TEXT("r.DLSS5.Enable"), bEnableNeuralRendering ? 1 : 0, Priority);
    SetIntCVar(TEXT("r.DLSS5.Style"), static_cast<int32>(Style), Priority);
    SetIntCVar(TEXT("r.DLSS5.Preset"), FMath::Clamp(Preset, 0, 3), Priority);
    SetIntCVar(TEXT("r.DLSS5.PassCount"), FMath::Clamp(PassCount, 1, 4), Priority);
    SetFloatCVar(TEXT("r.DLSS5.Intensity"), Intensity, Priority);
    SetFloatCVar(TEXT("r.DLSS5.LocalTone"), LocalTone, Priority);
    SetFloatCVar(TEXT("r.DLSS5.LocalStructure"), LocalStructure, Priority);
    SetFloatCVar(TEXT("r.DLSS5.SkinStructure"), SkinStructure, Priority);
    SetIntCVar(TEXT("r.DLSS5.AutoMask"), bAutoMask ? 1 : 0, Priority);
    SetIntCVar(TEXT("r.DLSS5.UICorrection"), bUICorrection ? 1 : 0, Priority);
    SetIntCVar(TEXT("r.DLSS5.EnableSceneGuides"), bEnableSceneGuides ? 1 : 0, Priority);
    SetIntCVar(TEXT("r.DLSS5.FeedSceneGuides"), bFeedSceneGuides ? 1 : 0, Priority);
    SetIntCVar(TEXT("r.DLSS5.UseDepth"), bUseDepth ? 1 : 0, Priority);
    SetIntCVar(TEXT("r.DLSS5.UseMotionVectors"), bUseMotionVectors ? 1 : 0, Priority);
    SetIntCVar(TEXT("r.DLSS5.AutoResetHistory"), bAutoResetHistory ? 1 : 0, Priority);
    SetIntCVar(TEXT("r.DLSS5.DepthInverted"), bDepthInverted ? 1 : 0, Priority);
    SetFloatCVar(TEXT("r.DLSS5.MVecScaleX"), MotionScaleXMultiplier, Priority);
    SetFloatCVar(TEXT("r.DLSS5.MVecScaleY"), MotionScaleYMultiplier, Priority);
    SetFloatCVar(TEXT("r.DLSS5.ExposureResetRatio"), ExposureResetRatio, Priority);
    SetIntCVar(TEXT("r.DLSS5.Debug"), bVerboseDiagnostics ? 1 : 0, Priority);
}

void UDLSS5NRSettings::ResetToDefaults()
{
    // Only the controls owned by this panel; do not touch other plugins or project settings.
    const TCHAR* Names[] = { TEXT("Art.Structure"), TEXT("Art.Tone"), TEXT("Mask.Enable"), TEXT("Mask.Preview"), TEXT("Mask.Group1.Enable"), TEXT("Mask.Group1.Stencil"), TEXT("Mask.Group1.Structure"), TEXT("Mask.Group1.Tone"), TEXT("Mask.Group2.Enable"), TEXT("Mask.Group2.Stencil"), TEXT("Mask.Group2.Structure"), TEXT("Mask.Group2.Tone"), TEXT("Mask.Group3.Enable"), TEXT("Mask.Group3.Stencil"), TEXT("Mask.Group3.Structure"), TEXT("Mask.Group3.Tone"), TEXT("Enable"), TEXT("Style"), TEXT("Preset"), TEXT("PassCount"),
        TEXT("Intensity"), TEXT("LocalTone"), TEXT("LocalStructure"), TEXT("SkinStructure"),
        TEXT("AutoMask"), TEXT("UICorrection"), TEXT("EnableSceneGuides"), TEXT("FeedSceneGuides"),
        TEXT("UseDepth"), TEXT("UseMotionVectors"), TEXT("AutoResetHistory"), TEXT("DepthInverted"),
        TEXT("MVecScaleX"), TEXT("MVecScaleY"), TEXT("ExposureResetRatio"), TEXT("Debug"),
        TEXT("Watermark"), TEXT("Watermark.Corner"), TEXT("Watermark.Scale"),
        TEXT("Blend"), TEXT("Compare"), TEXT("Compare.Split"),
        TEXT("ColorPreserve"), TEXT("TonePreserve"), TEXT("NeuralDetail") };
    for (const TCHAR* Name : Names)
    {
        if (IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(
            *FString::Printf(TEXT("r.DLSS5.%s"), Name)))
        {
            Variable->Set(*Variable->GetDefaultValue(), ECVF_SetByConsole);
        }
    }
    LoadFromCVars();
}

static IConsoleVariable* FindResetVariable(FName PropertyName)
{
    // The settings CDO is mutable (including after SaveConfig); it is not a
    // factory-default template. Never reset all CVars to obtain a single value.
    static const TPair<FName, const TCHAR*> Controls[] = {
        { GET_MEMBER_NAME_CHECKED(UDLSS5NRSettings, GlobalStructure), TEXT("Art.Structure") },
        { GET_MEMBER_NAME_CHECKED(UDLSS5NRSettings, GlobalTone), TEXT("Art.Tone") },
        { GET_MEMBER_NAME_CHECKED(UDLSS5NRSettings, bDeveloperMasking), TEXT("Mask.Enable") },
        { GET_MEMBER_NAME_CHECKED(UDLSS5NRSettings, bShowDeveloperMasks), TEXT("Mask.Preview") },
        { GET_MEMBER_NAME_CHECKED(UDLSS5NRSettings, bMaskGroup1Enabled), TEXT("Mask.Group1.Enable") },
        { GET_MEMBER_NAME_CHECKED(UDLSS5NRSettings, MaskGroup1Stencil), TEXT("Mask.Group1.Stencil") },
        { GET_MEMBER_NAME_CHECKED(UDLSS5NRSettings, MaskGroup1Structure), TEXT("Mask.Group1.Structure") },
        { GET_MEMBER_NAME_CHECKED(UDLSS5NRSettings, MaskGroup1Tone), TEXT("Mask.Group1.Tone") },
        { GET_MEMBER_NAME_CHECKED(UDLSS5NRSettings, bMaskGroup2Enabled), TEXT("Mask.Group2.Enable") },
        { GET_MEMBER_NAME_CHECKED(UDLSS5NRSettings, MaskGroup2Stencil), TEXT("Mask.Group2.Stencil") },
        { GET_MEMBER_NAME_CHECKED(UDLSS5NRSettings, MaskGroup2Structure), TEXT("Mask.Group2.Structure") },
        { GET_MEMBER_NAME_CHECKED(UDLSS5NRSettings, MaskGroup2Tone), TEXT("Mask.Group2.Tone") },
        { GET_MEMBER_NAME_CHECKED(UDLSS5NRSettings, bMaskGroup3Enabled), TEXT("Mask.Group3.Enable") },
        { GET_MEMBER_NAME_CHECKED(UDLSS5NRSettings, MaskGroup3Stencil), TEXT("Mask.Group3.Stencil") },
        { GET_MEMBER_NAME_CHECKED(UDLSS5NRSettings, MaskGroup3Structure), TEXT("Mask.Group3.Structure") },
        { GET_MEMBER_NAME_CHECKED(UDLSS5NRSettings, MaskGroup3Tone), TEXT("Mask.Group3.Tone") },
        { GET_MEMBER_NAME_CHECKED(UDLSS5NRSettings, bEnableNeuralRendering), TEXT("Enable") },
        { GET_MEMBER_NAME_CHECKED(UDLSS5NRSettings, Style), TEXT("Style") },
        { GET_MEMBER_NAME_CHECKED(UDLSS5NRSettings, Preset), TEXT("Preset") },
        { GET_MEMBER_NAME_CHECKED(UDLSS5NRSettings, EffectStrength), TEXT("Blend") },
        { GET_MEMBER_NAME_CHECKED(UDLSS5NRSettings, PassCount), TEXT("PassCount") },
        { GET_MEMBER_NAME_CHECKED(UDLSS5NRSettings, Intensity), TEXT("Intensity") },
        { GET_MEMBER_NAME_CHECKED(UDLSS5NRSettings, LocalTone), TEXT("LocalTone") },
        { GET_MEMBER_NAME_CHECKED(UDLSS5NRSettings, bShowWatermark), TEXT("Watermark") },
        { GET_MEMBER_NAME_CHECKED(UDLSS5NRSettings, WatermarkCorner), TEXT("Watermark.Corner") },
        { GET_MEMBER_NAME_CHECKED(UDLSS5NRSettings, WatermarkScale), TEXT("Watermark.Scale") },
        { GET_MEMBER_NAME_CHECKED(UDLSS5NRSettings, ColorPreserve), TEXT("ColorPreserve") },
        { GET_MEMBER_NAME_CHECKED(UDLSS5NRSettings, TonePreserve), TEXT("TonePreserve") },
        { GET_MEMBER_NAME_CHECKED(UDLSS5NRSettings, NeuralDetail), TEXT("NeuralDetail") },
        { GET_MEMBER_NAME_CHECKED(UDLSS5NRSettings, bCompare), TEXT("Compare") },
        { GET_MEMBER_NAME_CHECKED(UDLSS5NRSettings, SplitPosition), TEXT("Compare.Split") }
    };
    for (const auto& Control : Controls)
    {
        if (Control.Key != PropertyName) continue;
        return IConsoleManager::Get().FindConsoleVariable(
            *FString::Printf(TEXT("r.DLSS5.%s"), Control.Value));
    }
    return nullptr;
}

bool UDLSS5NRSettings::CanResetPropertyToDefault(FName PropertyName) const
{
    IConsoleVariable* Variable = FindResetVariable(PropertyName);
    const FProperty* Property = GetClass()->FindPropertyByName(PropertyName);
    if (!Variable || !Property) return false;

    // Compare the displayed value with the same immutable default used by reset.
    // Saved config mutates the CDO, and console overrides may differ from this object.
    const FString DefaultValue = Variable->GetDefaultValue();
    const void* Value = Property->ContainerPtrToValuePtr<void>(this);
    if (const FBoolProperty* Bool = CastField<FBoolProperty>(Property))
        return Bool->GetPropertyValue(Value) != (FCString::Atoi(*DefaultValue) != 0);
    if (const FEnumProperty* Enum = CastField<FEnumProperty>(Property))
        return Enum->GetUnderlyingProperty()->GetSignedIntPropertyValue(Value) != FCString::Atoi64(*DefaultValue);
    if (const FNumericProperty* Number = CastField<FNumericProperty>(Property))
    {
        if (Number->IsFloatingPoint())
            return !FMath::IsNearlyEqual(static_cast<float>(Number->GetFloatingPointPropertyValue(Value)), FCString::Atof(*DefaultValue), SMALL_NUMBER);
        return Number->GetSignedIntPropertyValue(Value) != FCString::Atoi64(*DefaultValue);
    }
    return false;
}

bool UDLSS5NRSettings::ResetPropertyToDefault(FName PropertyName)
{
    if (IConsoleVariable* Variable = FindResetVariable(PropertyName))
    {
        Variable->Set(*Variable->GetDefaultValue(), ECVF_SetByConsole);
        LoadFromCVars();
        return true;
    }
    return false;
}

void UDLSS5NRSettings::LoadUserSettings()
{
#if WITH_EDITOR
    // Overlay per-user choices on any defaults supplied by the project's Engine.ini.
    // Saved config avoids checking out a shared DefaultEngine.ini in Perforce projects.
    LoadConfig(GetClass(), *GEditorPerProjectIni);
#endif
}

void UDLSS5NRSettings::SaveUserSettings()
{
#if WITH_EDITOR
    SaveConfig(CPF_Config, *GEditorPerProjectIni);
#endif
}

#if WITH_EDITOR
void UDLSS5NRSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    ApplyToCVars();
    if (!(PropertyChangedEvent.ChangeType & EPropertyChangeType::Interactive)) SaveUserSettings();
}
#endif

void UDLSS5NRSettings::RefreshDiagnostics()
{
    const FDLSS5NRDiagnosticsSnapshot D = FDLSS5NRRuntime::Get().GetDiagnosticsSnapshot();

    DiagnosticRuntime = FString::Printf(
        TEXT("Runtime %s | Core %s | Core params %s | Legacy filename hint %s (diagnostic only)"),
        D.bRuntimeLoaded ? TEXT("READY") : TEXT("NOT LOADED"),
        D.bCoreFound ? TEXT("READY") : TEXT("WAITING"),
        D.bCoreParameters ? TEXT("READY") : TEXT("WAITING"),
        D.bCallerGateReady ? TEXT("PRESENT") : TEXT("ABSENT"));

    DiagnosticNGX = FString::Printf(
        TEXT("Initialized %s | Snippet %s | Feature %dx%d | Passes %d"),
        D.bRuntimeInitialized ? TEXT("YES") : TEXT("NO"),
        D.bSnippetPopulated ? TEXT("YES") : TEXT("NO"),
        D.FeatureSize.X, D.FeatureSize.Y,
        FMath::Clamp(PassCount, 1, 4));

    DiagnosticGuides = FString::Printf(
        TEXT("Depth: requested=%s available=%s fed=%s (%dx%d) | Motion: requested=%s available=%s fed=%s (%dx%d) | scale=(%.6f, %.6f)"),
        D.bDepthRequested ? TEXT("YES") : TEXT("NO"),
        D.bDepthAvailable ? TEXT("YES") : TEXT("NO"),
        D.bDepthUsedLastFrame ? TEXT("YES") : TEXT("NO"),
        D.SceneDepthExtent.X, D.SceneDepthExtent.Y,
        D.bMotionRequested ? TEXT("YES") : TEXT("NO"),
        D.bMotionAvailable ? TEXT("YES") : TEXT("NO"),
        D.bMotionUsedLastFrame ? TEXT("YES") : TEXT("NO"),
        D.SceneVelocityExtent.X, D.SceneVelocityExtent.Y,
        D.MotionScaleX, D.MotionScaleY);

    DiagnosticSubrects = FString::Printf(
        TEXT("Color %s | Depth %s | MVec %s | Output %s"),
        *RectToString(D.ColorRect), *RectToString(D.DepthRect), *RectToString(D.MotionRect), *RectToString(D.OutputRect));

    DiagnosticTemporal = FString::Printf(
        TEXT("Jitter=(%.4f, %.4f) px | PreExposure=%.4f | Reset=%s | CameraCut=%s | %s"),
        D.JitterPixels.X, D.JitterPixels.Y, D.PreExposure,
        D.bResetLastFrame ? TEXT("YES") : TEXT("NO"),
        D.bCameraCutLastFrame ? TEXT("YES") : TEXT("NO"),
        D.LastResetReason.IsEmpty() ? TEXT("history continuous") : *D.LastResetReason);

    DiagnosticCounters = FString::Printf(
        TEXT("%llu attempts | %llu successful | %llu failed | %llu resets"),
        D.EvaluateAttempts, D.SuccessfulFrames, D.FailedFrames, D.ResetFrames);

    DiagnosticLastResult = FString::Printf(TEXT("0x%08X%s%s"), D.LastNGXResult,
        D.LastError.IsEmpty() ? TEXT("") : TEXT(" | "),
        D.LastError.IsEmpty() ? TEXT("") : *D.LastError);
}

#undef LOCTEXT_NAMESPACE
