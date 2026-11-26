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
    struct FResult { bool bRead = false; FLinearColor Background, Label, State, Gap; int32 TextPixels = 0; int32 SmoothPixels = 0; };
    FResult Results[6];
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
        for (uint32 Index = 0; Index < 6; ++Index)
        {
            const uint32 State = Index % 2;
            View.bIsOfflineRender = Index >= 2 && Index < 4;
            Family.EngineShowFlags.SetAlphaInvert(Index >= 4);
            FRHIGPUTextureReadback Readback(TEXT("DLSS5NR.WatermarkTest"));
            FRDGBuilder Graph(RHICmdList);
            auto* Input = Graph.CreateTexture(FRDGTextureDesc::Create2D(FIntPoint(3872, 2184), PF_FloatRGBA,
                FClearValueBinding::None, TexCreate_ShaderResource | TexCreate_UAV), TEXT("DLSS5NR.WatermarkTestInput"));
            AddClearUAVPass(Graph, Graph.CreateUAV(Input), FVector4f(0.5f, 0.5f, 0.5f, 0.25f));
            const auto Output = AddDLSS5NRWatermarkPass(Graph, View,
                FScreenPassTexture(Input, Options.GetViewRect()), CreateDLSS5NRStatusTexture(Graph, State), 3, 1.0f, Index < 2 || Index >= 4, Index >= 4);
            AddEnqueueCopyPass(Graph, &Readback, Output.Texture);
            Graph.Execute();
            RHICmdList.SubmitAndBlockUntilGPUIdle();
            if (Readback.IsReady())
            {
                int32 Pitch = 0;
                const FFloat16Color* Pixels = static_cast<const FFloat16Color*>(Readback.Lock(Pitch));
                if (Pixels)
                {
                    const FIntPoint Origin(3856 - 24 - 420, 2168 - 24 - 80);
                    Results[Index].Background = Pixels[100 * Pitch + 100].GetFloats();
                    Results[Index].Label = Pixels[(Origin.Y + 4) * Pitch + Origin.X + 4].GetFloats();
                    Results[Index].State = Pixels[(Origin.Y + 75) * Pitch + Origin.X + 20].GetFloats();
                    Results[Index].Gap = Pixels[(Origin.Y + 67) * Pitch + Origin.X + 20].GetFloats();
                    for (int32 Y = 12; Y < 52; ++Y)
                    for (int32 X = 98; X < 322; ++X)
                    {
                        const auto C = Pixels[(Origin.Y + Y) * Pitch + Origin.X + X].GetFloats();
                        if (State ? C.R < .01f : C.R > .99f) ++Results[Index].TextPixels;
                        if (C.R > .2f && C.R < .8f) ++Results[Index].SmoothPixels;
                    }
                    Results[Index].bRead = true;
                }
                Readback.Unlock();
            }
        }
    });
    FlushRenderingCommands();
    for (int32 Index = 0; Index < 6; ++Index)
    {
        const FResult& Result = Results[Index];
        TestTrue(TEXT("Read back a real 4K GPU render"), Result.bRead);
        TestTrue(TEXT("Outside watermark stays unchanged"), Result.Background.Equals(FLinearColor(0.5f, 0.5f, 0.5f, 0.25f), 0.001f));
        const bool On = Index % 2 != 0;
        TestTrue(TEXT("ON has white panel; OFF has transparent black panel"), FMath::IsNearlyEqual(Result.Label.R, On ? 1.f : .14f, .001f));
        TestTrue(TEXT("Watermark composites panel opacity over scene alpha"), FMath::IsNearlyEqual(Result.Label.A, On ? (Index >= 2 && Index < 4 ? 0.f : 1.f) : (Index >= 2 && Index < 4 ? .07f : .79f), .001f));
        TestTrue(TEXT("Caption has visible black/white lettering"), Result.TextPixels > 100);
        TestTrue(TEXT("Caption edges are antialiased"), Result.SmoothPixels > 20);
        TestTrue(TEXT("Panel-strip gap remains transparent"), Result.Gap.Equals(Result.Background, .001f));
    }
    TestTrue(TEXT("Failed/disabled sample has black strip"), Results[0].State.R == 0 && Results[0].State.G == 0 && Results[0].State.B == 0);
    const FLinearColor Green(118.f / 255.f, 185.f / 255.f, 0.f);
    TestTrue(TEXT("Successful sample has reference green strip"), FMath::IsNearlyEqual(Results[1].State.R, Green.R, .001f) && FMath::IsNearlyEqual(Results[1].State.G, Green.G, .001f));
    const FLinearColor LinearGreen(FColor(118, 185, 0));
    TestTrue(TEXT("MRQ uses linear green before output encoding"), FMath::IsNearlyEqual(Results[3].State.R, LinearGreen.R, .001f) && FMath::IsNearlyEqual(Results[3].State.G, LinearGreen.G, .001f));
    TestTrue(TEXT("Offline MRQ glyph has opaque alpha after surface-reader inversion"), Results[2].State.A == 0 && Results[3].State.A == 0);
    TestTrue(TEXT("Viewport glyph has opaque straight alpha"), Results[0].State.A == 1 && Results[1].State.A == 1);
    TestTrue(TEXT("Final output uses opacity after UE alpha inversion"), Results[4].State.A == 1 && Results[5].State.A == 1);
    return true;
}
#endif
