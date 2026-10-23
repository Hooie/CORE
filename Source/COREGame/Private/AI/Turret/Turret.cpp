// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/Turret/Turret.h"
#include "AI/AggroSubsystem.h"
#include "AI/Turret/TurretAnimInstance.h"
#include "AI/RangedMinionProjectile.h"
#include "Net/UnrealNetwork.h"
#include "Character/CCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/DecalComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/OverlapResult.h"
#include "GAS/CAbilitySystemComponent.h"
#include "GAS/CAbilitySystemStatics.h"
#include "GAS/CAttributeSet.h"
#include "GAS/CHeroAttributeSet.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Particles/ParticleSystem.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Widgets/OverHeadStatsGauge.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

// Sets default values
ATurret::ATurret()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bAlwaysRelevant = true;
	bReplicates = true;
	SetNetUpdateFrequency(10.f);

	USceneComponent* SceneComp = CreateDefaultSubobject<USceneComponent>("Root Comp");
	SetRootComponent(SceneComp);

	Capsule = CreateDefaultSubobject<UCapsuleComponent>("Capsule Component");
	Capsule->SetupAttachment(GetRootComponent());

	Capsule->SetCollisionProfileName(TEXT("Pawn"));

	TurretMesh = CreateDefaultSubobject<USkeletalMeshComponent>("Turret Mesh");
	TurretMesh->SetupAttachment(GetRootComponent());
	
	DetectionRange = CreateDefaultSubobject<USphereComponent>("Detection Range");
	DetectionRange->SetupAttachment(GetRootComponent());
	DetectionRange->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DetectionRange->SetCollisionResponseToAllChannels(ECR_Ignore);
	DetectionRange->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	CAbilitySystemComponent = CreateDefaultSubobject<UCAbilitySystemComponent>("CAbility System Component");
	CAbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	AttributeSet = CreateDefaultSubobject<UCAttributeSet>("CAttribute Set"); 
	CHeroAttributeSet = CreateDefaultSubobject<UCHeroAttributeSet>("CHero Attribute Set");

	OverheadWidgetComponent = CreateDefaultSubobject<UWidgetComponent>("Overhead Widget Component");
	OverheadWidgetComponent->SetupAttachment(GetRootComponent());
	OverheadWidgetComponent->SetHiddenInGame(true);

	TargetingBeamComponent = CreateDefaultSubobject<UParticleSystemComponent>("Targeting Beam Component");
	TargetingBeamComponent->SetupAttachment(TurretMesh, TargetingBeamSocketName);
	TargetingBeamComponent->SetMobility(EComponentMobility::Movable);
	TargetingBeamComponent->bAutoActivate = false;

	DestroyedRuinVFX = CreateDefaultSubobject<UParticleSystemComponent>("Destroyed Ruin VFX");
	DestroyedRuinVFX->SetupAttachment(GetRootComponent());
	DestroyedRuinVFX->bAutoActivate = false;

	StimuliSourceComponent = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>("Stimuli Source Component");

	RangeDecalComponent = CreateDefaultSubobject<UDecalComponent>("Range Decal Component");
	RangeDecalComponent->SetupAttachment(GetRootComponent());
	RangeDecalComponent->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
}

void ATurret::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATurret, TeamId);
	DOREPLIFETIME(ATurret, bIsDestroyed);
	DOREPLIFETIME(ATurret, ConfirmedTarget);
}

void ATurret::SetGenericTeamId(const FGenericTeamId& NewTeamId)
{
	TeamId = NewTeamId;

	OnRep_TeamId();
}

UAbilitySystemComponent* ATurret::GetAbilitySystemComponent() const
{
	return CAbilitySystemComponent;
}

void ATurret::StartCombat()
{
	if ( !HasAuthority() || bIsDestroyed )
		return;

	GetWorldTimerManager().SetTimer(ScanTimerHandle, this, &ATurret::ScanForTarget, ScanInterval, true);
}

void ATurret::StopCombat()
{
	GetWorldTimerManager().ClearTimer(ScanTimerHandle);
	GetWorldTimerManager().ClearTimer(AttackTimerHandle);
	bAttackTimerStarted = false;

	CurrentThreatAttacker = nullptr;
	ThreatTimeRemaining = 0.f;

	ClearTarget();

	if ( TargetingBeamComponent )
	{
		TargetingBeamComponent->Deactivate();
	}
}

FVector ATurret::GetTargetLocation(AActor* RequestedBy) const
{
	FVector AimLocation = GetActorLocation();

	if ( AimPointHeight > 0.f )
	{
		AimLocation.Z += AimPointHeight;
	}
	else if ( TurretMesh )
	{
		AimLocation.Z = TurretMesh->Bounds.Origin.Z;
	}

	return AimLocation;
}

// Called when the game starts or when spawned
void ATurret::BeginPlay()
{
	Super::BeginPlay();
	
	if( StimuliSourceComponent)
	{
		StimuliSourceComponent->RegisterForSense(UAISense_Sight::StaticClass());
		StimuliSourceComponent->RegisterWithPerceptionSystem();
	}

	DetectionRange->SetSphereRadius(DetectionRadius);

	if ( UAggroSubsystem* AggroSubsystem = GetWorld()->GetSubsystem<UAggroSubsystem>() )
	{
		AggroSubsystem->OnThreatEvent.AddUObject(this, &ATurret::OnThreatEvent);
	}

	OnRep_TeamId();

	if ( RangeDecalComponent && RangeDecalMaterial )
	{
		RangeDecalComponent->DecalSize = FVector(200.f, DetectionRadius, DetectionRadius);

		RangeDecalMID = UMaterialInstanceDynamic::Create(RangeDecalMaterial, this);
		RangeDecalComponent->SetDecalMaterial(RangeDecalMID);

		UpdateTeamVisuals();
	}

	CurrentAimRotation = GetActorRotation();

	CAbilitySystemComponent->InitAbilityActorInfo(this, this);

	if ( HasAuthority() )
	{
		CAbilitySystemComponent->ServerSideInit();
		CAbilitySystemComponent->AddLooseGameplayTag(UCAbilitySystemStatics::GetOnlyBasicAttackDamageTag());
		CAbilitySystemComponent->AddReplicatedLooseGameplayTag(UCAbilitySystemStatics::GetOnlyBasicAttackDamageTag());
	}

	if ( UOverHeadStatsGauge* OverheadStatsGauge = Cast<UOverHeadStatsGauge>(OverheadWidgetComponent->GetUserWidgetObject()) )
	{
		OverheadStatsGauge->ConfigureWithASC(CAbilitySystemComponent);
		OverheadWidgetComponent->SetHiddenInGame(true); 

		CachedOverheadStatsGauge = OverheadStatsGauge;

		GetWorldTimerManager().ClearTimer(HeadStatGaugeVisiblityUpdateTimerHandle);
		GetWorldTimerManager().SetTimer(HeadStatGaugeVisiblityUpdateTimerHandle, this, &ATurret::UpdateHeadGaugeVisiblity, HeadStatGaugeVisiblityCheckUpdateGap, true, 0.f);

		OverheadStatsGauge->SetDistanceScaling(HeadGaugeMinScaleDistance, HeadGaugeMaxScaleDistance, HeadGaugeNearScale, HeadGaugeFarScale);
	
		UpdateTeamVisuals();
	}

	if ( CAbilitySystemComponent )
	{
		CAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UCAttributeSet::GetHealthAttribute()).AddUObject(this, &ATurret::HandleHealthChanged);
	}

	if ( HasAuthority() && TurretMesh )
	{
		TurretMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	}
}

// Called every frame
void ATurret::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if ( !bIsDestroyed && ConfirmedTarget.IsValid() )
	{
		TimeSinceTargetLost = 0.f; 

		UpdateAiming(DeltaTime);

		if ( !bAttackTimerStarted )
		{
			bAttackTimerStarted = true;

			const float Now = GetWorld()->GetTimeSeconds();
			const float FirstDelay = (LastFireTime < 0.f) ? 0.f : FMath::Max(0.f, LastFireTime + AttackCooldown - Now);

			GetWorldTimerManager().SetTimer(AttackTimerHandle, this, &ATurret::OnAttackTimerTick, AttackCooldown, true, FirstDelay);
		}
	}
	else
	{
		if ( UTurretAnimInstance* AnimInstance = GetTurretAnimInstance() )
		{
			AnimInstance->Aiming = false;
		}

		if ( TargetingBeamComponent && TargetingBeamComponent->IsActive() )
		{
			TargetingBeamComponent->Deactivate();
		}

		if ( bAttackTimerStarted )
		{
			TimeSinceTargetLost += DeltaTime;

			if ( TimeSinceTargetLost > TargetLossGracePeriod + 0.5f )
			{
				bAttackTimerStarted = false;
				GetWorldTimerManager().ClearTimer(AttackTimerHandle);
			}
		}
	}
}

UTurretAnimInstance* ATurret::GetTurretAnimInstance() const
{
	return TurretMesh ? Cast<UTurretAnimInstance>(TurretMesh->GetAnimInstance()) : nullptr;
}

void ATurret::ScanForTarget()	
{
	TRACE_CPUPROFILER_EVENT_SCOPE(Core_TurretScanForTarget);

	if ( ThreatTimeRemaining > 0.f )
	{
		ThreatTimeRemaining -= ScanInterval;
		if ( ThreatTimeRemaining <= 0.f )
		{
			CurrentThreatAttacker = nullptr;
		}
	}

	if ( CurrentThreatAttacker.IsValid() && !UCAbilitySystemStatics::IsActorDead(CurrentThreatAttacker.Get()) )
	{
		float DistSq = FVector::DistSquared(GetActorLocation(), CurrentThreatAttacker->GetActorLocation());
		if ( DistSq <= FMath::Square(DetectionRadius) )
		{
			if ( ConfirmedTarget.Get() != CurrentThreatAttacker.Get() )
			{
				ConfirmedTarget = CurrentThreatAttacker;
				ConsecutiveHitCount = 0;
			}

			return;
		}
	}

	if ( ConfirmedTarget.IsValid() )
	{
		float DistSq = FVector::DistSquared(GetActorLocation(), ConfirmedTarget->GetActorLocation());
		if ( DistSq <= FMath::Square(DetectionRadius) && !UCAbilitySystemStatics::IsActorDead(ConfirmedTarget.Get()) )
		{
			return;
		}

		ClearTarget();
	}

	AActor* BestCandidate = FindBestTargetInRange();
	if ( BestCandidate )
	{
		ConfirmedTarget = BestCandidate;
		ConsecutiveHitCount = 0;
	}
}

AActor* ATurret::FindBestTargetInRange() const
{
	TArray<FOverlapResult> Overlaps;
	FCollisionShape Sphere = FCollisionShape::MakeSphere(DetectionRadius);
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	GetWorld()->OverlapMultiByObjectType(Overlaps, GetActorLocation(), FQuat::Identity, FCollisionObjectQueryParams(ECC_Pawn), Sphere, Params);

	AActor* ClosestMinion = nullptr;
	float ClosestMinionDistSq = TNumericLimits<float>::Max();

	AActor* ClosestHero = nullptr;
	float ClosesetHeroDistSq = TNumericLimits<float>::Max();

	for ( const FOverlapResult& Overlap : Overlaps )
	{
		AActor* OtherActor = Overlap.GetActor();
		if ( !OtherActor || UCAbilitySystemStatics::IsActorDead(OtherActor) )
			continue;

		IGenericTeamAgentInterface* OtherTeamInterface = Cast<IGenericTeamAgentInterface>(OtherActor);
		if ( !OtherTeamInterface || OtherTeamInterface->GetTeamAttitudeTowards(*const_cast< ATurret* >(this)) != ETeamAttitude::Hostile )
			continue;

		float DistSq = FVector::DistSquared(GetActorLocation(), OtherActor->GetActorLocation());

		if ( UCAbilitySystemStatics::IsHero(OtherActor) )
		{
			if ( DistSq < ClosesetHeroDistSq )
			{
				ClosesetHeroDistSq = DistSq;
				ClosestHero = OtherActor;
			}
		}
		else
		{
			if ( DistSq < ClosestMinionDistSq )
			{
				ClosestMinionDistSq = DistSq;
				ClosestMinion = OtherActor;
			}
		}
	}

	return ClosestMinion ? ClosestMinion : ClosestHero;
}

void ATurret::ClearTarget()
{
	ConfirmedTarget = nullptr;
	bIsAttacking = false;

	if ( UTurretAnimInstance* AnimInstance = GetTurretAnimInstance() )
	{
		AnimInstance->Aiming = false;
	}

	ConsecutiveHitCount = 0;
}

void ATurret::UpdateAiming(float DeltaTime)
{
	FVector BoneWorldLocation = TurretMesh->GetSocketLocation(TEXT("telescope"));
	FVector TargetWorldLocation = ConfirmedTarget->GetActorLocation();

	FVector ToTarget = TargetWorldLocation - BoneWorldLocation;
	float Distance = ToTarget.Size();

	if ( UTurretAnimInstance* AnimInstance = GetTurretAnimInstance() )
	{
		AnimInstance->DistanceToTarget = Distance;
		AnimInstance->Aiming = true;
		AnimInstance->AimLookAtLocation = TargetWorldLocation;
	}

	if ( TargetingBeamComponent )
	{
		if ( !TargetingBeamComponent->IsActive() )
		{
			TargetingBeamComponent->Activate(true);
		}

		TargetingBeamComponent->SetBeamTargetPoint(0, TargetWorldLocation, 0);
	}
}

void ATurret::FireAtTarget()
{
	if ( !ConfirmedTarget.IsValid() || !ProjectileClass || !CAbilitySystemComponent )
		return;

	LastFireTime = GetWorld()->GetTimeSeconds();

	if ( UTurretAnimInstance* AnimInstance = GetTurretAnimInstance() )
	{
		AnimInstance->isFiring = true;
	}
	GetWorldTimerManager().SetTimer(FireVisualTimerHnalde, this, &ATurret::ResetFireVisual, FireVisualDuration, false);

	if ( MuzzleVFX && TurretMesh )
	{
		UGameplayStatics::SpawnEmitterAttached(MuzzleVFX, TurretMesh, MuzzleSocketName, FVector(30.f, 0.f, 0.f), FRotator::ZeroRotator, EAttachLocation::SnapToTarget, true);
	}

	if ( !HasAuthority() )
		return;

	FVector MuzzleLocation = GetActorLocation();
	if ( TurretMesh )
	{
		MuzzleLocation = TurretMesh->GetSocketLocation(MuzzleSocketName);
	}

	FVector LaunchDirection = (ConfirmedTarget->GetActorLocation() - MuzzleLocation).GetSafeNormal();

	bool bTargetIsHero = UCAbilitySystemStatics::IsHero(ConfirmedTarget.Get());

	FGameplayEffectSpecHandle DamageSpec;
	
	if ( bTargetIsHero )
	{
		float DamageMultiplier = FMath::Min(1.f + DamageIncreasePerHit * ConsecutiveHitCount, MaxDamageMultiplier);
		float FinalBaseDamage = BaseDamage * DamageMultiplier;
		
		DamageSpec = CAbilitySystemComponent->MakeOutgoingSpec(HitDamageEffect, 1, CAbilitySystemComponent->MakeEffectContext());

		if ( DamageSpec.IsValid() )
		{
			DamageSpec.Data->SetSetByCallerMagnitude(UCAbilitySystemStatics::GetAbilityBaseDamageTag(), FinalBaseDamage);
			DamageSpec.Data->SetSetByCallerMagnitude(UCAbilitySystemStatics::GetDamageStrengthRatioTag(), 0.f);
			DamageSpec.Data->SetSetByCallerMagnitude(UCAbilitySystemStatics::GetDamageIntelligenceRatioTag(), 0.f);
		}
		
		++ConsecutiveHitCount;
	}
	else
	{
		DamageSpec = CAbilitySystemComponent->MakeOutgoingSpec(MinionHitDamageEffect, 1, CAbilitySystemComponent->MakeEffectContext());
		if ( DamageSpec.IsValid() )
		{
			DamageSpec.Data->SetSetByCallerMagnitude(UCAbilitySystemStatics::GetMaxHealthPercentDataTag(), MinionMaxHealthDamagePercent);
		}
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ARangedMinionProjectile* Projectile = GetWorld()->SpawnActor<ARangedMinionProjectile>(ProjectileClass, MuzzleLocation, LaunchDirection.Rotation(), SpawnParams);
	if ( Projectile )
	{
		Projectile->Launch(ConfirmedTarget.Get(), ProjectileSpeed, ProjectileMaxDistance, DamageSpec);
	}
}

void ATurret::UpdateTeamVisuals()
{
	APawn* LocalPlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	const IGenericTeamAgentInterface* LocalTeamAgent = Cast<IGenericTeamAgentInterface>(LocalPlayerPawn);

	const bool bIsAlly = LocalTeamAgent && (LocalTeamAgent->GetGenericTeamId() == TeamId);

	// 사거리 데칼 (기존 동작 그대로: 로컬 팀을 모르면 적 색)
	if ( RangeDecalMID )
	{
		RangeDecalMID->SetVectorParameterValue(TEXT("Color"), bIsAlly ? AllyRangeColor : EnemyRangeColor);
	}

	// 체력바 (로컬 팀을 아직 모르면 BP 기본색)
	if ( CachedOverheadStatsGauge )
	{
		int32 Relation = ELocalRelation::None;
		if ( LocalTeamAgent && LocalTeamAgent->GetGenericTeamId() != FGenericTeamId::NoTeam && TeamId != FGenericTeamId::NoTeam )
		{
			Relation = bIsAlly ? ELocalRelation::Ally : ELocalRelation::Enemy;
		}

		if ( Relation != LastGaugeRelation )
		{
			CachedOverheadStatsGauge->SetTeamRelation(Relation);
			LastGaugeRelation = Relation;
		}
	}
}

void ATurret::OnRep_IsDestroyed()
{
	if ( !bIsDestroyed )
		return;
	
	GetWorldTimerManager().ClearTimer(HeadStatGaugeVisiblityUpdateTimerHandle);
	
	OverheadWidgetComponent->SetHiddenInGame(true);

	DetectionRange->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if ( RangeDecalComponent )
	{
		RangeDecalComponent->SetHiddenInGame(true);
	}

	if ( UCAbilitySystemComponent* ASC = Cast<UCAbilitySystemComponent>(GetAbilitySystemComponent()) )
	{
		ASC->AddReplicatedLooseGameplayTag(UCAbilitySystemStatics::GetDeadStatusTag());
	}

	PlayDestroyedPresentation();
}

void ATurret::HandleHealthChanged(const FOnAttributeChangeData& Data)
{
	if ( !HasAuthority() || bIsDestroyed )
		return;

	if ( Data.NewValue <= 0.f )
	{
		DestroyTower();
	}
}

void ATurret::DestroyTower()
{
	if ( bIsDestroyed )
		return;

	bIsDestroyed = true;
	
	GetWorldTimerManager().ClearTimer(ScanTimerHandle);
	GetWorldTimerManager().ClearTimer(AttackTimerHandle);
	GetWorldTimerManager().ClearTimer(HeadStatGaugeVisiblityUpdateTimerHandle);

	if ( TargetingBeamComponent )
	{
		TargetingBeamComponent->Deactivate();
	}

	ConfirmedTarget = nullptr;
	bIsAttacking = false;

	OnRep_IsDestroyed();
}

void ATurret::PlayDestroyedPresentation()
{
	if ( UAnimInstance* AnimInstance = GetTurretAnimInstance() )
	{
		if ( DestroyedMontage )
		{
			AnimInstance->Montage_Play(DestroyedMontage);
		}
	}

	if ( DestroyedRuinVFX )
	{
		DestroyedRuinVFX->Activate();
	}
}

void ATurret::OnAttackTimerTick()
{
	if ( ConfirmedTarget.IsValid() )
	{
		FireAtTarget();
		return;
	}

	GetWorldTimerManager().ClearTimer(AttackTimerHandle);
	bAttackTimerStarted = false;
}

void ATurret::ResetFireVisual()
{
	if ( UTurretAnimInstance* AnimInstance = GetTurretAnimInstance() )
	{
		AnimInstance->isFiring = false;
	}
}

void ATurret::OnRep_TeamId()
{
	USkeletalMesh** Skin = SkinMap.Find(GetGenericTeamId());

	if ( Skin )
	{
		TurretMesh->SetSkeletalMesh(*Skin);
	}

	UpdateTeamVisuals();
}

void ATurret::OnThreatEvent(AActor* Victim, AActor* Attacker)
{
	if ( !Victim || !Attacker || bIsDestroyed )
		return;

	if ( !UCAbilitySystemStatics::IsHero(Attacker) || !UCAbilitySystemStatics::IsHero(Victim) )
		return;

	if ( GetTeamAttitudeTowards(*Attacker) != ETeamAttitude::Hostile )
		return;

	if ( GetTeamAttitudeTowards(*Victim) == ETeamAttitude::Hostile )
		return;

	const float RangeSq = FMath::Square(DetectionRadius);
	if ( FVector::DistSquared(GetActorLocation(), Victim->GetActorLocation()) > RangeSq || FVector::DistSquared(GetActorLocation(), Attacker->GetActorLocation()) > RangeSq)
		return;

	CurrentThreatAttacker = Attacker;
	ThreatTimeRemaining = ThreatMemoryDuration;
}

void ATurret::UpdateHeadGaugeVisiblity()
{
	APawn* LocalPlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if ( !LocalPlayerPawn )
		return;

	const bool bInRange = FVector::DistSquared(GetActorLocation(), LocalPlayerPawn->GetActorLocation()) <= HeadStatGaugeVisiblityRangeSqaured;

	const bool bGaugeVisible = bInRange && UCAbilitySystemStatics::WasRecentlyRenderedOnScreen(TurretMesh, HeadGaugeRenderTolerance);
	
	if ( !bGaugeVisible && CachedOverheadStatsGauge )
	{
		CachedOverheadStatsGauge->SnapDistanceScale();
	}

	OverheadWidgetComponent->SetHiddenInGame(!bGaugeVisible);

	RangeDecalComponent->SetHiddenInGame(!bInRange);

	UpdateTeamVisuals();
}

void ATurret::RecordDamagingHero(AActor* HeroCauser)
{
	if ( !HasAuthority() || !HeroCauser )
		return;

	LastDamagingHero = HeroCauser;
	LastDamagingHeroTimestamp = GetWorld()->GetTimeSeconds();
}

AActor* ATurret::GetLastDamagingHeroIfRecent() const
{
	if ( !LastDamagingHero.IsValid() || LastDamagingHeroTimestamp < 0.f )
		return nullptr;

	if ( GetWorld()->GetTimeSeconds() - LastDamagingHeroTimestamp > LastDamagingHeroMemoryDuration )
		return nullptr;

	return LastDamagingHero.Get();
}

void ATurret::HideOverheadStatsGauge()
{
	GetWorldTimerManager().ClearTimer(HeadStatGaugeVisiblityUpdateTimerHandle);


	if( OverheadWidgetComponent )
	{
		OverheadWidgetComponent->SetHiddenInGame(true);
	}

	if ( RangeDecalComponent )
	{
		RangeDecalComponent->SetHiddenInGame(true);
	}
}