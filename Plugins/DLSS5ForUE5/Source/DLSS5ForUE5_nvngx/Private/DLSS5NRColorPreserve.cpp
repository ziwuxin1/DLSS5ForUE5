#include "DLSS5NRColorPreserve.h"
#include "DLSS5NRWatermark.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "SceneView.h"

class FDLSS5NRColorPrepareCS : public FGlobalShader
{
public:
    DECLARE_GLOBAL_SHADER(FDLSS5NRColorPrepareCS);
    SHADER_USE_PARAMETER_STRUCT(FDLSS5NRColorPrepareCS,FGlobalShader);
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, OriginalColor)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, NeuralColor)
        SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, SourceLabOut)
        SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, NeuralLabOut)
        SHADER_PARAMETER(FIntPoint, RectMin)
        SHADER_PARAMETER(FIntPoint, RectSize)
        SHADER_PARAMETER(uint32, EncodedSRGB)
    END_SHADER_PARAMETER_STRUCT()
    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& P)
    { return IsFeatureLevelSupported(P.Platform,ERHIFeatureLevel::SM5); }
};
class FDLSS5NRColorRefineCS : public FGlobalShader
{
public:
    DECLARE_GLOBAL_SHADER(FDLSS5NRColorRefineCS);
    SHADER_USE_PARAMETER_STRUCT(FDLSS5NRColorRefineCS,FGlobalShader);
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, NeuralColor)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D<uint>, EvaluationStatus)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SourceLab)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, NeuralLab)
        SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, RefinedColor)
        SHADER_PARAMETER(FIntPoint, RectMin)
        SHADER_PARAMETER(FIntPoint, RectSize)
        SHADER_PARAMETER(uint32, EncodedSRGB)
        SHADER_PARAMETER(float, ColorStrength)
        SHADER_PARAMETER(float, ToneStrength)
        SHADER_PARAMETER(float, DetailStrength)
    END_SHADER_PARAMETER_STRUCT()
    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& P)
    { return IsFeatureLevelSupported(P.Platform,ERHIFeatureLevel::SM5); }
};
IMPLEMENT_GLOBAL_SHADER(FDLSS5NRColorPrepareCS,"/DLSS5NR/Private/DLSS5NRColorPreserve.usf","PrepareCS",SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FDLSS5NRColorRefineCS,"/DLSS5NR/Private/DLSS5NRColorPreserve.usf","RefineCS",SF_Compute);

FRDGTextureRef AddDLSS5NRColorPreservePass(FRDGBuilder& Graph,const FSceneView& View,
    FRDGTextureRef Original,FRDGTextureRef Neural,const FIntRect& Rect,
    bool bEncodedSRGB,float Strength,float Tone,float Detail,FRDGTextureRef EvaluationStatus)
{
    if(Strength<=0) return Neural; // No allocation, dispatch or color conversion when disabled.
    const auto LabDesc=FRDGTextureDesc::Create2D(Rect.Size(),PF_A32B32G32R32F,
        FClearValueBinding::None,TexCreate_ShaderResource|TexCreate_UAV);
    auto* SourceLab=Graph.CreateTexture(LabDesc,TEXT("DLSS5NR.SourceLab"));
    auto* NeuralLab=Graph.CreateTexture(LabDesc,TEXT("DLSS5NR.NeuralLab"));
    auto* P=Graph.AllocParameters<FDLSS5NRColorPrepareCS::FParameters>();
    P->OriginalColor=Original;P->NeuralColor=Neural;
    P->SourceLabOut=Graph.CreateUAV(SourceLab);P->NeuralLabOut=Graph.CreateUAV(NeuralLab);
    P->RectMin=Rect.Min;P->RectSize=Rect.Size();P->EncodedSRGB=bEncodedSRGB;
    TShaderMapRef<FDLSS5NRColorPrepareCS> Prepare(GetGlobalShaderMap(View.GetFeatureLevel()));
    FComputeShaderUtils::AddPass(Graph,RDG_EVENT_NAME("DLSS5NR Prepare Source Colors"),Prepare,P,
        FComputeShaderUtils::GetGroupCount(Rect.Size(),FIntPoint(8,8)));
    auto* Output=Graph.CreateTexture(Neural->Desc,TEXT("DLSS5NR.ColorPreserved"));
    auto* R=Graph.AllocParameters<FDLSS5NRColorRefineCS::FParameters>();
    R->EvaluationStatus=EvaluationStatus?EvaluationStatus:CreateDLSS5NRStatusTexture(Graph,1);
    R->NeuralColor=Neural;R->SourceLab=SourceLab;R->NeuralLab=NeuralLab;R->RefinedColor=Graph.CreateUAV(Output);
    R->RectMin=Rect.Min;R->RectSize=Rect.Size();R->EncodedSRGB=bEncodedSRGB;
    R->ColorStrength=FMath::Clamp(Strength,0.f,1.f);R->ToneStrength=FMath::Clamp(Tone,0.f,1.f);
    R->DetailStrength=FMath::Clamp(Detail,0.f,2.f);
    TShaderMapRef<FDLSS5NRColorRefineCS> Refine(GetGlobalShaderMap(View.GetFeatureLevel()));
    FComputeShaderUtils::AddPass(Graph,RDG_EVENT_NAME("DLSS5NR Source Guided Color / Tone / Detail"),Refine,R,
        FComputeShaderUtils::GetGroupCount(Rect.Size(),FIntPoint(8,8)));
    return Output;
}
