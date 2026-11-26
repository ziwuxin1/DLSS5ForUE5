#include "DLSS5NRMotionVectorPass.h"
#include "Misc/AutomationTest.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "RenderingThread.h"
#include "RHIGPUReadback.h"
#include "SceneView.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDLSS5NRCompositeGPUTest, "DLSS5.GPU.CompositeControls4K",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter | EAutomationTestFlags::NonNullRHI)

bool FDLSS5NRCompositeGPUTest::RunTest(const FString&)
{
    struct FResult { bool bRead = false; FLinearColor Left; FLinearColor Right; };
    FResult Results[6];
    ENQUEUE_RENDER_COMMAND(DLSS5NRCompositeTest)([&Results](FRHICommandListImmediate& RHICmdList)
    {
        FSceneViewFamilyContext Family(FSceneViewFamily::ConstructionValues(nullptr, nullptr, FEngineShowFlags(ESFIM_Game)).SetTime(FGameTime()));
        FSceneViewInitOptions Options;
        Options.ViewFamily = &Family;
        // Non-zero origin catches split math that accidentally uses allocation coordinates.
        const FIntRect Rect(32, 16, 3872, 2176);
        Options.SetViewRectangle(Rect);
        Options.ViewRotationMatrix = FMatrix::Identity;
        Options.ProjectionMatrix = FMatrix::Identity;
        FSceneView View(Options);
        for (int32 Index = 0; Index < 6; ++Index)
        {
            FRHIGPUTextureReadback Readback(TEXT("DLSS5NR.CompositeTest"));
            FRDGBuilder Graph(RHICmdList);
            const auto Desc = FRDGTextureDesc::Create2D(FIntPoint(3904, 2192), PF_FloatRGBA,
                FClearValueBinding::None, TexCreate_ShaderResource | TexCreate_UAV);
            auto* Original = Graph.CreateTexture(Desc, TEXT("DLSS5NR.CompositeOriginal"));
            auto* Neural = Graph.CreateTexture(Desc, TEXT("DLSS5NR.CompositeNeural"));
            AddClearUAVPass(Graph, Graph.CreateUAV(Original), FVector4f(0.2f, 0.4f, 0.6f, 0.25f));
            AddClearUAVPass(Graph, Graph.CreateUAV(Neural), FVector4f(0.8f, 0.6f, 0.2f, 0.0f));
            const float Blend = Index < 3 ? Index * 0.5f : 1.0f;
            const float Split = Index >= 3 ? (Index - 3) * 0.5f : 0.5f;
            auto* Output = AddDLSS5NRHighlightPass(Graph, View, Original, Neural, Rect,
                0.7f, 0.9f, false, true, Blend, Index >= 3, Split);
            AddEnqueueCopyPass(Graph, &Readback, Output);
            Graph.Execute();
            RHICmdList.SubmitAndBlockUntilGPUIdle();
            if (Readback.IsReady())
            {
                int32 Pitch = 0;
                const FFloat16Color* Pixels = static_cast<const FFloat16Color*>(Readback.Lock(Pitch));
                if (Pixels)
                {
                    Results[Index].Left = Pixels[(Rect.Min.Y + 100) * Pitch + Rect.Min.X + 1919].GetFloats();
                    Results[Index].Right = Pixels[(Rect.Min.Y + 100) * Pitch + Rect.Min.X + 1920].GetFloats();
                    Results[Index].bRead = true;
                }
                Readback.Unlock();
            }
        }
    });
    FlushRenderingCommands();
    const FLinearColor Original(0.2f, 0.4f, 0.6f, 0.25f);
    const FLinearColor Neural(0.8f, 0.6f, 0.2f, 0.25f);
    const FLinearColor Half(0.5f, 0.5f, 0.4f, 0.25f);
    const FLinearColor ExpectedLeft[] = { Original, Half, Neural, Neural, Original, Original };
    const FLinearColor ExpectedRight[] = { Original, Half, Neural, Neural, Neural, Original };
    for (int32 Index = 0; Index < 6; ++Index)
    {
        TestTrue(FString::Printf(TEXT("Case %d has a real GPU readback"), Index), Results[Index].bRead);
        TestTrue(FString::Printf(TEXT("Case %d left RGB and original alpha"), Index), Results[Index].Left.Equals(ExpectedLeft[Index], 0.001f));
        TestTrue(FString::Printf(TEXT("Case %d right RGB and original alpha"), Index), Results[Index].Right.Equals(ExpectedRight[Index], 0.001f));
    }
    return true;
}
#endif
