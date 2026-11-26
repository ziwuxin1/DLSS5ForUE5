#include "DLSS5NRSettings.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "UObject/UnrealType.h"
#include "JsonObjectConverter.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

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
    Source->GlobalStructure=.3f;Source->GlobalTone=.7f;Source->bDeveloperMasking=true;
    Source->MaskGroup1Stencil=211;Source->MaskGroup1Structure=0;Source->MaskGroup1Tone=.2f;
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
        {TEXT("\"version\": 2"),TEXT("\"version\": 99")},
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
    TSharedPtr<FJsonObject> Legacy;
    FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Valid),Legacy);
    Legacy->SetNumberField(TEXT("version"),1);
    const auto Values=Legacy->GetObjectField(TEXT("settings"));
    for(TFieldIterator<FProperty> It(Source->GetClass());It;++It)
    {
        const FString Category=It->GetMetaData(TEXT("Category"));
        if(Category==TEXT("Global Controls") || Category==TEXT("Developer Masking") || Category.StartsWith(TEXT("Mask Group ")))
            Values->RemoveField(FJsonObjectConverter::StandardizeCase(It->GetName()));
    }
    FString LegacyText;FJsonSerializer::Serialize(Legacy.ToSharedRef(),TJsonWriterFactory<>::Create(&LegacyText));
    FFileHelper::SaveStringToFile(LegacyText,*File);
    TestTrue(TEXT("Migrate complete r4/r5 version 1 preset"),Loaded->ImportPreset(File,Error));
    TestEqual(TEXT("Legacy preset gets neutral global structure"),Loaded->GlobalStructure,1.f);
    TestEqual(TEXT("Legacy preset gets neutral global tone"),Loaded->GlobalTone,1.f);
    TestFalse(TEXT("Legacy preset disables new masking"),Loaded->bDeveloperMasking);
    TestEqual(TEXT("Legacy preset retains original intensity"),Loaded->Intensity,.65f);
    TestEqual(TEXT("Legacy preset gets factory group ID"),Loaded->MaskGroup1Stencil,201);
    IFileManager::Get().Delete(*File);
    return true;
}
#endif
