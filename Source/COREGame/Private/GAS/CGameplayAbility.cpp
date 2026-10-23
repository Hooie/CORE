// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/CGameplayAbility.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GAS/CAbilitySystemStatics.h"
#include "GAS/CAttributeSet.h"
#include "GAS/CHeroAttributeSet.h"
#include "GAS/GAP_Launched.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Player/CPlayerController.h"
#include "Widgets/GameplayWidget.h"

UCGameplayAbility::UCGameplayAbility()
{
    ActivationBlockedTags.AddTag(UCAbilitySystemStatics::GetStunStatusTag());
	ActivationBlockedTags.AddTag(UCAbilitySystemStatics::GetDeadStatusTag());
	ActivationBlockedTags.AddTag(UCAbilitySystemStatics::GetMatchNotStartedStatusTag());
}

bool UCGameplayAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, OUT FGameplayTagContainer* OptionalRelevantTags) const
{
    FGameplayAbilitySpec* AbilitySpec = ActorInfo->AbilitySystemComponent->FindAbilitySpecFromHandle(Handle);
    if (AbilitySpec && AbilitySpec->Level <= 0)
        return false;

    return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

AActor* UCGameplayAbility::GetAimTarget(float AimDistance, ETeamAttitude::Type TeamAttitude) const
{
    AActor* OwnerAvatarActor = GetAvatarActorFromActorInfo();

    if (OwnerAvatarActor)
    {
        FVector Location;
        FRotator Rotation;
        OwnerAvatarActor->GetActorEyesViewPoint(Location, Rotation);

        FVector AimEnd = Location + Rotation.Vector() * AimDistance;

        TArray<FHitResult> HitResults;

        FCollisionObjectQueryParams CollisionObjectQueryParams;
        CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

        FCollisionQueryParams CollisionQueryParams;
        CollisionQueryParams.AddIgnoredActor(OwnerAvatarActor);

        if (ShouldDrawDebug())
        {
            DrawDebugLine(GetWorld(), Location, AimEnd, FColor::Red, false, 2.f, 0U, 3.f);
        }

        if ((GetWorld()->LineTraceMultiByObjectType(HitResults, Location, AimEnd, CollisionObjectQueryParams, CollisionQueryParams)))
        {
            for (FHitResult& HitResult : HitResults)
            {
                if (IsActorTeamAttitude(HitResult.GetActor(), TeamAttitude))
                {
                    return HitResult.GetActor();
                }
            }
        }
    }

    return nullptr;
}

UAnimInstance* UCGameplayAbility::GetOwnerAnimInstance() const
{
    USkeletalMeshComponent* OwnerSkeletalMeshComp = GetOwningComponentFromActorInfo();

    if (OwnerSkeletalMeshComp)
    {
        return OwnerSkeletalMeshComp->GetAnimInstance();
    }

    return nullptr;
}

TArray<FHitResult> UCGameplayAbility::GetHitResultFromSweepLocationTargetData(const FGameplayAbilityTargetDataHandle& TargetDataHandle, float SphereSweepRadius, ETeamAttitude::Type TargetTeam, bool bDrawDebug, bool bIgnoreSelf) const
{
    TArray<FHitResult> OutResults;
    TSet<AActor*> HitActors;

    IGenericTeamAgentInterface* OwnerTeamInterface = Cast<IGenericTeamAgentInterface>(GetAvatarActorFromActorInfo());
    
    for (const TSharedPtr<FGameplayAbilityTargetData>& TargetData : TargetDataHandle.Data)
    {
        FVector StartLoc = TargetData->GetOrigin().GetTranslation();
        FVector EndLoc = TargetData->GetEndPoint();

        TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
        ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

        TArray<AActor*> ActorsToIgnore;
        if (bIgnoreSelf)
        {
            ActorsToIgnore.Add(GetAvatarActorFromActorInfo());
        }

        EDrawDebugTrace::Type DrawDebugTrace = bDrawDebug ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None;

        TArray<FHitResult> Results;

        UKismetSystemLibrary::SphereTraceMultiForObjects(this, StartLoc, EndLoc, SphereSweepRadius, ObjectTypes, false, ActorsToIgnore, DrawDebugTrace, Results, false);

        for (const FHitResult& Result : Results)
        {
            if (HitActors.Contains(Result.GetActor()))
                continue;

            if (OwnerTeamInterface)
            {
                ETeamAttitude::Type OtherActorTeamAttitude = OwnerTeamInterface->GetTeamAttitudeTowards(*Result.GetActor());
                if (OtherActorTeamAttitude != TargetTeam)
                    continue;
            }

            HitActors.Add(Result.GetActor());
            OutResults.Add(Result);
        }
    }

    return OutResults;
}

FGameplayEffectSpecHandle UCGameplayAbility::MakeScaledDamageSpec(TSubclassOf<UGameplayEffect> DamageEffect, float BaseDamage, float ScalingRatio, int Level)
{
	FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(DamageEffect, Level);
	Spec.Data->SetSetByCallerMagnitude(UCAbilitySystemStatics::GetAbilityBaseDamageTag(), BaseDamage);
	Spec.Data->SetSetByCallerMagnitude(UCAbilitySystemStatics::GetAbilityScalingRatioTag(), ScalingRatio);

	return Spec;
}

FGameplayEffectSpecHandle UCGameplayAbility::MakeScaledDamageSpec(TSubclassOf<UGameplayEffect> DamageEffect, float BaseDamage, float StrengthRatio, float IntelligenceRatio, int Level)
{
	FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(DamageEffect, Level);
	if ( Spec.IsValid() )
	{
		Spec.Data->SetSetByCallerMagnitude(UCAbilitySystemStatics::GetAbilityBaseDamageTag(), BaseDamage);
		Spec.Data->SetSetByCallerMagnitude(UCAbilitySystemStatics::GetDamageStrengthRatioTag(), StrengthRatio);
		Spec.Data->SetSetByCallerMagnitude(UCAbilitySystemStatics::GetDamageIntelligenceRatioTag(), IntelligenceRatio);
	}
	return Spec;
}

float UCGameplayAbility::GetLevelIndexedDamage(const TArray<float>& DamagePerLevel, int32 Level) const
{
	if (DamagePerLevel.Num() == 0)
		return 0.f;

	int32 Index = FMath::Clamp(Level - 1, 0, DamagePerLevel.Num() - 1);
	return DamagePerLevel[Index];
}

float UCGameplayAbility::GetScaledAmount(const TArray<float>& BasePerLevel, int32 Level, float StrengthRatio, float IntelligenceRatio) const
{
	float Amount = GetLevelIndexedDamage(BasePerLevel, Level);

	if ( const UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo() )
	{
		bool bFound = false;
		Amount += StrengthRatio * OwnerASC->GetGameplayAttributeValue(UCHeroAttributeSet::GetStrengthAttribute(), bFound);
		Amount += IntelligenceRatio * OwnerASC->GetGameplayAttributeValue(UCHeroAttributeSet::GetIntelligenceAttribute(), bFound);
	}

	return FMath::Max(0.f, Amount);
}

void UCGameplayAbility::PushSelf(const FVector& PushVel)
{
    ACharacter* OwningAvatarCharacter = GetOwningAvatarCharacter();
    if (OwningAvatarCharacter)
    {
        OwningAvatarCharacter->LaunchCharacter(PushVel, true, true);
    }
}

void UCGameplayAbility::PushTarget(AActor* Target, const FVector& PushVel)
{
    if (!Target)
        return;

    FGameplayEventData EventData;

    FGameplayAbilityTargetData_SingleTargetHit* HitData = new FGameplayAbilityTargetData_SingleTargetHit;
    FHitResult HitResult;

    // HitResult�� ImapctNormal ������ PushVel ���� �����ؼ� ����Ƽ�� �̺�Ʈ ������ �ѱ��
    HitResult.ImpactNormal = PushVel.GetSafeNormal();
    HitResult.Location = PushVel;

    HitData->HitResult = HitResult;
    EventData.TargetData.Add(HitData);

    UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Target, UGAP_Launched::GetLaunchedAbilityActivationTag(), EventData);
}

void UCGameplayAbility::PushTargets(const TArray<AActor*>& Targets, const FVector& PushVel)
{
    for (AActor* Target : Targets)
    {
        PushTarget(Target, PushVel);
    }
}

void UCGameplayAbility::PushTargets(const FGameplayAbilityTargetDataHandle& TargetDataHandle, const FVector& PushVel)
{
    TArray<AActor*> Targets = UAbilitySystemBlueprintLibrary::GetAllActorsFromTargetData(TargetDataHandle);
    PushTargets(Targets, PushVel);
}

void UCGameplayAbility::PushTargetsFromLocation(const TArray<AActor*>& Targets, const FVector& FromLocation, float PushSpeed, bool bWithZ)
{
    for (AActor* Target : Targets)
    {
        FVector PushDir = Target->GetActorLocation() - FromLocation;
        PushDir.Z = 0;
        PushDir.Normalize();

        FVector PushVel = PushDir * PushSpeed;
        PushVel.Z = PushSpeed * (bWithZ ? 0.5f : 0.f);

        PushTarget(Target, PushVel);
    }
}

void UCGameplayAbility::PushTargetsFromLocation(const FGameplayAbilityTargetDataHandle& TargetDataHandle, const FVector& FromLocation, float PushSpeed, bool bWithZ)
{
    TArray<AActor*> Targets = UAbilitySystemBlueprintLibrary::GetAllActorsFromTargetData(TargetDataHandle);

    PushTargetsFromLocation(Targets, FromLocation, PushSpeed, bWithZ);
}

void UCGameplayAbility::PushTargetsFromOwnerLocation(const TArray<AActor*>& Targets, float PushSpeed, bool bWithZ)
{
    AActor* OwnerAvatarActor = GetAvatarActorFromActorInfo();
    if (!OwnerAvatarActor)
        return;

    FVector OwnerAvatarActorLocation = OwnerAvatarActor->GetActorLocation();
    PushTargetsFromLocation(Targets, OwnerAvatarActorLocation, PushSpeed, bWithZ);
}

void UCGameplayAbility::PushTargetsFromOwnerLocation(const FGameplayAbilityTargetDataHandle& TargetDataHandle, float PushSpeed, bool bWithZ)
{
    TArray<AActor*> Targets = UAbilitySystemBlueprintLibrary::GetAllActorsFromTargetData(TargetDataHandle);

    PushTargetsFromOwnerLocation(Targets, PushSpeed, bWithZ);
}

void UCGameplayAbility::PlayMontageLocally(UAnimMontage* MontageToPlay)
{
    UAnimInstance* OwnerAnimInst = GetOwnerAnimInstance();
    if (OwnerAnimInst && !OwnerAnimInst->Montage_IsPlaying(MontageToPlay))
    {
        OwnerAnimInst->Montage_Play(MontageToPlay);
    }
}

void UCGameplayAbility::StopMontageAfterCurrentSection(UAnimMontage* MontageToStop)
{
    UAnimInstance* OwnerAnimInst = GetOwnerAnimInstance();
    if (OwnerAnimInst)
    {
        FName CurrentSectionName = OwnerAnimInst->Montage_GetCurrentSection(MontageToStop);
        OwnerAnimInst->Montage_SetNextSection(CurrentSectionName, NAME_None, MontageToStop);
    }
}

ACharacter* UCGameplayAbility::GetOwningAvatarCharacter()
{
    if (!AvatarCharacter)
    {
        AvatarCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    }

    return AvatarCharacter;
}

void UCGameplayAbility::ApplyGameplayEffectToHitResultActor(const FHitResult& HitResult, TSubclassOf<UGameplayEffect> GameplayEffect, int Level)
{
	FGameplayEffectSpecHandle EffectSpecHandle = MakeOutgoingGameplayEffectSpec(GameplayEffect, Level);

	ApplyGameplayEffectToHitResultActor(HitResult, EffectSpecHandle);
}

void UCGameplayAbility::ApplyGameplayEffectToHitResultActor(const FHitResult& HitResult, FGameplayEffectSpecHandle EffectSpecHandle)
{
	FGameplayEffectContextHandle EffectContext = MakeEffectContext(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo());
	EffectContext.AddHitResult(HitResult);

	EffectSpecHandle.Data->SetContext(EffectContext);

	ApplyGameplayEffectSpecToTarget(GetCurrentAbilitySpecHandle(), CurrentActorInfo, CurrentActivationInfo, EffectSpecHandle, UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActor(HitResult.GetActor()));
}

FGenericTeamId UCGameplayAbility::GetOwnerTeamId() const
{
    IGenericTeamAgentInterface* OwnerTeamInterface = Cast<IGenericTeamAgentInterface>(GetAvatarActorFromActorInfo());
    if (OwnerTeamInterface)
    {
        return OwnerTeamInterface->GetGenericTeamId();
    }
    
    return FGenericTeamId::NoTeam;
}


bool UCGameplayAbility::IsActorTeamAttitude(const AActor* OtherActor, ETeamAttitude::Type TeamAttitude) const
{
    if (!OtherActor)
        return false;

    IGenericTeamAgentInterface* OwnerTeamAgentInterface = Cast<IGenericTeamAgentInterface>(GetAvatarActorFromActorInfo());
    if (OwnerTeamAgentInterface)
    {
        return OwnerTeamAgentInterface->GetTeamAttitudeTowards(*OtherActor) == TeamAttitude;
    }

    return false;
}

void UCGameplayAbility::SendLocalGameplayEvent(const FGameplayTag& EventTag, const FGameplayEventData& EventData)
{
    UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo();
    if (OwnerASC)
    {
        OwnerASC->HandleGameplayEvent(EventTag, &EventData);
    }
}

UGameplayWidget* UCGameplayAbility::GetOwningGameplayWidget() const
{
	ACPlayerController* PC = Cast<ACPlayerController>(GetOwningActorFromActorInfo());
	if (!PC)
	{
		if (APawn* Pawn = Cast<APawn>(GetOwningActorFromActorInfo()))
		{
			PC = Cast <ACPlayerController>(Pawn->GetController());
		}
	}

	if (!PC || !PC->IsLocalController())
		return nullptr;

	return PC->GetGameplayWidget();
}
