// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/GAP_Dead.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AI/Turret/Turret.h"
#include "Character/CCharacter.h"
#include "Engine/OverlapResult.h"
#include "GAS/CAbilitySystemStatics.h"
#include "GAS/CAttributeSet.h"
#include "GAS/CHeroAttributeSet.h"
#include "GAS/Items/CItemStatics.h"
#include "Framework/CGameState.h"
#include "Player/KillFeedTypes.h"
#include "Player/CPlayerState.h"

UGAP_Dead::UGAP_Dead()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	TriggerData.TriggerTag = UCAbilitySystemStatics::GetDeadStatusTag();
	AbilityTriggers.Add(TriggerData);

	ActivationBlockedTags.RemoveTag(UCAbilitySystemStatics::GetStunStatusTag());
	ActivationBlockedTags.RemoveTag(UCAbilitySystemStatics::GetDeadStatusTag());
}

void UGAP_Dead::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if ( K2_HasAuthority() )
	{
		AActor* AvatarActor = GetAvatarActorFromActorInfo(); // 피해자
		bool bVictimIsHero = UCAbilitySystemStatics::IsHero(AvatarActor);
		ACCharacter* VictimCharacter = Cast<ACCharacter>(AvatarActor);
		ATurret* VictimTurret = Cast<ATurret>(AvatarActor);

		AActor* RawKiller = TriggerEventData->ContextHandle.GetEffectCauser();

		if ( RawKiller && !UCAbilitySystemStatics::IsHero(RawKiller) )
		{
			AActor* LastDamagingHero = nullptr;

			if ( bVictimIsHero && VictimCharacter )
			{
				LastDamagingHero = VictimCharacter->GetLastDamagingHeroIfRecent();
			}
			else if ( VictimTurret )
			{
				LastDamagingHero = VictimTurret->GetLastDamagingHeroIfRecent();
			}

			if ( LastDamagingHero )
			{
				RawKiller = LastDamagingHero;
			}
		}

		const bool bExecuted = bVictimIsHero && RawKiller && !UCAbilitySystemStatics::IsHero(RawKiller);

		if ( bVictimIsHero )
		{
			if ( ACPlayerState* VictimPS = UCAbilitySystemStatics::GetHeroPlayerState(AvatarActor) )
			{
				VictimPS->AddDeath();
			}

			if ( RawKiller && UCAbilitySystemStatics::IsHero(RawKiller) )
			{
				if ( ACPlayerState* KillerPS = UCAbilitySystemStatics::GetHeroPlayerState(RawKiller) )
				{
					KillerPS->AddKill();
				}
			}
		}
		else if( !VictimTurret )
		{
			if ( RawKiller && UCAbilitySystemStatics::IsHero(RawKiller) )
			{
				if ( ACPlayerState* KillerPS = UCAbilitySystemStatics::GetHeroPlayerState(RawKiller) )
				{
					KillerPS->AddMinionKill();
				}
			}
		}

		AActor* Killer = (RawKiller && UCAbilitySystemStatics::IsHero(RawKiller)) ? RawKiller : nullptr;

		UE_LOG(LogTemp, Verbose, TEXT("[GAP_Dead] Victim=%s (IsHero=%d) | RawKiller=%s | Killer=%s | Executed=%d"),
			*AvatarActor->GetName(), bVictimIsHero,
			RawKiller ? *RawKiller->GetName() : TEXT("None"),
			Killer ? *Killer->GetName() : TEXT("None"),
			bExecuted);

		// 처형이면 어시스트 목록 추가 X
		TArray<AActor*> AssistTargets;
		if ( bVictimIsHero && VictimCharacter && !bExecuted)
		{
			AssistTargets = VictimCharacter->GetRecentAssistContributors(Killer);
		}

		if ( bVictimIsHero && Killer )
		{
			if ( ACCharacter* KillerCharacter = Cast<ACCharacter>(Killer) )
			{
				TArray<AActor*> ShieldContributors = KillerCharacter->GetRecentShieldContributors(Killer);
				for ( AActor* ShieldProvider : ShieldContributors )
				{
					AssistTargets.AddUnique(ShieldProvider);
				}
			}
		}

		if ( bVictimIsHero || (VictimTurret && RawKiller) )
		{
			if ( ACGameState* CGameState = GetWorld()->GetGameState<ACGameState>() )
			{
				FKillFeedEntry Entry;
				UCAbilitySystemStatics::GetKillFeedIdentity(RawKiller, Entry.KillerName, Entry.KillerIcon, Entry.KillerTeamId);
				UCAbilitySystemStatics::GetKillFeedIdentity(AvatarActor, Entry.VictimName, Entry.VictimIcon, Entry.VictimTeamId);

				for ( AActor* Assister : AssistTargets )
				{
					FString UnusedName;
					TSoftObjectPtr<UTexture2D> AssistIcon;
					uint8 UnusedTeamId;
					UCAbilitySystemStatics::GetKillFeedIdentity(Assister, UnusedName, AssistIcon, UnusedTeamId);
					Entry.AssistIcons.Add(AssistIcon);
				}

				CGameState->Multicast_BroadcastKillFeed(Entry);
			}
		}

		for ( AActor* Assister : AssistTargets )
		{
			if ( ACPlayerState* AssistPS = UCAbilitySystemStatics::GetHeroPlayerState(Assister) )
			{
				AssistPS->AddAssist();
			}
		}

		if ( VictimTurret )
		{
			TArray<AActor*> StructureGoldTargets;

			for ( AActor* Nearby : GetRewardTargets() )
			{
				const ACCharacter* NearbyCharacter = Cast<ACCharacter>(Nearby);
				if ( NearbyCharacter && NearbyCharacter->IsDead() )
					continue;

				StructureGoldTargets.Add(Nearby);
			}

			if ( Killer )
			{
				StructureGoldTargets.AddUnique(Killer);
			}

			if ( StructureGoldTargets.Num() > 0 )
			{
				const float GoldPerTarget = BaseGoldReward / StructureGoldTargets.Num();

				FGameplayEffectSpecHandle StructureGoldSpec = MakeOutgoingGameplayEffectSpec(RewardEffect);
				StructureGoldSpec.Data->SetSetByCallerMagnitude(UCAbilitySystemStatics::GetExperienceAttributeTag(), 0.f);
				StructureGoldSpec.Data->SetSetByCallerMagnitude(UCAbilitySystemStatics::GetGoldAttributeTag(), GoldPerTarget);

				K2_ApplyGameplayEffectSpecToTarget(StructureGoldSpec, UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActorArray(StructureGoldTargets, true));
			}

			K2_EndAbility();

			return;
		}


		TArray<AActor*> ExperienceTargets; // 경험치를 받을 전원
		TArray<AActor*> MinionNonKillerGoldTargets; // 미니언 사망 전용 — 근처 비킬러
		TArray<AActor*> AssistGoldTargets; // 영웅 사망 전용 — 골드를 받을 어시스트한 영웅 

		if ( !bVictimIsHero )
		{
			ExperienceTargets = GetRewardTargets();
			if ( Killer && !ExperienceTargets.Contains(Killer) )
			{
				ExperienceTargets.Add(Killer);
			}

			for ( AActor* Target : ExperienceTargets )
			{
				if ( Target != Killer )
				{
					MinionNonKillerGoldTargets.Add(Target);
				}
			}
		}
		else
		{
			TSet<AActor*> ExpSet;
			if ( Killer )
			{
				ExpSet.Add(Killer);
			}
			for ( AActor* Assister : AssistTargets )
			{
				ExpSet.Add(Assister);
			}
			for ( AActor* Nearby : GetRewardTargets() ) 
			{
				ExpSet.Add(Nearby);
			}
			ExperienceTargets = ExpSet.Array();
			AssistGoldTargets = AssistTargets; // 어시스트 골드는 데미지를 준 사람만
		}

		if ( ExperienceTargets.Num() == 0 && !Killer )
		{
			K2_EndAbility();
			return;
		}

		bool bFound = false;
		float SelfExperience = GetAbilitySystemComponentFromActorInfo_Ensured()->GetGameplayAttributeValue(UCHeroAttributeSet::GetExperienceAttribute(), bFound);

		float TotalExperienceReward = BaseExperienceReward + ExperienceRewardPerExperience * SelfExperience;
		float TotalGoldReward = BaseGoldReward + GoldRewardPerExperience * SelfExperience;

		if ( !bVictimIsHero && ExperienceTargets.Num() > 0 )
		{
			float SharePercent = GetMinionSharePercentForCount(ExperienceTargets.Num());
			float PoolMultiplier = (SharePercent * ExperienceTargets.Num()) / 100.f;
			TotalExperienceReward *= PoolMultiplier;
		}

		// 킬러 골드 독점
		if ( Killer )
		{
			const float KillerGold = TotalGoldReward + UCItemStatics::GetBonusKillGold(Killer, bVictimIsHero);

			FGameplayEffectSpecHandle KillerGoldSpec = MakeOutgoingGameplayEffectSpec(RewardEffect);
			KillerGoldSpec.Data->SetSetByCallerMagnitude(UCAbilitySystemStatics::GetExperienceAttributeTag(), 0.f);
			KillerGoldSpec.Data->SetSetByCallerMagnitude(UCAbilitySystemStatics::GetGoldAttributeTag(), KillerGold);

			K2_ApplyGameplayEffectSpecToTarget(KillerGoldSpec, UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActor(Killer));
			
			if ( bVictimIsHero )
			{
				UCItemStatics::HandleHeroKill(Killer);
			}

			UE_LOG(LogTemp, Verbose, TEXT("[GAP_Dead] 킬러 골드 독점 지급: %s -> Gold=%.1f"), *Killer->GetName(), TotalGoldReward);
		}

		// 경험치 n분할 (전원)
		if ( ExperienceTargets.Num() > 0 )
		{
			
			float ExperiencePerTarget = TotalExperienceReward / ExperienceTargets.Num();

			FGameplayEffectSpecHandle ExperienceSpec = MakeOutgoingGameplayEffectSpec(RewardEffect);
			ExperienceSpec.Data->SetSetByCallerMagnitude(UCAbilitySystemStatics::GetExperienceAttributeTag(), ExperiencePerTarget);
			ExperienceSpec.Data->SetSetByCallerMagnitude(UCAbilitySystemStatics::GetGoldAttributeTag(), 0.f);

			K2_ApplyGameplayEffectSpecToTarget(ExperienceSpec, UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActorArray(ExperienceTargets, true));

			UE_LOG(LogTemp, Verbose, TEXT("[GAP_Dead] 경험치 n분할: 대상=%d명, 1인당 EXP=%.1f"), ExperienceTargets.Num(), ExperiencePerTarget);
		}

		// 미니언 사망 — 근처 비킬러 고정 골드 (기존 그대로)
		if ( !bVictimIsHero && NonKillerFlatGoldBonus > 0.f && MinionNonKillerGoldTargets.Num() > 0 )
		{
			FGameplayEffectSpecHandle FlatGoldSpec = MakeOutgoingGameplayEffectSpec(RewardEffect);
			FlatGoldSpec.Data->SetSetByCallerMagnitude(UCAbilitySystemStatics::GetExperienceAttributeTag(), 0.f);
			FlatGoldSpec.Data->SetSetByCallerMagnitude(UCAbilitySystemStatics::GetGoldAttributeTag(), NonKillerFlatGoldBonus);

			K2_ApplyGameplayEffectSpecToTarget(FlatGoldSpec, UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActorArray(MinionNonKillerGoldTargets, true));

			UE_LOG(LogTemp, Verbose, TEXT("[GAP_Dead] 비킬러 고정 골드(+%.1f) 지급 대상: %d명"), NonKillerFlatGoldBonus, MinionNonKillerGoldTargets.Num());
		}

		if ( !bVictimIsHero )
		{

			for ( AActor* Receiver : MinionNonKillerGoldTargets )
			{
				const float NonKillerBonusGold = UCItemStatics::GetBonusNonKillerGold(Receiver);
				if ( NonKillerBonusGold <= 0.f )
					continue;

				FGameplayEffectSpecHandle BonusGoldSpec = MakeOutgoingGameplayEffectSpec(RewardEffect);
				BonusGoldSpec.Data->SetSetByCallerMagnitude(UCAbilitySystemStatics::GetExperienceAttributeTag(), 0.f);
				BonusGoldSpec.Data->SetSetByCallerMagnitude(UCAbilitySystemStatics::GetGoldAttributeTag(), NonKillerBonusGold);

				K2_ApplyGameplayEffectSpecToTarget(BonusGoldSpec, UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActor(Receiver));
			}
		}

		// 영웅 사망 시, 어시스트한 영웅들에게 어시스트 골드(킬 골드의 절반 n등분)
		if ( bVictimIsHero && AssistGoldTargets.Num() > 0 )
		{
			float AssistGoldPool = TotalGoldReward * AssistGoldPoolRatio;
			float AssistGoldPerTarget = AssistGoldPool / AssistGoldTargets.Num();

			FGameplayEffectSpecHandle AssistGoldSpec = MakeOutgoingGameplayEffectSpec(RewardEffect);
			AssistGoldSpec.Data->SetSetByCallerMagnitude(UCAbilitySystemStatics::GetExperienceAttributeTag(), 0.f);
			AssistGoldSpec.Data->SetSetByCallerMagnitude(UCAbilitySystemStatics::GetGoldAttributeTag(), AssistGoldPerTarget);

			K2_ApplyGameplayEffectSpecToTarget(AssistGoldSpec, UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActorArray(AssistGoldTargets, true));

			UE_LOG(LogTemp, Verbose, TEXT("[GAP_Dead] 어시스트 골드 풀=%.1f, 대상=%d명, 1인당=%.1f"), AssistGoldPool, AssistGoldTargets.Num(), AssistGoldPerTarget);
		}

		K2_EndAbility();
	}
}

TArray<AActor*> UGAP_Dead::GetRewardTargets() const
{
	TSet<AActor*> OutActors;

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!AvatarActor || !GetWorld())
	{
		return OutActors.Array();
	}

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
	FCollisionShape CollisionShape;
	CollisionShape.SetSphere(RewardRange);

	TArray<FOverlapResult> OverlapResults;
	if (GetWorld()->OverlapMultiByObjectType(OverlapResults, AvatarActor->GetActorLocation(), FQuat::Identity, ObjectQueryParams, CollisionShape))
	{
		for (const FOverlapResult& OverlapResult : OverlapResults)
		{
			const IGenericTeamAgentInterface* OtherTeamInterface = Cast<IGenericTeamAgentInterface>(OverlapResult.GetActor());
			if (!OtherTeamInterface || OtherTeamInterface->GetTeamAttitudeTowards(*AvatarActor) != ETeamAttitude::Hostile)
			{
				continue;
			}

			if (!UCAbilitySystemStatics::IsHero(OverlapResult.GetActor()))
			{
				continue;
			}

			OutActors.Add(OverlapResult.GetActor());
		}
	}

	return OutActors.Array();
}

float UGAP_Dead::GetMinionSharePercentForCount(int32 PlayerCount) const
{
	if ( MinionSharePercentPerPlayerCount.Num() == 0 || PlayerCount <= 0 )
		return 100.f;

	int32 Index = FMath::Clamp(PlayerCount - 1, 0, MinionSharePercentPerPlayerCount.Num() - 1);

	return MinionSharePercentPerPlayerCount[ Index ];
}
