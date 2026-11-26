#include "DLSS5ForUE5EditorModule.h"

#include "DLSS5NRSettings.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Framework/Commands/UIAction.h"
#include "Framework/Docking/TabManager.h"
#include "HAL/IConsoleManager.h"
#include "IDetailCustomization.h"
#include "IDetailPropertyRow.h"
#include "IPropertyUtilities.h"
#include "IDetailsView.h"
#include "Interfaces/IPluginManager.h"
#include "PropertyEditorModule.h"
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/Paths.h"
#include "Misc/MessageDialog.h"
#include "HAL/FileManager.h"
#include "Styling/AppStyle.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "DLSS5ForUE5Editor"

namespace
{
    const FName DLSS5ForUE5TabName(TEXT("DLSS5ForUE5"));

    int32 GetPassCount()
    {
        const UDLSS5NRSettings* Settings = GetDefault<UDLSS5NRSettings>();
        return Settings ? FMath::Clamp(Settings->PassCount, 1, 4) : 1;
    }

    void SetPassCountFromSlider(float NewValue)
    {
        UDLSS5NRSettings* Settings = GetMutableDefault<UDLSS5NRSettings>();
        if (!Settings)
        {
            return;
        }

        const int32 NewCount = FMath::Clamp(FMath::RoundToInt(NewValue * 3.0f) + 1, 1, 4);
        if (Settings->PassCount != NewCount)
        {
            Settings->PassCount = NewCount;
            Settings->ApplyToCVars();
            Settings->SaveUserSettings();
            Settings->RefreshDiagnostics();
        }
    }

    class FDLSS5NRSettingsCustomization final : public IDetailCustomization
    {
    public:
        static TSharedRef<IDetailCustomization> MakeInstance()
        {
            return MakeShared<FDLSS5NRSettingsCustomization>();
        }

        virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override
        {
            const TWeakPtr<IPropertyUtilities> Utilities = DetailBuilder.GetPropertyUtilities();
            const auto MakeReset = [Utilities](FName PropertyName)
            {
                // Always show the familiar UE reset arrow, even when already at
                // default. The regular CDO-based comparison cannot work here.
                return FResetToDefaultOverride::Create(FSimpleDelegate::CreateLambda([Utilities, PropertyName]()
                {
                    UDLSS5NRSettings* Settings = GetMutableDefault<UDLSS5NRSettings>();
                    if (Settings->ResetPropertyToDefault(PropertyName))
                    {
                        Settings->SaveUserSettings();
                        if (const TSharedPtr<IPropertyUtilities> Pinned = Utilities.Pin()) Pinned->RequestRefresh();
                    }
                }));
            };
            const FName ResettableProperties[] = {
                GET_MEMBER_NAME_CHECKED(UDLSS5NRSettings, bEnableNeuralRendering),
                GET_MEMBER_NAME_CHECKED(UDLSS5NRSettings, Style),
                GET_MEMBER_NAME_CHECKED(UDLSS5NRSettings, Preset),
                GET_MEMBER_NAME_CHECKED(UDLSS5NRSettings, EffectStrength),
                GET_MEMBER_NAME_CHECKED(UDLSS5NRSettings, Intensity),
                GET_MEMBER_NAME_CHECKED(UDLSS5NRSettings, LocalTone),
                GET_MEMBER_NAME_CHECKED(UDLSS5NRSettings, bShowWatermark),
                GET_MEMBER_NAME_CHECKED(UDLSS5NRSettings, WatermarkCorner),
                GET_MEMBER_NAME_CHECKED(UDLSS5NRSettings, WatermarkScale),
                GET_MEMBER_NAME_CHECKED(UDLSS5NRSettings, ColorPreserve),
                GET_MEMBER_NAME_CHECKED(UDLSS5NRSettings, TonePreserve),
                GET_MEMBER_NAME_CHECKED(UDLSS5NRSettings, NeuralDetail),
                GET_MEMBER_NAME_CHECKED(UDLSS5NRSettings, bCompare),
                GET_MEMBER_NAME_CHECKED(UDLSS5NRSettings, SplitPosition)
            };
            for (FName PropertyName : ResettableProperties)
            {
                if (IDetailPropertyRow* Row = DetailBuilder.EditDefaultProperty(DetailBuilder.GetProperty(PropertyName)))
                    Row->OverrideResetToDefault(MakeReset(PropertyName));
            }
            // Retain the requested Style, Preset, Intensity and Local Tone controls.
            // Hide only the remaining explicitly removed legacy/experimental controls.
            DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(UDLSS5NRSettings, LocalStructure));
            DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(UDLSS5NRSettings, SkinStructure));
            DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(UDLSS5NRSettings, bAutoMask));
            DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(UDLSS5NRSettings, bUICorrection));
            DetailBuilder.HideCategory(TEXT("Scene Guides"));
            DetailBuilder.HideCategory(TEXT("Advanced"));
            DetailBuilder.EditCategory(TEXT("General"), LOCTEXT("GeneralCategory", "General"), ECategoryPriority::Important);

            IDetailCategoryBuilder& PassCategory = DetailBuilder.EditCategory(
                TEXT("DLSS5PassCount"),
                LOCTEXT("PassCategory", "DLSS 5 Pass Count"),
                ECategoryPriority::Default);

            PassCategory.AddCustomRow(LOCTEXT("PassCountSearch", "DLSS 5 Pass Count Passes Multi Pass"))
            .OverrideResetToDefault(MakeReset(GET_MEMBER_NAME_CHECKED(UDLSS5NRSettings, PassCount)))
            .NameContent()
            [
                SNew(STextBlock)
                .Text(LOCTEXT("PassCountLabel", "Passes"))
                .ToolTipText(LOCTEXT("PassCountTip", "Run Neural Rendering sequentially 1 to 4 times. Each additional pass consumes the previous pass output and increases GPU cost."))
            ]
            .ValueContent()
            .MinDesiredWidth(420.0f)
            .MaxDesiredWidth(1000.0f)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                .VAlign(VAlign_Center)
                [
                    SNew(SSlider)
                    .Value_Lambda([]()
                    {
                        return static_cast<float>(GetPassCount() - 1) / 3.0f;
                    })
                    .StepSize(1.0f / 3.0f)
                    .ToolTipText(LOCTEXT("PassSliderTip", "1 = one Neural Rendering pass. 4 = four sequential passes."))
                    .OnValueChanged_Lambda([](float NewValue)
                    {
                        SetPassCountFromSlider(NewValue);
                    })
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                .Padding(12.0f, 0.0f, 0.0f, 0.0f)
                [
                    SNew(STextBlock)
                    .MinDesiredWidth(22.0f)
                    .Justification(ETextJustify::Center)
                    .Text_Lambda([]()
                    {
                        return FText::AsNumber(GetPassCount());
                    })
                ]
            ];

            DetailBuilder.EditCategory(TEXT("Neural Controls"), LOCTEXT("NeuralCategory", "Neural Controls"), ECategoryPriority::Default);
            DetailBuilder.EditCategory(TEXT("Color Preservation"), LOCTEXT("ColorCategory", "Color Preservation"), ECategoryPriority::Default);
            DetailBuilder.EditCategory(TEXT("Comparison"), LOCTEXT("ComparisonCategory", "Before / After"), ECategoryPriority::Default);
        }
    };

    void OpenDLSS5WindowFromConsole()
    {
        FGlobalTabmanager::Get()->TryInvokeTab(DLSS5ForUE5TabName);
    }

    FAutoConsoleCommand GDLSS5OpenWindowCommand(
        TEXT("DLSS5.Open"),
        TEXT("Open the DLSS 5 for UE5 control window."),
        FConsoleCommandDelegate::CreateStatic(&OpenDLSS5WindowFromConsole));
}

void FDLSS5ForUE5EditorModule::StartupModule()
{
    FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
    PropertyEditor.RegisterCustomClassLayout(
        UDLSS5NRSettings::StaticClass()->GetFName(),
        FOnGetDetailCustomizationInstance::CreateStatic(&FDLSS5NRSettingsCustomization::MakeInstance));
    PropertyEditor.NotifyCustomizationModuleChanged();

    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
        DLSS5ForUE5TabName,
        FOnSpawnTab::CreateRaw(this, &FDLSS5ForUE5EditorModule::SpawnPluginTab))
        .SetDisplayName(LOCTEXT("TabTitle", "DLSS 5 for UE5"))
        .SetTooltipText(LOCTEXT("TabTooltip", "Open DLSS 5 for UE5 controls."))
        .SetMenuType(ETabSpawnerMenuType::Hidden);

    UToolMenus::RegisterStartupCallback(
        FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FDLSS5ForUE5EditorModule::RegisterMenus));
}

void FDLSS5ForUE5EditorModule::ShutdownModule()
{
    UToolMenus::UnRegisterStartupCallback(this);
    UToolMenus::UnregisterOwner(this);

    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(DLSS5ForUE5TabName);
    DetailsView.Reset();

    if (FModuleManager::Get().IsModuleLoaded(TEXT("PropertyEditor")))
    {
        FPropertyEditorModule& PropertyEditor = FModuleManager::GetModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
        PropertyEditor.UnregisterCustomClassLayout(UDLSS5NRSettings::StaticClass()->GetFName());
        PropertyEditor.NotifyCustomizationModuleChanged();
    }
}

void FDLSS5ForUE5EditorModule::RegisterMenus()
{
    FToolMenuOwnerScoped OwnerScoped(this);

    UToolMenu* ToolsMenu = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.MainMenu.Tools"));
    FToolMenuSection& Section = ToolsMenu->FindOrAddSection(TEXT("DLSS5ForUE5"));
    Section.AddMenuEntry(
        TEXT("OpenDLSS5ForUE5"),
        LOCTEXT("OpenWindow", "DLSS 5 for UE5"),
        LOCTEXT("OpenWindowTooltip", "Open Neural Rendering controls."),
        FSlateIcon(),
        FUIAction(FExecuteAction::CreateRaw(this, &FDLSS5ForUE5EditorModule::OpenWindow)));

    UToolMenu* Toolbar = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.LevelEditorToolBar.PlayToolBar"));
    FToolMenuSection& ToolbarSection = Toolbar->FindOrAddSection(TEXT("DLSS5ForUE5"));
    // The P4 toolbar callback may run later; resolve ordering when the menu is built.
    const TSharedPtr<IPlugin> P4Plugin = IPluginManager::Get().FindPlugin(TEXT("UE5P4VUploadTool"));
    if (P4Plugin.IsValid() && P4Plugin->IsEnabled())
        ToolbarSection.InsertPosition = FToolMenuInsert(TEXT("P4VUpload"), EToolMenuInsertType::After);
    // A text widget stays labelled D5 even when toolbar labels are hidden.
    ToolbarSection.AddEntry(FToolMenuEntry::InitWidget(
        TEXT("DLSS5ToolbarButton"),
        SNew(SButton)
        .ButtonStyle(FAppStyle::Get(), "SimpleButton")
        .ContentPadding(FMargin(8.0f, 4.0f))
        .ToolTipText(LOCTEXT("D5Tooltip", "Open DLSS 5 controls"))
        .OnClicked_Lambda([this]() { OpenWindow(); return FReply::Handled(); })
        [
            SNew(STextBlock)
            .Text(LOCTEXT("D5Label", "D5"))
            .Font(FAppStyle::GetFontStyle("BoldFont"))
        ],
        LOCTEXT("D5ToolbarLabel", "D5")));
}

void FDLSS5ForUE5EditorModule::OpenWindow()
{
    FGlobalTabmanager::Get()->TryInvokeTab(DLSS5ForUE5TabName);
}

TSharedRef<SDockTab> FDLSS5ForUE5EditorModule::SpawnPluginTab(const FSpawnTabArgs& Args)
{
    FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));

    FDetailsViewArgs DetailsArgs;
    DetailsArgs.bAllowSearch = true;
    DetailsArgs.bHideSelectionTip = true;
    DetailsArgs.bShowOptions = true;
    DetailsArgs.bShowModifiedPropertiesOption = true;
    DetailsArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;

    DetailsView = PropertyEditor.CreateDetailView(DetailsArgs);

    if (UDLSS5NRSettings* Settings = GetMutableDefault<UDLSS5NRSettings>())
    {
        Settings->LoadFromCVars();
        Settings->RefreshDiagnostics();
        DetailsView->SetObject(Settings);
    }

    return SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(12.0f, 10.0f, 12.0f, 6.0f)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                .VAlign(VAlign_Center)
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        SNew(STextBlock)
                        .Text(LOCTEXT("Header", "DLSS 5 for UE5"))
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 2.0f, 0.0f, 0.0f)
                    [
                        SNew(STextBlock)
                        .Text(LOCTEXT("Subtitle", "Changes are saved automatically for this project and restored next time you open the editor."))
                    ]
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                .Padding(12.0f, 0.0f, 0.0f, 0.0f)
                [
                    SNew(SButton)
                    .Text(LOCTEXT("ResetDefaults", "Reset Defaults"))
                    .ToolTipText(LOCTEXT("ResetDefaultsTip", "Restore all panel controls to factory defaults (including NR and watermark off) and save for this project."))
                    .OnClicked_Raw(this, &FDLSS5ForUE5EditorModule::ResetDefaults)
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                .Padding(12.0f, 0.0f)
                [
                    SNew(SButton)
                    .Text(LOCTEXT("SafeMode", "Safe Mode"))
                    .ToolTipText(LOCTEXT("SafeModeTip", "Disable Neural Rendering and all experimental scene-guide execution, then save the safe configuration."))
                    .OnClicked_Raw(this, &FDLSS5ForUE5EditorModule::EnterSafeMode)
                ]
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(12.0f, 0.0f, 12.0f, 8.0f)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth()
                [
                    SNew(SButton).Text(LOCTEXT("SavePreset", "Save Preset..."))
                    .ToolTipText(LOCTEXT("SavePresetTip", "Save the current DLSS5 settings to a reusable preset file."))
                    .OnClicked_Raw(this, &FDLSS5ForUE5EditorModule::SavePreset)
                ]
                + SHorizontalBox::Slot().AutoWidth().Padding(8.0f, 0.0f)
                [
                    SNew(SButton).Text(LOCTEXT("LoadPreset", "Load Preset..."))
                    .ToolTipText(LOCTEXT("LoadPresetTip", "Load, apply and remember a saved DLSS5 preset for this project."))
                    .OnClicked_Raw(this, &FDLSS5ForUE5EditorModule::LoadPreset)
                ]
            ]
            + SVerticalBox::Slot()
            .FillHeight(1.0f)
            .Padding(8.0f, 0.0f, 8.0f, 6.0f)
            [
                SNew(SBorder)
                .Padding(4.0f)
                [
                    DetailsView.ToSharedRef()
                ]
            ]
        ];
}

FReply FDLSS5ForUE5EditorModule::EnterSafeMode()
{
    if (UDLSS5NRSettings* Settings = GetMutableDefault<UDLSS5NRSettings>())
    {
        Settings->bEnableNeuralRendering = false;
        Settings->bEnableSceneGuides = false;
        Settings->bFeedSceneGuides = false;
        Settings->bUseMotionVectors = false;
        Settings->ApplyToCVars();
        Settings->SaveUserSettings();
        Settings->RefreshDiagnostics();
    }

    if (DetailsView.IsValid())
    {
        DetailsView->ForceRefresh();
    }

    return FReply::Handled();
}

FReply FDLSS5ForUE5EditorModule::ResetDefaults()
{
    if (UDLSS5NRSettings* Settings = GetMutableDefault<UDLSS5NRSettings>())
    {
        Settings->ResetToDefaults();
        Settings->SaveUserSettings();
    }
    if (DetailsView.IsValid()) DetailsView->ForceRefresh();
    return FReply::Handled();
}

FReply FDLSS5ForUE5EditorModule::SavePreset()
{
    static bool bDialogOpen = false;
    if (bDialogOpen) return FReply::Handled();
    TGuardValue<bool> DialogGuard(bDialogOpen, true);
    IDesktopPlatform* Desktop = FDesktopPlatformModule::Get();
    if (!Desktop) return FReply::Handled();
    const FString Folder = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("DLSS5Presets"));
    IFileManager::Get().MakeDirectory(*Folder, true);
    TArray<FString> Files;
    if (Desktop->SaveFileDialog(FSlateApplication::Get().FindBestParentWindowHandleForDialogs(DetailsView),
        TEXT("Save DLSS5 Preset"), Folder, TEXT("DLSS5Preset.json"), TEXT("DLSS5 Preset (*.json)|*.json"), 0, Files) && Files.Num())
    {
        FString Filename = Files[0];
        if (FPaths::GetExtension(Filename).IsEmpty()) Filename += TEXT(".json");
        UDLSS5NRSettings* Settings = GetMutableDefault<UDLSS5NRSettings>();
        Settings->LoadFromCVars();
        FText Error;
        if (!Settings->ExportPreset(Filename, Error)) FMessageDialog::Open(EAppMsgType::Ok, Error);
    }
    return FReply::Handled();
}

FReply FDLSS5ForUE5EditorModule::LoadPreset()
{
    static bool bDialogOpen = false;
    if (bDialogOpen) return FReply::Handled();
    TGuardValue<bool> DialogGuard(bDialogOpen, true);
    IDesktopPlatform* Desktop = FDesktopPlatformModule::Get();
    if (!Desktop) return FReply::Handled();
    TArray<FString> Files;
    if (Desktop->OpenFileDialog(FSlateApplication::Get().FindBestParentWindowHandleForDialogs(DetailsView),
        TEXT("Load DLSS5 Preset"), FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("DLSS5Presets")),
        TEXT(""), TEXT("DLSS5 Preset (*.json)|*.json"), 0, Files) && Files.Num())
    {
        UDLSS5NRSettings* Settings = GetMutableDefault<UDLSS5NRSettings>();
        FText Error;
        if (Settings->ImportPreset(Files[0], Error))
        {
            Settings->ApplyToCVars();
            Settings->SaveUserSettings();
            if (DetailsView.IsValid()) DetailsView->ForceRefresh();
        }
        else FMessageDialog::Open(EAppMsgType::Ok, Error);
    }
    return FReply::Handled();
}

IMPLEMENT_MODULE(FDLSS5ForUE5EditorModule, DLSS5ForUE5Editor)

#undef LOCTEXT_NAMESPACE
