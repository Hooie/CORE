// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/BaseZone.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/BoxComponent.h"
#include "GAS/CAbilitySystemStatics.h"
#include "GenericTeamAgentInterface.h"

// Sets default values
ABaseZone::ABaseZone()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	TriggerBoxVolume = CreateDefaultSubobject<UBoxComponent>("Trigger Box Volume");
	SetRootComponent(TriggerBoxVolume);

	TriggerBoxVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerBoxVolume->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	TriggerBoxVolume->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECR_Overlap);
}

float ABaseZone::GetTriggerVolumeHalfHeight()
{
	return TriggerBoxVolume->GetScaledBoxExtent().Z / 2.f;
}

// Called when the game starts or when spawned
void ABaseZone::BeginPlay()
{
	Super::BeginPlay();
	
	TriggerBoxVolume->OnComponentBeginOverlap.AddDynamic(this, &ABaseZone::OnOverlapBegin);
	TriggerBoxVolume->OnComponentEndOverlap.AddDynamic(this, &ABaseZone::OnOverlapEnd);

	if ( HasAuthority() )
	{
		GetWorldTimerManager().SetTimer(InitialOverlapCheckTimerHandle, this, &ABaseZone::CheckInitialOverlaps, 1.f, false);
	}
}

void ABaseZone::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if ( !HasAuthority() || !OtherActor )
		return;

	if ( ActorsInBase.Contains(OtherActor) )
		return;

	IGenericTeamAgentInterface* TeamInterface = Cast<IGenericTeamAgentInterface>(OtherActor);
	if ( !TeamInterface || TeamInterface->GetGenericTeamId().GetId() != TeamId )
		return;

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor);
	if ( !ASC || !InBaseEffect )
	{
		UE_LOG(LogTemp, Warning, TEXT("[BaseZone] ASC 또는 InBaseEffect 없음 — InBase 태그를 부여할 수 없습니다."));
		return;
	}

	ActorsInBase.Add(OtherActor);

	FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectToSelf(InBaseEffect->GetDefaultObject<UGameplayEffect>(), 1.f, ASC->MakeEffectContext());
	ActiveRegenEffects.Add(OtherActor, Handle);
}

void ABaseZone::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if ( !HasAuthority() || !OtherActor )
		return;

	if ( !ActorsInBase.Contains(OtherActor) )
		return;

	if ( TriggerBoxVolume->IsOverlappingActor(OtherActor) )
		return;

	ActorsInBase.Remove(OtherActor);

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor);
	if ( !ASC )
		return;

	if ( FActiveGameplayEffectHandle* Handle = ActiveRegenEffects.Find(OtherActor) )
	{
		ASC->RemoveActiveGameplayEffect(*Handle);
		ActiveRegenEffects.Remove(OtherActor);
	}
}

void ABaseZone::CheckInitialOverlaps()
{
	TArray<AActor*> OverlappingActors;
	TriggerBoxVolume->GetOverlappingActors(OverlappingActors, APawn::StaticClass());

	for ( AActor* OverlappingActor : OverlappingActors )
	{
		OnOverlapBegin(TriggerBoxVolume, OverlappingActor, nullptr, 0, false, FHitResult());
	}
}

