// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Muriel/GA_Muriel_ReversalOfFortune.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitTargetData.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Curves/CurveFloat.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/RootMotionSource.h"
#include "GAS/CAbilitySystemComponent.h"
#include "GAS/Muriel/TA_AllySelector.h"
#include "GAS/CAbilitySystemStatics.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GenericTeamAgentInterface.h"
#include "MotionWarpingComponent.h"
#include "Player/CPlayerCharacter.h"
#include "DrawDebugHelpers.h"

UGA_Muriel_ReversalOfFortune::UGA_Muriel_ReversalOfFortune()
{

}

void UGA_Muriel_ReversalOfFortune::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if ( !HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo) || !TargetingMontage || !CastMontage )
	{
		K2_EndAbility();
		return;
	}

	if ( !K2_CheckAbilityCost() || !K2_CheckAbilityCooldown() )
	{
		K2_EndAbility();
		return;
	}

	UAbilityTask_PlayMontageAndWait* PlayTargetingMontage = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, TargetingMontage);
	PlayTargetingMontage->ReadyForActivation();

	if ( ACPlayerCharacter* CPlayerCharacter = Cast<ACPlayerCharacter>(GetAvatarActorFromActorInfo()) )
	{
		CPlayerCharacter->SetMoveInputBlock(true);
	}

	if ( bUseTargetingReadyEvent )
	{
		UAbilityTask_WaitGameplayEvent* WaitReady = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, GetTargetingReadyTag());
		WaitReady->EventReceived.AddDynamic(this, &UGA_Muriel_ReversalOfFortune::OnTargetingReadyEvent);
		WaitReady->ReadyForActivation();
	}
	else
	{
		FTimerHandle TargetingDelayHandle;
		GetWorld()->GetTimerManager().SetTimer(TargetingDelayHandle, this, &UGA_Muriel_ReversalOfFortune::StartTargetingPhase, TargetingReadyDelay, false);
	}
}

void UGA_Muriel_ReversalOfFortune::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	StopAscent();
	StopCameraLock();

	if ( ACPlayerCharacter* CPlayerCharacter = Cast<ACPlayerCharacter>(GetAvatarActorFromActorInfo()) )
	{
		CPlayerCharacter->SetMoveInputBlock(false);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Muriel_ReversalOfFortune::OnTargetConfirmed(const FGameplayAbilityTargetDataHandle& TargetDataHandle)
{
	FHitResult TargetHit = UAbilitySystemBlueprintLibrary::GetHitResultFromTargetData(TargetDataHandle, 0);
	TargetAlly = TargetHit.GetActor();
	if ( !TargetAlly || UCAbilitySystemStatics::IsActorDead(TargetAlly) )
	{
		K2_EndAbility();
		return;
	}

	if ( !K2_CommitAbility() )
	{
		K2_EndAbility();
		return;
	}

	if ( K2_HasAuthority() )
	{
		UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo();
		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetAlly);

		if ( TargetASC )
		{
			int32 AbilityLevel = GetAbilityLevel(CurrentSpecHandle, CurrentActorInfo);
			const float ShieldAmount = GetScaledAmount(ShieldBaseAmountPerLevel, AbilityLevel, 0.f, ShieldIntelligenceRatio);

			UCAbilitySystemStatics::ApplyTimedShield(OwnerASC, TargetASC, ShieldEffect, ShieldBuffIconEffect, AbilityLevel, ShieldDuration, ShieldAmount);
		}
	}

	AActor* OwnerAvatarActor = GetAvatarActorFromActorInfo();
	if ( ACharacter* OwnerCharacter = Cast<ACharacter>(OwnerAvatarActor) )
	{
		OwnerCharacter->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
		OwnerCharacter->GetCharacterMovement()->StopMovementImmediately();
	}

	if ( ACPlayerCharacter* CPlayerCharacter = Cast<ACPlayerCharacter>(OwnerAvatarActor) )
	{
		CPlayerCharacter->SetMoveInputBlock(true);
	}

	if ( K2_HasAuthority() || IsLocallyControlled() )
	{
		UAbilityTask_PlayMontageAndWait* PlayCastMontage = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, CastMontage);
		PlayCastMontage->OnCompleted.AddDynamic(this, &UGA_Muriel_ReversalOfFortune::K2_EndAbility);
		PlayCastMontage->OnInterrupted.AddDynamic(this, &UGA_Muriel_ReversalOfFortune::K2_EndAbility);
		PlayCastMontage->OnCancelled.AddDynamic(this, &UGA_Muriel_ReversalOfFortune::K2_EndAbility);
		PlayCastMontage->ReadyForActivation();
	}

	StartAscent();

	CalculateFixedLandingData();

	if ( UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo() )
	{
		FGameplayCueParameters CueParams;
		CueParams.Location = FixedAirborneCenter;
		CueParams.Instigator = OwnerAvatarActor;

		OwnerASC->ExecuteGameplayCue(UCAbilitySystemStatics::GetLandingMarkerGameplayCueTag(), CueParams);
	}

	if ( K2_HasAuthority() || IsLocallyControlled() )
	{
		UAbilityTask_WaitGameplayEvent* WaitDescentStart = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, GetDescentStartTag());
		WaitDescentStart->EventReceived.AddDynamic(this, &UGA_Muriel_ReversalOfFortune::OnDescentStarted);
		WaitDescentStart->ReadyForActivation();
	}

	if ( K2_HasAuthority() || IsLocallyControlled() )
	{
		UAbilityTask_WaitGameplayEvent* WaitLand = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, GetLandTag());
		WaitLand->EventReceived.AddDynamic(this, &UGA_Muriel_ReversalOfFortune::OnLanded);
		WaitLand->ReadyForActivation();
	}
}

void UGA_Muriel_ReversalOfFortune::OnTargetCancelled(const FGameplayAbilityTargetDataHandle& TargetDataHandle)
{
	K2_EndAbility();
}

void UGA_Muriel_ReversalOfFortune::StartTargetingPhase()
{
	UAbilityTask_WaitTargetData* WaitTargetData = UAbilityTask_WaitTargetData::WaitTargetData(this, NAME_None, EGameplayTargetingConfirmation::UserConfirmed, SelectorClass);
	WaitTargetData->ValidData.AddDynamic(this, &UGA_Muriel_ReversalOfFortune::OnTargetConfirmed);
	WaitTargetData->Cancelled.AddDynamic(this, &UGA_Muriel_ReversalOfFortune::OnTargetCancelled);
	WaitTargetData->ReadyForActivation();

	AGameplayAbilityTargetActor* TargetActor;
	WaitTargetData->BeginSpawningActor(this, SelectorClass, TargetActor);
	WaitTargetData->FinishSpawningActor(this, TargetActor);
}

void UGA_Muriel_ReversalOfFortune::OnTargetingReadyEvent(FGameplayEventData PayLoad)
{
	if ( ACPlayerCharacter* CPlayerCharacter = Cast<ACPlayerCharacter>(GetAvatarActorFromActorInfo()) )
	{
		CPlayerCharacter->SetMoveInputBlock(false);
	}

	StartTargetingPhase();
}

FGameplayTag UGA_Muriel_ReversalOfFortune::GetTargetingReadyTag()
{
	return FGameplayTag::RequestGameplayTag("Ability.Muriel.Reversal.Ready");
}

void UGA_Muriel_ReversalOfFortune::CalculateFixedLandingData()
{
	AActor* OwnerAvatarActor = GetAvatarActorFromActorInfo();
	if ( !OwnerAvatarActor || !TargetAlly )
		return;

	FVector AllyFeetLocation = TargetAlly->GetActorLocation();
	if ( ACharacter* AllyCharacter = Cast<ACharacter>(TargetAlly) )
	{
		if ( UCapsuleComponent* AllyCapsule = AllyCharacter->GetCapsuleComponent() )
		{
			AllyFeetLocation.Z -= AllyCapsule->GetScaledCapsuleHalfHeight();
		}
	}

	FixedAirborneCenter = AllyFeetLocation;

	FVector DirectionFromAllyToStart = (AscentStartLocation - AllyFeetLocation).GetSafeNormal2D();
	if ( DirectionFromAllyToStart.IsNearlyZero() )
	{
		DirectionFromAllyToStart = OwnerAvatarActor->GetActorForwardVector().GetSafeNormal2D();
	}

	FixedLandingLocation = AllyFeetLocation + DirectionFromAllyToStart * LandingOffsetDistance;
	FixedLandingLocation.Z = AllyFeetLocation.Z;

	if ( ShouldDrawDebug() )
	{
		DrawDebugSphere(GetWorld(), FixedAirborneCenter, 30.f, 12, FColor::Yellow, false, 8.f, 0, 2.f);
		DrawDebugSphere(GetWorld(), FixedAirborneCenter, LaunchRadius, 24, FColor::Orange, false, 8.f, 0, 1.5f); 
		DrawDebugSphere(GetWorld(), FixedLandingLocation, 40.f, 12, FColor::Cyan, false, 8.f, 0, 2.f); 
		DrawDebugLine(GetWorld(), AscentStartLocation, AscentTargetLocation, FColor::Green, false, 8.f, 0, 1.5f);
		DrawDebugLine(GetWorld(), AscentTargetLocation, FixedLandingLocation, FColor::Blue, false, 8.f, 0, 1.5f);
	}
}

void UGA_Muriel_ReversalOfFortune::StartAscent()
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	UCharacterMovementComponent* MoveComp = OwnerCharacter ? OwnerCharacter->GetCharacterMovement() : nullptr;
	if ( !MoveComp )
		return;

	StopAscent();

	SetAnimRootMotionIgnored(true);

	AscentStartLocation = OwnerCharacter->GetActorLocation();
	AscentTargetLocation = AscentStartLocation + FVector(0.f, 0.f, AscentHeight);

	// 서버와 예측 클라이언트가 같은 루트 모션으로 이동 -> 프레임과 무관하게 부드럽고 네트워크 보정 없음
	TSharedPtr<FRootMotionSource_MoveToDynamicForce> AscentForce = MakeShared<FRootMotionSource_MoveToDynamicForce>();
	AscentForce->InstanceName = TEXT("ReversalAscent");
	AscentForce->AccumulateMode = ERootMotionAccumulateMode::Override;
	AscentForce->Priority = 500;
	AscentForce->StartLocation = AscentStartLocation;
	AscentForce->InitialTargetLocation = AscentTargetLocation;
	AscentForce->TargetLocation = AscentTargetLocation;
	AscentForce->Duration = FMath::Max(AscentDuration, 0.01f);
	AscentForce->bRestrictSpeedToExpected = false;
	AscentForce->TimeMappingCurve = AscentTimeCurve;
	AscentForce->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::SetVelocity;
	AscentForce->FinishVelocityParams.SetVelocity = FVector::ZeroVector;		

	AscentRootMotionSourceID = MoveComp->ApplyRootMotionSource(AscentForce);
}

void UGA_Muriel_ReversalOfFortune::StopAscent()
{
	SetAnimRootMotionIgnored(false);

	StartCameraLock();

	if ( AscentRootMotionSourceID == ( uint16 ) ERootMotionSourceID::Invalid )
		return;

	if ( ACharacter* OwnerCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo()) )
	{
		if ( UCharacterMovementComponent* MoveComp = OwnerCharacter->GetCharacterMovement() )
		{
			MoveComp->RemoveRootMotionSourceByID(AscentRootMotionSourceID);
		}
	}

	AscentRootMotionSourceID = ( uint16 ) ERootMotionSourceID::Invalid;
}

void UGA_Muriel_ReversalOfFortune::SetAnimRootMotionIgnored(bool bIgnore)
{
	if ( bAnimRootMotionIgnored == bIgnore )
		return;

	ACharacter* OwnerCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	UAnimInstance* AnimInstance = (OwnerCharacter && OwnerCharacter->GetMesh()) ? OwnerCharacter->GetMesh()->GetAnimInstance() : nullptr;
	if ( !AnimInstance )
		return;

	if ( bIgnore )
	{
		SavedRootMotionMode = ( uint8 ) AnimInstance->RootMotionMode.GetValue();
		AnimInstance->SetRootMotionMode(ERootMotionMode::IgnoreRootMotion);
	}
	else
	{
		AnimInstance->SetRootMotionMode(( ERootMotionMode::Type ) SavedRootMotionMode);
	}

	bAnimRootMotionIgnored = bIgnore;
}

void UGA_Muriel_ReversalOfFortune::StartCameraLock()
{
	if ( !bLockCameraOnTargetDuringDescent || bCameraLockActive || !IsLocallyControlled() || !TargetAlly )
		return;

	ACPlayerCharacter* CPlayerCharacter = Cast<ACPlayerCharacter>(GetAvatarActorFromActorInfo());
	if ( !CPlayerCharacter )
		return;

	// 원래 상태를 저장해 두었다가 그대로 복구 (다른 시스템이 막아 둔 경우를 덮어쓰지 않도록)
	bSavedCameraInputBlocked = CPlayerCharacter->IsCameraInputBlocked();
	CPlayerCharacter->SetCameraInputBlock(true);

	bCameraLockActive = true;
	UpdateCameraLock();
}

void UGA_Muriel_ReversalOfFortune::StopCameraLock()
{
	if ( !bCameraLockActive )
		return;

	bCameraLockActive = false;

	if ( ACPlayerCharacter* CPlayerCharacter = Cast<ACPlayerCharacter>(GetAvatarActorFromActorInfo()) )
	{
		CPlayerCharacter->SetCameraInputBlock(bSavedCameraInputBlocked);
	}
}

void UGA_Muriel_ReversalOfFortune::UpdateCameraLock()
{
	if ( !bCameraLockActive )
		return;

	APawn* OwnerPawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	APlayerController* PC = OwnerPawn ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr;
	if ( !PC || !PC->PlayerCameraManager )
	{
		StopCameraLock();
		return;
	}

	// 타겟이 움직이면 따라가고, 사라지면 낙하 지점을 바라봄
	const FVector LookTarget = IsValid(TargetAlly) ? TargetAlly->GetActorLocation() : FixedAirborneCenter;
	const FVector CameraLocation = PC->PlayerCameraManager->GetCameraLocation();
	const FVector ToTarget = LookTarget - CameraLocation;

	FRotator DesiredRotation = ToTarget.Rotation();

	// 타겟이 거의 수직 아래에 있으면 Yaw가 튀므로 현재 Yaw 유지
	if ( ToTarget.Size2D() < 50.f )
	{
		DesiredRotation.Yaw = PC->GetControlRotation().Yaw;
	}

	const float DeltaTime = GetWorld()->GetDeltaSeconds();
	const FRotator NewRotation = CameraLockInterpSpeed > 0.f
		? FMath::RInterpTo(PC->GetControlRotation(), DesiredRotation, DeltaTime, CameraLockInterpSpeed)
		: DesiredRotation;

	PC->SetControlRotation(NewRotation);

	// 매 프레임 갱신 (루핑 타이머는 프레임당 여러 번 호출될 수 있어 NextTick 체인 사용)
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UGA_Muriel_ReversalOfFortune::UpdateCameraLock);
}

void UGA_Muriel_ReversalOfFortune::OnDescentStarted(FGameplayEventData PayLoad)
{
	StopAscent();

	AActor* OwnerAvatarActor = GetAvatarActorFromActorInfo();
	ACharacter* OwnerCharacter = Cast<ACharacter>(OwnerAvatarActor);
	if ( !OwnerCharacter || !TargetAlly )
		return;

	if ( UMotionWarpingComponent* WarpingComp = OwnerCharacter->FindComponentByClass<UMotionWarpingComponent>() )
	{
		WarpingComp->AddOrUpdateWarpTargetFromLocationAndRotation(WarpTargetName, FixedLandingLocation, OwnerCharacter->GetActorRotation());
		UE_LOG(LogTemp, Log, TEXT("[Reversal] 워프 타겟 등록: %s"), *FixedLandingLocation.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[Reversal] MotionWarpingComponent를 못 찾음!"));
	}

	OwnerCharacter->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
}

void UGA_Muriel_ReversalOfFortune::OnLanded(FGameplayEventData PayLoad)
{
	StopCameraLock();

	if ( !K2_HasAuthority() || !TargetAlly )
		return;

	AActor* OwnerAvatarActor = GetAvatarActorFromActorInfo();
	if ( ACharacter* OwnerCharacter = Cast<ACharacter>(OwnerAvatarActor) )
	{
		OwnerCharacter->GetCharacterMovement()->SetMovementMode(MOVE_Walking); 
	}

	if ( ACPlayerCharacter* CPlayerCharacter = Cast<ACPlayerCharacter>(OwnerAvatarActor) )
	{
		CPlayerCharacter->SetMoveInputBlock(false);
	}

	FVector LandLocation = FixedAirborneCenter;

	TArray<FOverlapResult> Overlaps;
	FCollisionShape Sphere = FCollisionShape::MakeSphere(LaunchRadius);
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(OwnerAvatarActor);

	GetWorld()->OverlapMultiByObjectType(Overlaps, LandLocation, FQuat::Identity, FCollisionObjectQueryParams(ECC_Pawn), Sphere, Params);

	if ( ShouldDrawDebug() )
	{
		DrawDebugSphere(GetWorld(), LandLocation, LaunchRadius, 24, FColor::Red, false, 3.f, 0, 3.f);
	}

	IGenericTeamAgentInterface* OwnerTeamAgent = Cast<IGenericTeamAgentInterface>(OwnerAvatarActor);
	TSet<AActor*> Processed;

	for ( const FOverlapResult& Overlap : Overlaps )
	{
		AActor* OtherActor = Overlap.GetActor();

		if ( !OtherActor || Processed.Contains(OtherActor) )
			continue;

		Processed.Add(OtherActor);

		IGenericTeamAgentInterface* OtherTeamAgent = Cast<IGenericTeamAgentInterface>(OtherActor);
		if ( !OtherTeamAgent || !OwnerTeamAgent || OwnerTeamAgent->GetTeamAttitudeTowards(*OtherActor) != ETeamAttitude::Hostile )
		{
			UE_LOG(LogTemp, Log, TEXT("[Reversal]     -> 아군이거나 팀 판정 실패, 제외"));
			continue;
		}

		if ( ShouldDrawDebug() ) 
		{
			DrawDebugSphere(GetWorld(), OtherActor->GetActorLocation(), 50.f, 12, FColor::Magenta, false, 3.f);
		}

		FVector LaunchVelocity(0.f, 0.f, LaunchZVelocity);
		PushTarget(OtherActor, LaunchVelocity);
	}
}

FGameplayTag UGA_Muriel_ReversalOfFortune::GetDescentStartTag()
{
	return FGameplayTag::RequestGameplayTag("Ability.Muriel.Reversal.DescentStart");
}

FGameplayTag UGA_Muriel_ReversalOfFortune::GetLandTag()
{
	return FGameplayTag::RequestGameplayTag("Ability.Muriel.Reversal.Land");
}
