// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Items/GA_Item_LowHealthShield.h"
#include "GAS/CAbilitySystemStatics.h"
#include "GAS/Items/CItemStatics.h"

UGA_Item_LowHealthShield::UGA_Item_LowHealthShield()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	ActivationRequiredTags.Reset();
	ActivationRequiredTags.AddTag(TAG_Status_Health_Below30);

	AbilityTriggers.Reset();

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::OwnedTagAdded;
	TriggerData.TriggerTag = TAG_Status_Health_Below30;
	AbilityTriggers.Add(TriggerData);
}
