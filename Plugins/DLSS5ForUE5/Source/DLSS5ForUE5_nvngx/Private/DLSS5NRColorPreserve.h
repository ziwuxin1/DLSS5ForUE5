#pragma once
#include "DLSS5NRMotionVectorPass.h"
// Accepts only normalized SDR values in sRGB primaries; out-of-range pixels bypass refinement.
FRDGTextureRef AddDLSS5NRColorPreservePass(FRDGBuilder&,const FSceneView&,FRDGTextureRef,FRDGTextureRef,
    const FIntRect&,bool bEncodedSRGB,float Strength,float Tone,float Detail,FRDGTextureRef EvaluationStatus = nullptr);
