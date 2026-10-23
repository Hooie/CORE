// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Murdock/GA_Murdock_JudgementBlast.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitConfirmCancel.h"
#include "Player/CPlayerCharacter.h"
#include "Components/MeshComponent.h"
#include "GAS/CAbilitySystemStatics.h"
#include "GenericTeamAgentInterface.h"

UGA_Murdock_JudgementBlast::UGA_Murdock_JudgementBlast()
{
	ActivationOwnedTags.AddTag(UCAbilitySystemStatics::GetAimStatusTag());
	ActivationOwnedTags.AddTag(UCAbilitySystemStatics::GetBasicAttackAimStatusTag());
	ActivationOwnedTags.AddTag(UCAbilitySystemStatics::GetCrosshairTag());
}

void UGA_Murdock_JudgementBlast::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if ( !HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo) || !TargetingMontage || !ShootMontage )
	{
		K2_EndAbility();
		return;
	}

	if ( !K2_CheckAbilityCost() || !K2_CheckAbilityCooldown() )
	{
		K2_EndAbility();
		return;
	}

	if ( ACPlayerCharacter* CPlayerCharacter = Cast<ACPlayerCharacter>(GetOwningAvatarCharacter()))
	{
		CPlayerCharacter->SetBlockAbilityInput(true);
	}

	if( HasAuthority(&ActivationInfo) || IsLocallyControlled() )
	{
		UAbilityTask_PlayMontageAndWait* PlayTargetingMontage = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, TargetingMontage);
		PlayTargetingMontage->ReadyForActivation();
	}

	FTimerHandle TargetingDelayHandle;
	GetWorld()->GetTimerManager().SetTimer(TargetingDelayHandle, this, &UGA_Murdock_JudgementBlast::StartShootingPhase, ShootingReadyDelay, false);
}

void UGA_Murdock_JudgementBlast::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	StopLaserSight();

	if ( ACPlayerCharacter* CPlayerCharacter = Cast<ACPlayerCharacter>(GetOwningAvatarCharacter()) )
	{
		CPlayerCharacter->SetBlockAbilityInput(false);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

FGameplayTag UGA_Murdock_JudgementBlast::GetBlastFireTag()
{
	return FGameplayTag::RequestGameplayTag("Ability.Murdock.Blast.Fire");
}

void UGA_Murdock_JudgementBlast::OnConfirmed()
{
	if ( !K2_CommitAbility() )
	{
		K2_EndAbility();
		return;
	}

	AActor* OwnerAvatarActor = GetAvatarActorFromActorInfo();
	APawn* OwnerPawn = Cast<APawn>(OwnerAvatarActor);
	APlayerController* PC = OwnerPawn ? OwnerPawn->GetController<APlayerController>() : nullptr;

	if ( PC )
	{
		FVector CamLoc;
		FRotator CamRot;
		PC->GetPlayerViewPoint(CamLoc, CamRot);
		ConfirmedAimDirection = CamRot.Vector();
	}
	else if ( OwnerAvatarActor )
	{
		ConfirmedAimDirection = OwnerAvatarActor->GetActorForwardVector();
	}

	if ( K2_HasAuthority() || IsLocallyControlled() )
	{
		UAbilityTask_PlayMontageAndWait* PlayShootMontage = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, ShootMontage);
		PlayShootMontage->OnCompleted.AddDynamic(this, &UGA_Murdock_JudgementBlast::K2_EndAbility);
		PlayShootMontage->OnCancelled.AddDynamic(this, &UGA_Murdock_JudgementBlast::K2_EndAbility);
		PlayShootMontage->OnInterrupted.AddDynamic(this, &UGA_Murdock_JudgementBlast::K2_EndAbility);
		PlayShootMontage->ReadyForActivation();

		StartLaserSight();
	
		UAbilityTask_WaitGameplayEvent* WaitFireEvent = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, GetBlastFireTag());
		WaitFireEvent->EventReceived.AddDynamic(this, &UGA_Murdock_JudgementBlast::FireBlast);
		WaitFireEvent->ReadyForActivation();
	}
}

void UGA_Murdock_JudgementBlast::FireBlast(FGameplayEventData Payload)
{
	StopLaserSight();

	if ( !K2_HasAuthority() )
		return;

	AActor* OwnerAvatarActor = GetAvatarActorFromActorInfo();
	if ( !OwnerAvatarActor )
		return;

	FVector MuzzleLocation = OwnerAvatarActor->GetActorLocation();
	if ( USkeletalMeshComponent* MeshComp = OwnerAvatarActor->FindComponentByClass<USkeletalMeshComponent>() )
	{
		MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);
	}

	FVector TraceEnd = MuzzleLocation + ConfirmedAimDirection * BlastRange;

	if ( UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo() )
	{
		FHitResult BeamHitData;
		BeamHitData.TraceStart = MuzzleLocation;
		BeamHitData.Location = TraceEnd;

		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		Context.AddHitResult(BeamHitData);

		FGameplayCueParameters CueParams;
		CueParams.EffectContext = Context;

		ASC->ExecuteGameplayCue(UCAbilitySystemStatics::GetMurdockBlastBeamGameplayCueTag(), CueParams);
	}

	TArray<FHitResult> Hits;
	FCollisionShape SweepShape = FCollisionShape::MakeSphere(BlastRadius);
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(OwnerAvatarActor);

	bool bAnyHit = GetWorld()->SweepMultiByObjectType(Hits, MuzzleLocation, TraceEnd, FQuat::Identity, FCollisionObjectQueryParams(ECC_Pawn), SweepShape, Params);


	if ( ShouldDrawDebug() )
	{
		DrawDebugCylinder(GetWorld(), MuzzleLocation, TraceEnd, BlastRadius, 16, FColor::Purple, false, 3.f, 0, 2.f);

		DrawDebugLine(GetWorld(), MuzzleLocation, TraceEnd, FColor::White, false, 3.f, 0, 1.f);

		DrawDebugSphere(GetWorld(), MuzzleLocation, 20.f, 8, FColor::Blue, false, 3.f);
		DrawDebugSphere(GetWorld(), TraceEnd, 20.f, 8, FColor::White, false, 3.f);
	}

	IGenericTeamAgentInterface* OwnerTeamAgent = Cast<IGenericTeamAgentInterface>(OwnerAvatarActor);
	int32 AbilityLevel = GetAbilityLevel(CurrentSpecHandle, CurrentActorInfo);
	float BaseDamage = GetLevelIndexedDamage(BaseDamagePerLevel, AbilityLevel);

	TSet<AActor*> ProcessedTargets;

	for ( const FHitResult& Hit : Hits )
	{
		AActor* HitActor = Hit.GetActor();
		if ( !HitActor )
			continue;

		if ( ProcessedTargets.Contains(HitActor) )
		{
			continue;
		}

		ProcessedTargets.Add(HitActor);

		bool bIsAlly = false;
		if ( OwnerTeamAgent )
		{
			IGenericTeamAgentInterface* OtherTeamAgent = Cast<IGenericTeamAgentInterface>(HitActor);
			if ( OtherTeamAgent )
			{
				bIsAlly = OwnerTeamAgent->GetTeamAttitudeTowards(*HitActor) != ETeamAttitude::Hostile;
			}
		}

		if ( bIsAlly )
		{
			if ( ShouldDrawDebug() )
			{
				DrawDebugSphere(GetWorld(), HitActor->GetActorLocation(), 40.f, 12, FColor::Cyan, false, 3.f);
			}

			continue;
		}

		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor);
		if ( !TargetASC )
		{
			continue;
		}


		if ( ShouldDrawDebug() )
		{
			DrawDebugSphere(GetWorld(), HitActor->GetActorLocation(), 40.f, 12, FColor::Red, false, 3.f); 
		}

		FGameplayCueParameters CueParams;
		CueParams.Location = Hit.ImpactPoint;
		CueParams.Instigator = OwnerAvatarActor;
		TargetASC->ExecuteGameplayCue(UCAbilitySystemStatics::GetMurdockBlastHitEffectsGameplayCueTag(), CueParams);

		FGameplayEffectSpecHandle DamageSpec = MakeScaledDamageSpec(HitDamageEffect, BaseDamage, ScalingRatio, AbilityLevel);
		ApplyGameplayEffectSpecToTarget(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, DamageSpec, UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActor(HitActor));
	}
}

void UGA_Murdock_JudgementBlast::StartLaserSight()
{
	if ( bLaserSightActive )
		return;

	if ( !(K2_HasAuthority() || IsLocallyControlled()) )
		return;

	AActor* OwnerAvatarActor = GetAvatarActorFromActorInfo();
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if ( !OwnerAvatarActor || !ASC )
		return;

	FVector MuzzleLocation = OwnerAvatarActor->GetActorLocation();
	if ( USkeletalMeshComponent* MeshComp = OwnerAvatarActor->FindComponentByClass<USkeletalMeshComponent>() )
	{
		MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);
	}

	FHitResult LaserHitData;
	LaserHitData.TraceStart = MuzzleLocation;
	LaserHitData.Location = MuzzleLocation + ConfirmedAimDirection * BlastRange;

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddHitResult(LaserHitData);

	FGameplayCueParameters CueParams;
	CueParams.EffectContext = Context;

	ASC->AddGameplayCue(UCAbilitySystemStatics::GetMurdockLaserSightGameplayCueTag(), CueParams);
	bLaserSightActive = true;
}

void UGA_Murdock_JudgementBlast::StopLaserSight()
{
	if ( !bLaserSightActive )
		return;

	if ( UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo() )
	{
		ASC->RemoveGameplayCue(UCAbilitySystemStatics::GetMurdockLaserSightGameplayCueTag());
	}

	bLaserSightActive = false;
}

void UGA_Murdock_JudgementBlast::StartShootingPhase()
{
	UAbilityTask_WaitConfirmCancel* WaitConfirmCancel = UAbilityTask_WaitConfirmCancel::WaitConfirmCancel(this);
	WaitConfirmCancel->OnConfirm.AddDynamic(this, &UGA_Murdock_JudgementBlast::OnConfirmed);
	WaitConfirmCancel->OnCancel.AddDynamic(this, &UGA_Murdock_JudgementBlast::K2_EndAbility);
	WaitConfirmCancel->ReadyForActivation();

	if ( ACPlayerCharacter* CPlayerCharacter = Cast<ACPlayerCharacter>(GetOwningAvatarCharacter()) )
	{
		CPlayerCharacter->SetBlockAbilityInput(false);
	}
}
