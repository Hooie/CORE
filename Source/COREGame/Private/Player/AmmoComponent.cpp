// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/AmmoComponent.h"
#include "Net/UnrealNetwork.h"


// Sets default values for this component's properties
UAmmoComponent::UAmmoComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = false;
}


// Called when the game starts
void UAmmoComponent::BeginPlay()
{
	Super::BeginPlay();

	if ( GetOwner() && GetOwner()->HasAuthority() )
	{
		CurrentAmmo = MaxAmmo;
	}
}

void UAmmoComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UAmmoComponent, CurrentAmmo, COND_OwnerOnly);
}

bool UAmmoComponent::TryConsumeAmmo()
{
	if ( !GetOwner() || !GetOwner()->HasAuthority() || CurrentAmmo <= 0 )
		return false;

	--CurrentAmmo;
	OnAmmoChanged.Broadcast(CurrentAmmo, MaxAmmo);

	return true;
}

void UAmmoComponent::Reload()
{
	if ( !GetOwner() || !GetOwner()->HasAuthority() )
		return;

	CurrentAmmo = MaxAmmo;
	OnAmmoChanged.Broadcast(CurrentAmmo, MaxAmmo);
}

void UAmmoComponent::OnRep_CurrentAmmo()
{
	OnAmmoChanged.Broadcast(CurrentAmmo, MaxAmmo);
}


