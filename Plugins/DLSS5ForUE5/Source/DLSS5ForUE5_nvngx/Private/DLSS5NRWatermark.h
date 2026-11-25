#pragma once

#include "ScreenPass.h"

// A per-view, per-sample GPU status; zero until a native evaluation succeeds.
FRDGTextureRef CreateDLSS5NRStatusTexture(FRDGBuilder& GraphBuilder, uint32 Value);

FScreenPassTexture AddDLSS5NRWatermarkPass(
    FRDGBuilder& GraphBuilder, const FSceneView& View, const FScreenPassTexture& Color,
    FRDGTextureRef EvaluationStatus, int32 Corner, float Scale);
