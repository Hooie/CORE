// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/GA_Minion_RangedAttack.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AIController.h"
#include "AI/RangedMinionProjectile.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/SkeletalMeshComponent.h"

UGA_Minion_RangedAttack::UGA_Minion_RangedAttack()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void UGA_Minion_RangedAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if ( !K2_CommitAbility() || !AttackMontage )
	{
		K2_EndAbility();
		return;
	}

	AActor* OwnerAvatarActor = GetAvatarActorFromActorInfo();
	APawn* OwnerPawn = Cast<APawn>(OwnerAvatarActor);

	AAIController* AIController = OwnerPawn ? OwnerPawn->GetController<AAIController>() : nullptr;
	UBlackboardComponent* BlackboardComp = AIController ? AIController->GetBlackboardComponent() : nullptr;

	CachedTarget = BlackboardComp ? Cast<AActor>(BlackboardComp->GetValueAsObject(TargetBlackBoardName)) : nullptr;
	if ( !CachedTarget.IsValid() )
	{
		K2_EndAbility();
		return;
	}

	UAbilityTask_PlayMontageAndWait* PlayAttackMontage = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, AttackMontage);
	PlayAttackMontage->OnCompleted.AddDynamic(this, &UGA_Minion_RangedAttack::K2_EndAbility);
	PlayAttackMontage->OnInterrupted.AddDynamic(this, &UGA_Minion_RangedAttack::K2_EndAbility);
	PlayAttackMontage->OnCancelled.AddDynamic(this, &UGA_Minion_RangedAttack::K2_EndAbility);
	PlayAttackMontage->ReadyForActivation();

	UAbilityTask_WaitGameplayEvent* WaitFireEvent = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, GetMinionAttackFireTag());
	WaitFireEvent->EventReceived.AddDynamic(this, &UGA_Minion_RangedAttack::FireProjectile);
	WaitFireEvent->ReadyForActivation();
}

FGameplayTag UGA_Minion_RangedAttack::GetMinionAttackFireTag()
{
	return FGameplayTag::RequestGameplayTag("Ability.Minion.BasicAttack.Fire");
}

void UGA_Minion_RangedAttack::FireProjectile(FGameplayEventData PayLoad)
{
	if ( !K2_HasAuthority() || !ProjectileClass || !CachedTarget.IsValid() )
	{
		UE_LOG(LogTemp, Warning, TEXT("[RangedMinion] FireProjectile 조기 종료 - HasAuth=%d, ProjectileClass=%d, Target=%d"), 
			K2_HasAuthority(), ProjectileClass != nullptr, CachedTarget.IsValid());
		return;
	}

	AActor* OwnerAvatarActor = GetAvatarActorFromActorInfo();
	if ( !OwnerAvatarActor )
		return;

	FVector MuzzleLocation = OwnerAvatarActor->GetActorLocation();
	if ( USkeletalMeshComponent* MeshComp = OwnerAvatarActor->FindComponentByClass<USkeletalMeshComponent>() )
	{
		MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);
	}

	FVector TargetLocation = CachedTarget->GetTargetLocation();
	FVector LaunchDirection = (TargetLocation - MuzzleLocation).GetSafeNormal();

	FGameplayEffectSpecHandle DamageSpec = MakeScaledDamageSpec(HitDamageEffect, 0.f, 1.f, 1);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = OwnerAvatarActor;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ARangedMinionProjectile* Projectile = GetWorld()->SpawnActor<ARangedMinionProjectile>(ProjectileClass, MuzzleLocation, LaunchDirection.Rotation(), SpawnParams);
	if ( Projectile )
	{
		Projectile->Launch(CachedTarget.Get(), ProjectileSpeed, ProjectileMaxDistance, DamageSpec);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[RangedMinion] SpawnActor 실패!"));
	}
}