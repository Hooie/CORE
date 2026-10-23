// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Muriel/GA_Muriel_Alacrity.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitConfirmCancel.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GAS/CAbilitySystemStatics.h"
#include "GAS/Muriel/AllyShootProjectile.h"
#include "GameFramework/Character.h"
#include "GenericTeamAgentInterface.h"

UGA_Muriel_Alacrity::UGA_Muriel_Alacrity()
{
	ActivationOwnedTags.AddTag(UCAbilitySystemStatics::GetAimStatusTag());
	ActivationOwnedTags.AddTag(UCAbilitySystemStatics::GetBasicAttackAimStatusTag());
}

void UGA_Muriel_Alacrity::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if( !K2_CheckAbilityCost() || !K2_CheckAbilityCooldown() || !TargetingMontage || !ShootMontage )
	{
		K2_EndAbility();
		return;
	}

	if ( K2_HasAuthority() || IsLocallyControlled() )
	{
		UAbilityTask_PlayMontageAndWait* PlayTargetingMontage = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, TargetingMontage);
		PlayTargetingMontage->ReadyForActivation();

		UAbilityTask_WaitConfirmCancel* WaitConfirmCancel = UAbilityTask_WaitConfirmCancel::WaitConfirmCancel(this);
		WaitConfirmCancel->OnConfirm.AddDynamic(this, &UGA_Muriel_Alacrity::OnConfirmed);
		WaitConfirmCancel->OnCancel.AddDynamic(this, &UGA_Muriel_Alacrity::OnCancelled);
		WaitConfirmCancel->ReadyForActivation();
	}

}

FGameplayTag UGA_Muriel_Alacrity::GetAlacrityFireTag()
{
	return FGameplayTag::RequestGameplayTag("Ability.Muriel.Alacrity.Fire");
}

void UGA_Muriel_Alacrity::FireProjectile(FGameplayEventData PayLoad)
{
	if ( !K2_HasAuthority() || !ProjectileClass )
		return;

	AActor* OwnerAvatarActor = GetAvatarActorFromActorInfo();
	if ( !OwnerAvatarActor )
		return;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = OwnerAvatarActor;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	FVector SocketLocation = OwnerAvatarActor->GetActorLocation();
	if ( USkeletalMeshComponent* MeshComp = OwnerAvatarActor->FindComponentByClass<USkeletalMeshComponent>() )
	{
		SocketLocation = MeshComp->GetSocketLocation(MuzzleSocketName);
	}

	AActor* AllyTarget = nullptr;
	if ( bUseAllyTargeting )
	{
		AllyTarget = GetAimTarget(ProjectileMaxDistance, ETeamAttitude::Friendly);
	}

	FRotator LaunchRotation = OwnerAvatarActor->GetActorRotation();
	if ( !AllyTarget )
	{
		if ( ACharacter* OwnerCharacter = Cast<ACharacter>(OwnerAvatarActor) )
		{
			if ( UCapsuleComponent* Capsule = OwnerCharacter->GetCapsuleComponent() )
			{
				LaunchRotation = Capsule->GetComponentRotation();
			}
		}
	}

	FGameplayEffectSpecHandle MoveSpeedSpec = MakeOutgoingGameplayEffectSpec(MoveSpeedBuffEffect, GetAbilityLevel(CurrentSpecHandle, CurrentActorInfo));

	AAllyShootProjectile* Projectile = GetWorld()->SpawnActor<AAllyShootProjectile>(ProjectileClass, SocketLocation, LaunchRotation, SpawnParams);
	if ( Projectile )
	{
		Projectile->OnAllyHit.AddDynamic(this, &UGA_Muriel_Alacrity::OnAllyHitByProjectile);
		Projectile->ShootProjectile(ProjectileSpeed, ProjectileMaxDistance, AllyTarget, GetOwnerTeamId(), MoveSpeedSpec);
	}
}

void UGA_Muriel_Alacrity::OnConfirmed()
{
	if ( !K2_CommitAbility() )
	{
		K2_EndAbility();
		return;
	}

	if ( K2_HasAuthority() || IsLocallyControlled() ) 
	{
		UAbilityTask_PlayMontageAndWait* PlayShootMontage = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, ShootMontage);
		PlayShootMontage->OnCompleted.AddDynamic(this, &UGA_Muriel_Alacrity::K2_EndAbility);
		PlayShootMontage->OnInterrupted.AddDynamic(this, &UGA_Muriel_Alacrity::K2_EndAbility);
		PlayShootMontage->OnCancelled.AddDynamic(this, &UGA_Muriel_Alacrity::K2_EndAbility);
		PlayShootMontage->ReadyForActivation();
	}

	if ( K2_HasAuthority() )
	{
		UAbilityTask_WaitGameplayEvent* WaitFireEvent = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, GetAlacrityFireTag());
		WaitFireEvent->EventReceived.AddDynamic(this, &UGA_Muriel_Alacrity::FireProjectile);
		WaitFireEvent->ReadyForActivation();
	}
}

void UGA_Muriel_Alacrity::OnCancelled()
{
	K2_EndAbility();
}

void UGA_Muriel_Alacrity::OnAllyHitByProjectile(AActor* HitActor, UAbilitySystemComponent* HitASC)
{
	if ( !K2_HasAuthority() || !HitASC )
		return;

	UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo();
	int32 AbilityLevel = GetAbilityLevel(CurrentSpecHandle, CurrentActorInfo);

	const float ShieldAmount = GetScaledAmount(ShieldBaseAmountPerLevel, AbilityLevel, 0.f, ShieldIntelligenceRatio);

	UCAbilitySystemStatics::ApplyTimedShield(OwnerASC, HitASC, ShieldEffect, ShieldBuffIconEffect, AbilityLevel, ShieldDuration, ShieldAmount);
}
