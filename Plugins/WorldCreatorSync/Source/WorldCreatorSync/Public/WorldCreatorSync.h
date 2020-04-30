// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ModuleManager.h"

class FToolBarBuilder;
class FMenuBuilder;
class UObject;
struct FButtonStyle;

class FWorldCreatorSyncModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	/** This function will be bound to Command (by default it will bring up plugin window) */
	void PluginButtonClicked();  
	
private:

  struct ObjectProperties
  {
    float posX, posY, posZ;
    float scaleX, scaleY;
    float rotX, rotY, rotZ, rotW;
    unsigned int color;
  };

	void AddToolbarExtension(FToolBarBuilder& Builder);
	void AddMenuExtension(FMenuBuilder& Builder);

	TSharedRef<class SDockTab> OnSpawnPluginTab(const class FSpawnTabArgs& SpawnTabArgs);

  // World Creator
  ////////////////
  FReply SyncButtonClicked();
  FReply BrowseButtonClicked(); 

  const bool GetIsButtonEnabled();

  void LoadTGA(const TCHAR* path, TArray<uint8>& data, int& width, int& height, int& bpp);

  void OnSetReference(UObject* drop);

  FButtonStyle* youtubeButton;
  FButtonStyle* twitterButton;
  FButtonStyle* googlePlusButton;
  FButtonStyle* facebookButton;
  FButtonStyle* discordButton;  

  UObject* meshReference;

private:
	TSharedPtr<class FUICommandList> PluginCommands;
  FString selectedPath;
};