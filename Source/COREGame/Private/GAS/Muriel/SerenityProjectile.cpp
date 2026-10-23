// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Muriel/SerenityProjectile.h"
#include "GAS/CAbilitySystemStatics.h"
#include "GenericTeamAgentInterface.h"

void ASerenityProjectile::NotifyActorBeginOverlap(AActor* OtherActor)
{
	if ( OtherActor && GetTeamAttitudeTowards(*OtherActor) == ETeamAttitude::Hostile && !UCAbilitySystemStatics::IsHero(OtherActor) )
	{
		return;
	}

	Super::NotifyActorBeginOverlap(OtherActor);
}
