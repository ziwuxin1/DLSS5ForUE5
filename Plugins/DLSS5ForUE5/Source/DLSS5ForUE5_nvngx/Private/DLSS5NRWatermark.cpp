#include "DLSS5NRWatermark.h"
#include "DLSS5NRWatermarkText.h"

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
        SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<uint>, TextCoverage)
        SHADER_PARAMETER(FIntPoint, RectMin)
        SHADER_PARAMETER(FIntPoint, RectSize)
        SHADER_PARAMETER(FIntPoint, LabelOrigin)
        SHADER_PARAMETER(FIntPoint, LabelSize)
        SHADER_PARAMETER(FVector3f, AccentColor)
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
    FRDGTextureRef EvaluationStatus, int32 Corner, float Scale, bool bEncodedSRGB, bool bAfterOutputTransform)
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
    if (VisibleRect.Width() < 32 || VisibleRect.Height() < 16) return Color;

    const float DrawScale = FMath::Min(FMath::Clamp(Scale, 0.5f, 4.0f) * VisibleRect.Height() / 2160.0f,
        FMath::Min(VisibleRect.Width() / 468.0f, VisibleRect.Height() / 128.0f));
    const FIntPoint LabelSize(FMath::Max(1, FMath::RoundToInt(420 * DrawScale)), FMath::Max(1, FMath::RoundToInt(80 * DrawScale)));
    const int32 Margin = FMath::Max(1, FMath::RoundToInt(24 * DrawScale));
    const FIntPoint Origin(
        (Corner & 1) ? VisibleRect.Max.X - Margin - LabelSize.X : VisibleRect.Min.X + Margin,
        (Corner & 2) ? VisibleRect.Max.Y - Margin - LabelSize.Y : VisibleRect.Min.Y + Margin);

    FRDGTextureRef Output = GraphBuilder.CreateTexture(FRDGTextureDesc::Create2D(
        Color.Texture->Desc.Extent, PF_FloatRGBA, FClearValueBinding::None,
        TexCreate_ShaderResource | TexCreate_UAV | TexCreate_RenderTargetable), TEXT("DLSS5NR.Watermark"));
    auto* Parameters = GraphBuilder.AllocParameters<FDLSS5NRWatermarkCS::FParameters>();
    Parameters->InputColor = Color.Texture;
    Parameters->EvaluationStatus = EvaluationStatus;
    Parameters->TextCoverage = GraphBuilder.CreateSRV(CreateStructuredBuffer(GraphBuilder,
        TEXT("DLSS5NR.WatermarkText"), sizeof(uint32), UE_ARRAY_COUNT(DLSS5NRWatermarkText::Pixels),
        DLSS5NRWatermarkText::Pixels, sizeof(DLSS5NRWatermarkText::Pixels), ERDGInitialDataFlags::NoCopy));
    Parameters->RectMin = Color.ViewRect.Min;
    Parameters->RectSize = Color.ViewRect.Size();
    Parameters->LabelOrigin = Origin;
    Parameters->LabelSize = LabelSize;
    Parameters->AccentColor = bEncodedSRGB ? FVector3f(118.f / 255.f, 185.f / 255.f, 0)
        : FVector3f(FLinearColor(FColor(118, 185, 0)));
    // Classic MRQ's surface reader still inverts alpha after the family render.
    Parameters->CompositeInvertedAlpha = (View.bIsOfflineRender ||
        (!bAfterOutputTransform && View.Family && View.Family->EngineShowFlags.AlphaInvert)) ? 1u : 0u;
    Parameters->OutputColor = GraphBuilder.CreateUAV(Output);
    TShaderMapRef<FDLSS5NRWatermarkCS> Shader(GetGlobalShaderMap(View.GetFeatureLevel()));
    FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("DLSS5NR ON/OFF Watermark"), Shader, Parameters,
        FComputeShaderUtils::GetGroupCount(Color.ViewRect.Size(), FIntPoint(8, 8)));
    return FScreenPassTexture(Output, Color.ViewRect);
}
