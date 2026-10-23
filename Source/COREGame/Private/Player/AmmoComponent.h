// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AmmoComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnAmmoChanged, int32 /*Current*/, int32 /*Max*/);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UAmmoComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UAmmoComponent();

	FOnAmmoChanged OnAmmoChanged;
	
	bool TryConsumeAmmo();
	bool IsEmpty() const { return CurrentAmmo <= 0; }
	void Reload();

	int32 GetCurrentAmmo() const { return CurrentAmmo; }
	int32 GetMaxAmmo() const { return MaxAmmo; }

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Ammo")
	int32 MaxAmmo = 10;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentAmmo)
	int32 CurrentAmmo;
	
	UFUNCTION()
	void OnRep_CurrentAmmo();
};
