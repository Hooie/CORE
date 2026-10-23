// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/CKwangCharacter.h"
#include "Components/StaticMeshComponent.h"

ACKwangCharacter::ACKwangCharacter()
{
	SwordMeshComp = CreateDefaultSubobject<UStaticMeshComponent>("Sword Mesh Component");
	SwordMeshComp->SetupAttachment(GetMesh(), SwordAttachSocketName);
	SwordMeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ACKwangCharacter::Multicast_ToggleSwordVisibility_Implementation(bool bVisible)
{
	SwordMeshComp->SetVisibility(bVisible);
}

bool ACKwangCharacter::HasSword() const
{
	return SwordMeshComp->IsVisible();
}

void ACKwangCharacter::SetSpawnedSword(ATA_Sword* Sword)
{
	SpawnedSword = Sword;
}

#if WITH_EDITOR
void ACKwangCharacter::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	FName ChangedProp = PropertyChangedEvent.GetMemberPropertyName();

	if (ChangedProp == GET_MEMBER_NAME_CHECKED(ThisClass, SwordAttachSocketName))
	{
		SwordMeshComp->AttachToComponent(
			GetMesh(),
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			SwordAttachSocketName
		);
	}
}
#endif

void ACKwangCharacter::OnStun()
{
	SetMoveInputBlock(true);
}

void ACKwangCharacter::OnRecoveryFromStun()
{
	if ( IsDead() )
		return;

	SetMoveInputBlock(false);
}

void ACKwangCharacter::OnBind()
{
	SetBlockBasicAttack(true);
}

void ACKwangCharacter::OnRecoveryFromBind()
{
	if ( IsDead() )
		return;

	SetBlockBasicAttack(false);
}

void ACKwangCharacter::OnDead()
{
	SetMoveInputBlock(true);
}

void ACKwangCharacter::OnRespawn()
{
	SetMoveInputBlock(false);
}
