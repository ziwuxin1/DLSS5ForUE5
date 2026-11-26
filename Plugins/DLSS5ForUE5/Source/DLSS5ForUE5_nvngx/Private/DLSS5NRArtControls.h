#pragma once
#include "ScreenPass.h"

// UE-owned output controls, independent of the closed runtime's parameter hints.
struct FDLSS5NRArtControls
{
    float Structure = 1;
    float Tone = 1;
    // Stencil ID (-1 disables), structure, tone, reserved. First matching group wins.
    FVector4f Groups[3] = { FVector4f(-1, 1, 1, 0), FVector4f(-1, 1, 1, 0), FVector4f(-1, 1, 1, 0) };
    bool NeedsRefinement() const
    {
        if (Structure != 1 || Tone != 1) return true;
        for (const auto& Group : Groups) if (Group.X > 0 && (Group.Y != 1 || Group.Z != 1)) return true;
        return false;
    }
};
FRDGTextureRef AddDLSS5NRObjectMaskPass(FRDGBuilder&, const FSceneView&, FRDGTextureRef SceneDepth,
    FRDGTextureRef CustomDepth, FRDGTextureSRVRef CustomStencil, FIntRect DepthRect,
    FIntPoint ColorSize, const FDLSS5NRArtControls&);
FScreenPassTexture AddDLSS5NRMaskPreviewPass(FRDGBuilder&, const FSceneView&, FScreenPassTexture Color, FRDGTextureRef Groups);
