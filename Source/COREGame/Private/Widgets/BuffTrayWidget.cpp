// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/BuffTrayWidget.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/HorizontalBox.h"
#include "GAS/CAbilitySystemStatics.h"
#include "GAS/CGameplayAbilityTypes.h"
#include "Widgets/BuffIconWidget.h"

void UBuffTrayWidget::NativeConstruct()
{
	Super::NativeConstruct();

	UAbilitySystemComponent* OwnerASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwningPlayerPawn());
	if ( !OwnerASC )
		return;

	OwnerASC->OnActiveGameplayEffectAddedDelegateToSelf.AddUObject(this, &UBuffTrayWidget::OnEffectApplied);
}

void UBuffTrayWidget::OnEffectApplied(UAbilitySystemComponent* ASC, const FGameplayEffectSpec& Spec, FActiveGameplayEffectHandle Handle)
{
	if ( !Spec.Def || !BuffIconWidgetClass )
		return;

	const FGameplayTagContainer& GrantedTags = Spec.Def->InheritableOwnedTagsContainer.CombinedTags;

	const FBuffIconData* IconData = UCAbilitySystemStatics::FindBuffIconDataForTags(BuffIconDataTable, GrantedTags);
	if ( !IconData )
		return;

	UBuffIconWidget* NewIcon = CreateWidget<UBuffIconWidget>(this, BuffIconWidgetClass);
	if ( !NewIcon )
		return;

	UHorizontalBox* TargetContainer = IconData->bIsDebuff ? DebuffContainer : BuffContainer;
	TargetContainer->AddChild(NewIcon);

	float StartTime = 0.f, Duration = 0.f;
	ASC->GetActiveGameplayEffects().GetGameplayEffectStartTimeAndDuration(Handle, StartTime, Duration);

	if ( Duration == UGameplayEffect::INFINITE_DURATION )
	{
		Duration = -1.f;
	}

	NewIcon->Configure(IconData->Icon.LoadSynchronous(), StartTime, Duration, IconData->bIsDebuff);
	NewIcon->SetStackCount(Spec.GetStackCount());

	ActiveIcons.Add(Handle, NewIcon);

	if ( FOnActiveGameplayEffectRemoved_Info* RemovedDelegate = ASC->OnGameplayEffectRemoved_InfoDelegate(Handle) )
	{
		RemovedDelegate->AddUObject(this, &UBuffTrayWidget::OnEffectRemoved, Handle);
	}

	if ( FOnActiveGameplayEffectTimeChange* TimeChangedDelegate = ASC->OnGameplayEffectTimeChangeDelegate(Handle) )
	{
		TimeChangedDelegate->AddUObject(this, &UBuffTrayWidget::OnEffectTimeChanged);
	}

	if ( FOnActiveGameplayEffectStackChange* StackChangedDelegate = ASC->OnGameplayEffectStackChangeDelegate(Handle) )
	{
		StackChangedDelegate->AddUObject(this, &UBuffTrayWidget::OnEffectStackChanged);
	}
}

void UBuffTrayWidget::OnEffectRemoved(const FGameplayEffectRemovalInfo& RemovalInfo, FActiveGameplayEffectHandle Handle)
{
	if ( UBuffIconWidget** FoundIcon = ActiveIcons.Find(Handle) )
	{
		if ( *FoundIcon )
		{
			(*FoundIcon)->RemoveFromParent();
		}
		ActiveIcons.Remove(Handle);
	}
}

void UBuffTrayWidget::OnEffectTimeChanged(FActiveGameplayEffectHandle Handle, float NewStartTime, float NewDuration)
{
	UBuffIconWidget** FoundIcon = ActiveIcons.Find(Handle);
	if ( !FoundIcon || !*FoundIcon )
		return;

	if ( NewDuration == UGameplayEffect::INFINITE_DURATION )
	{
		NewDuration = -1.f;
	}

	(*FoundIcon)->RefreshTime(NewStartTime, NewDuration);
}

void UBuffTrayWidget::OnEffectStackChanged(FActiveGameplayEffectHandle Handle, int32 NewStackCount, int32 PreviousStackCount)
{
	if ( UBuffIconWidget** FoundIcon = ActiveIcons.Find(Handle) )
	{
		if ( *FoundIcon )
		{
			(*FoundIcon)->SetStackCount(NewStackCount);
		}
	}
}
