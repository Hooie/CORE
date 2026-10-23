// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/AggroSubsystem.h"

void UAggroSubsystem::BroadcastThreat(AActor* Victim, AActor* Attacker)
{
	if ( !Victim || !Attacker )
		return;

	if ( UWorld* World = GetWorld() )
	{
		FAttackRecord& Record = LastAttackByAttacker.FindOrAdd(Attacker);
		Record.Victim = Victim;
		Record.Time = World->GetTimeSeconds();
	}

	OnThreatEvent.Broadcast(Victim, Attacker);
}

AActor* UAggroSubsystem::GetRecentVictim(const AActor* Attacker, float MaxAge) const
{
	const UWorld* World = GetWorld();
	if ( !Attacker || !World )
		return nullptr;

	const FAttackRecord* Record = LastAttackByAttacker.Find(const_cast< AActor* >(Attacker));
	if ( !Record || World->GetTimeSeconds() - Record->Time > MaxAge )
		return nullptr;

	return Record->Victim.Get();
}
