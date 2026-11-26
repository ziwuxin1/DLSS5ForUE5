#pragma once

#include "CoreMinimal.h"

namespace DLSS5NR
{
    float GetArtStructure();
    float GetArtTone();
    int32 GetDeveloperMasking();
    int32 GetShowDeveloperMasks();
    FVector4f GetMaskGroup(int32 Index);
    float GetColorPreserve();
    float GetTonePreserve();
    float GetNeuralDetail();
    float GetBlend();
    int32 GetCompare();
    float GetCompareSplit();
    int32 GetHighlightPreserve();
    float GetHighlightStart();
    float GetHighlightEnd();
    extern int32 GetEnable();
    int32 GetWatermark();
    int32 GetWatermarkCorner();
    float GetWatermarkScale();
    extern int32 GetPreset();
    extern int32 GetPassCount();
    extern float GetIntensity();
    extern float GetLocalTone();
    extern float GetLocalStructure();
    extern float GetSkinStructure();
    extern int32 GetAutoMask();
    extern int32 GetStyle();
    extern int32 GetUICorrection();
    extern int32 GetEnableSceneGuides();
    extern int32 GetFeedSceneGuides();
    extern int32 GetUseDepth();
    extern int32 GetUseMotionVectors();
    extern int32 GetAutoResetHistory();
    extern int32 GetDepthInverted();
    extern float GetMVecScaleX();
    extern float GetMVecScaleY();
    extern float GetExposureResetRatio();
    extern int32 GetDebug();
}
