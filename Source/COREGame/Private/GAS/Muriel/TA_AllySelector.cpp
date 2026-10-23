// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Muriel/TA_AllySelector.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GAS/CAbilitySystemStatics.h"
#include "GenericTeamAgentInterface.h"
#include "Kismet/GameplayStatics.h"

ATA_AllySelector::ATA_AllySelector()
{
	PrimaryActorTick.bCanEverTick = true;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>("Root Comp"));

	IndicatorMesh = CreateDefaultSubobject<UStaticMeshComponent>("Indicator Mesh");
	IndicatorMesh->SetupAttachment(GetRootComponent());
	IndicatorMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	IndicatorMesh->SetCastShadow(false);
	IndicatorMesh->SetVisibility(false);
}

void ATA_AllySelector::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if ( !PrimaryPC || !PrimaryPC->IsLocalPlayerController() )
		return;

	AActor* NewSelection = FindNearestAllyToScreenCenter();
	if ( NewSelection != CurrentSelectedAlly )
	{
		CurrentSelectedAlly = NewSelection;

		if ( CurrentSelectedAlly )
		{
			IndicatorMesh->SetVisibility(true);
		}
		else
		{
			IndicatorMesh->SetVisibility(false);
		}
	}

	if ( CurrentSelectedAlly )
	{
		FVector FeetLocation = CurrentSelectedAlly->GetActorLocation();

		if ( ACharacter* AllyCharcter = Cast<ACharacter>(CurrentSelectedAlly) )
		{
			if ( UCapsuleComponent* Capsule = AllyCharcter->GetCapsuleComponent() )
			{
				FeetLocation.Z -= Capsule->GetScaledCapsuleHalfHeight();
			}
		}

		SetActorLocation(FeetLocation + FVector(0.f, 0.f, 5.f));
	}
}

void ATA_AllySelector::ConfirmTargetingAndContinue()
{
	if ( !CurrentSelectedAlly || UCAbilitySystemStatics::IsActorDead(CurrentSelectedAlly) )
	{
		HandleInvalidConfirm();
		return;
	}

	const AActor* OwnerAvatarActor = OwningAbility ? OwningAbility->GetCurrentActorInfo()->AvatarActor.Get() : nullptr;
	if ( !IsInSight(OwnerAvatarActor, CurrentSelectedAlly) )
	{
		HandleInvalidConfirm();
		return;
	}

	FGameplayAbilityTargetData_SingleTargetHit* HitData = new FGameplayAbilityTargetData_SingleTargetHit;
	HitData->HitResult.HitObjectHandle = FActorInstanceHandle(CurrentSelectedAlly);

	FGameplayAbilityTargetDataHandle TargetDataHandle;
	TargetDataHandle.Add(HitData);

	TargetDataReadyDelegate.Broadcast(TargetDataHandle);
}

void ATA_AllySelector::SetSelectionRules(float InMaxTargetRange, bool bInAllowSelfTarget)
{
	MaxTargetRange = InMaxTargetRange;
	bAllowSelfTarget = bInAllowSelfTarget;
}

AActor* ATA_AllySelector::FindNearestAllyToScreenCenter() const
{
	if ( !PrimaryPC || !OwningAbility )
		return nullptr;

	AActor* OwnerAvatarActor = OwningAbility->GetCurrentActorInfo()->AvatarActor.Get();
	IGenericTeamAgentInterface* OwnerTeamInterface = Cast<IGenericTeamAgentInterface>(OwnerAvatarActor);
	if ( !OwnerTeamInterface )
		return nullptr;

	int32 ViewportX, ViewportY;
	PrimaryPC->GetViewportSize(ViewportX, ViewportY);
	FVector2D ScreenCenter(ViewportX * 0.5f, ViewportY * 0.5f);

	AActor* BestCandidate = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();

	TArray<AActor*> AllHeroes;
	UGameplayStatics::GetAllActorsWithInterface(GetWorld(), UGenericTeamAgentInterface::StaticClass(), AllHeroes);
	
	for ( AActor* Candidate : AllHeroes )
	{
		if ( !Candidate || Candidate == OwnerAvatarActor || !UCAbilitySystemStatics::IsHero(Candidate) )
			continue;

		if ( UCAbilitySystemStatics::IsActorDead(Candidate) )
			continue;

		if ( MaxTargetRange > 0.f && FVector::DistSquared(OwnerAvatarActor->GetActorLocation(), Candidate->GetActorLocation()) > FMath::Square(MaxTargetRange) )
			continue;

		IGenericTeamAgentInterface* CandidateTeamInterface = Cast<IGenericTeamAgentInterface>(Candidate);
		if ( !CandidateTeamInterface || OwnerTeamInterface->GetTeamAttitudeTowards(*Candidate) == ETeamAttitude::Hostile )
			continue;

		FVector2D ScreenPos;
		bool bProjected = UGameplayStatics::ProjectWorldToScreen(PrimaryPC, Candidate->GetActorLocation(), ScreenPos);
		if ( !bProjected )
			continue;

		float DistSq = FVector2D::DistSquared(ScreenPos, ScreenCenter);
		if ( DistSq < BestDistSq && IsInSight(OwnerAvatarActor, Candidate) )
		{
			BestDistSq = DistSq;
			BestCandidate = Candidate;
		}
	}

	if ( !BestCandidate && bAllowSelfTarget )
		return OwnerAvatarActor;

	return BestCandidate;
}

bool ATA_AllySelector::IsInSight(const AActor* Viewer, const AActor* Target) const
{
	if ( !bRequireBeInSight || !Viewer || !Target || Viewer == Target )
		return true;

	UWorld* World = GetWorld();
	if ( !World )
		return true;

	const FVector StartLoc = PrimaryPC && PrimaryPC->PlayerCameraManager ? PrimaryPC->PlayerCameraManager->GetCameraLocation() : Viewer->GetActorLocation() + FVector(0.f, 0.f, 100.f);

	TArray<FVector, TInlineAllocator<2>> TargetPoints;
	TargetPoints.Add(Target->GetActorLocation());

	if ( const ACharacter* TargetCharacter = Cast<ACharacter>(Target))
	{
		if ( const UCapsuleComponent* Capsule = TargetCharacter->GetCapsuleComponent() )
		{
			TargetPoints.Add(Target->GetActorLocation() + FVector(0.f, 0.f, Capsule->GetScaledCapsuleHalfHeight() * 0.8f));
		}
	}

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(SightBlockingType);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AllySelectorLineOfSight), false, Viewer);
	QueryParams.AddIgnoredActor(Target);

	for ( const FVector& Point : TargetPoints )
	{
		if ( !World->LineTraceTestByObjectType(StartLoc, Point, ObjectParams, QueryParams) )
		{
			return true;
		}
	}

	return false;
}

void ATA_AllySelector::HandleInvalidConfirm()
{
	if ( bCancelOnInvalidConfirm )
	{
		CanceledDelegate.Broadcast(FGameplayAbilityTargetDataHandle());
		
		return;
	}

	const FGameplayAbilityActorInfo* ActorInfo = OwningAbility ? OwningAbility->GetCurrentActorInfo() : nullptr;
	UAbilitySystemComponent* OwnerASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;

	if ( OwnerASC && ActorInfo->IsLocallyControlled() )
	{
		OwnerASC->GenericLocalConfirmCallbacks.AddDynamic(this, &AGameplayAbilityTargetActor::ConfirmTargeting);
	}
}
