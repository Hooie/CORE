// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/MinionOverheadGauge.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GenericTeamAgentInterface.h"
#include "GAS/CAbilitySystemStatics.h"
#include "GAS/CAttributeSet.h"
#include "Kismet/GameplayStatics.h"
#include "Widgets/ValueGauge.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

void UMinionOverheadGauge::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	TimeSinceLastCheck += InDeltaTime;

	if ( TimeSinceLastCheck < LastHitCheckInterval )
		return;

	TimeSinceLastCheck = 0.f;
	UpdateLastHitHighlight();
}

void UMinionOverheadGauge::UpdateLastHitHighlight()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(Core_MinionLastHitHighlight);

	UAbilitySystemComponent* MinionASC = GetOwnerASC();
	AActor* MinionActor = GetOwnerActor();

	APawn* LocalPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	UAbilitySystemComponent* LocalASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(LocalPawn);
	if ( !MinionASC || !MinionActor || !LocalASC || UCAbilitySystemStatics::IsActorDead(MinionActor) )
	{
		SetLastHitHightlighted(false);
		return;
	}

	if ( bOnlyEnemyMinion )
	{
		const IGenericTeamAgentInterface* LocalTeamInterface = Cast<IGenericTeamAgentInterface>(LocalPawn);
		const IGenericTeamAgentInterface* MinionTeamInterface = Cast<IGenericTeamAgentInterface>(MinionActor);

		if ( !LocalTeamInterface || !MinionTeamInterface || LocalTeamInterface->GetGenericTeamId() == MinionTeamInterface->GetGenericTeamId() )
		{
			SetLastHitHightlighted(false);
			return;
		}
	}

	float BasicAttackDamage = 0.f;

	if ( !UCAbilitySystemStatics::GetBasicAttackDamageAgainst(LocalASC, MinionASC, BasicAttackDamage) )
	{
		SetLastHitHightlighted(false);
		return;
	}
		
	bool bFound = false;
	const float Health = MinionASC->GetGameplayAttributeValue(UCAttributeSet::GetHealthAttribute(), bFound);
	const float Shield = MinionASC->GetGameplayAttributeValue(UCAttributeSet::GetShieldAttribute(), bFound);

	SetLastHitHightlighted(Health > 0.f && Health + Shield <= BasicAttackDamage);
}

void UMinionOverheadGauge::SetLastHitHightlighted(bool bHighlighted)
{
	if ( bIsLastHitHighlighted == bHighlighted )
		return;

	bIsLastHitHighlighted = bHighlighted;

	if ( HealthBar )
	{
		HealthBar->SetHealthBarColorOverride(bHighlighted, LastHitColor);
	}
}
