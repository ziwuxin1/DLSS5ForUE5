#pragma once

#include "CoreMinimal.h"
#include "MoviePipelineSetting.h"
#include "MoviePipelineDLSS5Setting.generated.h"

/** Saved per MRQ job; changes are restored after completion or cancellation. */
UCLASS(BlueprintType, meta = (DisplayName = "DLSS5 Neural Rendering"))
class DLSS5FORUE5MOVIEPIPELINE_API UMoviePipelineDLSS5Setting : public UMoviePipelineSetting
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DLSS5")
    bool bEnableNeuralRendering = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DLSS5", meta = (ClampMin = "1", ClampMax = "4"))
    int32 PassCount = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Watermark", meta = (DisplayName = "Show DLSS5 ON / OFF Watermark"))
    bool bShowWatermark = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Watermark", meta = (EditCondition = "bShowWatermark", ClampMin = "0", ClampMax = "3", ToolTip = "0: top left, 1: top right, 2: bottom left, 3: bottom right"))
    int32 WatermarkCorner = 3;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Watermark", meta = (EditCondition = "bShowWatermark", ClampMin = "0.5", ClampMax = "4.0"))
    float WatermarkScale = 1.0f;

    virtual bool IsValidOnShots() const override { return false; }
    virtual bool IsValidOnPrimary() const override { return true; }
    virtual void ValidateStateImpl() override;
    virtual void GetFormatArguments(FMoviePipelineFormatArgs& InOutFormatArgs) const override;

protected:
    virtual void SetupForPipelineImpl(UMoviePipeline* InPipeline) override;
    virtual void TeardownForPipelineImpl(UMoviePipeline* InPipeline) override;

private:
    struct FSavedCVar
    {
        FString Name;
        FString Value;
        uint32 Priority = 0;
    };
    TArray<FSavedCVar> SavedCVars;
    void OverrideCVar(const TCHAR* Name, const FString& Value);
};
