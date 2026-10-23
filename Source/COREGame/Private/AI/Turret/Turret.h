// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Actor.h"
#include "GenericTeamAgentInterface.h"
#include "Turret.generated.h"

class UTexture2D;

UCLASS()
class ATurret : public AActor, public IGenericTeamAgentInterface, public IAbilitySystemInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATurret();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamId) override;
	virtual FGenericTeamId GetGenericTeamId() const override { return TeamId; }

	UFUNCTION(BlueprintCallable)
	virtual class UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	void StartCombat();
	void StopCombat();

	virtual FVector GetTargetLocation(AActor* RequestedBy = nullptr) const override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Component")
	class UCapsuleComponent* Capsule;

	UPROPERTY(VisibleAnywhere, Category = "Component")
	class USkeletalMeshComponent* TurretMesh;

	UPROPERTY(VisibleAnywhere, Category = "Component")
	class USphereComponent* DetectionRange;

	UPROPERTY(VisibleAnywhere, Category = "Component")
	class UCAbilitySystemComponent* CAbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, Category = "Component")
	class UParticleSystemComponent* DestroyedRuinVFX;

	UPROPERTY()
	class UCAttributeSet* AttributeSet;

	UPROPERTY(VisibleAnywhere, Category = "Component") 
	class UCHeroAttributeSet* CHeroAttributeSet;

	UPROPERTY(VisibleAnywhere, Category = "Component")
	class UAIPerceptionStimuliSourceComponent* StimuliSourceComponent;
	
	UPROPERTY(EditAnywhere, Category = "Team", ReplicatedUsing = OnRep_TeamId)
	FGenericTeamId TeamId;

	UFUNCTION()
	void OnRep_IsDestroyed();

	UPROPERTY(EditDefaultsOnly, Category = "Team")
	TMap<FGenericTeamId, USkeletalMesh*> SkinMap;

	/************************************/
	/***************Attack***************/
	/************************************/
private:
	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	float DetectionRadius = 1200.f;

	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	float ScanInterval = 0.2f;

	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	float TargetLossGracePeriod = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	float YawRotationSpeed = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	float AimAngleTolerance = 3.f;

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Damage")
	TSubclassOf<class UGameplayEffect> HitDamageEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Damage")
	TSubclassOf<class UGameplayEffect> MinionHitDamageEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Damage")
	float BaseDamage = 260.f;

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Damage")
	float MinionMaxHealthDamagePercent = 0.9f;

	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	TSubclassOf<class ARangedMinionProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	FName MuzzleSocketName = "Muzzle_01";

	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	class UParticleSystem* MuzzleVFX;

	UPROPERTY(Replicated)
	TWeakObjectPtr<AActor> ConfirmedTarget;
	bool bIsAttacking = false;

	FTimerHandle ScanTimerHandle;

	class UTurretAnimInstance* GetTurretAnimInstance() const;

	void ScanForTarget();
	AActor* FindBestTargetInRange() const;

	void ClearTarget();

	void UpdateAiming(float DeltaTime);
	void FireAtTarget();

	UPROPERTY(EditDefaultsOnly, Category = "Destruction")
	UAnimMontage* DestroyedMontage;

	UPROPERTY(ReplicatedUsing = OnRep_IsDestroyed)
	bool bIsDestroyed = false;

	void HandleHealthChanged(const struct FOnAttributeChangeData& Data);

	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	float ProjectileSpeed = 1400.f;

	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	float ProjectileMaxDistance = 3000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	float AttackCooldown = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	float FireVisualDuration = 0.3f;

	bool bAttackTimerStarted = false;
	FTimerHandle AttackTimerHandle; 

	float LastFireTime = -1.f;

	float TimeSinceTargetLost = 0.f;

	void OnAttackTimerTick();

	FTimerHandle FireVisualTimerHnalde;

	void ResetFireVisual();
	
	UFUNCTION()
	void OnRep_TeamId();

	FRotator CurrentAimRotation;

	UPROPERTY(VisibleAnywhere, Category = "Component")
	class UParticleSystemComponent* TargetingBeamComponent;

	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	FName TargetingBeamSocketName = "TargetingBeam";

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Aggro")
	float ThreatMemoryDuration = 4.f;

	TWeakObjectPtr<AActor> CurrentThreatAttacker;
	float ThreatTimeRemaining = 0.f;

	UFUNCTION()
	void OnThreatEvent(AActor* Victim, AActor* Attacker);

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Damage")
	float DamageIncreasePerHit = 0.3f;

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Damage")
	float MaxDamageMultiplier = 2.1f;

	int32 ConsecutiveHitCount = 0;

	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	float AimPointHeight = 0.f;

	/************************************/
	/***************Widget***************/
	/************************************/
public:
	void HideOverheadStatsGauge();

private:
	UPROPERTY(VisibleAnywhere, Category = "Component")
	class UWidgetComponent* OverheadWidgetComponent;

	UPROPERTY(EditDefaultsOnly, Category = "OverheadWidget")
	float HeadStatGaugeVisiblityCheckUpdateGap = 0.1f;

	UPROPERTY(EditDefaultsOnly, Category = "OverheadWidget")
	float HeadStatGaugeVisiblityRangeSqaured = 30000000.f;

	UPROPERTY(EditDefaultsOnly, Category = "OverheadWidget")
	float HeadGaugeRenderTolerance = 0.2f;

	FTimerHandle HeadStatGaugeVisiblityUpdateTimerHandle;
	void UpdateHeadGaugeVisiblity();

	UPROPERTY(EditDefaultsOnly, Category = "OverheadWidget")
	float HeadGaugeMinScaleDistance = 800.f;

	UPROPERTY(EditDefaultsOnly, Category = "OverheadWidget")
	float HeadGaugeMaxScaleDistance = 6000.f;

	UPROPERTY(EditDefaultsOnly, Category = "OverheadWidget")
	float HeadGaugeNearScale = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "OverheadWidget")
	float HeadGaugeFarScale = 0.2f;

	class UOverHeadStatsGauge* CachedOverheadStatsGauge;

	UPROPERTY(VisibleAnywhere, Category = "Component")
	class UDecalComponent* RangeDecalComponent;

	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	class UMaterialInterface* RangeDecalMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	FLinearColor AllyRangeColor = FLinearColor(0.1f, 0.4f, 1.f, 1.f);

	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	FLinearColor EnemyRangeColor = FLinearColor(1.f, 0.15f, 0.15f, 1.f);

	UPROPERTY()
	class UMaterialInstanceDynamic* RangeDecalMID;

	void UpdateTeamVisuals();

	// 타이머가 0.1초마다 부르므로 관계가 바뀔 때만 체력바 색 갱신
	int32 LastGaugeRelation = -1;

/************************************/
/*************Destruction************/
/************************************/
protected:
	virtual void DestroyTower();

	// 파괴 몽타주 + 잔해 VFX. 넥서스는 재정의해서 재생 시점을 늦춤
	virtual void PlayDestroyedPresentation();

	bool IsTowerDestroyed() const { return bIsDestroyed; }

/************************************/
/**************KillFeed**************/
/************************************/
public:
	const FString& GetKillFeedDisplayName() const { return KillFeedDisplayName; }
	const TSoftObjectPtr<UTexture2D>& GetKillFeedIcon() const { return KillFeedIcon; }
	
	void RecordDamagingHero(AActor* HeroCauser);
	AActor* GetLastDamagingHeroIfRecent() const;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "KillFeed")
	FString KillFeedDisplayName = TEXT("포탑");

	UPROPERTY(EditDefaultsOnly, Category = "KillFeed")
	TSoftObjectPtr<UTexture2D> KillFeedIcon;

private:
	UPROPERTY(EditDefaultsOnly, Category = "KillFeed")
	float LastDamagingHeroMemoryDuration = 10.f;

	TWeakObjectPtr<AActor> LastDamagingHero;
	float LastDamagingHeroTimestamp = -1.f;

};
