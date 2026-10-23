// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/OverHeadStatsGauge.h"
#include "AbilitySystemComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Character/CCharacter.h"
#include "Components/TextBlock.h"
#include "GameFramework/Pawn.h"
#include "GAS/CAttributeSet.h"
#include "GAS/CHeroAttributeSet.h"
#include "Kismet/GameplayStatics.h"
#include "Widgets/ValueGauge.h"

void UOverHeadStatsGauge::ConfigureWithASC(UAbilitySystemComponent* AbilitySystemComponent)
{
	if ( !AbilitySystemComponent )
		return;

	OwnerASC = AbilitySystemComponent;
	OwnerActor = AbilitySystemComponent->GetOwner();

	HealthBar->SetAndBoundToGameplayAttribute(AbilitySystemComponent, UCAttributeSet::GetHealthAttribute(), UCAttributeSet::GetMaxHealthAttribute(), UCHeroAttributeSet::GetHealthRegenAttribute(), UCAttributeSet::GetShieldAttribute());

	if ( ManaBar )
	{
		ManaBar->SetAndBoundToGameplayAttribute(AbilitySystemComponent, UCAttributeSet::GetManaAttribute(), UCAttributeSet::GetMaxManaAttribute(), UCHeroAttributeSet::GetManaRegenAttribute());
	}

	if ( LevelText )
	{
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UCHeroAttributeSet::GetLevelAttribute()).RemoveAll(this);
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UCHeroAttributeSet::GetLevelAttribute()).AddUObject(this, &UOverHeadStatsGauge::OnLevelChanged);

		bool bFound = false;
		float CurrentLevel = AbilitySystemComponent->GetGameplayAttributeValue(UCHeroAttributeSet::GetLevelAttribute(), bFound);

		if ( bFound )
		{
			LevelText->SetText(FText::FromString(FString::FromInt(( int ) CurrentLevel)));
		}
	}
}

void UOverHeadStatsGauge::SetDistanceScaling(float InMinDistance, float InMaxDistance, float InNearScale, float InFarScale)
{
	MinScaleDistance = InMinDistance;
	MaxScaleDistance = InMaxDistance;
	NearScale = InNearScale;
	FarScale = InFarScale;

	bDistanceScalingEnabled = true;
	CurrentScale = -1.f;
}

void UOverHeadStatsGauge::SetTeamRelation(int32 Relation)
{
	if ( !HealthBar || !bUseTeamHealthColor )
		return;

	switch ( Relation )
	{
	case ELocalRelation::Self:
		HealthBar->SetHealthBarBaseColor(SelfHealthColor);
		break;

	case ELocalRelation::Ally:
		HealthBar->SetHealthBarBaseColor(AllyHealthColor);
		break;

	case ELocalRelation::Enemy:
		HealthBar->SetHealthBarBaseColor(EnemyHealthColor);
		break;

	default:
		HealthBar->ResetHealthBarBaseColor();
		break;
	}
}

void UOverHeadStatsGauge::SetPlayerName(const FString& PlayerName)
{
	if ( !NameText )
		return;

	NameText->SetText(FText::FromString(PlayerName));
	NameText->SetVisibility(PlayerName.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
}

void UOverHeadStatsGauge::SnapDistanceScale()
{
	if ( !bDistanceScalingEnabled )
		return;

	float TargetScale = 0.f;
	if ( !ComputeTargetScale(TargetScale) )
		return;

	if ( FMath::IsNearlyEqual(TargetScale, CurrentScale, 0.0005f) )
		return;

	CurrentScale = TargetScale;
	SetRenderScale(FVector2D(CurrentScale, CurrentScale));
}

void UOverHeadStatsGauge::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	UpdateDistanceScale(InDeltaTime);
}

void UOverHeadStatsGauge::OnLevelChanged(const FOnAttributeChangeData& Data)
{
	if ( LevelText )
	{
		LevelText->SetText(FText::FromString(FString::FromInt(( int ) Data.NewValue)));
	}
}

void UOverHeadStatsGauge::UpdateDistanceScale(float DeltaTime)
{
	if ( !bDistanceScalingEnabled )
		return;

	float TargetScale = 0.f;
	if ( !ComputeTargetScale(TargetScale) )
		return;

	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	const bool bSnap = CurrentScale < 0.f || (Now - LastScalUpdateTime) > 0.25f || ScaleInterpSpeed <= 0.f;

	LastScalUpdateTime = Now;

	const float NewScale = bSnap ? TargetScale : FMath::FInterpTo(CurrentScale, TargetScale, DeltaTime, ScaleInterpSpeed);

	if ( FMath::IsNearlyEqual(NewScale, CurrentScale, 0.0005f) )
		return;

	CurrentScale = NewScale;
	SetRenderScale(FVector2D(CurrentScale, CurrentScale));
}

bool UOverHeadStatsGauge::ComputeTargetScale(float& OutScale) const
{
	const AActor* Owner = GetOwnerActor();
	if ( !Owner )
		return false;

	FVector ViewerLocation;
	if ( const APawn* LocalPawn = UGameplayStatics::GetPlayerPawn(this, 0) )
	{
		ViewerLocation = LocalPawn->GetActorLocation();
	}
	else if ( const APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0) )
	{
		ViewerLocation = CameraManager->GetCameraLocation();
	}
	else
	{
		return false;
	}

	const float Distance = FVector::Dist(ViewerLocation, Owner->GetActorLocation());
	const float Alpha = FMath::GetMappedRangeValueClamped(FVector2D(MinScaleDistance, MaxScaleDistance), FVector2D(0.f, 1.f), Distance);

	OutScale = FMath::Lerp(NearScale, FarScale, Alpha);
	return true;
}
