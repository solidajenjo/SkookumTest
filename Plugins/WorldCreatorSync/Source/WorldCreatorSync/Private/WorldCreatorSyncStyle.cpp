// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "WorldCreatorSyncStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Framework/Application/SlateApplication.h"
#include "SlateGameResources.h"
#include "IPluginManager.h"

TSharedPtr< FSlateStyleSet > FWorldCreatorSyncStyle::StyleInstance = NULL;

void FWorldCreatorSyncStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FWorldCreatorSyncStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FWorldCreatorSyncStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("WorldCreatorSyncStyle"));
	return StyleSetName;
}

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define TTF_FONT( RelativePath, ... ) FSlateFontInfo( Style->RootToContentDir( RelativePath, TEXT(".ttf") ), __VA_ARGS__ )
#define OTF_FONT( RelativePath, ... ) FSlateFontInfo( Style->RootToContentDir( RelativePath, TEXT(".otf") ), __VA_ARGS__ )

const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);
const FVector2D Icon40x40(40.0f, 40.0f);

const FVector2D WorldCreator(380.0f, 72.0f);
const FVector2D LinkIcon(60.0f, 60.0f);


TSharedRef< FSlateStyleSet > FWorldCreatorSyncStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("WorldCreatorSyncStyle"));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("WorldCreatorSync")->GetBaseDir() / TEXT("Resources"));

	Style->Set("WorldCreatorSync.OpenPluginWindow", new IMAGE_BRUSH(TEXT("ButtonIcon_40x"), Icon40x40));


  Style->Set("WorldCreatorSync.WorldCreator", new IMAGE_BRUSH(TEXT("worldcreator"), WorldCreator));
  Style->Set("WorldCreatorSync.WorldCreatorInv", new IMAGE_BRUSH(TEXT("worldcreator_inv"), WorldCreator));
  Style->Set("WorldCreatorSync.Twitter", new IMAGE_BRUSH(TEXT("twitter"), LinkIcon));
  Style->Set("WorldCreatorSync.Youtube", new IMAGE_BRUSH(TEXT("youtube"), LinkIcon));
  Style->Set("WorldCreatorSync.Googleplus", new IMAGE_BRUSH(TEXT("googleplus"), LinkIcon));
  Style->Set("WorldCreatorSync.Facebook", new IMAGE_BRUSH(TEXT("facebook"), LinkIcon));
  Style->Set("WorldCreatorSync.Discord", new IMAGE_BRUSH(TEXT("discord"), LinkIcon));

  

	return Style;
}

#undef IMAGE_BRUSH
#undef BOX_BRUSH
#undef BORDER_BRUSH
#undef TTF_FONT
#undef OTF_FONT

void FWorldCreatorSyncStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FWorldCreatorSyncStyle::Get()
{
	return *StyleInstance;
}
