// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Items/GA_Item_StarlitCrown.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Engine/OverlapResult.h"
#include "GAS/CAbilitySystemStatics.h"
#include "GenericTeamAgentInterface.h"

UGA_Item_StarlitCrown::UGA_Item_StarlitCrown()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGA_Item_StarlitCrown::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if ( !HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo) || !K2_CommitAbility() )
	{
		K2_EndAbility();
		return;
	}

	AActor* OwnerAvatarActor = GetAvatarActorFromActorInfo();
	UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo();

	if ( !OwnerAvatarActor || !OwnerASC )
	{
		K2_EndAbility();
		return;
	}

	// 시각 효과 — 모든 클라이언트에 전파
	FGameplayCueParameters CueParams;
	CueParams.Location = OwnerAvatarActor->GetActorLocation();
	CueParams.RawMagnitude = ShieldRadius;
	OwnerASC->ExecuteGameplayCue(GetStarlitCrownCueTag(), CueParams);

	if ( !K2_HasAuthority() )
	{
		K2_EndAbility();
		return;
	}

	if ( !ShieldGrantEffect || !ShieldBuffIconEffect )
	{
		K2_EndAbility();
		return;
	}

	int32 AbilityLevel = GetAbilityLevel(CurrentSpecHandle, CurrentActorInfo);

	// 본인에게 먼저 적용 — 오버랩 결과에 자기 자신이 안 잡힐 수도 있으므로 별도 처리
	if ( bIncludeSelf )
	{
		UCAbilitySystemStatics::ApplyTimedShield(OwnerASC, OwnerASC, ShieldGrantEffect, ShieldBuffIconEffect, AbilityLevel, ShieldDuration);
	}

	TArray<FOverlapResult> Overlaps;
	FCollisionShape Sphere = FCollisionShape::MakeSphere(ShieldRadius);
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(OwnerAvatarActor);

	GetWorld()->OverlapMultiByObjectType(Overlaps, OwnerAvatarActor->GetActorLocation(), FQuat::Identity, FCollisionObjectQueryParams(ECC_Pawn), Sphere, Params);

	IGenericTeamAgentInterface* OwnerTeamAgent = Cast<IGenericTeamAgentInterface>(OwnerAvatarActor);
	TSet<AActor*> Processed;

	for ( const FOverlapResult& Overlap : Overlaps )
	{
		AActor* OtherActor = Overlap.GetActor();

		if ( !OtherActor || Processed.Contains(OtherActor) )
			continue;

		Processed.Add(OtherActor);

		if ( !UCAbilitySystemStatics::IsHero(OtherActor) || UCAbilitySystemStatics::IsActorDead(OtherActor) )
			continue;

		IGenericTeamAgentInterface* OtherTeamAgent = Cast<IGenericTeamAgentInterface>(OtherActor);
		if ( !OtherTeamAgent || !OwnerTeamAgent )
			continue;

		if ( OwnerTeamAgent->GetTeamAttitudeTowards(*OtherActor) == ETeamAttitude::Hostile )
			continue; // 아군만

		UAbilitySystemComponent* OtherASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor);
		if ( !OtherASC )
			continue;

		UCAbilitySystemStatics::ApplyTimedShield(OwnerASC, OtherASC, ShieldGrantEffect, ShieldBuffIconEffect, AbilityLevel, ShieldDuration);
	}

	K2_EndAbility();
}

FGameplayTag UGA_Item_StarlitCrown::GetStarlitCrownCueTag()
{
	return FGameplayTag::RequestGameplayTag("GameplayCue.Item.StarlitCrown");
}
