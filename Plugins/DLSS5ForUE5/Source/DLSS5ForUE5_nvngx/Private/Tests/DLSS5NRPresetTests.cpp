#include "DLSS5NRSettings.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "UObject/UnrealType.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDLSS5NRPresetTest,"DLSS5.Runtime.PresetRoundTripAndValidation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDLSS5NRPresetTest::RunTest(const FString&)
{
    auto* Source=NewObject<UDLSS5NRSettings>();
    Source->Style=EDLSS5NRStyle::Cinematic;Source->Preset=2;Source->PassCount=3;
    Source->Intensity=.65f;Source->LocalTone=1.7f;Source->EffectStrength=.8f;
    Source->ColorPreserve=.9f;Source->TonePreserve=.6f;Source->NeuralDetail=1.3f;
    Source->bCompare=true;Source->SplitPosition=.4f;Source->bShowWatermark=true;
    const FString File=FPaths::CreateTempFilename(*FPaths::ProjectSavedDir(),TEXT("DLSS5Preset"),TEXT(".json"));
    FText Error;
    TestTrue(TEXT("Export preset"),Source->ExportPreset(File,Error));
    auto* Loaded=NewObject<UDLSS5NRSettings>();
    TestTrue(TEXT("Import complete preset"),Loaded->ImportPreset(File,Error));
    for(TFieldIterator<FProperty> It(Source->GetClass());It;++It)
        if(It->HasAnyPropertyFlags(CPF_Config)) TestTrue(It->GetName()+TEXT(" round trips"),It->Identical_InContainer(Source,Loaded));
    FString Valid;
    FFileHelper::LoadFileToString(Valid,*File);
    struct FCase { const TCHAR* From; const TCHAR* To; };
    const FCase Cases[]={{TEXT("\"preset\": 2"),TEXT("\"preset\": 2.5")},
        {TEXT("\"version\": 1"),TEXT("\"version\": 99")},
        {TEXT("\"preset\": 2"),TEXT("\"preset\": 999")},
        {TEXT("\"colorPreserve\""),TEXT("\"unknownField\"")},
        {TEXT("\"Cinematic\""),TEXT("\"InvalidStyle\"")}};
    for(const auto& Case:Cases)
    {
        const FString Invalid=Valid.Replace(Case.From,Case.To);
        TestTrue(TEXT("Invalid fixture really changes JSON"),Invalid!=Valid);
        FFileHelper::SaveStringToFile(Invalid,*File);
        TestFalse(TEXT("Reject malformed preset"),Loaded->ImportPreset(File,Error));
        for(TFieldIterator<FProperty> It(Source->GetClass());It;++It)
            if(It->HasAnyPropertyFlags(CPF_Config)) TestTrue(It->GetName()+TEXT(" unchanged after rejection"),It->Identical_InContainer(Source,Loaded));
    }
    IFileManager::Get().Delete(*File);
    return true;
}
#endif
