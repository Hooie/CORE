// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/Turret/Nexus.h"
#include "Camera/CameraComponent.h"
#include "Net/UnrealNetwork.h"

ANexus::ANexus()
{
	ViewCam = CreateDefaultSubobject<UCameraComponent>("View Cam");
	ViewCam->SetupAttachment(GetRootComponent());

	KillFeedDisplayName = TEXT("넥서스");
}

void ANexus::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANexus, bExplosionStarted);
}

void ANexus::StartExplosion()
{
	if ( !HasAuthority() || bExplosionStarted )
		return;

	bExplosionStarted = true;
	OnRep_ExplosionStarted();
}

void ANexus::DestroyTower()
{
	if ( IsTowerDestroyed() )
		return;

	Super::DestroyTower();

	if ( HasAuthority() )
	{
		OnNexusDestroyed.Broadcast(this);
	}
}

void ANexus::PlayDestroyedPresentation()
{
	// 당장 재생하지 않음. 카메라 이동이 완료된 후 StartExplosion에서 OnRep_ExplosionStarted 호출 후 재생
}

void ANexus::OnRep_ExplosionStarted()
{
	if ( bExplosionStarted )
	{
		Super::PlayDestroyedPresentation();
	}
}
