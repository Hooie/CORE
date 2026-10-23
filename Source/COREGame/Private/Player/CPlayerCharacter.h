// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/CCharacter.h"
#include "GAS/CGameplayAbilityTypes.h"
#include "InputActionValue.h"
#include "Navigation/CrowdAgentInterface.h"
#include "CPlayerCharacter.generated.h"

/**
 * 
 */
UCLASS()
class ACPlayerCharacter : public ACCharacter, public ICrowdAgentInterface
{
	GENERATED_BODY()
	
public:
	ACPlayerCharacter();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void PawnClientRestart() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetActorEyesViewPoint(FVector& OutLocation, FRotator& OutRotation) const override;

	UFUNCTION(BlueprintCallable)
	UAnimMontage* GetHitReactionMontage() const { return HitReactionMontage; }

	/***************************************************************/
	/*                   ICrowd Agent Interface                    */
	/***************************************************************/
	virtual FVector GetCrowdAgentLocation() const override;
	virtual FVector GetCrowdAgentVelocity() const override;
	virtual void GetCrowdAgentCollisions(float& CylinderRadius, float& CylinderHalfHeight) const override;
	virtual float GetCrowdAgentMaxSpeed() const override;

private:
	void SetCrowdObstacleRegistered(bool bRegister);
	bool bRegisteredAsCrowdObstacle = false;

	/***************************************************************/
	/*                            View                             */
	/***************************************************************/
private:
	UPROPERTY(VisibleDefaultsOnly, Category = "View")
	class USpringArmComponent* CameraBoom;
	 
	UPROPERTY(VisibleDefaultsOnly, Category = "View")
	class UCameraComponent* ViewCam;

	FVector GetLookRightDir() const;
	FVector GetLookForwardDir() const;
	FVector GetMoveFowardDir() const;

	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode = 0) override;

	/***************************************************************/
	/*                      Gameplay Ability                       */
	/***************************************************************/
public:
	void GrantInBaseTag();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SetChronoLensVisuals(bool bActive);

private:
	UPROPERTY(VisibleAnywhere, Category = "VFX")
	class UStaticMeshComponent* ChronoLensBubbleComponent;

	bool bChronoLensBubbleActive = false;
	float ChronoLensBubbleElapsed = 0.f;

	FTimerHandle ChronoLensBubbleTimerHandle;

	void TickChronoLensBubble();

	UPROPERTY()
	class UCHeroAttributeSet* HeroAtrributeSet;

	UPROPERTY(EditDefaultsOnly, Category = "Hit")
	UAnimMontage* HitReactionMontage;

	/***************************************************************/
	/*                           Input                             */
	/***************************************************************/
public:
	void SetMoveInputBlock(bool bBlocked) { bBlockMoveInput = bBlocked; }
	bool IsMoveInputBlocked() const { return bBlockMoveInput; }
	
	void SetCameraInputBlock(bool bBlocked) { bBlockCameraInput = bBlocked; }
	bool IsCameraInputBlocked() const { return bBlockCameraInput; }

	void SetBlockBasicAttack(bool bBlocked) { bBlockBasicAttack = bBlocked; }
	bool GetbBlockBasicAttack() const { return bBlockBasicAttack; }

	void SetBlockAbilityInput(bool bBlocked) { bBlockAbilityInput = bBlocked; }
	bool GetbBlockAbilityInput() const { return bBlockAbilityInput; }

	virtual void DisableJumpTagUpdated(const FGameplayTag Tag, int32 NewCount) override;
	virtual void Jump() override;

private:
	class ACPlayerController* CachedPlayerController;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* JumpInputAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* LookInputAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* MoveInputAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* LearnAbilityLeaderAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* UseInventoryItemAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TMap<ECAbilityInputID, class UInputAction*> GameplayAbilityInputActions;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputMappingContext* GameplayInputMappingContext;

	void HandleLookInput(const FInputActionValue& InputActionValue);
	void HandleMoveInput(const FInputActionValue& InputActionValue);
	void LearnAbilityLeaderDown(const FInputActionValue& InputActionValue);
	void LearnAbilityLeaderUp(const FInputActionValue& InputActionValue);
	bool bIsLearningAbilityLeaderDown = false;

	void HandleAbilityInput(const FInputActionValue& InputActionValue, ECAbilityInputID InputID);

	void HandleAbilityInputPressed(const FInputActionValue&, ECAbilityInputID InputID);
	void HandleAbilityInputReleased(const FInputActionValue&, ECAbilityInputID InputID);
	
	void UseInventoryItemLeaderDown(const FInputActionValue& InputActionValue);
	void UseInventoryItemLeaderUp(const FInputActionValue& InputActionValue);
	void UseInventoryItem(const FInputActionValue& InputActionValue);
	bool bIsUsingItemLeaderDown = false;

	void SetInputEnabledFromPlayerController(bool bEnabled);

	bool bBlockMoveInput = false;
	bool bBlockBasicAttack = false;
	bool bBlockCameraInput = false;
	bool bBlockJumpInput = false;
	bool bBlockAbilityInput = false;

	float CachedMouseSensitivity = 1.f;

	UFUNCTION()
	void MouseSensitivityChanged(float NewSensitivity);

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

	/***************************************************************/
	/*                         Camera View                         */
	/***************************************************************/
private:
	virtual void OnBasicAttackAimStateChanged(bool bIsAiming) override;
	virtual void OnSkillAimStateChanged(bool bIsAiming) override;

	UPROPERTY(EditDefaultsOnly, Category = "View")
	FVector BasicAttackCameraOffset;

	UPROPERTY(EditDefaultsOnly, Category = "View")
	FVector SkillCameraOffset;

	UPROPERTY(EditDefaultsOnly, Category = "View")
	float CameraLerpSpeed = 20.f;

	FTimerHandle CameraLerpTimerHandle;

	void LerpCameraToLocalOffsetLocation(const FVector& Goal);
	void TickCameraLocalOffsetLerp(FVector Goal);

	/***************************************************************/
	/*                          Inventory                          */
	/***************************************************************/
public:
	class UInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

private:
	class UInventoryComponent* InventoryComponent;
};
