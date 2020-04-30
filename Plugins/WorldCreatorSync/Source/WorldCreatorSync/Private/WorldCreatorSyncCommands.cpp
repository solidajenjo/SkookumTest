// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "WorldCreatorSyncCommands.h"

#define LOCTEXT_NAMESPACE "FWorldCreatorSyncModule"

void FWorldCreatorSyncCommands::RegisterCommands()
{
	UI_COMMAND(OpenPluginWindow, "WorldCreatorSync", "Bring up WorldCreatorSync window", EUserInterfaceActionType::Button, FInputGesture());
}

#undef LOCTEXT_NAMESPACE
