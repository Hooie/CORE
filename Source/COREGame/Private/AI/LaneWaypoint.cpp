// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/LaneWaypoint.h"
#include "Components/BillboardComponent.h"
#include "Components/SceneComponent.h"

ALaneWaypoint::ALaneWaypoint()
{
	PrimaryActorTick.bCanEverTick = false;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>("Root Comp"));

	IconComponent = CreateDefaultSubobject<UBillboardComponent>("Icon Component");
	IconComponent->SetupAttachment(GetRootComponent());
	IconComponent->SetHiddenInGame(true);
}
