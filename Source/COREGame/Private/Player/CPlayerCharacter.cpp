// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/CPlayerCharacter.h"
#include "COREGame/COREGame.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Framework/CGameUserSettings.h"
#include "GAS/CAbilitySystemStatics.h"
#include "GAS/CHeroAttributeSet.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PlayerController.h"
#include "Inventory/InventoryComponent.h"
#include "Navigation/CrowdManager.h"
#include "Player/CPlayerController.h"

ACPlayerCharacter::ACPlayerCharacter()
{
	bAlwaysRelevant = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>("Camera Boom");
	CameraBoom->SetupAttachment(GetRootComponent());
	CameraBoom->TargetArmLength = 600.f;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->ProbeChannel = ECC_SpringArm;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->bEnableCameraRotationLag = true;
	CameraBoom->CameraLagSpeed = 25.f;
	CameraBoom->CameraRotationLagSpeed = 40.f;

	ViewCam = CreateDefaultSubobject<UCameraComponent>("View Cam");
	ViewCam->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	ViewCam->FieldOfView = 95.f;
	
	ChronoLensBubbleComponent = CreateDefaultSubobject<UStaticMeshComponent>("ChronoLensBubble");
	ChronoLensBubbleComponent->SetupAttachment(GetRootComponent());
	ChronoLensBubbleComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ChronoLensBubbleComponent->SetCastShadow(false);
	ChronoLensBubbleComponent->SetVisibility(false);

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0, 720.f, 0.f);

	if ( UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponent() )
	{
		OwnerASC->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	}

	HeroAtrributeSet = CreateDefaultSubobject<UCHeroAttributeSet>("Hero Attribute Set");

	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>("Inventory Component");
}

void ACPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	SetCrowdObstacleRegistered(true);
	
	if ( UCGameUserSettings* Settings = UCGameUserSettings::Get() )
	{
		CachedMouseSensitivity = Settings->MouseSensitivity;

		Settings->OnMouseSensitivityChanged.AddUObject(this, &ACPlayerCharacter::MouseSensitivityChanged);
	}
}

void ACPlayerCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SetCrowdObstacleRegistered(false);

	Super::EndPlay(EndPlayReason);
}

void ACPlayerCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();

	CachedPlayerController = GetController<ACPlayerController>();

	APlayerController* OwningPlayerController = GetController<APlayerController>();
	if (OwningPlayerController)
	{
		UEnhancedInputLocalPlayerSubsystem* InputSubsystem = OwningPlayerController->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();

		if (InputSubsystem)
		{
			InputSubsystem->RemoveMappingContext(GameplayInputMappingContext);
			InputSubsystem->AddMappingContext(GameplayInputMappingContext, 0);
		}
	}
}

void ACPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInputComp = Cast<UEnhancedInputComponent>(PlayerInputComponent);

	if (EnhancedInputComp)
	{
		EnhancedInputComp->BindAction(JumpInputAction, ETriggerEvent::Triggered, this, &ACPlayerCharacter::Jump);
		EnhancedInputComp->BindAction(LookInputAction, ETriggerEvent::Triggered, this, &ACPlayerCharacter::HandleLookInput);
		EnhancedInputComp->BindAction(MoveInputAction, ETriggerEvent::Triggered, this, &ACPlayerCharacter::HandleMoveInput);

		EnhancedInputComp->BindAction(LearnAbilityLeaderAction, ETriggerEvent::Started, this, &ACPlayerCharacter::LearnAbilityLeaderDown);
		EnhancedInputComp->BindAction(LearnAbilityLeaderAction, ETriggerEvent::Completed, this, &ACPlayerCharacter::LearnAbilityLeaderUp);

		for (const TPair<ECAbilityInputID, UInputAction*>& InputActionPair : GameplayAbilityInputActions)
		{
			EnhancedInputComp->BindAction(InputActionPair.Value, ETriggerEvent::Triggered, this, &ACPlayerCharacter::HandleAbilityInput, InputActionPair.Key);
		}

		EnhancedInputComp->BindAction(UseInventoryItemAction, ETriggerEvent::Triggered, this, &ACPlayerCharacter::UseInventoryItem);
	}
}

void ACPlayerCharacter::GetActorEyesViewPoint(FVector& OutLocation, FRotator& OutRotation) const
{
	OutLocation = ViewCam->GetComponentLocation();
	OutRotation = GetBaseAimRotation();
}

FVector ACPlayerCharacter::GetCrowdAgentLocation() const
{
	return GetNavAgentLocation();
}

FVector ACPlayerCharacter::GetCrowdAgentVelocity() const
{
	return GetVelocity();
}

void ACPlayerCharacter::GetCrowdAgentCollisions(float& CylinderRadius, float& CylinderHalfHeight) const
{
	GetCapsuleComponent()->GetScaledCapsuleSize(CylinderRadius, CylinderHalfHeight);
}

float ACPlayerCharacter::GetCrowdAgentMaxSpeed() const
{
	return GetCharacterMovement() ? GetCharacterMovement()->GetMaxSpeed() : 0.f;
}

void ACPlayerCharacter::SetCrowdObstacleRegistered(bool bRegister)
{
	if ( !HasAuthority() || bRegisteredAsCrowdObstacle == bRegister )
		return;

	UCrowdManager* CrowdManager = UCrowdManager::GetCurrent(this);
	if ( !CrowdManager )
		return;

	if ( bRegister )
	{
		CrowdManager->RegisterAgent(this);
	}
	else
	{
		CrowdManager->UnregisterAgent(this);
	}

	bRegisteredAsCrowdObstacle = bRegister;
}

void ACPlayerCharacter::HandleLookInput(const FInputActionValue& InputActionValue)
{
	if ( IsCameraInputBlocked() )
		return;

	FVector2D InputVal = InputActionValue.Get<FVector2D>();

	AddControllerPitchInput(-InputVal.Y * CachedMouseSensitivity);
	AddControllerYawInput(InputVal.X * CachedMouseSensitivity);
}

void ACPlayerCharacter::HandleMoveInput(const FInputActionValue& InputActionValue)
{
	if (GetIsInFocusMode() || IsMoveInputBlocked())
		return;

	if ( UAbilitySystemComponent* ASC = GetAbilitySystemComponent() )
	{
		FGameplayTagContainer CancelTags;
		CancelTags.AddTag(FGameplayTag::RequestGameplayTag("Ability.Type.Recalling"));
		ASC->CancelAbilities(&CancelTags, nullptr, nullptr);
	}

	FVector2D InputVal = InputActionValue.Get<FVector2D>();

	FVector ForwardDir;
	FVector RightDir;

	if (GetCharacterMovement()->IsFalling())
	{
		ForwardDir = GetLookForwardDir();
		RightDir = GetLookRightDir();
	}
	else
	{
		FRotator YawOnly(0.f, ViewCam->GetComponentRotation().Yaw, 0.f);
		ForwardDir = FRotationMatrix(YawOnly).GetScaledAxis(EAxis::X);
		RightDir = FRotationMatrix(YawOnly).GetScaledAxis(EAxis::Y);
	}

	AddMovementInput(ForwardDir * InputVal.Y + RightDir * InputVal.X);
}

void ACPlayerCharacter::LearnAbilityLeaderDown(const FInputActionValue& InputActionValue)
{
	bIsLearningAbilityLeaderDown = true;
}

void ACPlayerCharacter::LearnAbilityLeaderUp(const FInputActionValue& InputActionValue)
{
	bIsLearningAbilityLeaderDown = false;
}

void ACPlayerCharacter::HandleAbilityInput(const FInputActionValue& InputActionValue, ECAbilityInputID InputID)
{
	if ( bBlockAbilityInput )
		return;

	bool bPressed = InputActionValue.Get<bool>();

	if (bPressed && bIsLearningAbilityLeaderDown)
	{
		if( InputID == ECAbilityInputID::AbilityOne || InputID == ECAbilityInputID::AbilityTwo || InputID == ECAbilityInputID::AbilityThree || InputID == ECAbilityInputID::Ultimate )
		{
			UpgradeAbilityWithInputID(InputID);
		}

		return;
	}

	if ( bPressed && bIsUsingItemLeaderDown )
	{
		UseInventoryItem(InputActionValue);

		return;
	}

	if ( InputID != ECAbilityInputID::Recall )
	{
		if ( UAbilitySystemComponent* ASC = GetAbilitySystemComponent() )
		{
			if ( ASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag("Ability.Type.Recalling")) )
			{
				FGameplayTagContainer CancelTags;
				CancelTags.AddTag(FGameplayTag::RequestGameplayTag("Ability.Type.Recalling"));
				ASC->CancelAbilities(&CancelTags, nullptr, nullptr);
			}
		}
	}

	if (bPressed)
	{
		GetAbilitySystemComponent()->AbilityLocalInputPressed((int32)InputID);
	}
	else
	{
		GetAbilitySystemComponent()->AbilityLocalInputReleased((int32)InputID);
	}

	if ( bBlockBasicAttack )
		return;

	if ( InputID == ECAbilityInputID::BasicAttack )
	{
		FGameplayTag BasicAttackTag = bPressed ? UCAbilitySystemStatics::GetBasicAttackInputPressedTag() : UCAbilitySystemStatics::GetBasicAttackInputReleasedTag();

		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, BasicAttackTag, FGameplayEventData());
		Server_SendGameplayEventToSelf(BasicAttackTag, FGameplayEventData());
	}
}

void ACPlayerCharacter::HandleAbilityInputPressed(const FInputActionValue&, ECAbilityInputID InputID)
{
	GetAbilitySystemComponent()->AbilityLocalInputPressed((int32)InputID);
}

void ACPlayerCharacter::HandleAbilityInputReleased(const FInputActionValue&, ECAbilityInputID InputID)
{
	GetAbilitySystemComponent()->AbilityLocalInputReleased((int32)InputID);
}

void ACPlayerCharacter::UseInventoryItemLeaderDown(const FInputActionValue& InputActionValue)
{
	bIsUsingItemLeaderDown = true;
}

void ACPlayerCharacter::UseInventoryItemLeaderUp(const FInputActionValue& InputActionValue)
{
	bIsUsingItemLeaderDown = false;
}

void ACPlayerCharacter::UseInventoryItem(const FInputActionValue& InputActionValue)
{
	int Value = FMath::RoundToInt(InputActionValue.Get<float>());
	InventoryComponent->TryActivateItemInSlot(Value - 1);
}

void ACPlayerCharacter::SetInputEnabledFromPlayerController(bool bEnabled)
{
	APlayerController* PlayerController = GetController<APlayerController>();
	
	if (!PlayerController)
		return;

	if (bEnabled)
	{
		EnableInput(PlayerController);
	}
	else
	{
		DisableInput(PlayerController);
	}
}

void ACPlayerCharacter::OnStun()
{
	SetMoveInputBlock(true);
}

void ACPlayerCharacter::OnRecoveryFromStun()
{
	if (IsDead())
		return;
	
	SetMoveInputBlock(false);
}

void ACPlayerCharacter::OnBind()
{
	bBlockBasicAttack = true;
}

void ACPlayerCharacter::OnRecoveryFromBind()
{
	if ( IsDead() )
		return;

	bBlockBasicAttack = false;
}

void ACPlayerCharacter::OnDead()
{
	SetMoveInputBlock(true);
	SetBlockAbilityInput(true);

	SetCrowdObstacleRegistered(false);
}

void ACPlayerCharacter::OnRespawn()
{
	SetMoveInputBlock(false);
	SetBlockAbilityInput(false);

	SetCrowdObstacleRegistered(true);
}

void ACPlayerCharacter::OnBasicAttackAimStateChanged(bool bIsAiming)
{
	LerpCameraToLocalOffsetLocation(bIsAiming ? BasicAttackCameraOffset : FVector{ 0.f });
}

void ACPlayerCharacter::OnSkillAimStateChanged(bool bIsAiming)
{
	LerpCameraToLocalOffsetLocation(bIsAiming ? SkillCameraOffset : FVector{ 0.f });
}

FVector ACPlayerCharacter::GetLookRightDir() const
{
	return ViewCam->GetRightVector();
}

FVector ACPlayerCharacter::GetLookForwardDir() const
{
	return ViewCam->GetForwardVector();
}

FVector ACPlayerCharacter::GetMoveFowardDir() const
{
	return FVector::CrossProduct(GetLookRightDir(), FVector::UpVector);
}

void ACPlayerCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);

	if ( GetCharacterMovement()->MovementMode == MOVE_Falling && PrevMovementMode != MOVE_Falling )
	{
		if ( UAbilitySystemComponent* ASC = GetAbilitySystemComponent() )
		{
			FGameplayTagContainer CancelTags;
			CancelTags.AddTag(FGameplayTag::RequestGameplayTag("Ability.Type.Recalling"));
			ASC->CancelAbilities(&CancelTags, nullptr, nullptr);
		}
	}
}

void ACPlayerCharacter::GrantInBaseTag()
{
	if ( UAbilitySystemComponent* ASC = GetAbilitySystemComponent() )
	{
		ASC->AddReplicatedLooseGameplayTag(UCAbilitySystemStatics::GetInBaseStatusTag());
	}
}

void ACPlayerCharacter::TickChronoLensBubble()
{
	if ( !bChronoLensBubbleActive || !ChronoLensBubbleComponent )
		return;

	float DeltaSeconds = GetWorld()->GetDeltaSeconds();
	ChronoLensBubbleElapsed += DeltaSeconds;

	float PulseFactor = 1.f + FMath::Sin(ChronoLensBubbleElapsed * 2.f) * 0.05f;
	ChronoLensBubbleComponent->SetRelativeScale3D(FVector(3.f * PulseFactor));

	ChronoLensBubbleTimerHandle = GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &ACPlayerCharacter::TickChronoLensBubble));
}

void ACPlayerCharacter::Multicast_SetChronoLensVisuals_Implementation(bool bActive)
{
	if ( USkeletalMeshComponent* SkelMesh = GetMesh() )
	{
		SkelMesh->bPauseAnims = bActive;
	}

	if(bActive )
	{
		if ( GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Falling )
		{
			GetCharacterMovement()->StopMovementImmediately();
			GetCharacterMovement()->MovementMode = EMovementMode::MOVE_Flying;
		}
	}
	else
	{
		if ( GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Flying )
		{
			GetCharacterMovement()->MovementMode = EMovementMode::MOVE_Falling;
		}
	}

	if ( ChronoLensBubbleComponent )
	{
		ChronoLensBubbleComponent->SetVisibility(bActive);
	}

	bUseControllerRotationYaw = !bActive;
	GetCharacterMovement()->bOrientRotationToMovement = !bActive;

	bChronoLensBubbleActive = bActive;
	ChronoLensBubbleElapsed = 0.f;

	if ( bActive )
	{
		TickChronoLensBubble();
	}
	else
	{
		GetWorldTimerManager().ClearTimer(ChronoLensBubbleTimerHandle);
	}

}

void ACPlayerCharacter::DisableJumpTagUpdated(const FGameplayTag Tag, int32 NewCount)
{
	if ( NewCount > 0 )
		bBlockJumpInput = true;
	else
		bBlockJumpInput = false;
}

void ACPlayerCharacter::Jump()
{
	if ( bBlockJumpInput || GetIsInFocusMode() )
	{
		return;
	}

	if ( UAbilitySystemComponent* ASC = GetAbilitySystemComponent() )
	{
		if ( ASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag("Ability.Type.Recalling")) )
		{
			FGameplayTagContainer CancelTags;
			CancelTags.AddTag(FGameplayTag::RequestGameplayTag("Ability.Type.Recalling"));
			ASC->CancelAbilities(&CancelTags, nullptr, nullptr);
		}
	}

	Super::Jump();
	
}

void ACPlayerCharacter::LerpCameraToLocalOffsetLocation(const FVector& Goal)
{
	GetWorldTimerManager().ClearTimer(CameraLerpTimerHandle);
	CameraLerpTimerHandle = GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &ACPlayerCharacter::TickCameraLocalOffsetLerp, Goal));
}

void ACPlayerCharacter::TickCameraLocalOffsetLerp(FVector Goal)
{
	FVector CurrentLocalOffset = ViewCam->GetRelativeLocation();
	if (FVector::Distance(CurrentLocalOffset, Goal) < 1.f)
	{
		ViewCam->SetRelativeLocation(Goal);
		return;
	}

	float LerpAlpha = FMath::Clamp(GetWorld()->GetDeltaSeconds() * CameraLerpSpeed, 0.f, 1.f);
	FVector NewLocalOffset = FMath::Lerp(CurrentLocalOffset, Goal, LerpAlpha);
	ViewCam->SetRelativeLocation(NewLocalOffset);

	CameraLerpTimerHandle = GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &ACPlayerCharacter::TickCameraLocalOffsetLerp, Goal));
}

void ACPlayerCharacter::MouseSensitivityChanged(float NewSensitivity)
{
	CachedMouseSensitivity = NewSensitivity;
}