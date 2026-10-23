// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "GenericTeamAgentInterface.h"
#include "GAS/CGameplayAbilityTypes.h"
#include "Widgets/RenderActorTargetInterface.h"

#include "CCharacter.generated.h"

// 로컬 플레이어와의 관계
namespace ELocalRelation
{
	enum Type : int32
	{
		None	= 0,
		Self	= 1,
		Ally	= 2,
		Enemy	= 3,
	};
}

UCLASS()
class ACCharacter : public ACharacter, public IAbilitySystemInterface, public IGenericTeamAgentInterface, public IRenderActorTargetInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ACCharacter();
	void ServerSideInit();
	void ClientSideInit();
	bool IsLocallyControlledByPlayer() const;

	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const override;

	const TMap<ECAbilityInputID, TSubclassOf<UGameplayAbility>>& GetAbilities() const;

	virtual FVector GetCaptureLocalPosition() const override;
	virtual FRotator GetCaptureLocalRotation() const override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Capture")
	FVector HeadShotCaptureLocalPosition;

	UPROPERTY(EditDefaultsOnly, Category = "Capture")
	FRotator HeadShotCaptureLocalRotation;
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// 
	virtual void PossessedBy(AController* NewController) override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SetSkeletalMesh(USkeletalMesh* NewMesh);

/*******************************************************************************/
/*                              Gameplay Ability                               */
/*******************************************************************************/
public:
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	// RPC
	// Reliable : TCP
	UFUNCTION(Server, Reliable, WithValidation) 
	void Server_SendGameplayEventToSelf(const FGameplayTag& EventTag, const FGameplayEventData& EventData);

	FORCEINLINE bool GetIsInFocusMode() const { return bIsInFocusMode; }

	void AddShieldBuffHandle(const FActiveGameplayEffectHandle& Handle);
	void RemoveShieldBuffHandle(const FActiveGameplayEffectHandle& Handle);
	void ClearShieldBuffEarly();

protected:
	void UpgradeAbilityWithInputID(ECAbilityInputID InputID);
	
private:
	void BindGASChangeDelegates();
	void DeathTagUpdated(const FGameplayTag Tag, int32 NewCount);
	void StunTagUpdated(const FGameplayTag Tag, int32 NewCount);
	void AimTagUpdated(const FGameplayTag Tag, int32 NewCount);
	void FocusTagUpdated(const FGameplayTag Tag, int32 NewCount);
	void BindTagUpdated(const FGameplayTag Tag, int32 NewCount);
	virtual void DisableJumpTagUpdated(const FGameplayTag Tag, int32 NewCount);
	void MatchNotStartedTagUpdated(const FGameplayTag Tag, int32 NewCount);	

	bool bIsInFocusMode = false;

	void MoveSpeedUpdated(const FOnAttributeChangeData& Data);
	void MoveAccelerationUpdated(const FOnAttributeChangeData& Data);
	void MaxHealthUpdated(const FOnAttributeChangeData& Data);
	void MaxManaUpdated(const FOnAttributeChangeData& Data);


	void SetIsAimming(bool bIsAimming);

	virtual void OnAimStateChanged(bool bIsAiming);
	
	UPROPERTY(VisibleDefaultsOnly, Category = "Gameplay Ability")
	class UCAbilitySystemComponent* CAbilitySystemComponent;
	UPROPERTY()
	class UCAttributeSet* CAttributeSet;

	void BasicAttackAimTagUpdated(const FGameplayTag Tag, int32 NewCount);
	void SkillAimTagUpdated(const FGameplayTag Tag, int32 NewCount);

	virtual void OnBasicAttackAimStateChanged(bool bIsAiming);
	virtual void OnSkillAimStateChanged(bool bIsAiming);

	TArray<FActiveGameplayEffectHandle> ActiveShieldBuffHandles;

	/*******************************************************************************/
	/*                                     UI                                      */
	/*******************************************************************************/
public:
	void HideOverheadStatsGauge();
	class UWidgetComponent* GetOverheadWidgetComponent() { return OverheadWidgetComponent; };

private:
	UPROPERTY(VisibleDefaultsOnly, Category = "UI")
	class UWidgetComponent* OverheadWidgetComponent;

	void ConfigureOverheadStatusWidget();

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	float HeadStatGaugeVisiblityCheckUpdateGap = 0.2f;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	float HeadStatGaugeVisiblityRangeSqaured = 30000000.f;

	FTimerHandle HeadStatGaugeVisiblityUpdateTimerHandle;

	void UpdateHeadGaugeVisiblity();
	void SetStatusGaugeEnabled(bool bIsEnabled);

	UPROPERTY(EditDefaultsOnly, Category = "UI|Scale")
	float HeadGaugeMinScaleDistance = 800.f;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Scale")
	float HeadGaugeMaxScaleDistance = 6000.f;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Scale")
	float HeadGaugeNearScale = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Scale")
	float HeadGaugeFarScale = 0.2f;

	UPROPERTY()
	class UOverHeadStatsGauge* CachedOverheadStatsGauge;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	float HeadGaugeRenderTolerance = 0.2f;

	bool bOverheadGaugeHidden = false;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	bool bShowOverheadGaugeForSelf = true;
	
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	FVector OverheadGaugeOffset = FVector(0.f, 0.f, 150.f);

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	FVector OverheadGaugeOffsetForSelf = FVector(0.f, 0.f, -60.f);

	/******************************************************************************/
	/*                                  Outline                                   */
	/******************************************************************************/
public:
	// 로컬 플레이어 기준 관계로 외곽선과 머리 위 체력바 색 갱신
	void UpdateTeamVisuals();
	void SetOutlineEnabled(bool bEnabled);

	static void RefreshAllTeamVisuals(const UObject* WorldContextObject);

	// 로컬 플레이어 기준 관계 (아직 판단할 수 없으면 None)
	int32 GetLocalRelation() const;

	// 머리 위 게이지의 닉네임 갱신
	void UpdateOverheadName();

	virtual void OnRep_PlayerState() override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Outline")
	bool bUseOutline = true;

	UPROPERTY(EditDefaultsOnly, Category = "Outline")
	bool bOutlineSelf = false;

	bool bOutlineEnabled = true;

	void ApplyOutline(int32 OutlineType);
	static bool GetLocalTeamId(const UWorld* World, FGenericTeamId& OutTeamId);

	UPROPERTY(EditDefaultsOnly, Category = "Outline")
	TObjectPtr<class UMaterialInterface> OutlineMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "Outline")
	FLinearColor SelfOutlineColor = FLinearColor(0.2f, 1.f, 0.3f);

	UPROPERTY(EditDefaultsOnly, Category = "Outline")
	FLinearColor AllyOutlineColor = FLinearColor(0.3f, 0.9f, 0.3f);

	UPROPERTY(EditDefaultsOnly, Category = "Outline")
	FLinearColor EnemyOutlineColor = FLinearColor(1.f, 0.23f, 0.28f);
	UPROPERTY(Transient)
	TObjectPtr<class UMaterialInstanceDynamic> OutlineMID;

	/*******************************************************************************/
	/*                                   Stun                                      */
	/*******************************************************************************/
private:
	UPROPERTY(EditDefaultsOnly, Category = "Stun")
	UAnimMontage* StunMontage;

	virtual void OnStun();
	virtual void OnRecoveryFromStun();

	/*******************************************************************************/
	/*                                   Bind                                      */
	/*******************************************************************************/
	UPROPERTY(EditDefaultsOnly, Category = "Bind")
	UAnimMontage* BindMontage;

	virtual void OnBind();
	virtual void OnRecoveryFromBind();

	/*******************************************************************************/
	/*                             Death And Respawn                               */
	/*******************************************************************************/
public:
	bool IsDead() const;
	void RespawnImmediately();

	FString GetKillFeedDisplayName() const { return KillFeedDisplayName; }
	TSoftObjectPtr<UTexture2D> GetKillFeedIcon() const { return KillerFeedIcon; }

	void RecordDamagingHero(AActor* HeroCauser);
	AActor* GetLastDamagingHeroIfRecent() const;

	TArray<AActor*> GetRecentAssistContributors(AActor* ExcludeActor = nullptr) const;

	float GetRespawnDurationTimeForLevel(int32 Level) const;

	void RecordShieldProvider(AActor* ShieldProvider);
	TArray<AActor*> GetRecentShieldContributors(AActor* ExcludeActor) const;

private:
	FTransform MeshRelativeTransform;
	
	UPROPERTY(EditDefaultsOnly, Category = "Respawn")
	UAnimMontage* RespawnMontage;
	
	UPROPERTY(EditDefaultsOnly, Category = "Death")
	float DeathMontageFinishTimeShift = -0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Death")
	UAnimMontage* DeathMontage;

	FTimerHandle DeathMontageTimerHandle;

	void DeathMontageFinished();
	void SetRagdollEnabled(bool bIsEnabled);

	void PlayDeathAnimation();

	void StartDeathSequence();
	void Respawn();

	virtual void OnDead();
	virtual void OnRespawn();

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	FString KillFeedDisplayName = TEXT("미니언");

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSoftObjectPtr<UTexture2D> KillerFeedIcon;

	UPROPERTY(EditDefaultsOnly, Category = "Death")
	float LastDamagingHeroMemoryDuration = 10.f;

	float LastDamagingHeroTimestamp = -1.f;
	TWeakObjectPtr<AActor> LastDamagingHero;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float AssistMemoryDuration = 10.f;

	UPROPERTY(EditDefaultsOnly, Category = "Assist")
	float ShieldAssistMemoryDuration = 10.f;

	TMap<TWeakObjectPtr<AActor>, float> RecentDamagingHeroTimestamps;
	TMap<TWeakObjectPtr<AActor>, float> RecentShieldProviderTimestamps;

	UPROPERTY(EditDefaultsOnly, Category = "Respawn")
	TArray<float> RespawnDurationTimeForLevel = { 5.f, 5.f, 7.f, 7.f, 8.f, 10.f, 14.f, 16.f, 18.f, 20.f, 25.f, 27.f, 29.f, 31.f, 33.f, 35.f, 37.f, 40.f };

	UPROPERTY(EditDefaultsOnly, Category = "Death")
	float NonHeroDeathSequenceCleanupDelay = 3.f;
	
	/*******************************************************************************/
	/*                                   Team                                      */
	/*******************************************************************************/
public:
	/** Assigns Team Agent to given TeamID */
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;

	/** Retrieve team identifier in form of FGenericTeamId */
	virtual FGenericTeamId GetGenericTeamId() const override;

protected:
	UFUNCTION()
	virtual void OnRep_TeamID();

private:
	UPROPERTY(ReplicatedUsing = OnRep_TeamID)
	FGenericTeamId TeamID;

	/*******************************************************************************/
	/*                                    AI                                       */
	/*******************************************************************************/
private:
	void SetAIPerceptionStimuliSourceEnabled(bool bIsEnabled);

	UPROPERTY()
	class UAIPerceptionStimuliSourceComponent* PerceptionStimuliSourceComponent;
};
