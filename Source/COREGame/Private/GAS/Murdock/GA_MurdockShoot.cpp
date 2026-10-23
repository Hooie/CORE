// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Murdock/GA_MurdockShoot.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Character/CMurdockCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "GAS/CAbilitySystemStatics.h"
#include "GAS/GA_Reload.h"
#include "GAS/MMC_AbilityDamage.h"
#include "GenericTeamAgentInterface.h"
#include "Player/AmmoComponent.h"

#include "DrawDebugHelpers.h"

UGA_MurdockShoot::UGA_MurdockShoot()
{
	ActivationOwnedTags.AddTag(UCAbilitySystemStatics::GetBasicAttackAimStatusTag());
	ActivationOwnedTags.AddTag(UCAbilitySystemStatics::GetCrosshairTag());

	BlockAbilitiesWithTag.AddTag(UCAbilitySystemStatics::GetDisarmedTag());
	CancelAbilitiesWithTag.AddTag(UCAbilitySystemStatics::GetDisarmedTag());
}

void UGA_MurdockShoot::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if ( !K2_CommitAbility() || !StandByMontage || !ShootMontage )
	{
		K2_EndAbility();
		return;
	}

	if ( K2_HasAuthority() || IsLocallyControlled() )
	{
		if ( AActor* Avatar = GetAvatarActorFromActorInfo() )
		{
			CachedAmmoComponent = Avatar->FindComponentByClass<UAmmoComponent>();
			if ( CachedAmmoComponent )
			{
				CachedAmmoComponent->OnAmmoChanged.AddUObject(this, &UGA_MurdockShoot::OnAmmoComponentChanged);
			}
		}

		UAbilityTask_PlayMontageAndWait* PlayStandByMontage = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, StandByMontage);
		PlayStandByMontage->ReadyForActivation();

		UAbilityTask_WaitGameplayEvent* WaitStartShootingEvent = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, UCAbilitySystemStatics::GetBasicAttackInputPressedTag());
		WaitStartShootingEvent->EventReceived.AddDynamic(this, &UGA_MurdockShoot::StartShooting);
		WaitStartShootingEvent->ReadyForActivation();

		UAbilityTask_WaitGameplayEvent* WaitStopShootingEvent = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, UCAbilitySystemStatics::GetBasicAttackInputReleasedTag());
		WaitStopShootingEvent->EventReceived.AddDynamic(this, &UGA_MurdockShoot::StopShooting);
		WaitStopShootingEvent->ReadyForActivation();

		UAbilityTask_WaitGameplayEvent* WaitFireEvent = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, GetShootTag(), nullptr, false, false);
		WaitFireEvent->EventReceived.AddDynamic(this, &UGA_MurdockShoot::FireHitScan);
		WaitFireEvent->ReadyForActivation();
	}
}

void UGA_MurdockShoot::InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	K2_EndAbility();
}

void UGA_MurdockShoot::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	bIsHoldingFire = false;
	bIsShotInProgress = false;

	if ( CachedAmmoComponent )
	{
		CachedAmmoComponent->OnAmmoChanged.RemoveAll(this);
		CachedAmmoComponent = nullptr;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

FGameplayTag UGA_MurdockShoot::GetShootTag()
{
	return FGameplayTag::RequestGameplayTag("Ability.Shoot");
}

bool UGA_MurdockShoot::GetBasicAttackDamagePreview(const UAbilitySystemComponent* SourceASC, float& OutRawDamage, bool& bOutIsMagical) const
{
	return UMMC_AbilityDamage::PreviewRawDamage(HitDamageEffeect, SourceASC, BaseDamage, ScalingRatio, 0.f, 0.f, OutRawDamage, bOutIsMagical);
}

void UGA_MurdockShoot::StartShooting(FGameplayEventData Payload)
{
	if ( IsReloading() )
		return;

	bIsHoldingFire = true;

	TryFireNextShot();
}

void UGA_MurdockShoot::StopShooting(FGameplayEventData Payload)
{
	bIsHoldingFire = false;
}

void UGA_MurdockShoot::FireHitScan(FGameplayEventData Payload)
{
	if ( IsReloading() || !K2_HasAuthority() )
		return;

	AActor* OwnerAvatarActor = GetAvatarActorFromActorInfo();
	UAmmoComponent* AmmoComponent = OwnerAvatarActor ? OwnerAvatarActor->FindComponentByClass<UAmmoComponent>() : nullptr;

	if ( AmmoComponent && !AmmoComponent->TryConsumeAmmo() )
	{
		if ( UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo() )
		{
			FGameplayTagContainer ReloadAbilityTags(UCAbilitySystemStatics::GetReloadingStatusTag());
			ASC->TryActivateAbilitiesByTag(ReloadAbilityTags);
		}
		return;
	}

	APawn* OwnerPawn = Cast<APawn>(OwnerAvatarActor);
	APlayerController* PC = OwnerPawn ? OwnerPawn->GetController<APlayerController>() : nullptr;

	FVector ComputedMuzzleLocation = OwnerAvatarActor->GetActorLocation() + FVector(0.f, 0.f, 50.f);
	if ( USkeletalMeshComponent* MeshComp = OwnerAvatarActor->FindComponentByClass<USkeletalMeshComponent>() )
	{
		ComputedMuzzleLocation = MeshComp->GetSocketLocation(GunMuzzleSocketName);
	}

	AActor* CameraFinalHitActor = nullptr;
	FHitResult CameraFinalHit;
	FVector CameraTraceEnd;

	if ( PC )
	{
		FVector CameraLoc;
		FRotator CameraRot;
		PC->GetPlayerViewPoint(CameraLoc, CameraRot);

		CameraTraceEnd = CameraLoc + CameraRot.Vector() * ShootRange;
		PenetratingTrace(CameraLoc, CameraTraceEnd, OwnerAvatarActor, CameraFinalHit, CameraFinalHitActor);

		//DrawDebugLine(GetWorld(), CameraLoc, CameraFinalHitActor ? CameraFinalHit.Location : CameraTraceEnd, FColor::Yellow, false, 3.f, 0, 1.f);
	}
	else
	{
		CameraTraceEnd = OwnerAvatarActor->GetActorLocation() + OwnerAvatarActor->GetActorForwardVector() * ShootRange;
	}

	FVector TrueTargetPoint = CameraFinalHitActor ? CameraFinalHit.ImpactPoint : CameraTraceEnd;

	FVector MuzzleDirection = (TrueTargetPoint - ComputedMuzzleLocation).GetSafeNormal();
	FVector MuzzleTraceEnd = ComputedMuzzleLocation + MuzzleDirection * ShootRange;

	AActor* VerifiedActor = nullptr;
	FHitResult VerifiedHit;
	PenetratingTrace(ComputedMuzzleLocation, MuzzleTraceEnd, OwnerAvatarActor, VerifiedHit, VerifiedActor);

	//DrawDebugLine(GetWorld(), ComputedMuzzleLocation, VerifiedActor ? VerifiedHit.Location : MuzzleTraceEnd, FColor::Magenta, false, 3.f, 0, 1.5f);
	//DrawDebugSphere(GetWorld(), ComputedMuzzleLocation, 6.f, 8, FColor::Blue, false, 3.f);

	AActor* FinalHitActor = nullptr;
	FHitResult FinalHitResult;
	FVector TracerEndLocation;

	if ( CameraFinalHitActor && VerifiedActor == CameraFinalHitActor )
	{
		FinalHitActor = CameraFinalHitActor;
		FinalHitResult = CameraFinalHit;
		TracerEndLocation = CameraFinalHit.ImpactPoint;
	}
	else if ( VerifiedActor )
	{
		TracerEndLocation = VerifiedHit.Location;
	}
	else
	{
		TracerEndLocation = CameraFinalHitActor ? CameraFinalHit.ImpactPoint : MuzzleTraceEnd;
	}

	//DrawDebugLine(GetWorld(), ComputedMuzzleLocation, TracerEndLocation, FinalHitActor ? FColor::Red : FColor::Green, false, 3.f, 0, 3.f);

	if ( UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo() )
	{
		FHitResult TracerHitData;
		TracerHitData.TraceStart = ComputedMuzzleLocation;
		TracerHitData.Location = TracerEndLocation;
		TracerHitData.ImpactPoint = TracerEndLocation;
		
		FGameplayEffectContextHandle TracerContext = OwnerASC->MakeEffectContext();
		TracerContext.AddHitResult(TracerHitData);

		FGameplayCueParameters TracerParams;
		TracerParams.EffectContext = TracerContext;
		TracerParams.Instigator = OwnerAvatarActor;

		OwnerASC->ExecuteGameplayCue(UCAbilitySystemStatics::GetTracerGameplayCueTag(), TracerParams);
	}

	if ( FinalHitActor )
	{
		if ( UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(FinalHitActor) )
		{
			FGameplayCueParameters CueParams;
			CueParams.Location = FinalHitResult.ImpactPoint;
			CueParams.Normal = FinalHitResult.ImpactNormal;
			CueParams.Instigator = OwnerAvatarActor;

			for ( const FGameplayTag& Tag : TriggerGameplayCueTags )
			{
				TargetASC->ExecuteGameplayCue(Tag, CueParams);
			}
		}

		FGameplayEffectSpecHandle DamageSpec = MakeScaledDamageSpec(HitDamageEffeect, BaseDamage, ScalingRatio);
		ApplyGameplayEffectSpecToTarget(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, DamageSpec, UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActor(FinalHitActor));
	}

	if ( AmmoComponent && AmmoComponent->IsEmpty() )
	{
		if ( UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo() )
		{
			FGameplayTagContainer ReloadAbilityTags(UCAbilitySystemStatics::GetReloadingStatusTag());
			ASC->TryActivateAbilitiesByTag(ReloadAbilityTags);
		}
	}
}

void UGA_MurdockShoot::TryFireNextShot()
{
	if ( !bIsHoldingFire || IsReloading() || bIsShotInProgress )
		return;

	bIsShotInProgress = true;

	if ( K2_HasAuthority() || IsLocallyControlled() )
	{
		UAbilityTask_PlayMontageAndWait* PlayShootMontage = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, ShootMontage);
		PlayShootMontage->OnBlendOut.AddDynamic(this, &UGA_MurdockShoot::OnShootMontageCompleted);
		PlayShootMontage->OnInterrupted.AddDynamic(this, &UGA_MurdockShoot::OnShootMontageCompleted);
		PlayShootMontage->OnCancelled.AddDynamic(this, &UGA_MurdockShoot::OnShootMontageCompleted);
		PlayShootMontage->ReadyForActivation();
	}
}

void UGA_MurdockShoot::OnShootMontageCompleted()
{
	bIsShotInProgress = false;

	if ( bIsHoldingFire )
	{
		TryFireNextShot();
	}
	else if ( !IsReloading() )
	{
		if ( K2_HasAuthority() || IsLocallyControlled() )
		{
			UAbilityTask_PlayMontageAndWait* PlayStandBy = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, StandByMontage);
			PlayStandBy->ReadyForActivation();
		}
	}
}

void UGA_MurdockShoot::OnAmmoComponentChanged(int32 Current, int32 Max)
{
	if ( Current <= 0 )
	{
		if ( UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo() )
		{
			FGameplayTagContainer ReloadAbilityTags(UCAbilitySystemStatics::GetReloadingStatusTag());
			ASC->TryActivateAbilitiesByTag(ReloadAbilityTags);
		}
	}
}

bool UGA_MurdockShoot::PenetratingTrace(const FVector& Start, const FVector& End, AActor* OwnerActor, FHitResult& OutHit, AActor*& OutHitActor) const
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

bool UGA_MurdockShoot::IsReloading() const
{
	if ( UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo() )
	{
		return OwnerASC->HasMatchingGameplayTag(UCAbilitySystemStatics::GetReloadingStatusTag());
	}

	return false;
}
