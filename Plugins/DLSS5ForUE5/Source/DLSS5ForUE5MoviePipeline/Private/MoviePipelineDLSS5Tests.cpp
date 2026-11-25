#include "MoviePipelineDLSS5Setting.h"
#include "HAL/IConsoleManager.h"
#include "Misc/AutomationTest.h"
#include "MoviePipeline.h"
#include "MoviePipelineHighResSetting.h"
#include "MoviePipelinePrimaryConfig.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDLSS5MRQSettingTest, "DLSS5.MRQ.SettingLifecycle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDLSS5MRQSettingTest::RunTest(const FString&)
{
    auto* Config = NewObject<UMoviePipelinePrimaryConfig>();
    auto* Setting = CastChecked<UMoviePipelineDLSS5Setting>(Config->FindOrAddSettingByClass(UMoviePipelineDLSS5Setting::StaticClass()));
    auto* HighRes = CastChecked<UMoviePipelineHighResSetting>(Config->FindOrAddSettingByClass(UMoviePipelineHighResSetting::StaticClass()));
    HighRes->TileCount = 2;
    Setting->ValidateState();
    TestEqual(TEXT("Tiled capture is rejected before render"), Setting->GetValidationState(), EMoviePipelineValidationState::Errors);
    HighRes->TileCount = 1;
    Setting->ValidateState();
    TestEqual(TEXT("Full-frame capture is valid"), Setting->GetValidationState(), EMoviePipelineValidationState::Valid);
    IConsoleVariable* Enable = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DLSS5.Enable"));
    IConsoleVariable* Watermark = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DLSS5.Watermark"));
    const int32 OldEnable = Enable->GetInt();
    const int32 OldWatermark = Watermark->GetInt();
    const uint32 OldFlags = Enable->GetFlags();
    auto* Pipeline = NewObject<UMoviePipeline>();
    auto* ImplicitSetting = NewObject<UMoviePipelineDLSS5Setting>(Config);
    ImplicitSetting->bEnableNeuralRendering = OldEnable == 0;
    ImplicitSetting->OnMoviePipelineInitialized(Pipeline);
    TestEqual(TEXT("Unconfigured transient setting does not enable NR"), Enable->GetInt(), OldEnable);
    ImplicitSetting->OnMoviePipelineShutdown(Pipeline);
    Setting->bEnableNeuralRendering = OldEnable == 0;
    Setting->bShowWatermark = OldWatermark == 0;
    Setting->OnMoviePipelineInitialized(Pipeline);
    TestEqual(TEXT("Job applies NR switch"), Enable->GetInt(), OldEnable == 0 ? 1 : 0);
    TestEqual(TEXT("Job applies watermark switch"), Watermark->GetInt(), OldWatermark == 0 ? 1 : 0);
    Setting->OnMoviePipelineShutdown(Pipeline);
    TestEqual(TEXT("Completion/cancellation restores NR"), Enable->GetInt(), OldEnable);
    TestEqual(TEXT("Completion/cancellation restores watermark"), Watermark->GetInt(), OldWatermark);
    TestEqual(TEXT("CVar priority is restored"), uint32(Enable->GetFlags()), OldFlags);
    return true;
}
#endif
