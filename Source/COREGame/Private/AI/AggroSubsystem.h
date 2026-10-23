// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AggroSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnThreatEvent, AActor* /*Victim*/, AActor* /*Attacker*/);

/**
 * 
 */
UCLASS()
class UAggroSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:
	FOnThreatEvent OnThreatEvent;

	void BroadcastThreat(AActor* Victim, AActor* Attacker);	

	AActor* GetRecentVictim(const AActor* Attacker, float MaxAge) const;

private:
	struct FAttackRecord
	{
		TWeakObjectPtr<AActor> Victim;
		float Time = 0.f;
	};

	TMap<TWeakObjectPtr<AActor>, FAttackRecord> LastAttackByAttacker;
};
