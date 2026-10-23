// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/CharacterDisplay.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Character/PA_CharacterDefinition.h"
#include "Components/SkeletalMeshComponent.h"
#include "Framework/CAssetManager.h"

// Sets default values
ACharacterDisplay::ACharacterDisplay()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;
	bNetLoadOnClient = false;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>("Root Component"));

	MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>("Mesh Component");
	MeshComponent->SetupAttachment(GetRootComponent());

	ViewCameraComponent = CreateDefaultSubobject<UCameraComponent>("View Camera Component");
	ViewCameraComponent->SetupAttachment(GetRootComponent());
}

void ACharacterDisplay::ConfigureWithCharacterDefinition(const UPA_CharacterDefinition* CharacterDefinition)
{
	PendingDefinition = CharacterDefinition;

	if ( !CharacterDefinition )
		return;

	if ( CharacterDefinition->IsDisplayAssetsLoaded() )
	{
		ApplyCharacterDefinition(CharacterDefinition);
		return;
	}

	TWeakObjectPtr<ACharacterDisplay> WeakThis(this);
	TWeakObjectPtr<const UPA_CharacterDefinition> WeakDefinition(CharacterDefinition);

	UCAssetManager::Get().RequestCharacterDisplayAssets(CharacterDefinition, FStreamableDelegate::CreateLambda([ WeakThis, WeakDefinition ] ()
		{
			if ( WeakThis.IsValid() && WeakDefinition.IsValid() && WeakThis->PendingDefinition == WeakDefinition.Get() )
			{
				WeakThis->ApplyCharacterDefinition(WeakDefinition.Get());
			}
		}));
}

void ACharacterDisplay::ApplyCharacterDefinition(const UPA_CharacterDefinition* CharacterDefinition)
{
	MeshComponent->SetSkeletalMesh(CharacterDefinition->GetDisplayMeshSoftPtr().Get());
	MeshComponent->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	MeshComponent->SetAnimInstanceClass(CharacterDefinition->GetDisplayAnimBPSoftPtr().Get());
}
