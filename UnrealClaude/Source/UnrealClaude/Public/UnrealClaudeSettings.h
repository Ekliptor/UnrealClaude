// Copyright Natali Caggiano. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "UnrealClaudeSettings.generated.h"

/**
 * Per-project user settings for the UnrealClaude plugin.
 * Exposed in Editor Preferences > Plugins > UnrealClaude.
 */
UCLASS(config=EditorPerProjectUserSettings, defaultconfig, meta=(DisplayName="UnrealClaude"))
class UNREALCLAUDE_API UUnrealClaudeSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UUnrealClaudeSettings();

	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }

	/** Skip the permission dialog for Python scripts. Faster iteration; only enable when you trust the agent. */
	UPROPERTY(EditAnywhere, config, Category="Script Execution")
	bool bAutoApprovePythonScriptExecution = false;

	/** Skip the permission dialog for Console command scripts. */
	UPROPERTY(EditAnywhere, config, Category="Script Execution")
	bool bAutoApproveConsoleScriptExecution = false;

	/** Skip the permission dialog for C++ (Live Coding) scripts. Not recommended - compile failures can destabilize the editor. */
	UPROPERTY(EditAnywhere, config, Category="Script Execution")
	bool bAutoApproveCppScriptExecution = false;
};
