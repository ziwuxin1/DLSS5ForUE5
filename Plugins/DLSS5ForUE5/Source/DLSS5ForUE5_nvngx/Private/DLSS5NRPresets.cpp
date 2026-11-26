#include "DLSS5NRSettings.h"
#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "UObject/UnrealType.h"

bool UDLSS5NRSettings::ExportPreset(const FString& Filename, FText& Error) const
{
    const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
    const TSharedRef<FJsonObject> Values = MakeShared<FJsonObject>();
    Root->SetStringField(TEXT("format"), TEXT("DLSS5ForUE5.Preset"));
    Root->SetNumberField(TEXT("version"), 2);
    Root->SetObjectField(TEXT("settings"), Values);
    FString Text;
    if (!FJsonObjectConverter::UStructToJsonObject(GetClass(), this, Values, CPF_Config, 0) ||
        !FJsonSerializer::Serialize(Root, TJsonWriterFactory<>::Create(&Text)) ||
        !FFileHelper::SaveStringToFile(Text, *Filename, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
    {
        Error = FText::FromString(TEXT("Could not save the DLSS5 preset. Check the destination folder and write permissions."));
        return false;
    }
    return true;
}

bool UDLSS5NRSettings::ImportPreset(const FString& Filename, FText& Error)
{
    const auto Reject = [&Error](const FString& Reason)
    {
        Error = FText::FromString(Reason + TEXT(" Current settings were not changed."));
        return false;
    };
    const int64 Size = IFileManager::Get().FileSize(*Filename);
    if (Size <= 0 || Size > 1024 * 1024) return Reject(TEXT("The preset is missing, empty or larger than 1 MB."));
    FString Text;
    TSharedPtr<FJsonObject> Root;
    if (!FFileHelper::LoadFileToString(Text, *Filename) ||
        !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root.IsValid())
        return Reject(TEXT("The preset is not valid JSON."));
    FString Format;
    double Version = 0;
    const TSharedPtr<FJsonObject>* Values = nullptr;
    if (!Root->TryGetStringField(TEXT("format"), Format) || Format != TEXT("DLSS5ForUE5.Preset") ||
        !Root->TryGetNumberField(TEXT("version"), Version) || (Version != 1 && Version != 2) ||
        !Root->TryGetObjectField(TEXT("settings"), Values) || !Values || !Values->IsValid())
        return Reject(TEXT("This is not a supported DLSS5 preset."));

    if (Version == 1)
    {
        if ((*Values)->HasField(FJsonObjectConverter::StandardizeCase(TEXT("GlobalStructure")))) return Reject(TEXT("The version 1 preset contains version 2 fields."));
        (*Values)->SetNumberField(FJsonObjectConverter::StandardizeCase(TEXT("GlobalStructure")), 1.0);
        if ((*Values)->HasField(FJsonObjectConverter::StandardizeCase(TEXT("GlobalTone")))) return Reject(TEXT("The version 1 preset contains version 2 fields."));
        (*Values)->SetNumberField(FJsonObjectConverter::StandardizeCase(TEXT("GlobalTone")), 1.0);
        if ((*Values)->HasField(FJsonObjectConverter::StandardizeCase(TEXT("bDeveloperMasking")))) return Reject(TEXT("The version 1 preset contains version 2 fields."));
        (*Values)->SetBoolField(FJsonObjectConverter::StandardizeCase(TEXT("bDeveloperMasking")), false);
        if ((*Values)->HasField(FJsonObjectConverter::StandardizeCase(TEXT("bShowDeveloperMasks")))) return Reject(TEXT("The version 1 preset contains version 2 fields."));
        (*Values)->SetBoolField(FJsonObjectConverter::StandardizeCase(TEXT("bShowDeveloperMasks")), false);
        if ((*Values)->HasField(FJsonObjectConverter::StandardizeCase(TEXT("bMaskGroup1Enabled")))) return Reject(TEXT("The version 1 preset contains version 2 fields."));
        (*Values)->SetBoolField(FJsonObjectConverter::StandardizeCase(TEXT("bMaskGroup1Enabled")), true);
        if ((*Values)->HasField(FJsonObjectConverter::StandardizeCase(TEXT("MaskGroup1Stencil")))) return Reject(TEXT("The version 1 preset contains version 2 fields."));
        (*Values)->SetNumberField(FJsonObjectConverter::StandardizeCase(TEXT("MaskGroup1Stencil")), 201);
        if ((*Values)->HasField(FJsonObjectConverter::StandardizeCase(TEXT("MaskGroup1Structure")))) return Reject(TEXT("The version 1 preset contains version 2 fields."));
        (*Values)->SetNumberField(FJsonObjectConverter::StandardizeCase(TEXT("MaskGroup1Structure")), 1.0);
        if ((*Values)->HasField(FJsonObjectConverter::StandardizeCase(TEXT("MaskGroup1Tone")))) return Reject(TEXT("The version 1 preset contains version 2 fields."));
        (*Values)->SetNumberField(FJsonObjectConverter::StandardizeCase(TEXT("MaskGroup1Tone")), 1.0);
        if ((*Values)->HasField(FJsonObjectConverter::StandardizeCase(TEXT("bMaskGroup2Enabled")))) return Reject(TEXT("The version 1 preset contains version 2 fields."));
        (*Values)->SetBoolField(FJsonObjectConverter::StandardizeCase(TEXT("bMaskGroup2Enabled")), true);
        if ((*Values)->HasField(FJsonObjectConverter::StandardizeCase(TEXT("MaskGroup2Stencil")))) return Reject(TEXT("The version 1 preset contains version 2 fields."));
        (*Values)->SetNumberField(FJsonObjectConverter::StandardizeCase(TEXT("MaskGroup2Stencil")), 202);
        if ((*Values)->HasField(FJsonObjectConverter::StandardizeCase(TEXT("MaskGroup2Structure")))) return Reject(TEXT("The version 1 preset contains version 2 fields."));
        (*Values)->SetNumberField(FJsonObjectConverter::StandardizeCase(TEXT("MaskGroup2Structure")), 1.0);
        if ((*Values)->HasField(FJsonObjectConverter::StandardizeCase(TEXT("MaskGroup2Tone")))) return Reject(TEXT("The version 1 preset contains version 2 fields."));
        (*Values)->SetNumberField(FJsonObjectConverter::StandardizeCase(TEXT("MaskGroup2Tone")), 1.0);
        if ((*Values)->HasField(FJsonObjectConverter::StandardizeCase(TEXT("bMaskGroup3Enabled")))) return Reject(TEXT("The version 1 preset contains version 2 fields."));
        (*Values)->SetBoolField(FJsonObjectConverter::StandardizeCase(TEXT("bMaskGroup3Enabled")), true);
        if ((*Values)->HasField(FJsonObjectConverter::StandardizeCase(TEXT("MaskGroup3Stencil")))) return Reject(TEXT("The version 1 preset contains version 2 fields."));
        (*Values)->SetNumberField(FJsonObjectConverter::StandardizeCase(TEXT("MaskGroup3Stencil")), 203);
        if ((*Values)->HasField(FJsonObjectConverter::StandardizeCase(TEXT("MaskGroup3Structure")))) return Reject(TEXT("The version 1 preset contains version 2 fields."));
        (*Values)->SetNumberField(FJsonObjectConverter::StandardizeCase(TEXT("MaskGroup3Structure")), 1.0);
        if ((*Values)->HasField(FJsonObjectConverter::StandardizeCase(TEXT("MaskGroup3Tone")))) return Reject(TEXT("The version 1 preset contains version 2 fields."));
        (*Values)->SetNumberField(FJsonObjectConverter::StandardizeCase(TEXT("MaskGroup3Tone")), 1.0);
    }

    // Require the complete current schema, with correct JSON types. This prevents
    // a partial or malformed import from silently inheriting unrelated user values.
    const TSharedRef<FJsonObject> Expected = MakeShared<FJsonObject>();
    FJsonObjectConverter::UStructToJsonObject(GetClass(), this, Expected, CPF_Config, 0);
    if ((*Values)->Values.Num() != Expected->Values.Num()) return Reject(TEXT("The preset has missing or unsupported settings."));
    for (const auto& Entry : Expected->Values)
    {
        const TSharedPtr<FJsonValue>* Value = (*Values)->Values.Find(Entry.Key);
        if (!Value || !Value->IsValid() || (*Value)->Type != Entry.Value->Type)
            return Reject(TEXT("A preset field is missing or has an invalid type."));
        if ((*Value)->Type == EJson::Number && !FMath::IsFinite((*Value)->AsNumber()))
            return Reject(TEXT("A preset field contains a non-finite number."));
    }
    for (TFieldIterator<FProperty> It(GetClass()); It; ++It)
    {
        const FNumericProperty* Number = CastField<FNumericProperty>(*It);
        if (!It->HasAnyPropertyFlags(CPF_Config) || !Number || !Number->IsInteger()) continue;
        const double Value = (*Values)->GetNumberField(FJsonObjectConverter::StandardizeCase(It->GetName()));
        if (Value < MIN_int32 || Value > MAX_int32 || Value != FMath::TruncToDouble(Value))
            return Reject(TEXT("Expected an integer: ") + It->GetName());
    }
    const FString StyleValue = (*Values)->GetStringField(TEXT("style"));
    if (StyleValue != TEXT("Natural") && StyleValue != TEXT("Cinematic"))
        return Reject(TEXT("The preset has an invalid style."));
    UDLSS5NRSettings* Candidate = NewObject<UDLSS5NRSettings>();
    FText ConversionError;
    if (!FJsonObjectConverter::JsonObjectToUStruct((*Values).ToSharedRef(), GetClass(), Candidate, CPF_Config, 0, true, &ConversionError))
        return Reject(TEXT("Invalid preset settings: ") + ConversionError.ToString());

    for (TFieldIterator<FProperty> It(GetClass()); It; ++It)
    {
        FProperty* Property = *It;
        if (!Property->HasAnyPropertyFlags(CPF_Config)) continue;
        if (const FNumericProperty* Number = CastField<FNumericProperty>(Property))
        {
            const void* Address = Property->ContainerPtrToValuePtr<void>(Candidate);
            const double Value = Number->IsFloatingPoint() ? Number->GetFloatingPointPropertyValue(Address) : Number->GetSignedIntPropertyValue(Address);
            if (!FMath::IsFinite(Value)
#if WITH_EDITOR
                ||
                (Property->HasMetaData(TEXT("ClampMin")) && Value < FCString::Atod(*Property->GetMetaData(TEXT("ClampMin")))) ||
                (Property->HasMetaData(TEXT("ClampMax")) && Value > FCString::Atod(*Property->GetMetaData(TEXT("ClampMax"))))
#endif
                )
                return Reject(TEXT("Preset value is outside its range: ") + Property->GetName());
        }
    }
    if (Candidate->PassCount < 1 || Candidate->PassCount > 4 ||
        (Candidate->Style != EDLSS5NRStyle::Natural && Candidate->Style != EDLSS5NRStyle::Cinematic))
        return Reject(TEXT("The preset has an invalid style or pass count."));
    for (TFieldIterator<FProperty> It(GetClass()); It; ++It)
        if (It->HasAnyPropertyFlags(CPF_Config)) It->CopyCompleteValue_InContainer(this, Candidate);
    return true;
}
