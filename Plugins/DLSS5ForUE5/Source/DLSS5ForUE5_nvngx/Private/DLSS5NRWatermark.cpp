#include "DLSS5NRWatermark.h"

#include "GlobalShader.h"
#include "HighResScreenshot.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "SceneView.h"
#include "ShaderParameterStruct.h"

class FDLSS5NRWatermarkCS : public FGlobalShader
{
public:
    DECLARE_GLOBAL_SHADER(FDLSS5NRWatermarkCS);
    SHADER_USE_PARAMETER_STRUCT(FDLSS5NRWatermarkCS, FGlobalShader);
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, InputColor)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D<uint>, EvaluationStatus)
        SHADER_PARAMETER(FIntPoint, RectMin)
        SHADER_PARAMETER(FIntPoint, RectSize)
        SHADER_PARAMETER(FIntPoint, LabelOrigin)
        SHADER_PARAMETER(int32, PixelScale)
        SHADER_PARAMETER(uint32, CompositeInvertedAlpha)
        SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutputColor)
    END_SHADER_PARAMETER_STRUCT()

    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
    {
        return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
    }
};
IMPLEMENT_GLOBAL_SHADER(FDLSS5NRWatermarkCS, "/DLSS5NR/Private/DLSS5NRWatermark.usf", "MainCS", SF_Compute);

FRDGTextureRef CreateDLSS5NRStatusTexture(FRDGBuilder& GraphBuilder, uint32 Value)
{
    FRDGTextureRef Texture = GraphBuilder.CreateTexture(FRDGTextureDesc::Create2D(
        FIntPoint(1, 1), PF_R32_UINT, FClearValueBinding::None, TexCreate_ShaderResource | TexCreate_UAV),
        TEXT("DLSS5NR.EvaluationStatus"));
    AddClearUAVPass(GraphBuilder, GraphBuilder.CreateUAV(Texture), Value);
    return Texture;
}

FScreenPassTexture AddDLSS5NRWatermarkPass(
    FRDGBuilder& GraphBuilder, const FSceneView& View, const FScreenPassTexture& Color,
    FRDGTextureRef EvaluationStatus, int32 Corner, float Scale)
{
    FIntRect VisibleRect = Color.ViewRect;
    if (GIsHighResScreenshot && GetHighResScreenshotConfig().CaptureRegion.Area() > 0)
    {
        // UE crops the image after post-processing. Position the label inside that crop.
        const FIntRect Crop = GetHighResScreenshotConfig().CaptureRegion;
        const FIntPoint DisplaySize = View.UnscaledViewRect.Size();
        const auto MapPixel = [&](FIntPoint Pixel)
        {
            return Color.ViewRect.Min + FIntPoint(
                FMath::RoundToInt(float(Pixel.X - View.UnscaledViewRect.Min.X) * Color.ViewRect.Width() / FMath::Max(1, DisplaySize.X)),
                FMath::RoundToInt(float(Pixel.Y - View.UnscaledViewRect.Min.Y) * Color.ViewRect.Height() / FMath::Max(1, DisplaySize.Y)));
        };
        VisibleRect.Clip(FIntRect(MapPixel(Crop.Min), MapPixel(Crop.Max)));
    }
    if (VisibleRect.Width() < 62 || VisibleRect.Height() < 15) return Color;

    const int32 RequestedScale = FMath::Max(1, FMath::RoundToInt(3.0f * Scale * VisibleRect.Height() / 1080.0f));
    const int32 PixelScale = FMath::Min(RequestedScale, FMath::Min(VisibleRect.Width() / 62, VisibleRect.Height() / 15));
    const FIntPoint LabelSize(58 * PixelScale, 11 * PixelScale);
    const int32 Margin = 2 * PixelScale;
    const FIntPoint Origin(
        (Corner & 1) ? VisibleRect.Max.X - Margin - LabelSize.X : VisibleRect.Min.X + Margin,
        (Corner & 2) ? VisibleRect.Max.Y - Margin - LabelSize.Y : VisibleRect.Min.Y + Margin);

    FRDGTextureRef Output = GraphBuilder.CreateTexture(FRDGTextureDesc::Create2D(
        Color.Texture->Desc.Extent, PF_FloatRGBA, FClearValueBinding::None,
        TexCreate_ShaderResource | TexCreate_UAV | TexCreate_RenderTargetable), TEXT("DLSS5NR.Watermark"));
    auto* Parameters = GraphBuilder.AllocParameters<FDLSS5NRWatermarkCS::FParameters>();
    Parameters->InputColor = Color.Texture;
    Parameters->EvaluationStatus = EvaluationStatus;
    Parameters->RectMin = Color.ViewRect.Min;
    Parameters->RectSize = Color.ViewRect.Size();
    Parameters->LabelOrigin = Origin;
    Parameters->PixelScale = PixelScale;
    Parameters->CompositeInvertedAlpha = (View.bIsOfflineRender || (View.Family && View.Family->EngineShowFlags.AlphaInvert)) ? 1u : 0u;
    Parameters->OutputColor = GraphBuilder.CreateUAV(Output);
    TShaderMapRef<FDLSS5NRWatermarkCS> Shader(GetGlobalShaderMap(View.GetFeatureLevel()));
    FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("DLSS5NR ON/OFF Watermark"), Shader, Parameters,
        FComputeShaderUtils::GetGroupCount(Color.ViewRect.Size(), FIntPoint(8, 8)));
    return FScreenPassTexture(Output, Color.ViewRect);
}
