#include "DLSS5NRWatermark.h"
#include "Misc/AutomationTest.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "RenderingThread.h"
#include "RHIGPUReadback.h"
#include "SceneView.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDLSS5NRCaptureGPUTest, "DLSS5.GPU.Watermark4K",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter | EAutomationTestFlags::NonNullRHI)

bool FDLSS5NRCaptureGPUTest::RunTest(const FString&)
{
    struct FResult { bool bRead = false; FLinearColor Background; FLinearColor Label; FLinearColor State; };
    FResult Results[4];
    ENQUEUE_RENDER_COMMAND(DLSS5NRWatermarkTest)([&Results](FRHICommandListImmediate& RHICmdList)
    {
        FSceneViewFamilyContext Family(FSceneViewFamily::ConstructionValues(nullptr, nullptr, FEngineShowFlags(ESFIM_Game)).SetTime(FGameTime()));
        FSceneViewInitOptions Options;
        Options.ViewFamily = &Family;
        // Exercise a 4K subrectangle within a larger allocation, as in editor views.
        Options.SetViewRectangle(FIntRect(16, 8, 3856, 2168));
        Options.ViewRotationMatrix = FMatrix::Identity;
        Options.ProjectionMatrix = FMatrix::Identity;
        FSceneView View(Options);
        for (uint32 Index = 0; Index < 4; ++Index)
        {
            const uint32 State = Index % 2;
            View.bIsOfflineRender = Index >= 2;
            FRHIGPUTextureReadback Readback(TEXT("DLSS5NR.WatermarkTest"));
            FRDGBuilder Graph(RHICmdList);
            auto* Input = Graph.CreateTexture(FRDGTextureDesc::Create2D(FIntPoint(3872, 2184), PF_FloatRGBA,
                FClearValueBinding::None, TexCreate_ShaderResource | TexCreate_UAV), TEXT("DLSS5NR.WatermarkTestInput"));
            AddClearUAVPass(Graph, Graph.CreateUAV(Input), FVector4f(0.5f, 0.5f, 0.5f, 0.25f));
            const auto Output = AddDLSS5NRWatermarkPass(Graph, View,
                FScreenPassTexture(Input, Options.GetViewRect()), CreateDLSS5NRStatusTexture(Graph, State), 3, 1.0f);
            AddEnqueueCopyPass(Graph, &Readback, Output.Texture);
            Graph.Execute();
            RHICmdList.SubmitAndBlockUntilGPUIdle();
            if (Readback.IsReady())
            {
                int32 Pitch = 0;
                const FFloat16Color* Pixels = static_cast<const FFloat16Color*>(Readback.Lock(Pitch));
                if (Pixels)
                {
                    const FIntPoint Origin(3856 - 12 - 348, 2168 - 12 - 66);
                    Results[Index].Background = Pixels[100 * Pitch + 100].GetFloats();
                    Results[Index].Label = Pixels[Origin.Y * Pitch + Origin.X].GetFloats();
                    // Top row of the O glyph: lit in both ON and OFF, with different colors.
                    Results[Index].State = Pixels[(Origin.Y + 12) * Pitch + Origin.X + 12 + 37 * 6].GetFloats();
                    Results[Index].bRead = true;
                }
                Readback.Unlock();
            }
        }
    });
    FlushRenderingCommands();
    for (int32 Index = 0; Index < 4; ++Index)
    {
        const FResult& Result = Results[Index];
        TestTrue(TEXT("Read back a real 4K GPU render"), Result.bRead);
        TestTrue(TEXT("Outside watermark stays unchanged"), Result.Background.Equals(FLinearColor(0.5f, 0.5f, 0.5f, 0.25f), 0.001f));
        TestTrue(TEXT("Watermark panel reaches intended bottom-right position"), Result.Label.R < 0.1f);
        TestTrue(TEXT("Watermark composites panel opacity over scene alpha"), FMath::IsNearlyEqual(Result.Label.A, Index >= 2 ? 0.0375f : 0.8875f, 0.001f));
    }
    TestTrue(TEXT("Failed/disabled sample is orange OFF"), Results[0].State.R > 0.9f && Results[0].State.G < 0.8f);
    TestTrue(TEXT("Successful sample is green ON"), Results[1].State.G > 0.9f && Results[1].State.R < 0.5f);
    TestTrue(TEXT("Offline MRQ glyph has opaque alpha after surface-reader inversion"), Results[2].State.A == 0 && Results[3].State.A == 0);
    TestTrue(TEXT("Viewport glyph has opaque straight alpha"), Results[0].State.A == 1 && Results[1].State.A == 1);
    return true;
}
#endif
