// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/GCN_DamagePopup.h"
#include "GAS/CAbilitySystemStatics.h"
#include "Kismet/GameplayStatics.h"
#include "Widgets/DamagePopupInstance.h"

AGCN_DamagePopup::AGCN_DamagePopup()
{
	PrimaryActorTick.bCanEverTick = false;
	bAutoDestroyOnRemove = true; // GAS가 큐 제거 시 자동으로 이 액터를 정리
}

bool AGCN_DamagePopup::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	if ( !PopupInstanceClass )
		return true;

	// 이 화면의 주인(로컬 플레이어)과 관련된 팝업만 표시
	const APawn* LocalPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if ( !LocalPawn )
		return true;

	const bool bTargetIsMe = MyTarget == LocalPawn;
	const bool bCausedByMe = Parameters.Instigator.Get() == LocalPawn || Parameters.EffectContext.GetEffectCauser() == LocalPawn;

	// 힐: 내가 받았거나 내가 준 힐 / 데미지: 내가 입었거나 입힌 피해
	const bool bRelatedToMe = bTargetIsMe || bCausedByMe;
	if ( !bRelatedToMe )
		return true;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const FVector RandomOffset(
		FMath::FRandRange(-15.f, 15.f),
		FMath::FRandRange(0.f, 30.f),
		FMath::FRandRange(50.f, 100.f)
	);

	ADamagePopupInstance* Instance = GetWorld()->SpawnActor<ADamagePopupInstance>(PopupInstanceClass, Parameters.Location + RandomOffset, FRotator::ZeroRotator, SpawnParams);
	if ( !Instance )
		return true;

	const bool bSecondaryFlag = bIsHealCue
		? Parameters.AggregatedSourceTags.HasTag(UCAbilitySystemStatics::GetLifeStealHealTypeTag())
		: Parameters.AggregatedSourceTags.HasTag(UCAbilitySystemStatics::GetMagicalDamageTypeTag());

	const bool bIsItemBonus = !bIsHealCue && Parameters.AggregatedSourceTags.HasTag(UCAbilitySystemStatics::GetItemBonusDamageTag());

	// 힐은 항상 OutGoing 스타일, 데미지는 내가 맞았으면 Incoming / 내가 때렸으면 OutGoing
	const EDamagePopupDirection Direction = (bIsHealCue || !bTargetIsMe) ? EDamagePopupDirection::OutGoing : EDamagePopupDirection::Incoming;

	Instance->Show(DamageWidgetClass, Parameters.RawMagnitude, bIsHealCue, bSecondaryFlag, Direction, bIsItemBonus);

	return true;
}
