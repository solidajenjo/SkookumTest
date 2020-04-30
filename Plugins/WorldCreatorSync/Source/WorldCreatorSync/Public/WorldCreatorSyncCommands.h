// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "WorldCreatorSyncStyle.h"

class FWorldCreatorSyncCommands : public TCommands<FWorldCreatorSyncCommands>
{
public:

	FWorldCreatorSyncCommands()
		: TCommands<FWorldCreatorSyncCommands>(TEXT("WorldCreatorSync"), NSLOCTEXT("Contexts", "WorldCreatorSync", "WorldCreatorSync Plugin"), NAME_None, FWorldCreatorSyncStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > OpenPluginWindow;
};