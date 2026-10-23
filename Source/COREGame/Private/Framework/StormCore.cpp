// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/StormCore.h"
#include "AIController.h"
#include "Camera/CameraComponent.h"
#include "Components/SphereComponent.h"
#include "Components/DecalComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GenericTeamAgentInterface.h"
#include "Net/UnrealNetwork.h"

// Sets default values
AStormCore::AStormCore()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	InfluenceRange = CreateDefaultSubobject<USphereComponent>("Influence Range");
	InfluenceRange->SetupAttachment(GetRootComponent());

	InfluenceRange->OnComponentBeginOverlap.AddDynamic(this, &AStormCore::NewInfluencerInRange);
	InfluenceRange->OnComponentEndOverlap.AddDynamic(this, &AStormCore::InfluencerLeftRange);

	ViewCam = CreateDefaultSubobject<UCameraComponent>("View Cam");
	ViewCam->SetupAttachment(GetRootComponent());

	GroundDecalComponent = CreateDefaultSubobject<UDecalComponent>("Ground Decal Component");
	GroundDecalComponent->SetupAttachment(GetRootComponent());
}

void AStormCore::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(AStormCore, CoreToCapture, COND_None, REPNOTIFY_Always);
}

float AStormCore::GetProgress() const
{
	FVector TeamTwoGoalLoc = TeamTwoGoal->GetActorLocation();
	FVector VectorFromTeamOne = GetActorLocation() - TeamTwoGoalLoc;
	VectorFromTeamOne.Z = 0.f;

	return VectorFromTeamOne.Length() / TravelLength;
}

// Called when the game starts or when spawned
void AStormCore::BeginPlay()
{
	Super::BeginPlay();
	
	FVector TeamOneGoalLoc = TeamOneGoal->GetActorLocation();
	FVector TeamTwoGoalLoc = TeamTwoGoal->GetActorLocation();

	FVector GoalOffset = TeamOneGoalLoc - TeamTwoGoalLoc;

	GoalOffset.Z = 0.f;

	TravelLength = GoalOffset.Length();
}

void AStormCore::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	OwnerAIC = Cast<AAIController>(NewController);
}

// Called every frame
void AStormCore::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (CoreToCapture)
	{
		FVector CoreMoveDir = (GetMesh()->GetComponentLocation() - CoreToCapture->GetActorLocation()).GetSafeNormal();
		CoreToCapture->AddActorWorldOffset(CoreMoveDir * CoreCaptureSpeed * DeltaTime);
	}
}

// Called to bind functionality to input
void AStormCore::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

#if WITH_EDITOR
void AStormCore::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	FName PropertyName = PropertyChangedEvent.GetPropertyName();
	if (PropertyName == GET_MEMBER_NAME_CHECKED(AStormCore, InfluenceRadius))
	{
		InfluenceRange->SetSphereRadius(InfluenceRadius);
		FVector DecalSize = GroundDecalComponent->DecalSize;
		GroundDecalComponent->DecalSize = FVector{ DecalSize.X, InfluenceRadius, InfluenceRadius };
	}
}
#endif

void AStormCore::NewInfluencerInRange(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor == TeamOneGoal)
	{
		GoalReached(0);
	}

	if (OtherActor == TeamTwoGoal)
	{
		GoalReached(1);
	}

	IGenericTeamAgentInterface* OtherTeamInterface = Cast<IGenericTeamAgentInterface>(OtherActor);

	if (OtherTeamInterface)
	{
		if (OtherTeamInterface->GetGenericTeamId().GetId() == 0)
		{
			++TeamOneInfluencerCount;
		}
		else if(OtherTeamInterface->GetGenericTeamId().GetId() == 1)
		{
			++TeamTwoInfluencerCount;
		}

		UpdateTeamWeight();
	}
}

void AStormCore::InfluencerLeftRange(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	IGenericTeamAgentInterface* OtherTeamInterface = Cast<IGenericTeamAgentInterface>(OtherActor);

	if (OtherTeamInterface)
	{
		if (OtherTeamInterface->GetGenericTeamId().GetId() == 0)
		{
			--TeamOneInfluencerCount;

			if (TeamOneInfluencerCount < 0)
				TeamOneInfluencerCount = 0;
		}
		else if (OtherTeamInterface->GetGenericTeamId().GetId() == 1)
		{
			--TeamTwoInfluencerCount;

			if (TeamTwoInfluencerCount < 0)
				TeamTwoInfluencerCount = 0;
		}

		UpdateTeamWeight();
	}
}

void AStormCore::UpdateTeamWeight()
{
	OnTeamInfluencerCountUpdatedDelegate.Broadcast(TeamOneInfluencerCount, TeamTwoInfluencerCount);

	if (TeamOneInfluencerCount == TeamTwoInfluencerCount)
	{
		TeamWeight = 0.f;
	}
	else
	{
		float TeamOffset = TeamOneInfluencerCount - TeamTwoInfluencerCount;
		float TeamTotal = TeamOneInfluencerCount + TeamTwoInfluencerCount;

		TeamWeight = TeamOffset / TeamTotal;
	}

	UE_LOG(LogTemp, Warning, TEXT("Team One Count : %d, Team Two Count : %d, Weight : %f"), TeamOneInfluencerCount, TeamTwoInfluencerCount, TeamWeight);

	UpdateGoal();
}

void AStormCore::UpdateGoal()
{
	if (!HasAuthority())
		return;

	if (!OwnerAIC)
		return;

	if (!GetCharacterMovement())
		return;

	if (TeamWeight > 0)
	{
		OwnerAIC->MoveToActor(TeamOneGoal);
	}
	else
	{
		OwnerAIC->MoveToActor(TeamTwoGoal);
	}

	float Speed = MaxMoveSpeed * FMath::Abs(TeamWeight);

	GetCharacterMovement()->MaxWalkSpeed = Speed;
}

void AStormCore::OnRep_CoreToCapture()
{
	if (CoreToCapture)
	{
		CaptureCore();
	}
}

void AStormCore::GoalReached(int WinningTeam)
{
	OnGoalReachedDelegate.Broadcast(this, WinningTeam);

	if (!HasAuthority())
		return;

	MaxMoveSpeed = 0.f;
	CoreToCapture = WinningTeam == 0 ? TeamTwoCore : TeamOneCore;

	CaptureCore();
}

void AStormCore::CaptureCore()
{
	float ExpandDuration = GetMesh()->GetAnimInstance()->Montage_Play(ExpandMontage);

	CoreCaptureSpeed = FVector::Distance(GetMesh()->GetComponentLocation(), CoreToCapture->GetActorLocation()) / ExpandDuration;

	CoreToCapture->SetActorEnableCollision(false);
	GetCharacterMovement()->MaxWalkSpeed = 0.f;

	FTimerHandle ExpandTimerHandle;
	GetWorldTimerManager().SetTimer(ExpandTimerHandle, this, &AStormCore::ExpandFinished, ExpandDuration);
}

void AStormCore::ExpandFinished()
{
	CoreToCapture->SetActorLocation(GetMesh()->GetComponentLocation());
	CoreToCapture->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepWorldTransform, "root");

	GetMesh()->GetAnimInstance()->Montage_Play(CaptureMontage);
}

