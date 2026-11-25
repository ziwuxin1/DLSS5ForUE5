#include "DLSS5NRSettings.h"
#include "DLSS5NRRecovery.h"
#include "HAL/IConsoleManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDLSS5NRConfigurationTest,"DLSS5.Runtime.ConfigurationAndRecovery",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDLSS5NRConfigurationTest::RunTest(const FString&)
{
    UDLSS5NRSettings* Settings=NewObject<UDLSS5NRSettings>();
    struct FSaved { IConsoleVariable* V; FString Value; EConsoleVariableFlags Priority; };
    TArray<FSaved> Saved;
    const TCHAR* Names[]={TEXT("Enable"),TEXT("Style"),TEXT("Preset"),TEXT("PassCount"),TEXT("Intensity"),
        TEXT("LocalTone"),TEXT("LocalStructure"),TEXT("SkinStructure"),TEXT("AutoMask"),TEXT("UICorrection"),
        TEXT("EnableSceneGuides"),TEXT("FeedSceneGuides"),TEXT("UseDepth"),TEXT("UseMotionVectors"),
        TEXT("AutoResetHistory"),TEXT("DepthInverted"),TEXT("MVecScaleX"),TEXT("MVecScaleY"),TEXT("ExposureResetRatio"),TEXT("Debug"),TEXT("Watermark"),TEXT("Watermark.Corner"),TEXT("Watermark.Scale")};
    for(const TCHAR* Name:Names)
    {
        IConsoleVariable* V=IConsoleManager::Get().FindConsoleVariable(*FString::Printf(TEXT("r.DLSS5.%s"),Name));
        if(!TestNotNull(Name,V))return false;
        Saved.Add({V,V->GetString(),EConsoleVariableFlags(V->GetFlags()&ECVF_SetByMask)});
    }
    ON_SCOPE_EXIT
    {
        for(const auto& Old:Saved)
        {
            Old.V->Set(*Old.Value,ECVF_SetByConsole);
            Old.V->SetFlags(EConsoleVariableFlags((Old.V->GetFlags() & ~ECVF_SetByMask) | Old.Priority));
        }
    };
    IConsoleVariable* Enable=Saved[0].V;
    Enable->Set(1,ECVF_SetByConsole);
    Settings->LoadFromCVars();
    TestTrue(TEXT("Configured enable is mirrored, not forced off"),Settings->bEnableNeuralRendering);
    Settings->bEnableNeuralRendering=false;Settings->ApplyToCVars();
    TestEqual(TEXT("Runtime-safe disable reaches live CVar"),Enable->GetInt(),0);
    for(int32 i=0;i<8;++i)
    {
        Settings->bEnableNeuralRendering=(i%2)==0;Settings->ApplyToCVars();Settings->LoadFromCVars();
        TestEqual(TEXT("Repeated toggle remains synchronized"),Enable->GetInt(),Settings->bEnableNeuralRendering?1:0);
    }
    Settings->bShowWatermark = true;
    Settings->WatermarkCorner = 2;
    Settings->WatermarkScale = 1.5f;
    Settings->ApplyToCVars(); Settings->LoadFromCVars();
    TestTrue(TEXT("Watermark stays enabled independently of NR"), Settings->bShowWatermark);
    TestEqual(TEXT("Watermark position survives UI/CVar sync"), Settings->WatermarkCorner, 2);
    TestEqual(TEXT("Watermark scale survives UI/CVar sync"), Settings->WatermarkScale, 1.5f);
    const FString Ini = FPaths::CreateTempFilename(*FPaths::ProjectSavedDir(), TEXT("DLSS5SettingsTest"), TEXT(".ini"));
    Settings->PassCount = 3;
    Settings->Intensity = 0.65f;
    Settings->Style = EDLSS5NRStyle::Cinematic;
    Settings->SaveConfig(CPF_Config, *Ini);
    UDLSS5NRSettings* Reloaded = NewObject<UDLSS5NRSettings>();
    Reloaded->LoadConfig(Reloaded->GetClass(), *Ini);
    TestEqual(TEXT("Pass count survives disk reload"), Reloaded->PassCount, 3);
    TestEqual(TEXT("Style survives disk reload"), Reloaded->Style, EDLSS5NRStyle::Cinematic);
    TestEqual(TEXT("Intensity survives disk reload"), Reloaded->Intensity, 0.65f);
    TestTrue(TEXT("Watermark survives disk reload"), Reloaded->bShowWatermark);
    IConsoleVariable* Passes = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DLSS5.PassCount"));
    Passes->Set(4, ECVF_SetByConsole);
    Reloaded->ApplyToCVars(true);
    TestEqual(TEXT("Explicit console value wins over saved startup value"), Passes->GetInt(), 4);
    Passes->SetFlags(EConsoleVariableFlags(Passes->GetFlags() & ~ECVF_SetByMask));
    Reloaded->ApplyToCVars(true);
    TestEqual(TEXT("Saved startup value replaces constructor default"), Passes->GetInt(), 3);
    IFileManager::Get().Delete(*Ini);
    using DLSS5NRRecovery::IsRecoverable;
    TestTrue(TEXT("Recent dead NR-active process triggers recovery"),IsRecoverable(true,false,0.1));
    TestFalse(TEXT("Live concurrent editor is not a crash"),IsRecoverable(true,true,0.1));
    TestFalse(TEXT("Old marker does not override config"),IsRecoverable(true,false,8));
    TestFalse(TEXT("Malformed marker does not override config"),IsRecoverable(false,false,0.1));
    TestFalse(TEXT("Future timestamp does not override config"),IsRecoverable(true,false,-1));
    return true;
}
#endif
