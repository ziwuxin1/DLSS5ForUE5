#include "MoviePipelineDLSS5Setting.h"

#include "HAL/IConsoleManager.h"
#include "Modules/ModuleManager.h"
#include "MoviePipelineAntiAliasingSetting.h"
#include "MoviePipelineColorSetting.h"
#include "MoviePipelineHighResSetting.h"
#include "MoviePipelinePrimaryConfig.h"
#include "MovieRenderPipelineDataTypes.h"

IMPLEMENT_MODULE(FDefaultModuleImpl, DLSS5ForUE5MoviePipeline);

void UMoviePipelineDLSS5Setting::OverrideCVar(const TCHAR* Name, const FString& Value)
{
    if (IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(Name))
    {
        SavedCVars.Add({Name, Variable->GetString(), uint32(Variable->GetFlags() & ECVF_SetByMask)});
        Variable->Set(*Value, ECVF_SetByConsole);
    }
}

void UMoviePipelineDLSS5Setting::SetupForPipelineImpl(UMoviePipeline* InPipeline)
{
    Super::SetupForPipelineImpl(InPipeline);
    // MRQ instantiates every available setting as a transient default, even if a
    // user never added it. Installing this plugin must not enable NR in those jobs.
    const auto* Config = GetTypedOuter<UMoviePipelinePrimaryConfig>();
    if (!Config || !Config->GetUserSettings().Contains(this)) return;
    // Defensive against duplicate setup; never overwrite the original restore point.
    if (!SavedCVars.IsEmpty()) return;
    OverrideCVar(TEXT("r.DLSS5.Enable"), bEnableNeuralRendering ? TEXT("1") : TEXT("0"));
    OverrideCVar(TEXT("r.DLSS5.PassCount"), FString::FromInt(FMath::Clamp(PassCount, 1, 4)));
    OverrideCVar(TEXT("r.DLSS5.Watermark"), bShowWatermark ? TEXT("1") : TEXT("0"));
    OverrideCVar(TEXT("r.DLSS5.Watermark.Corner"), FString::FromInt(FMath::Clamp(WatermarkCorner, 0, 3)));
    OverrideCVar(TEXT("r.DLSS5.Watermark.Scale"), FString::SanitizeFloat(FMath::Clamp(WatermarkScale, 0.5f, 4.0f)));
}

void UMoviePipelineDLSS5Setting::TeardownForPipelineImpl(UMoviePipeline* InPipeline)
{
    for (int32 Index = SavedCVars.Num() - 1; Index >= 0; --Index)
    {
        const FSavedCVar& Saved = SavedCVars[Index];
        if (IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(*Saved.Name))
        {
            Variable->Set(*Saved.Value, ECVF_SetByConsole);
            Variable->SetFlags(EConsoleVariableFlags((Variable->GetFlags() & ~ECVF_SetByMask) | Saved.Priority));
        }
    }
    SavedCVars.Reset();
    Super::TeardownForPipelineImpl(InPipeline);
}

void UMoviePipelineDLSS5Setting::ValidateStateImpl()
{
    Super::ValidateStateImpl();
    const UMoviePipelinePrimaryConfig* Config = GetTypedOuter<UMoviePipelinePrimaryConfig>();
    if (!Config || (!bEnableNeuralRendering && !bShowWatermark)) return;
    if (const auto* HighRes = Config->FindSetting<UMoviePipelineHighResSetting>())
    {
        if (HighRes->TileCount > 1)
        {
            ValidationState = EMoviePipelineValidationState::Errors;
            ValidationResults.Add(NSLOCTEXT("DLSS5", "Tiles", "DLSS5 capture requires High Resolution Tile Count = 1. Render 3840 x 2160 as a full frame; tiled NR and repeated watermarks are not supported."));
        }
    }
    if (bEnableNeuralRendering)
    {
        if (const auto* Color = Config->FindSetting<UMoviePipelineColorSetting>())
        {
            if (Color->bDisableToneCurve || Color->OCIOConfiguration.bIsEnabled)
            {
                ValidationState = EMoviePipelineValidationState::Errors;
                ValidationResults.Add(NSLOCTEXT("DLSS5", "ToneCurve", "Keep Disable Tone Curve unchecked for the post-tonemap DLSS5 Neural Rendering path."));
            }
        }
        if (const auto* AA = Config->FindSetting<UMoviePipelineAntiAliasingSetting>())
        {
            if (AA->SpatialSampleCount > 1 || AA->TemporalSampleCount > 1)
            {
                if (ValidationState == EMoviePipelineValidationState::Valid) ValidationState = EMoviePipelineValidationState::Warnings;
                ValidationResults.Add(NSLOCTEXT("DLSS5", "Samples", "Start with one spatial and one temporal sample. NR runs per sample before MRQ accumulation; validate ghosting before increasing samples."));
            }
        }
    }
}

void UMoviePipelineDLSS5Setting::GetFormatArguments(FMoviePipelineFormatArgs& InOutFormatArgs) const
{
    Super::GetFormatArguments(InOutFormatArgs);
    // This metadata records the requested job setting, not proof of successful inference.
    InOutFormatArgs.FilenameArguments.Add(TEXT("dlss5"), bEnableNeuralRendering ? TEXT("ON") : TEXT("OFF"));
    InOutFormatArgs.FileMetadata.Add(TEXT("unreal/dlss5/requested"), bEnableNeuralRendering ? TEXT("ON") : TEXT("OFF"));
}
