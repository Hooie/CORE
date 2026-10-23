// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Muriel/GA_Muriel_Serenity.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitConfirmCancel.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "GAS/Muriel/SerenityProjectile.h"

void UGA_Muriel_Serenity::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if ( !K2_CheckAbilityCost() || !K2_CheckAbilityCooldown() )
	{
		K2_EndAbility();
		return;
	}

	if ( HasAuthority(&ActivationInfo) || IsLocallyControlled() )
	{
		UAbilityTask_PlayMontageAndWait* PlayTargetingMontage = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, TargetingMontage);
		PlayTargetingMontage->ReadyForActivation();

		UAbilityTask_WaitConfirmCancel* WaitConfirmCancel = UAbilityTask_WaitConfirmCancel::WaitConfirmCancel(this);
		WaitConfirmCancel->OnConfirm.AddDynamic(this, &UGA_Muriel_Serenity::OnConfirmed);
		WaitConfirmCancel->OnCancel.AddDynamic(this, &UGA_Muriel_Serenity::OnCancelled);
		WaitConfirmCancel->ReadyForActivation();
	}

	AddBasicAttackAimEffect();
}

void UGA_Muriel_Serenity::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	RemoveBasicAttackAimEffect();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

FGameplayTag UGA_Muriel_Serenity::GetFireTag()
{
	return FGameplayTag::RequestGameplayTag("Ability.Muriel.Serenity.Fire");
}

void UGA_Muriel_Serenity::AddBasicAttackAimEffect()
{
	if( CrosshairEffect && !CrosshairEffectHandle.IsValid() )
	{
		CrosshairEffectHandle = BP_ApplyGameplayEffectToOwner(CrosshairEffect);
	}

	if ( BasicAttackAimEffect && !BasicAttackAimEffectHandle.IsValid() )
	{
		BasicAttackAimEffectHandle = BP_ApplyGameplayEffectToOwner(BasicAttackAimEffect);
	}
}

void UGA_Muriel_Serenity::RemoveBasicAttackAimEffect()
{
	if ( CrosshairEffectHandle.IsValid() )
	{
		BP_RemoveGameplayEffectFromOwnerWithHandle(CrosshairEffectHandle);
	}

	if ( BasicAttackAimEffectHandle.IsValid() )
	{
		BP_RemoveGameplayEffectFromOwnerWithHandle(BasicAttackAimEffectHandle);
	}
}

void UGA_Muriel_Serenity::OnConfirmed()
{
	if ( !K2_CommitAbility() )
	{
		K2_EndAbility();
		return;
	}

	RemoveBasicAttackAimEffect();

	if ( K2_HasAuthority() || IsLocallyControlled() )
	{
		UAbilityTask_PlayMontageAndWait* PlayShootMontage = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, ShootMontage);
		PlayShootMontage->OnCompleted.AddDynamic(this, &UGA_Muriel_Serenity::K2_EndAbility); 
		PlayShootMontage->OnInterrupted.AddDynamic(this, &UGA_Muriel_Serenity::K2_EndAbility);
		PlayShootMontage->OnCancelled.AddDynamic(this, &UGA_Muriel_Serenity::K2_EndAbility); 
		PlayShootMontage->ReadyForActivation();
	}

	if ( K2_HasAuthority() )
	{
		UAbilityTask_WaitGameplayEvent* WaitFireEvent = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, GetFireTag());
		WaitFireEvent->EventReceived.AddDynamic(this, &UGA_Muriel_Serenity::FireProjectile);
		WaitFireEvent->ReadyForActivation();
	}
}

void UGA_Muriel_Serenity::OnCancelled()
{
	K2_EndAbility();
}

void UGA_Muriel_Serenity::FireProjectile(FGameplayEventData PayLoad)
{
	if ( !K2_HasAuthority() || !ProjectileClass )
		return;
	
	AActor* OwnerAvatarActor = GetAvatarActorFromActorInfo();
	if ( !OwnerAvatarActor )
		return;

	FVector MuzzleLocation = OwnerAvatarActor->GetActorLocation();
	if ( USkeletalMeshComponent* MeshComp = OwnerAvatarActor->FindComponentByClass<USkeletalMeshComponent>() )
	{
		MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);
	}

	APawn* OwnerPawn = Cast<APawn>(OwnerAvatarActor);
	APlayerController* PC = OwnerPawn ? OwnerPawn->GetController<APlayerController>() : nullptr;

	FVector TargetPoint;
	if ( PC )
	{
		FVector CamLoc;
		FRotator CamRot;
		PC->GetPlayerViewPoint(CamLoc, CamRot);

		FVector TraceEnd = CamLoc + CamRot.Vector() * ProjectileMaxDistance;

		FHitResult CameraHit;
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(OwnerAvatarActor);

		bool bCameraHit = GetWorld()->LineTraceSingleByChannel(CameraHit, CamLoc, TraceEnd, ECC_Pawn, QueryParams);
		TargetPoint = bCameraHit ? CameraHit.Location : TraceEnd;
	}
	else
	{
		TargetPoint = MuzzleLocation + OwnerAvatarActor->GetActorForwardVector() * ProjectileMaxDistance; 
	}

	FVector LaunchDirection = (TargetPoint - MuzzleLocation).GetSafeNormal();
	FRotator LaunchRotation = LaunchDirection.Rotation();

	int32 AbilityLevel = GetAbilityLevel(CurrentSpecHandle, CurrentActorInfo);
	float BaseDamage = GetLevelIndexedDamage(BaseDamagePerLevel, AbilityLevel);

	FGameplayEffectSpecHandle DamageSpec = MakeScaledDamageSpec(HitEffect, BaseDamage, ScalingRatio, AbilityLevel);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = OwnerAvatarActor;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ASerenityProjectile* Projectile = GetWorld()->SpawnActor<ASerenityProjectile>(ProjectileClass, MuzzleLocation, LaunchRotation, SpawnParams);
	if ( Projectile )
	{
		Projectile->ShootProjectile(ProjectileSpeed, ProjectileMaxDistance, nullptr, GetOwnerTeamId(), DamageSpec);
	}
}