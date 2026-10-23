// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LaneWaypoint.generated.h"

UCLASS()
class ALaneWaypoint : public AActor
{
	GENERATED_BODY()
	
public:
	ALaneWaypoint();

private:
	UPROPERTY(VisibleAnywhere, Category = "Waypoint")
	class UBillboardComponent* IconComponent;
};
