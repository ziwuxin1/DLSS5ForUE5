#include "DLSS5NRColorPreserve.h"
#include "DLSS5NRWatermark.h"
#include "Misc/AutomationTest.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "RenderingThread.h"
#include "RHIGPUReadback.h"
#include "SceneView.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDLSS5NRColorGPUTest,"DLSS5.GPU.SourceGuidedColor",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter | EAutomationTestFlags::NonNullRHI)
bool FDLSS5NRColorGPUTest::RunTest(const FString&)
{
    struct FResult { bool Read=false; bool Bypass=false; FLinearColor Center,Edge; };
    FResult Results[9];
    ENQUEUE_RENDER_COMMAND(DLSS5NRColorTest)([&Results](FRHICommandListImmediate& RHICmdList)
    {
        FSceneViewFamilyContext Family(FSceneViewFamily::ConstructionValues(nullptr,nullptr,FEngineShowFlags(ESFIM_Game)).SetTime(FGameTime()));
        FSceneViewInitOptions Options;Options.ViewFamily=&Family;
        const FIntRect Rect(8,8,72,72);Options.SetViewRectangle(Rect);
        Options.ViewRotationMatrix=FMatrix::Identity;Options.ProjectionMatrix=FMatrix::Identity;
        FSceneView View(Options);
        for(int32 Index=0;Index<9;++Index)
        {
            FRHIGPUTextureReadback Readback(TEXT("DLSS5NR.ColorTest"));FRDGBuilder Graph(RHICmdList);
            const auto Desc=FRDGTextureDesc::Create2D(FIntPoint(80,80),PF_FloatRGBA,FClearValueBinding::None,TexCreate_ShaderResource|TexCreate_UAV);
            auto* Source=Graph.CreateTexture(Desc,TEXT("DLSS5NR.SourceTest"));
            auto* Neural=Graph.CreateTexture(Desc,TEXT("DLSS5NR.NeuralTest"));
            const FVector4f SourceValue(.2f,.4f,.6f,.25f);
            AddClearUAVPass(Graph,Graph.CreateUAV(Source),SourceValue);
            AddClearUAVPass(Graph,Graph.CreateUAV(Neural),Index==3?SourceValue:Index==4?FVector4f(2.f,.6f,.2f,.75f):FVector4f(.8f,.6f,.2f,.75f));
            if(Index>=5)
            {
                auto* Spot=Graph.CreateTexture(FRDGTextureDesc::Create2D(FIntPoint(1,1),PF_FloatRGBA,FClearValueBinding::None,TexCreate_ShaderResource|TexCreate_UAV),TEXT("DLSS5NR.DetailSpot"));
                AddClearUAVPass(Graph,Graph.CreateUAV(Spot),FVector4f(.6f,.4f,.2f,.75f));
                FRHICopyTextureInfo Copy;Copy.DestPosition=FIntVector(40,40,0);Copy.Size=FIntVector(1,1,1);
                AddCopyTexturePass(Graph,Spot,Neural,Copy);
            }
            auto* Output=AddDLSS5NRColorPreservePass(Graph,View,Source,Neural,Rect,
                Index!=2,Index==0?0.f:1.f,1.f,Index>=5?float(Index-5):1.f,Index==8?CreateDLSS5NRStatusTexture(Graph,0):nullptr);
            Results[Index].Bypass=Output==Neural;
            AddEnqueueCopyPass(Graph,&Readback,Output);Graph.Execute();RHICmdList.SubmitAndBlockUntilGPUIdle();
            if(Readback.IsReady())
            {
                int32 Pitch=0;const auto* Pixels=static_cast<const FFloat16Color*>(Readback.Lock(Pitch));
                if(Pixels){Results[Index].Center=Pixels[40*Pitch+40].GetFloats();Results[Index].Edge=Pixels[8*Pitch+8].GetFloats();Results[Index].Read=true;}
                Readback.Unlock();
            }
        }
    });FlushRenderingCommands();
    for(int32 i=0;i<9;++i) TestTrue(FString::Printf(TEXT("Case %d GPU readback"),i),Results[i].Read);
    TestTrue(TEXT("Failed native evaluation bypasses refinement"),Results[8].Center.Equals(FLinearColor(.6f,.4f,.2f,.75f),.001f));
    TestTrue(TEXT("Zero strength schedules no refinement"),Results[0].Bypass);
    TestTrue(TEXT("Zero strength is exact original neural image"),Results[0].Center.Equals(FLinearColor(.8f,.6f,.2f,.75f),.001f));
    for(int32 i=1;i<=2;++i)
    {
        TestTrue(TEXT("Full color and tone reconstruct constant source"),Results[i].Center.Equals(FLinearColor(.2f,.4f,.6f,.75f),.001f));
        TestTrue(TEXT("Non-zero rect boundary clamps valid samples"),Results[i].Edge.Equals(Results[i].Center,.001f));
    }
    TestTrue(TEXT("Identical source/neural remain unchanged"),Results[3].Center.Equals(FLinearColor(.2f,.4f,.6f,.25f),.001f));
    TestTrue(TEXT("HDR values bypass rather than clip"),Results[4].Center.Equals(FLinearColor(2.f,.6f,.2f,.75f),.001f));
    TestTrue(TEXT("Detail zero removes the isolated residual"),Results[5].Center.Equals(FLinearColor(.2f,.4f,.6f,.75f),.001f));
    TestFalse(TEXT("Detail one retains residual"),Results[6].Center.Equals(Results[5].Center,.005f));
    TestFalse(TEXT("Detail two changes residual again"),Results[7].Center.Equals(Results[6].Center,.005f));
    return true;
}
#endif
