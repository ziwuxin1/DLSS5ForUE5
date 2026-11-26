#include "DLSS5NRArtControls.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "SceneView.h"

class FDLSS5NRObjectMaskCS : public FGlobalShader
{
public:
    DECLARE_GLOBAL_SHADER(FDLSS5NRObjectMaskCS);
    SHADER_USE_PARAMETER_STRUCT(FDLSS5NRObjectMaskCS, FGlobalShader);
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneDepth)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, CustomDepth)
        SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<uint2>, CustomStencil)
        SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<uint>, GroupOutput)
        SHADER_PARAMETER(FIntPoint, DepthMin)
        SHADER_PARAMETER(FIntPoint, DepthSize)
        SHADER_PARAMETER(FIntPoint, ColorSize)
        SHADER_PARAMETER(FVector4f, StencilIDs)
    END_SHADER_PARAMETER_STRUCT()
    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& P)
    { return IsFeatureLevelSupported(P.Platform, ERHIFeatureLevel::SM5); }
};
class FDLSS5NRMaskPreviewCS : public FGlobalShader
{
public:
    DECLARE_GLOBAL_SHADER(FDLSS5NRMaskPreviewCS);
    SHADER_USE_PARAMETER_STRUCT(FDLSS5NRMaskPreviewCS, FGlobalShader);
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, InputColor)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D<uint>, GroupMask)
        SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, PreviewOutput)
        SHADER_PARAMETER(FIntPoint, RectMin)
        SHADER_PARAMETER(FIntPoint, ColorSize)
    END_SHADER_PARAMETER_STRUCT()
    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& P)
    { return IsFeatureLevelSupported(P.Platform, ERHIFeatureLevel::SM5); }
};
IMPLEMENT_GLOBAL_SHADER(FDLSS5NRObjectMaskCS, "/DLSS5NR/Private/DLSS5NRArtControls.usf", "ResolveMaskCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FDLSS5NRMaskPreviewCS, "/DLSS5NR/Private/DLSS5NRArtControls.usf", "PreviewCS", SF_Compute);

FRDGTextureRef AddDLSS5NRObjectMaskPass(FRDGBuilder& Graph, const FSceneView& View,
    FRDGTextureRef SceneDepth, FRDGTextureRef CustomDepth, FRDGTextureSRVRef CustomStencil,
    FIntRect DepthRect, FIntPoint ColorSize, const FDLSS5NRArtControls& Art)
{
    if (!SceneDepth || !CustomDepth || !CustomStencil || ColorSize.X <= 0 || ColorSize.Y <= 0) return nullptr;
    DepthRect.Clip(FIntRect(FIntPoint::ZeroValue, SceneDepth->Desc.Extent));
    DepthRect.Clip(FIntRect(FIntPoint::ZeroValue, CustomDepth->Desc.Extent));
    if (DepthRect.Width() <= 0 || DepthRect.Height() <= 0) return nullptr;
    auto* Mask = Graph.CreateTexture(FRDGTextureDesc::Create2D(ColorSize, PF_R8_UINT, FClearValueBinding::None,
        TexCreate_ShaderResource | TexCreate_UAV), TEXT("DLSS5NR.ObjectGroups"));
    auto* P = Graph.AllocParameters<FDLSS5NRObjectMaskCS::FParameters>();
    P->SceneDepth = SceneDepth; P->CustomDepth = CustomDepth; P->CustomStencil = CustomStencil;
    P->DepthMin = DepthRect.Min; P->DepthSize = DepthRect.Size(); P->ColorSize = ColorSize;
    P->StencilIDs = FVector4f(Art.Groups[0].X, Art.Groups[1].X, Art.Groups[2].X, -1);
    P->GroupOutput = Graph.CreateUAV(Mask);
    TShaderMapRef<FDLSS5NRObjectMaskCS> Shader(GetGlobalShaderMap(View.GetFeatureLevel()));
    FComputeShaderUtils::AddPass(Graph, RDG_EVENT_NAME("DLSS5NR Visible Object Masks"), Shader, P,
        FComputeShaderUtils::GetGroupCount(ColorSize, FIntPoint(8, 8)));
    return Mask;
}
FScreenPassTexture AddDLSS5NRMaskPreviewPass(FRDGBuilder& Graph, const FSceneView& View,
    FScreenPassTexture Color, FRDGTextureRef Groups)
{
    if (!Groups) return Color;
    auto* Out = Graph.CreateTexture(FRDGTextureDesc::Create2D(Color.Texture->Desc.Extent, PF_FloatRGBA,
        FClearValueBinding::None, TexCreate_ShaderResource | TexCreate_UAV), TEXT("DLSS5NR.MaskPreview"));
    auto* P = Graph.AllocParameters<FDLSS5NRMaskPreviewCS::FParameters>();
    P->InputColor = Color.Texture; P->GroupMask = Groups; P->PreviewOutput = Graph.CreateUAV(Out);
    P->RectMin = Color.ViewRect.Min; P->ColorSize = Color.ViewRect.Size();
    TShaderMapRef<FDLSS5NRMaskPreviewCS> Shader(GetGlobalShaderMap(View.GetFeatureLevel()));
    FComputeShaderUtils::AddPass(Graph, RDG_EVENT_NAME("DLSS5NR Mask Preview"), Shader, P,
        FComputeShaderUtils::GetGroupCount(Color.ViewRect.Size(), FIntPoint(8, 8)));
    return FScreenPassTexture(Out, Color.ViewRect);
}
