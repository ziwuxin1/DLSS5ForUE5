#include "DLSS5NRSettings.h"
#include "DLSS5NRRecovery.h"
#include "HAL/IConsoleManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Misc/Paths.h"
#include "Misc/ConfigCacheIni.h"
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
        TEXT("AutoResetHistory"),TEXT("DepthInverted"),TEXT("MVecScaleX"),TEXT("MVecScaleY"),TEXT("ExposureResetRatio"),TEXT("Debug"),TEXT("Watermark"),TEXT("Watermark.Corner"),TEXT("Watermark.Scale"),TEXT("Blend"),TEXT("Compare"),TEXT("Compare.Split"),TEXT("ColorPreserve"),TEXT("TonePreserve"),TEXT("NeuralDetail")};
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
        GetMutableDefault<UDLSS5NRSettings>()->LoadFromCVars();
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
    const FString Ini = FConfigCacheIni::NormalizeConfigIniPath(FPaths::ConvertRelativePathToFull(
        FPaths::CreateTempFilename(*FPaths::ProjectSavedDir(), TEXT("DLSS5SettingsTest"), TEXT(".ini"))));
    Settings->EffectStrength = 0.35f;
    Settings->bCompare = true;
    Settings->SplitPosition = 0.7f;
    Settings->PassCount = 3;
    Settings->ColorPreserve = .9f; Settings->TonePreserve = .6f; Settings->NeuralDetail = 1.3f;
    Settings->LocalTone = 1.7f; Settings->Preset = 2;
    Settings->Intensity = 0.65f;
    Settings->Style = EDLSS5NRStyle::Cinematic;
    Settings->SaveConfig(CPF_Config, *Ini);
    UDLSS5NRSettings* Reloaded = NewObject<UDLSS5NRSettings>();
    Reloaded->LoadConfig(Reloaded->GetClass(), *Ini);
    TestEqual(TEXT("Pass count survives disk reload"), Reloaded->PassCount, 3);
    TestEqual(TEXT("Style survives disk reload"), Reloaded->Style, EDLSS5NRStyle::Cinematic);
    TestEqual(TEXT("Intensity survives disk reload"), Reloaded->Intensity, 0.65f);
    TestEqual(TEXT("Color preserve survives disk reload"), Reloaded->ColorPreserve, .9f);
    TestEqual(TEXT("Tone preserve survives disk reload"), Reloaded->TonePreserve, .6f);
    TestEqual(TEXT("Neural detail survives disk reload"), Reloaded->NeuralDetail, 1.3f);
    TestEqual(TEXT("Local Tone survives disk reload"), Reloaded->LocalTone, 1.7f);
    TestEqual(TEXT("NR Preset survives disk reload"), Reloaded->Preset, 2);
    TestTrue(TEXT("Watermark survives disk reload"), Reloaded->bShowWatermark);
    TestEqual(TEXT("Final-image strength survives disk reload"), Reloaded->EffectStrength, 0.35f);
    TestTrue(TEXT("Comparison toggle survives disk reload"), Reloaded->bCompare);
    TestEqual(TEXT("Comparison position survives disk reload"), Reloaded->SplitPosition, 0.7f);
    IConsoleVariable* Passes = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DLSS5.PassCount"));
    Passes->Set(4, ECVF_SetByConsole);
    Reloaded->ApplyToCVars(true);
    TestEqual(TEXT("Explicit console value wins over saved startup value"), Passes->GetInt(), 4);
    Passes->SetFlags(EConsoleVariableFlags(Passes->GetFlags() & ~ECVF_SetByMask));
    Reloaded->ApplyToCVars(true);
    TestEqual(TEXT("Saved startup value replaces constructor default"), Passes->GetInt(), 3);
    // SaveConfig above mutates the CDO: resetting from that CDO would retain user values.
    // Exercise the actual reset against both visible controls and hidden legacy controls.
    Reloaded->bEnableSceneGuides = true;
    Reloaded->MotionScaleXMultiplier = 2.0f;
    Reloaded->ApplyToCVars();
    Reloaded->ResetToDefaults();
    TestFalse(TEXT("Reset disables NR"), Reloaded->bEnableNeuralRendering);
    TestFalse(TEXT("Reset disables experimental guides"), Reloaded->bEnableSceneGuides);
    TestEqual(TEXT("Reset restores hidden motion scale"), Reloaded->MotionScaleXMultiplier, 1.0f);
    TestEqual(TEXT("Reset uses factory pass count, not saved CDO"), Reloaded->PassCount, 1);
    TestEqual(TEXT("Reset restores full effect strength"), Reloaded->EffectStrength, 1.0f);
    TestEqual(TEXT("Reset disables color filter"), Reloaded->ColorPreserve, 0.f);
    TestEqual(TEXT("Reset restores neutral detail"), Reloaded->NeuralDetail, 1.f);
    TestFalse(TEXT("Reset disables comparison"), Reloaded->bCompare);
    TestFalse(TEXT("Reset disables watermark"), Reloaded->bShowWatermark);
    // UE 5.8 marks loose INI files opened by LoadConfig as NoSave. Unlike the real
    // EditorPerProjectUserSettings hierarchy, this standalone test file must be
    // unloaded before SaveConfig creates a writable temporary branch again.
    GConfig->UnloadFile(Ini);
    Reloaded->SaveConfig(CPF_Config, *Ini);
    FConfigFile DiskConfig;
    DiskConfig.Read(Ini);
    int32 DiskPasses = -1;
    DiskConfig.GetInt(TEXT("/Script/DLSS5ForUE5_nvngx.DLSS5NRSettings"), TEXT("PassCount"), DiskPasses);
    TestEqual(TEXT("Reset writes factory pass count to disk"), DiskPasses, 1);
    GConfig->UnloadFile(Ini);
    Settings->LoadConfig(Settings->GetClass(), *Ini);
    TestFalse(TEXT("Reset remains disabled after disk reload"), Settings->bEnableNeuralRendering);
    TestEqual(TEXT("Reset pass count persists"), Settings->PassCount, 1);
    TestEqual(TEXT("Reset strength persists"), Settings->EffectStrength, 1.0f);
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
