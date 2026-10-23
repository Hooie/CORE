// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Player/CPlayerCharacter.h"
#include "CKwangCharacter.generated.h"

class ATA_Sword;

/**
 * 
 */
UCLASS()
class ACKwangCharacter : public ACPlayerCharacter
{
	GENERATED_BODY()
	
public:
	ACKwangCharacter();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_ToggleSwordVisibility(bool bVisible);
	
	bool HasSword() const;

	void SetSpawnedSword(ATA_Sword* Sword);
	ATA_Sword* GetSpawnedSword() const { return SpawnedSword; }
	
protected:
#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* SwordMeshComp;

	UPROPERTY(EditDefaultsOnly, Category = "Mesh")
	FName SwordAttachSocketName;

	UPROPERTY()
	ATA_Sword* SpawnedSword;

	/***************************************************************/
	/*                            Stun                             */
	/***************************************************************/
	virtual void OnStun() override;
	virtual void OnRecoveryFromStun() override;

	/***************************************************************/
	/*                            Bind                             */
	/***************************************************************/
	virtual void OnBind() override;
	virtual void OnRecoveryFromBind() override;

	/***************************************************************/
	/*                     Death And Respawn                       */
	/***************************************************************/
	virtual void OnDead() override;
	virtual void OnRespawn() override;
};
