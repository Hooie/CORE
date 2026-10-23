// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Murdock/GA_Murdock_Shotgun.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitConfirmCancel.h"
#include "GAS/CAbilitySystemStatics.h"
#include "GameFramework/PlayerController.h"
#include "GenericTeamAgentInterface.h"
#include "Components/SkeletalMeshComponent.h"

#include "DrawDebugHelpers.h"

UGA_Murdock_Shotgun::UGA_Murdock_Shotgun()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	ActivationOwnedTags.AddTag(UCAbilitySystemStatics::GetAimStatusTag());
	ActivationOwnedTags.AddTag(UCAbilitySystemStatics::GetBasicAttackAimStatusTag());
	ActivationOwnedTags.AddTag(UCAbilitySystemStatics::GetCrosshairTag());
}

void UGA_Murdock_Shotgun::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if ( !HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo) || !TargetingMontage || !ShootMontage )
	{
		K2_EndAbility();
		return;
	}

	if ( !CheckCost(Handle, ActorInfo) || !CheckCooldown(Handle, ActorInfo) )
	{
		K2_EndAbility();
		return;
	}

	UAbilityTask_PlayMontageAndWait* PlayTargetingMontage = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, TargetingMontage);
	PlayTargetingMontage->ReadyForActivation();

	UAbilityTask_WaitConfirmCancel* WaitConfirmCancel = UAbilityTask_WaitConfirmCancel::WaitConfirmCancel(this);
	WaitConfirmCancel->OnConfirm.AddDynamic(this, &UGA_Murdock_Shotgun::OnConfirmed);
	WaitConfirmCancel->OnCancel.AddDynamic(this, &UGA_Murdock_Shotgun::OnCancelled);
	WaitConfirmCancel->ReadyForActivation();
}

FGameplayTag UGA_Murdock_Shotgun::GetShotgunFireTag()
{
	return FGameplayTag::RequestGameplayTag("Ability.Murdock.Shotgun.Fire");
}

void UGA_Murdock_Shotgun::OnConfirmed()
{
	if ( !K2_CommitAbility() )
	{
		K2_EndAbility();
		return;
	}

	if ( K2_HasAuthority() || IsLocallyControlled() )
	{
		UAbilityTask_PlayMontageAndWait* PlayShootMontage = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, ShootMontage);
		PlayShootMontage->OnCompleted.AddDynamic(this, &UGA_Murdock_Shotgun::K2_EndAbility);
		PlayShootMontage->OnInterrupted.AddDynamic(this, &UGA_Murdock_Shotgun::K2_EndAbility);
		PlayShootMontage->OnCancelled.AddDynamic(this, &UGA_Murdock_Shotgun::K2_EndAbility);
		PlayShootMontage->ReadyForActivation();
	}

	if ( K2_HasAuthority() )
	{
		UAbilityTask_WaitGameplayEvent* WaitFireEvent = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, GetShotgunFireTag());
		WaitFireEvent->EventReceived.AddDynamic(this, &UGA_Murdock_Shotgun::FireShotgun);
		WaitFireEvent->ReadyForActivation();
	}
}

void UGA_Murdock_Shotgun::OnCancelled()
{
	K2_EndAbility();
}

bool UGA_Murdock_Shotgun::PenetratingTrace(const FVector& Start, const FVector& End, AActor* OwnerActor, FHitResult& OutHit, AActor*& OutHitActor) const
{
	OutHitActor = nullptr;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(OwnerActor);

	FVector CurrentStart = Start;
	const int32 MaxPenetrations = 10;

	for ( int32 i = 0; i < MaxPenetrations; ++i )
	{
		FHitResult LoopHitResult;
		bool bHit = GetWorld()->LineTraceSingleByChannel(LoopHitResult, CurrentStart, End, TraceChannel, Params);

		if ( !bHit )
			return false;

		AActor* HitActor = LoopHitResult.GetActor();

		IGenericTeamAgentInterface* OwnerTeamAgent = Cast<IGenericTeamAgentInterface>(OwnerActor);
		bool bIsAlly = false;
		if ( OwnerTeamAgent && HitActor )
		{
			bIsAlly = OwnerTeamAgent->GetTeamAttitudeTowards(*HitActor) != ETeamAttitude::Hostile;
		}

		if ( bIsAlly )
		{
			Params.AddIgnoredActor(HitActor);
			CurrentStart = LoopHitResult.Location;
			continue;
		}

		OutHit = LoopHitResult;
		OutHitActor = HitActor;
		return true;
	}

	return false;
}

void UGA_Murdock_Shotgun::FireShotgun(FGameplayEventData Payload)
{
	if ( !K2_HasAuthority() )
		return;

	AActor* OwnerAvatarActor = GetAvatarActorFromActorInfo();
	if ( !OwnerAvatarActor )
		return;

	FVector ComputedMuzzleLocation = OwnerAvatarActor->GetActorLocation() + FVector(0.f, 0.f, 50.f);
	if ( USkeletalMeshComponent* MeshComp = OwnerAvatarActor->FindComponentByClass<USkeletalMeshComponent>() )
	{
		ComputedMuzzleLocation = MeshComp->GetSocketLocation(ShotgunFireSocketName);
	}

	APawn* OwnerPawn = Cast<APawn>(OwnerAvatarActor);
	APlayerController* PC = OwnerPawn ? OwnerPawn->GetController<APlayerController>() : nullptr;

	AActor* CameraFinalHitActor = nullptr;
	FHitResult CameraFinalHit;
	FVector CameraTraceEnd;
	FVector CameraLoc = FVector::ZeroVector;

	if ( PC )
	{
		FRotator CameraRot;
		PC->GetPlayerViewPoint(CameraLoc, CameraRot);

		CameraTraceEnd = CameraLoc + CameraRot.Vector() * ShotgunShotRange;
		PenetratingTrace(CameraLoc, CameraTraceEnd, OwnerAvatarActor, CameraFinalHit, CameraFinalHitActor);

		if ( ShouldDrawDebug() )
		{
			DrawDebugLine(GetWorld(), CameraLoc, CameraFinalHitActor ? CameraFinalHit.Location : CameraTraceEnd, FColor::Yellow, false, 3.f, 0, 1.f);
			if ( CameraFinalHitActor )
			{
				DrawDebugSphere(GetWorld(), CameraFinalHit.Location, 10.f, 8, FColor::Yellow, false, 3.f);
			}
		}
	}
	else
	{
		CameraTraceEnd = OwnerAvatarActor->GetActorLocation() + OwnerAvatarActor->GetActorForwardVector() * ShotgunShotRange;
	}

	FVector TrueTargetPoint = CameraFinalHitActor ? CameraFinalHit.ImpactPoint : CameraTraceEnd;

	FVector MuzzleDirection = (TrueTargetPoint - ComputedMuzzleLocation).GetSafeNormal();
	FRotator MuzzleCenterRotation = MuzzleDirection.Rotation();

	if ( ShouldDrawDebug() )
	{
		DrawDebugSphere(GetWorld(), ComputedMuzzleLocation, 6.f, 8, FColor::Blue, false, 3.f);

		FHitResult VerifyHit;
		AActor* VerifyActor = nullptr;
		PenetratingTrace(ComputedMuzzleLocation, ComputedMuzzleLocation + MuzzleDirection * ShotgunShotRange, OwnerAvatarActor, VerifyHit, VerifyActor);
		DrawDebugLine(GetWorld(), ComputedMuzzleLocation, VerifyActor ? VerifyHit.Location : (ComputedMuzzleLocation + MuzzleDirection * ShotgunShotRange), FColor::Magenta, false, 3.f, 0, 1.5f);
	}

	int32 AbilityLevel = GetAbilityLevel(CurrentSpecHandle, CurrentActorInfo);
	float BaseDamagePerShot = GetLevelIndexedDamage(BaseDamagePerShotPerLevel, AbilityLevel);

	float HalfAngle = ShotgunSpreadAngle * 0.5f;
	float AngleStep = (ShotgunSpreadCount > 1) ? (ShotgunSpreadAngle / (ShotgunSpreadCount - 1)) : 0.f;

	TMap<AActor*, int32> HitCountPerTarget;

	for ( int32 i = 0; i < ShotgunSpreadCount; ++i )
	{
		float PelletYawOffset = (ShotgunSpreadCount > 1) ? (-HalfAngle + AngleStep * i) : 0.f;
		FRotator PelletRotation(MuzzleCenterRotation.Pitch, MuzzleCenterRotation.Yaw + PelletYawOffset, 0.f);
		FVector PelletDirection = PelletRotation.Vector();

		FVector TraceEnd = ComputedMuzzleLocation + PelletDirection * ShotgunShotRange;

		FHitResult FinalHit;
		AActor* FinalHitActor = nullptr;
		PenetratingTrace(ComputedMuzzleLocation, TraceEnd, OwnerAvatarActor, FinalHit, FinalHitActor);

		if ( ShouldDrawDebug() )
		{
			DrawDebugLine(GetWorld(), ComputedMuzzleLocation, FinalHitActor ? FinalHit.Location : TraceEnd, FinalHitActor ? FColor::Red : FColor::Green, false, 3.f, 0, 1.5f);
		}

		if ( FinalHitActor )
		{
			HitCountPerTarget.FindOrAdd(FinalHitActor)++;
		}
	}

	FVector OwnerLocation = GetAvatarActorFromActorInfo()->GetActorLocation();

	for ( const TPair<AActor*, int32>& Pair : HitCountPerTarget )
	{
		AActor* HitTarget = Pair.Key;
		int32 HitCount = Pair.Value;

		float CombinedBaseDamage = BaseDamagePerShot * HitCount;
		float CombinedScalingRatio = ScalingRatio * HitCount;

		if ( UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitTarget) )
		{
			FGameplayCueParameters CueParams;
			CueParams.Instigator = OwnerAvatarActor;
			TargetASC->ExecuteGameplayCue(UCAbilitySystemStatics::GetMurdockHitEffectsGameplayCueTag(), CueParams);
		}

		float CombinedPushSpeed = PushSpeedPerHit * HitCount;
		PushTargetsFromOwnerLocation(TArray<AActor*>{ HitTarget }, CombinedPushSpeed, bPushWithZ);

		FGameplayEffectSpecHandle DamageSpec = MakeScaledDamageSpec(HitDamageEffect, CombinedBaseDamage, CombinedScalingRatio, AbilityLevel);
		ApplyGameplayEffectSpecToTarget(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, DamageSpec, UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActor(HitTarget));
	}
}
