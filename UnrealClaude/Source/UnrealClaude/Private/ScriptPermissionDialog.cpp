// Copyright Natali Caggiano. All Rights Reserved.

#include "ScriptPermissionDialog.h"
#include "UnrealClaudeModule.h"
#include "UnrealClaudeSettings.h"

#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Framework/Application/SlateApplication.h"

bool FScriptPermissionDialog::Show(
	const FString& ScriptPreview,
	EScriptType Type,
	const FString& Description)
{
	// Must be on game thread for Slate
	if (!IsInGameThread())
	{
		UE_LOG(LogUnrealClaude, Error, TEXT("Permission dialog must be shown on game thread"));
		return false;
	}

	// Use shared pointer for approval state to avoid stack variable capture issues
	TSharedPtr<bool> bApprovedPtr = MakeShared<bool>(false);
	TSharedPtr<bool> bRememberPtr = MakeShared<bool>(false);

	// Truncate preview if too long
	FString DisplayPreview = ScriptPreview;
	if (DisplayPreview.Len() > MaxPreviewLength)
	{
		DisplayPreview = DisplayPreview.Left(MaxPreviewLength) + TEXT("\n\n... (truncated)");
	}

	FString TypeStr = ScriptTypeToString(Type);

	// Create the modal window
	TSharedPtr<SWindow> PermissionWindow = SNew(SWindow)
		.Title(FText::FromString(FString::Printf(TEXT("Execute %s Script?"), *TypeStr.ToUpper())))
		.ClientSize(FVector2D(DialogWidth, DialogHeight))
		.SupportsMaximize(false)
		.SupportsMinimize(false);

	// Build and set content
	PermissionWindow->SetContent(
		BuildContent(DisplayPreview, TypeStr, Description, Type, bApprovedPtr, bRememberPtr, PermissionWindow)
	);

	// Show as modal and wait for user response
	FSlateApplication::Get().AddModalWindow(PermissionWindow.ToSharedRef(), nullptr);

	return *bApprovedPtr;
}

TSharedRef<SWidget> FScriptPermissionDialog::BuildContent(
	const FString& DisplayPreview,
	const FString& TypeStr,
	const FString& Description,
	EScriptType Type,
	TSharedPtr<bool> bApprovedPtr,
	TSharedPtr<bool> bRememberPtr,
	TSharedPtr<SWindow> Window)
{
	return SNew(SVerticalBox)
		// Header with warning and description
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			BuildHeaderSection(TypeStr, Description)
		]
		// Script preview
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.Padding(10)
		[
			BuildPreviewSection(DisplayPreview)
		]
		// Remember-this-choice checkbox
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(10, 0)
		[
			BuildRememberRow(TypeStr, bRememberPtr)
		]
		// Buttons
		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Right)
		.Padding(10)
		[
			BuildButtonBar(Type, bApprovedPtr, bRememberPtr, Window)
		];
}

TSharedRef<SVerticalBox> FScriptPermissionDialog::BuildHeaderSection(
	const FString& TypeStr,
	const FString& Description)
{
	return SNew(SVerticalBox)
		// Warning header
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(10)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("⚠️ Claude wants to execute a script. Review the code below:")))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
		]
		// Description
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(10, 0)
		[
			SNew(STextBlock)
			.Text(FText::FromString(FString::Printf(TEXT("Description: %s"), *Description)))
			.AutoWrapText(true)
		];
}

TSharedRef<SWidget> FScriptPermissionDialog::BuildPreviewSection(const FString& DisplayPreview)
{
	return SNew(SBox)
		.Padding(5)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				SNew(STextBlock)
				.Text(FText::FromString(DisplayPreview))
				.Font(FCoreStyle::GetDefaultFontStyle("Mono", 9))
				.AutoWrapText(false)
			]
		];
}

TSharedRef<SWidget> FScriptPermissionDialog::BuildRememberRow(
	const FString& TypeStr,
	TSharedPtr<bool> bRememberPtr)
{
	const FString Label = FString::Printf(
		TEXT("Remember this choice for future %s scripts (also editable in Editor Preferences > Plugins > UnrealClaude)"),
		*TypeStr);

	return SNew(SCheckBox)
		.IsChecked(ECheckBoxState::Unchecked)
		.OnCheckStateChanged_Lambda([bRememberPtr](ECheckBoxState NewState) {
			*bRememberPtr = (NewState == ECheckBoxState::Checked);
		})
		[
			SNew(STextBlock)
			.Text(FText::FromString(Label))
			.AutoWrapText(true)
		];
}

namespace
{
	void ApplyRememberedChoice(EScriptType Type, bool bApproved)
	{
		UUnrealClaudeSettings* Settings = GetMutableDefault<UUnrealClaudeSettings>();
		if (!Settings)
		{
			return;
		}

		switch (Type)
		{
			case EScriptType::Python:  Settings->bAutoApprovePythonScriptExecution = bApproved; break;
			case EScriptType::Console: Settings->bAutoApproveConsoleScriptExecution = bApproved; break;
			case EScriptType::Cpp:     Settings->bAutoApproveCppScriptExecution = bApproved; break;
			default: return;
		}

		Settings->SaveConfig();

		UE_LOG(LogUnrealClaude, Log, TEXT("Remembered choice for %s scripts: auto-approve = %s"),
			*ScriptTypeToString(Type), bApproved ? TEXT("true") : TEXT("false"));
	}
}

TSharedRef<SWidget> FScriptPermissionDialog::BuildButtonBar(
	EScriptType Type,
	TSharedPtr<bool> bApprovedPtr,
	TSharedPtr<bool> bRememberPtr,
	TSharedPtr<SWindow> Window)
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(0, 0, 10, 0)
		[
			SNew(SButton)
			.Text(FText::FromString(TEXT("Allow")))
			.OnClicked_Lambda([Type, bApprovedPtr, bRememberPtr, Window]() {
				*bApprovedPtr = true;
				if (*bRememberPtr)
				{
					ApplyRememberedChoice(Type, true);
				}
				if (Window.IsValid())
				{
					Window->RequestDestroyWindow();
				}
				return FReply::Handled();
			})
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SButton)
			.Text(FText::FromString(TEXT("Deny")))
			.OnClicked_Lambda([Type, bApprovedPtr, bRememberPtr, Window]() {
				*bApprovedPtr = false;
				if (*bRememberPtr)
				{
					ApplyRememberedChoice(Type, false);
				}
				if (Window.IsValid())
				{
					Window->RequestDestroyWindow();
				}
				return FReply::Handled();
			})
		];
}
