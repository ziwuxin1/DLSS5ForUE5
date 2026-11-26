#include "DLSS5NRArtControls.h"
#include "DLSS5NRColorPreserve.h"
#include "Misc/AutomationTest.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "RenderingThread.h"
#include "RHIGPUReadback.h"
#include "SceneView.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDLSS5NRArtGPUTest, "DLSS5.GPU.ArtControlsAndObjectMasks",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter | EAutomationTestFlags::NonNullRHI)
bool FDLSS5NRArtGPUTest::RunTest(const FString&)
{
    struct FResult { bool Read = false, Bypass = false; FLinearColor Center, Edge; };
    FResult Results[13];
    ENQUEUE_RENDER_COMMAND(DLSS5NRArtTest)([&Results](FRHICommandListImmediate& RHICmdList)
    {
        FSceneViewFamilyContext Family(FSceneViewFamily::ConstructionValues(nullptr, nullptr, FEngineShowFlags(ESFIM_Game)).SetTime(FGameTime()));
        FSceneViewInitOptions Options; Options.ViewFamily = &Family;
        const FIntRect Rect(8, 8, 72, 72); Options.SetViewRectangle(Rect);
        Options.ViewRotationMatrix = FMatrix::Identity; Options.ProjectionMatrix = FMatrix::Identity;
        FSceneView View(Options);
        for (int32 Index = 0; Index < 13; ++Index)
        {
            FRHIGPUTextureReadback Readback(TEXT("DLSS5NR.ArtReadback")); FRDGBuilder Graph(RHICmdList);
            const auto Desc = FRDGTextureDesc::Create2D(FIntPoint(80, 80), PF_FloatRGBA, FClearValueBinding::None, TexCreate_ShaderResource | TexCreate_UAV);
            auto* Source = Graph.CreateTexture(Desc, TEXT("DLSS5NR.ArtSource"));
            auto* Neural = Graph.CreateTexture(Desc, TEXT("DLSS5NR.ArtNeural"));
            AddClearUAVPass(Graph, Graph.CreateUAV(Source), FVector4f(.25f, .25f, .25f, .25f));
            AddClearUAVPass(Graph, Graph.CreateUAV(Neural), FVector4f(.75f, .75f, .75f, .75f));
            FDLSS5NRArtControls Art;
            if (Index == 1) { Art.Structure = 0; Art.Tone = 0; }
            if (Index == 2) Art.Tone = 0;
            if (Index == 3 || Index == 4) Art.Structure = 0;
            if (Index == 4 || Index == 5)
            {
                auto* Spot = Graph.CreateTexture(FRDGTextureDesc::Create2D(FIntPoint(1, 1), PF_FloatRGBA, FClearValueBinding::None, TexCreate_ShaderResource | TexCreate_UAV), TEXT("DLSS5NR.ArtSpot"));
                AddClearUAVPass(Graph, Graph.CreateUAV(Spot), FVector4f(.6f, .6f, .6f, .75f));
                FRHICopyTextureInfo Copy; Copy.DestPosition = FIntVector(40, 40, 0); Copy.Size = FIntVector(1, 1, 1);
                AddCopyTexturePass(Graph, Spot, Neural, Copy);
                Art.Tone = .8f;
            }
            FRDGTextureRef Groups = nullptr;
            if (Index >= 6)
            {
                for (int32 G = 0; G < 3; ++G) Art.Groups[G] = FVector4f(201 + G, 0, 0, 0);
                const auto DepthDesc = FRDGTextureDesc::Create2D(FIntPoint(80, 80), PF_R32_FLOAT, FClearValueBinding::None, TexCreate_ShaderResource | TexCreate_UAV);
                auto* Depth = Graph.CreateTexture(DepthDesc, TEXT("DLSS5NR.TestDepth"));
                auto* Custom = Graph.CreateTexture(DepthDesc, TEXT("DLSS5NR.TestCustomDepth"));
                AddClearUAVPass(Graph, Graph.CreateUAV(Depth), .5f);
                AddClearUAVPass(Graph, Graph.CreateUAV(Custom), Index == 8 ? .25f : .5f);
                const auto StencilDesc = FRDGTextureDesc::Create2D(FIntPoint(80, 80), PF_R32G32_UINT, FClearValueBinding::None, TexCreate_ShaderResource | TexCreate_UAV);
                auto* Stencil = Graph.CreateTexture(StencilDesc, TEXT("DLSS5NR.TestStencil"));
                AddClearUAVPass(Graph, Graph.CreateUAV(Stencil), FUintVector4(250, 250, 0, 0));
                auto* Tile = Graph.CreateTexture(FRDGTextureDesc::Create2D(FIntPoint(32, 32), PF_R32G32_UINT, FClearValueBinding::None, TexCreate_ShaderResource | TexCreate_UAV), TEXT("DLSS5NR.TestStencilTile"));
                const uint32 ID = Index == 7 ? 250 : Index >= 10 ? 201 + Index - 10 : 201;
                AddClearUAVPass(Graph, Graph.CreateUAV(Tile), FUintVector4(ID, ID, 0, 0));
                FRHICopyTextureInfo Copy; Copy.DestPosition = FIntVector(8, 8, 0); Copy.Size = FIntVector(32, 32, 1);
                AddCopyTexturePass(Graph, Tile, Stencil, Copy);
                Groups = AddDLSS5NRObjectMaskPass(Graph, View, Depth, Custom, Graph.CreateSRV(Stencil), FIntRect(8, 8, 40, 40), Rect.Size(), Art);
            }
            auto* Output = Index >= 10 ? AddDLSS5NRMaskPreviewPass(Graph, View, FScreenPassTexture(Source, Rect), Groups).Texture
                : AddDLSS5NRColorPreservePass(Graph, View, Source, Neural, Rect, true, 0, 0, 1, nullptr, &Art, Groups);
            Results[Index].Bypass = Output == Neural;
            AddEnqueueCopyPass(Graph, &Readback, Output); Graph.Execute(); RHICmdList.SubmitAndBlockUntilGPUIdle();
            if (Readback.IsReady())
            {
                int32 Pitch = 0; const auto* Pixels = static_cast<const FFloat16Color*>(Readback.Lock(Pitch));
                if (Pixels) { Results[Index].Read = true; Results[Index].Center = Pixels[40 * Pitch + 40].GetFloats(); Results[Index].Edge = Pixels[8 * Pitch + 8].GetFloats(); }
                Readback.Unlock();
            }
        }
    });
    FlushRenderingCommands();
    for (int32 I = 0; I < 13; ++I) TestTrue(FString::Printf(TEXT("Art case %d GPU readback"), I), Results[I].Read);
    TestTrue(TEXT("Neutral art controls incur no refinement"), Results[0].Bypass);
    TestTrue(TEXT("Both zero exactly protect source pixels"), Results[1].Center.Equals(FLinearColor(.25f, .25f, .25f, .75f), .001f));
    TestTrue(TEXT("Tone zero restores constant source base"), Results[2].Center.Equals(Results[1].Center, .001f));
    TestTrue(TEXT("Structure zero retains constant neural base"), Results[3].Center.Equals(Results[0].Center, .001f));
    TestFalse(TEXT("Structure independently controls high-frequency residual"), Results[4].Center.Equals(Results[5].Center, .005f));
    TestTrue(TEXT("Visible group overrides global settings"), Results[6].Center.Equals(Results[1].Center, .001f));
    TestTrue(TEXT("Unassigned stencil follows global settings"), Results[7].Center.Equals(Results[0].Center, .001f));
    TestTrue(TEXT("Occluded custom depth cannot mask foreground"), Results[8].Center.Equals(Results[0].Center, .001f));
    TestTrue(TEXT("Resampled non-zero depth rect handles edge"), Results[9].Edge.Equals(Results[1].Center, .001f));
    TestTrue(TEXT("Group 1 preview is red"), Results[10].Center.R > .65f && Results[10].Center.G < .2f);
    TestTrue(TEXT("Group 2 preview is green"), Results[11].Center.G > .65f && Results[11].Center.B < .2f);
    TestTrue(TEXT("Group 3 preview is blue"), Results[12].Center.B > .65f && Results[12].Center.R < .2f);
    for (int32 I = 10; I < 13; ++I) TestEqual(TEXT("Mask preview preserves source alpha"), Results[I].Center.A, .25f);
    return true;
}
#endif
