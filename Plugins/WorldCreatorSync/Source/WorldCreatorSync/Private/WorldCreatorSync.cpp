// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
#include "WorldCreatorSync.h"
#include "XmlHelper.h"
#include "WorldCreatorSyncStyle.h"
#include "WorldCreatorSyncCommands.h"
#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Editor/UnrealEd/Public/Editor.h"
#include "Editor/UnrealEd/Public/Editor.h"
#include "Runtime/Landscape/Classes/Landscape.h"
#include "Runtime/Landscape/Classes/LandscapeGizmoActiveActor.h"
#include "Runtime/Landscape/Classes/LandscapeLayerInfoObject.h"
#include "Runtime/Landscape/Classes/LandscapeInfo.h"
#include "Runtime/Landscape/Classes/LandscapeStreamingProxy.h"
#include "Runtime/Landscape/Classes/LandscapeComponent.h"
#include "Runtime/Core/Public/Misc/FileHelper.h"
#include "Runtime/Engine/Public/EngineUtils.h"
#include "Runtime/ImageWrapper/Public/IImageWrapper.h"
#include "Runtime/ImageWrapper/Public/IImageWrapperModule.h"
#include "Runtime/XmlParser/Public/XmlParser.h"
#include "Runtime/Core/Public/Misc/Base64.h"
#include "Runtime/Engine/Classes/Components/InstancedStaticMeshComponent.h"
#include "Runtime/Foliage/Public/InstancedFoliageActor.h"
#include "Runtime/CoreUObject/Public/UObject/UObjectGlobals.h"
#include "Runtime/Engine/Classes/Engine/StaticMesh.h"
#include "Developer/RawMesh/Public/RawMesh.h"
#include "Developer/DesktopPlatform/Public/DesktopPlatformModule.h"
#include "Runtime/Slate/Public/Framework/Application/SlateApplication.h"
#include "Runtime/SlateCore/Public/Widgets/Images/SImage.h"
#include "Editor/Kismet/Public/WorkflowOrientedApp/SContentReference.h"
#include "Runtime/Core/Public/Misc/Compression.h"
#include "Runtime/Engine/Classes/Materials/Material.h"
#include "Runtime/Foliage/Public/FoliageType.h"

#define PACKAGE 1




static const FName WorldCreatorSyncTabName("WorldCreatorSync");

#define LOCTEXT_NAMESPACE "FWorldCreatorSyncModule"

void FWorldCreatorSyncModule::StartupModule()
{
  // This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

  FWorldCreatorSyncStyle::Initialize();
  FWorldCreatorSyncStyle::ReloadTextures();

  FWorldCreatorSyncCommands::Register();

  PluginCommands = MakeShareable(new FUICommandList);

  PluginCommands->MapAction(
    FWorldCreatorSyncCommands::Get().OpenPluginWindow,
    FExecuteAction::CreateRaw(this, &FWorldCreatorSyncModule::PluginButtonClicked),
    FCanExecuteAction());

  FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");

  {
    TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());
    MenuExtender->AddMenuExtension("WindowLayout", EExtensionHook::After, PluginCommands, FMenuExtensionDelegate::CreateRaw(this, &FWorldCreatorSyncModule::AddMenuExtension));

    LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);
  }

  {
    TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
    ToolbarExtender->AddToolBarExtension("Settings", EExtensionHook::After, PluginCommands, FToolBarExtensionDelegate::CreateRaw(this, &FWorldCreatorSyncModule::AddToolbarExtension));

    LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);
  }

  FGlobalTabmanager::Get()->RegisterNomadTabSpawner(WorldCreatorSyncTabName, FOnSpawnTab::CreateRaw(this, &FWorldCreatorSyncModule::OnSpawnPluginTab))
    .SetDisplayName(LOCTEXT("FWorldCreatorSyncTabTitle", "WorldCreatorSync"))
    .SetMenuType(ETabSpawnerMenuType::Hidden);

  this->selectedPath = TEXT("");

  // Init Brushes
  ///////////////
  auto youtubeBrush = FWorldCreatorSyncStyle::Get().GetBrush("WorldCreatorSync.Youtube");
  auto twitterBrush = FWorldCreatorSyncStyle::Get().GetBrush("WorldCreatorSync.Twitter");
  auto googlePlusBrush = FWorldCreatorSyncStyle::Get().GetBrush("WorldCreatorSync.Googleplus");
  auto facebookBrush = FWorldCreatorSyncStyle::Get().GetBrush("WorldCreatorSync.Facebook");
  auto discordBrush = FWorldCreatorSyncStyle::Get().GetBrush("WorldCreatorSync.Discord");

  youtubeButton = new FButtonStyle();
  youtubeButton->SetDisabled(*youtubeBrush);
  youtubeButton->SetHovered(*youtubeBrush);
  youtubeButton->SetNormal(*youtubeBrush);
  youtubeButton->SetPressed(*youtubeBrush);

  twitterButton = new FButtonStyle();
  twitterButton->SetDisabled(*twitterBrush);
  twitterButton->SetHovered(*twitterBrush);
  twitterButton->SetNormal(*twitterBrush);
  twitterButton->SetPressed(*twitterBrush);

  googlePlusButton = new FButtonStyle();
  googlePlusButton->SetDisabled(*googlePlusBrush);
  googlePlusButton->SetHovered(*googlePlusBrush);
  googlePlusButton->SetNormal(*googlePlusBrush);
  googlePlusButton->SetPressed(*googlePlusBrush);

  facebookButton = new FButtonStyle();
  facebookButton->SetDisabled(*facebookBrush);
  facebookButton->SetHovered(*facebookBrush);
  facebookButton->SetNormal(*facebookBrush);
  facebookButton->SetPressed(*facebookBrush);

  discordButton = new FButtonStyle();
  discordButton->SetDisabled(*discordBrush);
  discordButton->SetHovered(*discordBrush);
  discordButton->SetNormal(*discordBrush);
  discordButton->SetPressed(*discordBrush);
}

void FWorldCreatorSyncModule::ShutdownModule()
{
  // This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
  // we call this function before unloading the module.
  FWorldCreatorSyncStyle::Shutdown();

  FWorldCreatorSyncCommands::Unregister();

  FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(WorldCreatorSyncTabName);

  // Delete Brushes
  /////////////////
  delete youtubeButton;
  delete twitterButton;
  delete googlePlusButton;
  delete facebookButton;
  delete discordButton;
}

TSharedRef<SDockTab> FWorldCreatorSyncModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
  //FText WidgetText = FText::Format(
  //	LOCTEXT("WindowWidgetText", "Add code to {0} in {1} to override this window's contents"),
  //	FText::FromString(TEXT("FWorldCreatorSyncModule::OnSpawnPluginTab")),
  //	FText::FromString(TEXT("WorldCreatorSync.cpp"))
  //	);  


  //FModuleManager::GetModuleChecked<FWorldCreatorSyncModule>(FName("WorldCreatorSync")).GetSlateGameResources();

  auto wcBrush = FWorldCreatorSyncStyle::Get().GetBrush("WorldCreatorSync.WorldCreator");

  this->meshReference = StaticLoadObject(UStaticMesh::StaticClass(), NULL, TEXT("/WorldCreatorSync/SyncTool/Shape_Cone.Shape_Cone"));

  return SNew(SDockTab)
    .TabRole(ETabRole::NomadTab)
    [
      SNew(SVerticalBox)
      + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
    [
      SNew(SImage).Image(wcBrush)
    ]
  + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
    [
      SNew(SHorizontalBox)
      + SHorizontalBox::Slot().AutoWidth()[SNew(SButton).ButtonStyle(youtubeButton).OnClicked(
        FOnClicked::CreateLambda([]
  {
    system("start https://www.youtube.com/channel/UClabqa6PHVjXzR2Y7s1MP0Q/feed");
    return FReply::Handled();
  }))]

    + SHorizontalBox::Slot().AutoWidth()[SNew(SButton).ButtonStyle(twitterButton).OnClicked(
      FOnClicked::CreateLambda([]
  {
    system("start https://twitter.com/worldcreator3d");
    return FReply::Handled();
  }))]

    + SHorizontalBox::Slot().AutoWidth()[SNew(SButton).ButtonStyle(googlePlusButton).OnClicked(
      FOnClicked::CreateLambda([]
  {
    system("start https://plus.google.com/u/0/106848544975612145621");
    return FReply::Handled();
  }))]

    + SHorizontalBox::Slot().AutoWidth()[SNew(SButton).ButtonStyle(facebookButton).OnClicked(
      FOnClicked::CreateLambda([]
  {
    system("start https://www.facebook.com/worldcreator3d/");
    return FReply::Handled();
  }))]

    + SHorizontalBox::Slot().AutoWidth()[SNew(SButton).ButtonStyle(discordButton).OnClicked(
      FOnClicked::CreateLambda([]
  {
    system("start https://discordapp.com/invite/bjMteus");
    return FReply::Handled();
  }))]
    ]

  //+ SVerticalBox::Slot().HAlign(HAlign_Center)
  //  [
  //    SNew(SContentReference)
  //    .OnSetReference(SContentReference::FOnSetReference::CreateRaw(this, &FWorldCreatorSyncModule::OnSetReference))
  //    .AllowSelectingNewAsset(true)
  //    .AllowClearingReference(true)      
  //    //.AllowedClass(UStaticMesh::StaticClass())
  //    .AssetReference(meshReference)
  //  ]

  +SVerticalBox::Slot().HAlign(HAlign_Center)
    [
      SNew(SHorizontalBox)
      + SHorizontalBox::Slot().AutoWidth().VAlign(EVerticalAlignment::VAlign_Center)
    [
      SNew(STextBlock).Text(FText::FromString("Select SYNC File:"))
    ]
  + SHorizontalBox::Slot().AutoWidth().VAlign(EVerticalAlignment::VAlign_Center).Padding(10, 0, 0, 0)
    [
      SNew(SButton)
      .Text(FText::FromString("Browse...")).OnClicked(
        FOnClicked::CreateRaw(this, &FWorldCreatorSyncModule::BrowseButtonClicked))
    ]
  + SHorizontalBox::Slot().AutoWidth().VAlign(EVerticalAlignment::VAlign_Center).Padding(10, 0, 0, 0)
    [
      SNew(SButton)
      .Text(FText::FromString("Synchronize")).OnClicked(FOnClicked::CreateRaw(
        this, &FWorldCreatorSyncModule::SyncButtonClicked))

    ]
    ]
    ];
}

FReply FWorldCreatorSyncModule::BrowseButtonClicked()
{
  // Get Window Handle
  ////////////////////

  //TSharedPtr<SWindow> ParentWindow = FSlateApplication::GetGlobalTabManager()->GetActiveTab()->GetParentWindow();
  //const void* ParentWindowHandle = (ParentWindow.IsValid() && ParentWindow->GetNativeWindow().IsValid())
  //  ? ParentWindow->GetNativeWindow()->GetOSWindowHandle()
  //  : nullptr;


  TArray<FString> fileNames;
  if (FDesktopPlatformModule::Get()->OpenFileDialog(nullptr,
    TEXT("Select Sync File"), TEXT(""), TEXT(""), TEXT("World Creator Sync File | *.xml"), 0, fileNames))
  {
    this->selectedPath = fileNames[0];
  }
  return FReply::Handled();
}

FReply FWorldCreatorSyncModule::SyncButtonClicked()
{
  if (selectedPath.Len() <= 0)
    return FReply::Handled();

  int quadsPerSection = 63;
  const float scaleFactor = 100;
  const float terrainBaseScale = 512.0f;
  auto context = GEditor->GetEditorWorldContext();
  auto world = context.World();
  auto level = world->GetCurrentLevel();
  UMaterial* material = Cast<UMaterial>(StaticLoadObject(UMaterial::StaticClass(), NULL, TEXT("/WorldCreatorSync/SyncTool/TexturedLandscape.TexturedLandscape")));

  // Create Landscape
  ///////////////////
  auto landscapeClass = ALandscape::StaticClass();
  auto gizmoClass = ALandscapeGizmoActiveActor::StaticClass();
  ALandscape* landscapeActor = nullptr;
  ALandscapeGizmoActiveActor* landscapeGizmo = nullptr;

  // TODO: World Composition at void WorldTileCollectionModel.cpp::FWorldTileCollectionModel::ImportTiledLandscape_Executed();


  // Find existing landscape
  //////////////////////////
  auto actorIter = FActorIterator(world);
  while (actorIter)
  {
    if (actorIter->GetClass() == landscapeClass && actorIter->GetActorLabel() == TEXT("WC_Landscape"))
    {
      landscapeActor = Cast<ALandscape>(*actorIter);
      break;
    }
    ++actorIter;
  }

  // Find existing gizmo
  //////////////////////
  if (landscapeActor != nullptr)
  {
    auto actorIter = FActorIterator(world);
    while (actorIter)
    {
      if (actorIter->GetClass() == gizmoClass)
      {
        auto gizmo = Cast<ALandscapeGizmoActiveActor>(*actorIter);
        if (gizmo->TargetLandscapeInfo == landscapeActor->GetLandscapeInfo())
        {
          landscapeGizmo = gizmo;
          break;
        }
      }
      ++actorIter;
    }
  }



  TArray<FLandscapeImportLayerInfo> layerInfos;

  // Load Heightmap
  /////////////////
  auto syncDir = FPaths::GetPath(selectedPath);
  auto heightmapPath = FString::Printf(TEXT("%s/heightmap.raw"), syncDir.GetCharArray().GetData());
  TArray<uint8> file;
  FFileHelper::LoadFileToArray(file, heightmapPath.GetCharArray().GetData());

  // Load Config File
  ///////////////////
  FXmlFile configFile(selectedPath);
  auto root = configFile.GetRootNode();
  auto node = root->FindChildNode(TEXT("Surface"));
  if (node == nullptr)
  {
    UE_LOG(LogTemp, Error, TEXT("Could not load worldcreator sync config file"));
    configFile.Clear();
    return FReply::Handled();
  }
  float heightCenter = XmlHelper::GetFloat(node, "HeightCenter");
  float minHeightScale = XmlHelper::GetFloat(node, "MinHeightScale");
  float maxHeightScale = XmlHelper::GetFloat(node, "MaxHeightScale");
  float minHeight = XmlHelper::GetFloat(node, "MinHeight");
  float maxHeight = XmlHelper::GetFloat(node, "MaxHeight");
  int width = XmlHelper::GetInt(node, "Width");
  int length = XmlHelper::GetInt(node, "Length");
  int height = XmlHelper::GetInt(node, "Height");
  int resX = XmlHelper::GetInt(node, "ResolutionX");
  int resY = XmlHelper::GetInt(node, "ResolutionY");
  float heightScale = maxHeight - minHeight;
  float terrainScale = ((heightScale * height) / terrainBaseScale) * scaleFactor;
  float terrainOffset = heightCenter * scaleFactor;

  int ueResX = (resX / quadsPerSection) * quadsPerSection;
  int ueResY = (resY / quadsPerSection) * quadsPerSection;
  float scaleX = ((float)width / (resX - 1)) * scaleFactor;
  float scaleY = ((float)length / (resY - 1)) * scaleFactor;
  float objScaleX = ((float)width / (resX)) * scaleFactor;
  float objScaleY = ((float)length / (resY)) * scaleFactor;

  uint16* originalData = (uint16*)file.GetData();
  uint16* heightData = (uint16*)FMemory::Malloc(resX * resY * sizeof(uint16));
  for (int y = 0; y < resY; y++)
  {
    for (int x = 0; x < resX; x++)
    {
      heightData[y * resX + (resX - 1) - x] = originalData[y * resX + x]; //x < resX / 2 ? 0 : 0xFFFF;
    }
  }


  float ueScaleX = (float)resX / width;
  float ueScaleY = (float)resY / length;
  auto getHeightInterpolated = [heightData, resX, resY, ueScaleX, ueScaleY, heightScale, width](float x, float y)
  {
    float realX = (width - x) * ueScaleX;
    float realY = y * ueScaleY;

    realX = realX < 0 ? 0 : realX >= resX ? resX - 1 : realX;
    realY = realY < 0 ? 0 : realY >= resY ? resY - 1 : realY;

    int idX = (int)floor(realX);
    int idY = (int)floor(realY);
    float frX = realX - idX;
    float frY = realY - idY;

    int maxX = idX + 1;
    maxX = maxX >= resX ? resX - 1 : maxX;
    int maxY = idY + 1;
    maxY = maxY >= resY ? resY - 1 : maxY;

    float lt = ((float)heightData[idY * resX + idX] / 0xFFFF);
    float rt = ((float)heightData[idY * resX + maxX] / 0xFFFF);
    float lb = ((float)heightData[maxY * resX + idX] / 0xFFFF);
    float rb = ((float)heightData[maxY * resX + maxX] / 0xFFFF);

    const auto lerp = [](float v0, float v1, float f) {return v0 + f * (v1 - v0); };
    float value = lerp(lerp(lt, rt, frX), lerp(lb, rb, frX), frY);
    return value;
  };

  // Load Splatmap
  ////////////////
  auto texturingNode = root->FindChildNode(TEXT("Texturing"));
  if (texturingNode != nullptr)
  {
    //Load From File
    auto childNodes = texturingNode->GetChildrenNodes();
    int textureCount = 0;
    for (auto child : childNodes)
    {
      auto splatmapName = child->GetAttribute(TEXT("Name"));
      auto textures = child->GetChildrenNodes();
      auto filePath = FString::Printf(TEXT("%s/%s"), syncDir.GetCharArray().GetData(), splatmapName.GetCharArray().GetData());


      int fileWidth, fileHeight, fileBpp;
      TArray<uint8> fileData;
      //LoadTGA(filePath.GetCharArray().GetData(), fileData, fileWidth, fileHeight, fileBpp);

      FFileHelper::LoadFileToArray(fileData, filePath.GetCharArray().GetData());
      uint8* dataPtr = (fileData).GetData();
      fileWidth = *(short*)(&dataPtr[12]);
      fileHeight = *(short*)(&dataPtr[14]);
      fileBpp = (*(uint8*)(&dataPtr[16])) / 8;
      (fileData).RemoveAt(0, 18);

      for (int i = 0; i < textures.Num(); i++)
      {
        TArray<uint8> layerData;
        for (int y = 0; y < fileHeight; y++)
        {
          for (int x = 0; x < fileWidth; x++)
          {
            switch (i)
            {
            case 0:
              layerData.Add((fileData)[(y * fileWidth + x) * fileBpp + 2]);
              break;
            case 1:
              layerData.Add((fileData)[(y * fileWidth + x) * fileBpp + 1]);
              break;
            case 2:
              layerData.Add((fileData)[(y * fileWidth + x) * fileBpp]);
              break;
            case 3:
              layerData.Add((fileData)[(y * fileWidth + x) * fileBpp + 3]);
              break;
            }
          }
        }


        FLandscapeImportLayerInfo info;
        info.LayerData = layerData;
        info.LayerName = FString::Printf(TEXT("Texture%d"), textureCount).GetCharArray().GetData();//FName(FString::Printf(TEXT("%s"), textures[i]->GetAttribute("Name").GetCharArray().GetData()).GetCharArray().GetData());

        info.LayerInfo = NewObject<ULandscapeLayerInfoObject>();
        info.LayerInfo->bNoWeightBlend = 0;
        info.LayerInfo->Hardness = 1;
        info.LayerInfo->IsReferencedFromLoadedData = false;

        info.LayerInfo->LayerName = info.LayerName;
        layerInfos.Add(info);

        auto textureNode = textures[i];
        auto colorAtt = textureNode->GetAttribute(TEXT("Color"));
        FLinearColor color = FLinearColor(1, 1, 1, 1);
        if (colorAtt.StartsWith(TEXT("RGBA(")))
        {
          colorAtt.RemoveFromStart(TEXT("RGBA("));
          colorAtt.RemoveFromEnd(TEXT(")"));
          TArray<FString> array;
          colorAtt.ParseIntoArray(array, TEXT(","));
          color.R = FCString::Atof(array[0].GetCharArray().GetData());
          color.G = FCString::Atof(array[1].GetCharArray().GetData());
          color.B = FCString::Atof(array[2].GetCharArray().GetData());
        }
        material->SetVectorParameterValueEditorOnly(FString::Printf(TEXT("Color%d"), textureCount).GetCharArray().GetData(), color);

        textureCount++;
      }
    }
  }

  // Load Details
  ///////////////
  auto detailNode = root->FindChildNode(TEXT("Details"));
  if (detailNode != nullptr)
  {
    int detailCount = 0;
    TArray<uint8> RawFileData;
    auto layers = detailNode->FindChildNode("Layers");
    for (auto layer : layers->GetChildrenNodes())
    {
      for (auto detail : layer->GetChildrenNodes())
      {
        float heightCenter = XmlHelper::GetFloat(detail, "HeightCenter");
        float density = XmlHelper::GetFloat(detail, "Density");
        FString name = XmlHelper::GetString(detail, "Name");
        FString tag = XmlHelper::GetString(detail, "Tag");
        FString fileName = XmlHelper::GetString(detail, "FileName");
        auto filePath = FString::Printf(TEXT("%s/%s"), syncDir.GetCharArray().GetData(), fileName.GetCharArray().GetData());

        // Load Splatmap File
        ////////////////////
        IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
        TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);
        FFileHelper::LoadFileToArray(RawFileData, filePath.GetCharArray().GetData());
        if (ImageWrapper.IsValid() && ImageWrapper->SetCompressed(RawFileData.GetData(), RawFileData.Num()))
        {
          const TArray<uint8>* UncompressedBGRA = NULL;
          if (ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, UncompressedBGRA))
          {
            int imgWidth = ImageWrapper->GetWidth();
            int imgHeight = ImageWrapper->GetHeight();

            TArray<uint8> layerData;
            for (int y = 0; y < imgHeight; y++)
            {
              for (int x = 0; x < imgWidth; x++)
              {
                layerData.Add((*UncompressedBGRA)[(y * imgWidth + x) * 4 + 2]);
              }
            }

            FLandscapeImportLayerInfo info;
            info.LayerData = layerData;
            info.LayerName = FString::Printf(TEXT("Detail%d"), detailCount).GetCharArray().GetData();

            info.LayerInfo = NewObject<ULandscapeLayerInfoObject>();
            info.LayerInfo->bNoWeightBlend = 1;
            info.LayerInfo->Hardness = 1;
            info.LayerInfo->IsReferencedFromLoadedData = false;

            info.LayerInfo->LayerName = info.LayerName;
            layerInfos.Add(info);
            detailCount++;
          }
        }
      }
    }
  }


  if (landscapeActor != nullptr)
  {
    if (landscapeGizmo != nullptr)
      landscapeGizmo->Destroy();

    auto name = landscapeActor->GetName();
    auto location = landscapeActor->GetActorLocation();
    auto rotation = landscapeActor->GetActorRotation();
    auto material = landscapeActor->GetLandscapeMaterial();
    location.Z = terrainOffset;

    landscapeActor->Destroy();

    FActorSpawnParameters spawnParams;
    spawnParams.Name = name.GetCharArray().GetData();
    landscapeActor = world->SpawnActor<ALandscape>(location, rotation);
    landscapeActor->SetLandscapeGuid(FGuid::NewGuid());
    landscapeActor->LandscapeMaterial = material;


    landscapeActor->Import(FGuid::NewGuid(), 0, 0, resX - 1, resY - 1, 1, quadsPerSection,
      heightData, L"", layerInfos, ELandscapeImportAlphamapType::Additive);
    auto info = landscapeActor->CreateLandscapeInfo();
    landscapeGizmo = world->SpawnActor<ALandscapeGizmoActiveActor>(location, rotation);
    landscapeGizmo->SetTargetLandscape(info);
    info->UpdateLayerInfoMap(landscapeActor, true);
    landscapeActor->SetActorScale3D(FVector(scaleX, scaleY, terrainScale));
    landscapeActor->SetActorLabel(TEXT("WC_Landscape"), false);
  }
  else
  {
    FVector location(0, 0, terrainOffset);
    FRotator rotation(0, 0, 0);
    landscapeActor = world->SpawnActor<ALandscape>(location, rotation);

    if (material != nullptr)
      landscapeActor->LandscapeMaterial = (UMaterialInterface*)material;

    landscapeActor->SetLandscapeGuid(FGuid::NewGuid());
    landscapeActor->Import(FGuid::NewGuid(), 0, 0, resX - 1, resY - 1, 1, quadsPerSection,
      heightData, L"", layerInfos, ELandscapeImportAlphamapType::Additive);
    landscapeActor->SetActorScale3D(FVector(scaleX, scaleY, terrainScale));
    landscapeActor->SetActorLabel(TEXT("WC_Landscape"), false);
  }


  // Load Objects
  ///////////////
  auto objectNode = root->FindChildNode(TEXT("Objects"));
  if (objectNode != nullptr)
  {
    TActorIterator<AInstancedFoliageActor> foliageIterator(world);
    AInstancedFoliageActor* foliageActor;
    if (!foliageIterator)
      foliageActor = world->SpawnActor<AInstancedFoliageActor>();
    else
      foliageActor = *foliageIterator;

    TArray<UInstancedStaticMeshComponent*> components;
    foliageActor->GetComponents<UInstancedStaticMeshComponent>(components);
    foliageActor->CleanupDeletedFoliageType();

    // Collect all existing WC foliage types (starting with "WC_")
    /////////////////////////////////////////////////////////////
    TMap<FString, TPair<UFoliageType*, FFoliageMeshInfo_Deprecated2*>> wcComponents;
    for (auto& c : foliageActor->FoliageMeshes)
    {
      if (!c.Key->IsA<UFoliageType_InstancedStaticMesh>())
        continue;

      auto m = (UFoliageType_InstancedStaticMesh*)c.Key;
      auto mesh = m->GetStaticMesh();
      if (mesh == nullptr)
        continue;
      auto name = mesh->GetName();

      if (name.StartsWith(TEXT("WC_")))
        wcComponents.Add(name, TPair<UFoliageType*, FFoliageMeshInfo_Deprecated2*>(c.Key, &c.Value.Get()));
    }

    auto layersNode = objectNode->FindChildNode(TEXT("Layers"));
    int objectIndex = 0;
    if (layersNode != nullptr)
    {
      for (auto layer : layersNode->GetChildrenNodes())
      {
        for (auto object : layer->GetChildrenNodes())
        {
          FString objectName = XmlHelper::GetString(object, "Name");
          FString objectTag = XmlHelper::GetString(object, "Tag");
          float objectHeightOffset = XmlHelper::GetFloat(object, "HeightOffset");

          FString meshName = FString::Printf(TEXT("WC_%s_%d"), objectName.GetCharArray().GetData(), objectIndex);


          FFoliageMeshInfo* meshInfo = nullptr;
          TPair<UFoliageType*, FFoliageMeshInfo_Deprecated2*>* foundComponent = wcComponents.Find(meshName);


          UFoliageType_InstancedStaticMesh* foliageType;
          if (foundComponent == nullptr)
          {
            // Create new Foliage Type
            //////////////////////////
            foliageType = NewObject<UFoliageType_InstancedStaticMesh>(
              (UObject*)GetTransientPackage(), UFoliageType_InstancedStaticMesh::StaticClass(),
              FName(TEXT("Foliage")), RF_Transactional);

            // Load Basic Mesh, valid mesh is somehow needed to remove instance data later on
            // TODO: Include mesh in plugin, dont load from Project content
#ifdef PACKAGE
            UStaticMesh* mesh = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(),
              NULL, TEXT("/WorldCreatorSync/SyncTool/Shape_Cone.Shape_Cone")));
#else
            UStaticMesh* mesh = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(),
              NULL, TEXT("/Game/SyncTool/Shape_Cone.Shape_Cone")));
#endif
            // IMPORTANT: Use game as root directory instead of WorldCreatorSync for testing

            if (mesh == nullptr)
              continue;

            // Create a new model from the loaded data because
            // using the same model instance would cause the foliage data to merge
            ///////////////////////////////////////////////////////////////////
            FStaticMeshSourceModel* sourceModel = &mesh->SourceModels[0];
            FRawMesh rawMesh;
            sourceModel->LoadRawMesh(rawMesh);


            auto modelName = FString::Printf(TEXT("%s_Mesh"), meshName.GetCharArray().GetData());
            UStaticMesh* newModel = NewObject<UStaticMesh>(foliageType, FName(modelName.GetCharArray().GetData()));
            newModel->AddSourceModel();
            newModel->SourceModels[0].SaveRawMesh(rawMesh);
            TArray<FText> buildErrors;
            newModel->Build(true, &buildErrors);



            // Set the new mesh and add the created foliage type
            ////////////////////////////////////////////////////
            foliageType->SetStaticMesh(newModel);
            auto newType = foliageActor->AddFoliageType(foliageType, &meshInfo);
          }
          else
          {
            // Remove all foliage instances on the existing foliage type
            ////////////////////////////////////////////////////////////
            wcComponents.Remove(meshName);
            foliageType = Cast<UFoliageType_InstancedStaticMesh>(foundComponent->Key);
            meshInfo = foundComponent->Value;
            meshInfo->SelectInstances(foliageActor, true);
            auto selected = meshInfo->SelectedIndices.Array();
            meshInfo->RemoveInstances(foliageActor, selected, true);
          }

          // Remove all unused WC foliage types
          /////////////////////////////////////
          for (auto c : wcComponents)
          {
            foliageActor->RemoveFoliageType(&c.Value.Key, 1);
          }

          // Load Foliage data from the sync file
          ///////////////////////////////////////
          auto dataNode = object->FindChildNode(TEXT("Data"));
          int dataCount = XmlHelper::GetInt(dataNode, "DataCount");
          TArray<uint8> data;
          FBase64::Decode(dataNode->GetContent(), data);

          ObjectProperties* propertyArray = (ObjectProperties*)data.GetData();
          // Add all instances to the foliage type
          ////////////////////////////////////////
          for (int i = 0; i < dataCount; i++)
          {
            auto& properties = propertyArray[i];
            properties.posY = getHeightInterpolated(properties.posX, properties.posZ) + properties.posY;
            properties.posX = (1.0f - properties.posX / width);
            properties.posZ = (properties.posZ / length);
            FFoliageInstance instance;

            instance.Location = FVector(properties.posX * width * scaleFactor, properties.posZ * length * scaleFactor, (properties.posY * heightScale + minHeight) * height * scaleFactor);
            auto euler = FQuat(properties.rotX, properties.rotY, properties.rotZ, properties.rotW).Euler();
            euler = FVector(-euler.X, -euler.Z, -euler.Y);
            instance.Rotation = FRotator::MakeFromEuler(euler);
            instance.DrawScale3D = FVector(properties.scaleX, properties.scaleX, properties.scaleY);
            meshInfo->AddInstance(foliageActor, foliageType, instance, true);
          }

          meshInfo->UpdateComponentSettings(foliageType);
          objectIndex++;
        }
      }
    }
  }


  // Cleanup memory  
  ////////////////
  configFile.Clear();
  FMemory::Free(heightData);

  return FReply::Handled();
}

const bool FWorldCreatorSyncModule::GetIsButtonEnabled()
{
  return !this->selectedPath.IsEmpty();
}

void FWorldCreatorSyncModule::PluginButtonClicked()
{
  FGlobalTabmanager::Get()->InvokeTab(WorldCreatorSyncTabName);
}

void FWorldCreatorSyncModule::AddMenuExtension(FMenuBuilder& Builder)
{
  Builder.AddMenuEntry(FWorldCreatorSyncCommands::Get().OpenPluginWindow);
}

void FWorldCreatorSyncModule::AddToolbarExtension(FToolBarBuilder& Builder)
{
  Builder.AddToolBarButton(FWorldCreatorSyncCommands::Get().OpenPluginWindow);
}

void FWorldCreatorSyncModule::LoadTGA(const TCHAR* path, TArray<uint8>& data, int& width, int& height, int& bpp)
{
  FFileHelper::LoadFileToArray(data, path);
  auto dataPtr = (data).GetData();
  width = *(short*)(dataPtr[12]);
  height = *(short*)(dataPtr[14]);
  (data).RemoveAt(0, 16);
}

void FWorldCreatorSyncModule::OnSetReference(UObject* drop)
{
  this->meshReference = drop;
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FWorldCreatorSyncModule, WorldCreatorSync)