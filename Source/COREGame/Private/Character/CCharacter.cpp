// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/CCharacter.h"
#include "COREGame/COREGame.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GAS/CAbilitySystemComponent.h"
#include "GAS/CAttributeSet.h"
#include "GAS/CAbilitySystemStatics.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "Widgets/OverHeadStatsGauge.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Perception/AISense_Sight.h"
#include "Player/CPlayerState.h"
#include "Framework/CoreBench.h"


// Sets default values
ACCharacter::ACCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetReceivesDecals(false);

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_SpringArm, ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Target, ECR_Ignore);

	CAbilitySystemComponent = CreateDefaultSubobject<UCAbilitySystemComponent>("CAbility System Component");
	CAbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	CAttributeSet = CreateDefaultSubobject<UCAttributeSet>("CAttribute Set");

	OverheadWidgetComponent = CreateDefaultSubobject<UWidgetComponent>("Overhead Widget Component");
	OverheadWidgetComponent->SetupAttachment(GetRootComponent());
	OverheadWidgetComponent->SetHiddenInGame(true);
	OverheadWidgetComponent->SetTickMode(ETickMode::Automatic);

	BindGASChangeDelegates();

	PerceptionStimuliSourceComponent = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>("Perception Stimuli Source Component");
}

void ACCharacter::ServerSideInit()
{
	CAbilitySystemComponent->InitAbilityActorInfo(this, this);
	CAbilitySystemComponent->ServerSideInit();
}

void ACCharacter::ClientSideInit()
{
	CAbilitySystemComponent->InitAbilityActorInfo(this, this);
}

bool ACCharacter::IsLocallyControlledByPlayer() const
{
	return GetController() && GetController()->IsLocalPlayerController();
}

void ACCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACCharacter, TeamID);
}

const TMap<ECAbilityInputID, TSubclassOf<UGameplayAbility>>& ACCharacter::GetAbilities() const
{
	return CAbilitySystemComponent->GetAbilities();
}

FVector ACCharacter::GetCaptureLocalPosition() const
{
	return HeadShotCaptureLocalPosition;
}

FRotator ACCharacter::GetCaptureLocalRotation() const
{
	return HeadShotCaptureLocalRotation;
}

void ACCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	if (NewController && !NewController->IsPlayerController())
	{
		ServerSideInit();
	}

	UpdateOverheadName();
}

// Called when the game starts or when spawned
void ACCharacter::BeginPlay()
{
	Super::BeginPlay();

	ConfigureOverheadStatusWidget();
	
	MeshRelativeTransform = GetMesh()->GetRelativeTransform();

	PerceptionStimuliSourceComponent->RegisterForSense(UAISense_Sight::StaticClass());

	UpdateTeamVisuals();
}

// Called every frame
void ACCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void ACCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void ACCharacter::Multicast_SetSkeletalMesh_Implementation(USkeletalMesh* NewMesh)
{
	GetMesh()->SetSkeletalMesh(NewMesh);
}

void ACCharacter::Server_SendGameplayEventToSelf_Implementation(const FGameplayTag& EventTag, const FGameplayEventData& EventData)
{
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, EventTag, EventData);
}

bool ACCharacter::Server_SendGameplayEventToSelf_Validate(const FGameplayTag& EventTag, const FGameplayEventData& EventData)
{
	return true;
}

UAbilitySystemComponent* ACCharacter::GetAbilitySystemComponent() const
{
	return CAbilitySystemComponent;
}

void ACCharacter::AddShieldBuffHandle(const FActiveGameplayEffectHandle& Handle)
{
	ActiveShieldBuffHandles.Add(Handle);
}

void ACCharacter::RemoveShieldBuffHandle(const FActiveGameplayEffectHandle& Handle)
{
	ActiveShieldBuffHandles.RemoveSingleSwap(Handle);
}

void ACCharacter::ClearShieldBuffEarly()
{
	if ( !HasAuthority() )
		return;

	if ( UAbilitySystemComponent* ASC = GetAbilitySystemComponent() )
	{
		for(const FActiveGameplayEffectHandle& Handle : ActiveShieldBuffHandles )
		{
			if( Handle.IsValid() )
			{
				ASC->RemoveActiveGameplayEffect(Handle);
			}
		}
	}

	ActiveShieldBuffHandles.Empty();
}

void ACCharacter::UpgradeAbilityWithInputID(ECAbilityInputID InputID)
{
	if (CAbilitySystemComponent)
	{
		CAbilitySystemComponent->Server_UpgradeAbilityWithID(InputID);
	}
}

void ACCharacter::BindGASChangeDelegates()
{
	if (CAbilitySystemComponent)
	{
		CAbilitySystemComponent->RegisterGameplayTagEvent(UCAbilitySystemStatics::GetDeadStatusTag()).AddUObject(this, &ACCharacter::DeathTagUpdated);
		CAbilitySystemComponent->RegisterGameplayTagEvent(UCAbilitySystemStatics::GetStunStatusTag()).AddUObject(this, &ACCharacter::StunTagUpdated);
		CAbilitySystemComponent->RegisterGameplayTagEvent(UCAbilitySystemStatics::GetAimStatusTag()).AddUObject(this, &ACCharacter::AimTagUpdated);
		CAbilitySystemComponent->RegisterGameplayTagEvent(UCAbilitySystemStatics::GetFocusStatusTag()).AddUObject(this, &ACCharacter::FocusTagUpdated);
		CAbilitySystemComponent->RegisterGameplayTagEvent(UCAbilitySystemStatics::GetBindStatusTag()).AddUObject(this, &ACCharacter::BindTagUpdated);
		CAbilitySystemComponent->RegisterGameplayTagEvent(UCAbilitySystemStatics::GetDisableJumpTag()).AddUObject(this, &ACCharacter::DisableJumpTagUpdated);
		CAbilitySystemComponent->RegisterGameplayTagEvent(UCAbilitySystemStatics::GetMatchNotStartedStatusTag()).AddUObject(this, &ACCharacter::MatchNotStartedTagUpdated);

		CAbilitySystemComponent->RegisterGameplayTagEvent(UCAbilitySystemStatics::GetBasicAttackAimStatusTag()).AddUObject(this, &ACCharacter::BasicAttackAimTagUpdated);
		CAbilitySystemComponent->RegisterGameplayTagEvent(UCAbilitySystemStatics::GetSkillAimStatusTag()).AddUObject(this, &ACCharacter::SkillAimTagUpdated);

		CAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UCAttributeSet::GetMoveSpeedAttribute()).AddUObject(this, &ACCharacter::MoveSpeedUpdated);
		CAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UCAttributeSet::GetMoveAccelerationAttribute()).AddUObject(this, &ACCharacter::MoveAccelerationUpdated);
		CAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UCAttributeSet::GetMaxHealthAttribute()).AddUObject(this, &ACCharacter::MaxHealthUpdated);
		CAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UCAttributeSet::GetMaxManaAttribute()).AddUObject(this, &ACCharacter::MaxManaUpdated);
	}
}

void ACCharacter::DeathTagUpdated(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount != 0)
	{
		StartDeathSequence();
	}
	else
	{
		Respawn();
	}
}

void ACCharacter::StunTagUpdated(const FGameplayTag Tag, int32 NewCount)
{
	if (IsDead())
		return;

	if (NewCount != 0)
	{
		OnStun();
		PlayAnimMontage(StunMontage);
	}
	else
	{
		OnRecoveryFromStun();
		StopAnimMontage(StunMontage);
	}
}

void ACCharacter::AimTagUpdated(const FGameplayTag Tag, int32 NewCount)
{
	SetIsAimming(NewCount != 0);
}

void ACCharacter::FocusTagUpdated(const FGameplayTag Tag, int32 NewCount)
{
	bIsInFocusMode = NewCount > 0;
}

void ACCharacter::BindTagUpdated(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount != 0)
	{
		OnBind();
		GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_None);
		PlayAnimMontage(BindMontage);
	}
	else
	{
		OnRecoveryFromBind();
		GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);
		StopAnimMontage(BindMontage);
	}
}

void ACCharacter::DisableJumpTagUpdated(const FGameplayTag Tag, int32 NewCount)
{
}

void ACCharacter::MatchNotStartedTagUpdated(const FGameplayTag Tag, int32 NewCount)
{
	if ( IsDead() )
		return;

	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	
	if ( NewCount != 0 )
	{
		MoveComp->StopMovementImmediately();
		MoveComp->SetMovementMode(EMovementMode::MOVE_None);
	}
	else if ( !CAbilitySystemComponent->HasMatchingGameplayTag(UCAbilitySystemStatics::GetBindStatusTag()) )
	{
		MoveComp->SetMovementMode(EMovementMode::MOVE_Walking);
	}
}

void ACCharacter::MoveSpeedUpdated(const FOnAttributeChangeData& Data)
{
	GetCharacterMovement()->MaxWalkSpeed = Data.NewValue;
}

void ACCharacter::MoveAccelerationUpdated(const FOnAttributeChangeData& Data)
{
	GetCharacterMovement()->MaxAcceleration = Data.NewValue;
}

void ACCharacter::MaxHealthUpdated(const FOnAttributeChangeData& Data)
{
	if (IsValid(CAttributeSet))
		CAttributeSet->RescaleHealth();
}

void ACCharacter::MaxManaUpdated(const FOnAttributeChangeData& Data)
{
	if (IsValid(CAttributeSet))
		CAttributeSet->RescaleMana();
}

void ACCharacter::SetIsAimming(bool bIsAimming)
{
	bUseControllerRotationYaw = bIsAimming;
	GetCharacterMovement()->bOrientRotationToMovement = !bIsAimming;
	OnAimStateChanged(bIsAimming);
}

void ACCharacter::OnAimStateChanged(bool bIsAiming)
{
	// 자식 클래스에서 오버라이드
}

void ACCharacter::BasicAttackAimTagUpdated(const FGameplayTag Tag, int32 NewCount)
{
	OnBasicAttackAimStateChanged(NewCount != 0);
}

void ACCharacter::SkillAimTagUpdated(const FGameplayTag Tag, int32 NewCount)
{
	OnSkillAimStateChanged(NewCount != 0);
}

void ACCharacter::OnBasicAttackAimStateChanged(bool bIsAiming)
{
	// 자식 클래스에서 오버라이드
}

void ACCharacter::OnSkillAimStateChanged(bool bIsAiming)
{
	// 자식 클래스에서 오버라이드
}

void ACCharacter::HideOverheadStatsGauge()
{
	bOverheadGaugeHidden = true;

	GetWorldTimerManager().ClearTimer(HeadStatGaugeVisiblityUpdateTimerHandle);

	if ( OverheadWidgetComponent )
	{
		OverheadWidgetComponent->SetHiddenInGame(true);
	}
}

void ACCharacter::ConfigureOverheadStatusWidget()
{
	if (!OverheadWidgetComponent)
		return;

	if ( bOverheadGaugeHidden )
	{
		GetWorldTimerManager().ClearTimer(HeadStatGaugeVisiblityUpdateTimerHandle);
		OverheadWidgetComponent->SetHiddenInGame(true);
		return;
	}

	if (IsLocallyControlledByPlayer())
	{
		if ( !bShowOverheadGaugeForSelf )
		{
			OverheadWidgetComponent->SetHiddenInGame(true);
			return;
		}

		OverheadWidgetComponent->SetRelativeLocation(OverheadGaugeOffset + OverheadGaugeOffsetForSelf);
	}
	else
	{
		OverheadWidgetComponent->SetRelativeLocation(OverheadGaugeOffset);
	}

	UOverHeadStatsGauge* OverheadStatsGauge = Cast <UOverHeadStatsGauge>(OverheadWidgetComponent->GetUserWidgetObject());

	if (OverheadStatsGauge)
	{
		OverheadStatsGauge->ConfigureWithASC(GetAbilitySystemComponent());
		OverheadWidgetComponent->SetHiddenInGame(true);
		
		CachedOverheadStatsGauge = OverheadStatsGauge;

		GetWorldTimerManager().ClearTimer(HeadStatGaugeVisiblityUpdateTimerHandle);
		GetWorldTimerManager().SetTimer(HeadStatGaugeVisiblityUpdateTimerHandle, this, &ACCharacter::UpdateHeadGaugeVisiblity, HeadStatGaugeVisiblityCheckUpdateGap, true, 0.f);

		OverheadStatsGauge->SetDistanceScaling(HeadGaugeMinScaleDistance, HeadGaugeMaxScaleDistance, HeadGaugeNearScale, HeadGaugeFarScale);

		UpdateOverheadName();
		UpdateTeamVisuals();
	}
}

void ACCharacter::UpdateHeadGaugeVisiblity()
{
	APawn* LocalPlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if ( !LocalPlayerPawn )
		return;

	const bool bInRange = FVector::DistSquared(GetActorLocation(), LocalPlayerPawn->GetActorLocation()) <= HeadStatGaugeVisiblityRangeSqaured;

	const bool bVisible = bInRange && UCAbilitySystemStatics::WasRecentlyRenderedOnScreen(GetMesh(), HeadGaugeRenderTolerance);

	if ( !bVisible && CachedOverheadStatsGauge )
	{
		CachedOverheadStatsGauge->SnapDistanceScale();
	}

	OverheadWidgetComponent->SetHiddenInGame(!bVisible);
}

void ACCharacter::SetStatusGaugeEnabled(bool bIsEnabled)
{
	GetWorldTimerManager().ClearTimer(HeadStatGaugeVisiblityUpdateTimerHandle);
	
	if (bIsEnabled)
	{
		ConfigureOverheadStatusWidget();
	}
	else
	{
		OverheadWidgetComponent->SetHiddenInGame(true);
	}
}

void ACCharacter::UpdateTeamVisuals()
{
	if ( IsNetMode(ENetMode::NM_DedicatedServer) )
		return;

	const int32 Relation = GetLocalRelation();

	// 머리 위 체력바 색 (외곽선 사용 여부와 무관하게 항상)
	if ( CachedOverheadStatsGauge )
	{
		CachedOverheadStatsGauge->SetTeamRelation(Relation);
	}

	// 외곽선
	int32 OutlineType = Relation;
	if ( !bUseOutline || !bOutlineEnabled || (Relation == ELocalRelation::Self && !bOutlineSelf) )
	{
		OutlineType = ELocalRelation::None;
	}

	ApplyOutline(OutlineType);
}

void ACCharacter::SetOutlineEnabled(bool bEnabled)
{
	bOutlineEnabled = bEnabled;
	UpdateTeamVisuals();
}

void ACCharacter::RefreshAllTeamVisuals(const UObject* WorldContextObject)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if ( !World || World->GetNetMode() == ENetMode::NM_DedicatedServer )
		return;

	for ( TActorIterator<ACCharacter> It(World); It; ++It )
	{
		It->UpdateTeamVisuals();
	}
}

int32 ACCharacter::GetLocalRelation() const
{
	FGenericTeamId LocalTeamId;
	if ( GetGenericTeamId() == FGenericTeamId::NoTeam || !GetLocalTeamId(GetWorld(), LocalTeamId) )
		return ELocalRelation::None;

	if ( IsPlayerControlled() && IsLocallyControlled() )
		return ELocalRelation::Self;

	return (GetGenericTeamId() == LocalTeamId) ? ELocalRelation::Ally : ELocalRelation::Enemy;
}

void ACCharacter::UpdateOverheadName()
{
	if ( !CachedOverheadStatsGauge )
		return;

	const APlayerState* PS = GetPlayerState();
	CachedOverheadStatsGauge->SetPlayerName(PS ? PS->GetPlayerName() : FString());
}

void ACCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	UpdateOverheadName();
}

void ACCharacter::ApplyOutline(int32 OutlineType)
{
	USkeletalMeshComponent* SMComp = GetMesh();
	if ( !SMComp )
		return;

	UMaterialInterface* OverlayToApply = nullptr;

	if ( OutlineType != ELocalRelation::None && OutlineMaterial )
	{
		if ( !OutlineMID )
		{
			OutlineMID = UMaterialInstanceDynamic::Create(OutlineMaterial, this);
		}

		const FLinearColor Color = OutlineType == ELocalRelation::Enemy ? EnemyOutlineColor : OutlineType == ELocalRelation::Ally ? AllyOutlineColor : SelfOutlineColor;

		OutlineMID->SetVectorParameterValue(TEXT("OutlineColor"), Color);
		OverlayToApply = OutlineMID;
	}

	TArray<USceneComponent*> Components;
	SMComp->GetChildrenComponents(true, Components);
	Components.Add(SMComp);

	for ( USceneComponent* Component : Components )
	{
		UMeshComponent* MeshComp = Cast<USkeletalMeshComponent>(Component);
		if ( !MeshComp )
		{
			MeshComp = Cast<UStaticMeshComponent>(Component);
		}

		if ( MeshComp )
		{
			MeshComp->SetOverlayMaterial(OverlayToApply);
		}
	}
}

bool ACCharacter::GetLocalTeamId(const UWorld* World, FGenericTeamId& OutTeamId)
{
	APlayerController* LocalPC = World ? World->GetFirstPlayerController() : nullptr;
	if ( !LocalPC || !LocalPC->IsLocalController() )
		return false;

	if ( const IGenericTeamAgentInterface* PawnTeamInterface = Cast<IGenericTeamAgentInterface>(LocalPC->GetPawn()) )
	{
		OutTeamId = PawnTeamInterface->GetGenericTeamId();
		if ( OutTeamId != FGenericTeamId::NoTeam )
			return true;
	}

	if ( const IGenericTeamAgentInterface* ControllerTeamInterface = Cast<IGenericTeamAgentInterface>(LocalPC) )
	{
		OutTeamId = ControllerTeamInterface->GetGenericTeamId();
		return OutTeamId != FGenericTeamId::NoTeam;
	}

	return false;
}

void ACCharacter::OnStun()
{
}

void ACCharacter::OnRecoveryFromStun()
{
}

void ACCharacter::OnBind()
{
}

void ACCharacter::OnRecoveryFromBind()
{
}

bool ACCharacter::IsDead() const
{
	return GetAbilitySystemComponent()->HasMatchingGameplayTag(UCAbilitySystemStatics::GetDeadStatusTag());
}

void ACCharacter::RespawnImmediately()
{
	if (HasAuthority())
	{
		GetAbilitySystemComponent()->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(UCAbilitySystemStatics::GetDeadStatusTag()));
	}
}

void ACCharacter::RecordDamagingHero(AActor* HeroCauser)
{
	if ( !HasAuthority() || !HeroCauser )
		return;

	LastDamagingHero = HeroCauser;
	LastDamagingHeroTimestamp = GetWorld()->GetTimeSeconds();

	RecentDamagingHeroTimestamps.Add(HeroCauser, GetWorld()->GetTimeSeconds());
}

AActor* ACCharacter::GetLastDamagingHeroIfRecent() const
{
	if ( !LastDamagingHero.IsValid() )
		return nullptr;

	if ( LastDamagingHeroTimestamp < 0.f )
		return nullptr;

	if ( GetWorld()->GetTimeSeconds() - LastDamagingHeroTimestamp > LastDamagingHeroMemoryDuration )
		return nullptr;

	return LastDamagingHero.Get();
}

TArray<AActor*> ACCharacter::GetRecentAssistContributors(AActor* ExcludeActor) const
{
	TArray<AActor*> Result;
	float Now = GetWorld()->GetTimeSeconds();

	for ( const TPair<TWeakObjectPtr<AActor>, float>& Pair : RecentDamagingHeroTimestamps )
	{
		AActor* Hero = Pair.Key.Get();
		if ( !Hero || Hero == ExcludeActor )
			continue;

		if ( Now - Pair.Value > AssistMemoryDuration )
			continue;

		Result.Add(Hero);
	}

	return Result;
}

float ACCharacter::GetRespawnDurationTimeForLevel(int32 Level) const
{
	if ( RespawnDurationTimeForLevel.Num() == 0 )
		return 5.f;

	int32 Index = FMath::Clamp(Level - 1, 0, RespawnDurationTimeForLevel.Num() - 1);

	return RespawnDurationTimeForLevel[ Index ];
}

void ACCharacter::RecordShieldProvider(AActor* ShieldProvider)
{
	if ( !HasAuthority() || !ShieldProvider || ShieldProvider == this )
		return;

	RecentShieldProviderTimestamps.Add(ShieldProvider, GetWorld()->GetTimeSeconds());
}

TArray<AActor*> ACCharacter::GetRecentShieldContributors(AActor* ExcludeActor) const
{
	TArray<AActor*> Result;
	float Now = GetWorld()->GetTimeSeconds();

	for ( const TPair<TWeakObjectPtr<AActor>, float>& Pair : RecentShieldProviderTimestamps )
	{
		AActor* Provider = Pair.Key.Get();
		if ( !Provider || Provider == ExcludeActor )
			continue;

		if ( Now - Pair.Value > ShieldAssistMemoryDuration )
			continue;

		Result.Add(Provider);
	}

	return Result;
}

void ACCharacter::DeathMontageFinished()
{
	if (IsDead())
	{
		SetRagdollEnabled(true);
	}
}

void ACCharacter::SetRagdollEnabled(bool bIsEnabled)
{
	if (bIsEnabled)
	{
		GetMesh()->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
		GetMesh()->SetSimulatePhysics(true);
		GetMesh()->SetAllBodiesSimulatePhysics(true);
	}
	else
	{
		GetMesh()->SetSimulatePhysics(false);
		GetMesh()->SetCollisionProfileName(TEXT("NoCollision"));
		GetMesh()->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		GetMesh()->SetRelativeTransform(MeshRelativeTransform);
	}
}

void ACCharacter::PlayDeathAnimation()
{
	static const bool bNoOptDeathAnim = CoreBench::IsOptOff(TEXT("NoOptDeathAnim"));

	if ( GetNetMode() == NM_DedicatedServer && !bNoOptDeathAnim )
		return;

	if (DeathMontage)
	{
		float MontageDuration = PlayAnimMontage(DeathMontage);
		GetWorldTimerManager().SetTimer(DeathMontageTimerHandle, this, &ACCharacter::DeathMontageFinished, MontageDuration + DeathMontageFinishTimeShift);
	}
	else
	{
		SetRagdollEnabled(true);
	}
}

void ACCharacter::StartDeathSequence()
{
	SetOutlineEnabled(false);

	OnDead();

	if (CAbilitySystemComponent)
	{
		CAbilitySystemComponent->CancelAllAbilities();
	}

	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_None);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PlayDeathAnimation();
	SetStatusGaugeEnabled(false);

	SetAIPerceptionStimuliSourceEnabled(false);
}

void ACCharacter::Respawn()
{
	SetOutlineEnabled(true);

	SetAIPerceptionStimuliSourceEnabled(true);

	SetRagdollEnabled(false);

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);

	GetMesh()->GetAnimInstance()->StopAllMontages(0.f);

	SetStatusGaugeEnabled(true);

	if (HasAuthority() && GetController())
	{
		TWeakObjectPtr<AActor> StartSpot = GetController()->StartSpot;

		if (StartSpot.IsValid())
		{
			SetActorTransform(StartSpot->GetActorTransform());
		}
	}

	if ( HasAuthority() )
	{
		if ( ACPlayerState* CPlayerState = GetPlayerState<ACPlayerState>() )
		{
			CPlayerState->SetRespawnTimeStamp(-1.f);
		}
	}

	if ( RespawnMontage )
	{
		PlayAnimMontage(RespawnMontage);
	}

	if (CAbilitySystemComponent)
	{
		CAbilitySystemComponent->ApplyFullStatEffect();
	}

	OnRespawn();
}

void ACCharacter::OnDead()
{
}

void ACCharacter::OnRespawn()
{
}

void ACCharacter::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	TeamID = NewTeamID;
	UpdateTeamVisuals();
}

FGenericTeamId ACCharacter::GetGenericTeamId() const
{
	return TeamID;
}

void ACCharacter::OnRep_TeamID()
{
	UpdateTeamVisuals();

	if ( IsPlayerControlled() && IsLocallyControlled() )
	{
		RefreshAllTeamVisuals(this);
	}
}

void ACCharacter::SetAIPerceptionStimuliSourceEnabled(bool bIsEnabled)
{
	if (!PerceptionStimuliSourceComponent)
		return;
	 
	if (bIsEnabled)
	{
		PerceptionStimuliSourceComponent->RegisterWithPerceptionSystem();
	}
	else
	{
		PerceptionStimuliSourceComponent->UnregisterFromPerceptionSystem();
	}
}

