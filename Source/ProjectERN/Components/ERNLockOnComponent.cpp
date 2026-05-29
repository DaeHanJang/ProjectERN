// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/ERNLockOnComponent.h"

#include "Camera/CameraComponent.h"
#include "Character/ERNCharacterBase.h"
#include "Components/SphereComponent.h"

// Sets default values for this component's properties
UERNLockOnComponent::UERNLockOnComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetIsReplicatedByDefault(true);
}

void UERNLockOnComponent::Initialize(USphereComponent* InDetectionSphere, UCameraComponent* InCameraComponent)
{
	if (!IsValid(InDetectionSphere))
	{
		return;
	}
	
	DetectionSphere = InDetectionSphere;
	DetectionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DetectionSphere->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	DetectionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	DetectionSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECR_Overlap);

	DetectionSphere->SetSphereRadius(SphereRadius);
	
	if (InCameraComponent)
	{
		CameraComponent = InCameraComponent;
	}
}

bool UERNLockOnComponent::ToggleLockOn()
{
	if (IsLockedOn())
	{
		ClearLockOn();
		return false;
	}
	
	return TryLockOn();
}

bool UERNLockOnComponent::TryLockOn()
{
	if (!IsValid(DetectionSphere))
	{
		return false;
	}
	
	LockOnState = ELockOnState::Searching;
	AActor* Target = FindBaseTarget();
	
	if (!IsValid(Target))
	{
		ClearLockOn();
		return false;
	}
	
	CurrentTarget = Target;
	LockOnState = ELockOnState::Locked;
	SetComponentTickEnabled(true);
	return true;
}

void UERNLockOnComponent::ClearLockOn()
{
	LockOnState = ELockOnState::None;
	CurrentTarget = nullptr;
	CurrentLockOnLostGraceTime = 0.0f;
	GetWorld()->GetTimerManager().ClearTimer(LostGraceTimerHandle);
	SetComponentTickEnabled(false);
}

FRotator UERNLockOnComponent::GetDesiredRotationToTarget() const
{
	if (!IsValid(GetOwner()) || !IsValid(CurrentTarget))
	{
		return FRotator::ZeroRotator;
	}
	
	const FVector ToTarget = CurrentTarget->GetActorLocation() - GetOwner()->GetActorLocation();
	return FRotator(0.0f, ToTarget.Rotation().Yaw, 0.0f);
}

void UERNLockOnComponent::TickComponent(float DeltaTime, enum ELevelTick TickType,
                                     FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	if (!IsLockedOn())
	{
		ClearLockOn();
		return;
	}
	
	if (!IsValidTarget(CurrentTarget))
	{
		StartLockOnLostGraceTimer();
	}
}

AActor* UERNLockOnComponent::FindBaseTarget() const
{
	AActor* Target = nullptr;;
	float TargetDistSq = FLT_MAX;
	
	TArray<AActor*> Candidates;
	DetectionSphere->GetOverlappingActors(Candidates);
	if (Candidates.IsEmpty())
	{
		return nullptr;
	}
	
	const FVector ForwardVector = GetForwardVector();
	const float MinDot = FMath::Cos(FMath::DegreesToRadians(Degree));
	
	DrawDebugCone(GetWorld(), GetSourceLocation(), GetForwardVector(), DetectionSphere->GetScaledSphereRadius(), FMath::DegreesToRadians(Degree), FMath::DegreesToRadians(Degree), 32, FColor::Green, false, 1.0f, 0, 1.0f);
	
	for (AActor* Candidate : Candidates)
	{
		if (!IsValid(Candidate) || Candidate == GetOwner())
		{
			continue;
		}
		
		bool bMatchesTargetClass = TargetActorClasses.IsEmpty();
		
		for (const TSubclassOf<AActor>& ActorType : TargetActorClasses)
		{
			if (ActorType && Candidate->IsA(ActorType))
			{
				bMatchesTargetClass = true;
				break;
			}
		}
		
		if (!bMatchesTargetClass)
		{
			continue;
		}
		
		const FVector ToTarget = (Candidate->GetActorLocation() - GetSourceLocation()).GetSafeNormal();
		const float Dot = FVector::DotProduct(ForwardVector, ToTarget);
		
		if (Dot < MinDot)
		{
			continue;
		}
		
		if (!HasLineOfSightToTarget(Candidate))
		{
			continue;
		}
		
		const float DistSq = FVector::DistSquared(GetOwner()->GetActorLocation(), Candidate->GetActorLocation());
		if (DistSq < TargetDistSq)
		{
			TargetDistSq = DistSq;
			Target = Candidate;
		}
	}
	
	return Target;
}

FVector UERNLockOnComponent::GetSourceLocation() const
{
	const AActor* Owner = GetOwner();
	if (!IsValid(Owner))
	{
		return FVector::ForwardVector;
	}
	
	switch (LockOnForwardSource)
	{
	case ELockOnForwardSource::OwnerActor:
	case ELockOnForwardSource::Controller:
		return Owner->GetActorLocation();
	case ELockOnForwardSource::CameraComponent:
		{
			if (CameraComponent)
			{
				return CameraComponent->GetComponentLocation();
			}
			
			return Owner->GetActorLocation();
		}
	}
	
	return Owner->GetActorLocation();
}

FVector UERNLockOnComponent::GetForwardVector() const
{
	AActor* Owner = GetOwner();
	if (!IsValid(Owner))
	{
		return FVector::ForwardVector;
	}
	
	switch (LockOnForwardSource)
	{
		case ELockOnForwardSource::OwnerActor:
		{
			return Owner->GetActorForwardVector();
		}
		
		case ELockOnForwardSource::Controller:
		{
			const APawn* Pawn = Cast<APawn>(Owner);
			if (!IsValid(Pawn))
			{
				return Owner->GetActorForwardVector();
			}
				
			return Pawn->GetControlRotation().Vector();
		}
		
		case ELockOnForwardSource::CameraComponent:
		{
			if (CameraComponent)
			{
				return CameraComponent->GetForwardVector();
			}
				
			const APawn* Pawn = Cast<APawn>(Owner);
			if (!IsValid(Pawn))
			{
				return Owner->GetActorForwardVector();
			}
				
			const APlayerController* PC = Cast<APlayerController>(Pawn->GetController());
			if (!IsValid(PC) || !IsValid(PC->PlayerCameraManager))
			{
				return Owner->GetActorForwardVector();
			}
				
			return PC->PlayerCameraManager->GetCameraRotation().Vector();
		}
	}
	
	return Owner->GetActorForwardVector();
}

bool UERNLockOnComponent::HasLineOfSightToTarget(AActor* Target) const
{
	if (!IsValid(Target) || !IsValid(GetOwner()))
	{
		return false;
	}
	
	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(GetOwner());
		
	DrawDebugLine(GetWorld(), GetOwner()->GetActorLocation(), Target->GetActorLocation(), FColor::Cyan, false, 1.0f, 0, 1.0f);
		
	const bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, GetOwner()->GetActorLocation(), Target->GetActorLocation(), ECollisionChannel::ECC_Visibility, Params);
	
	return !bHit || HitResult.GetActor() == Target;
}

bool UERNLockOnComponent::IsValidTarget(AActor* Target) const
{
	if (!IsValid(Target))
	{
		return false;
	}
	if (const AERNCharacterBase* Character = Cast<AERNCharacterBase>(Target))
	{
		if (Character->IsDead())
		{
			return false;
		}
	}
	
	const float DistSq = FVector::DistSquared(GetOwner()->GetActorLocation(), Target->GetActorLocation());
	if (DistSq > FMath::Square(SphereRadius))
	{
		return false;
	}
	
	return HasLineOfSightToTarget(Target);
}

void UERNLockOnComponent::StartLockOnLostGraceTimer()
{
	if (LockOnState == ELockOnState::Lost)
	{
		return;
	}
	
	SetComponentTickEnabled(false);
	LockOnState = ELockOnState::Lost;
	CurrentLockOnLostGraceTime = 0.0f;
	GetWorld()->GetTimerManager().SetTimer(LostGraceTimerHandle, this, &UERNLockOnComponent::LockOnLostGraceTimerHandler, 0.1f, true, 0.0f);
}

void UERNLockOnComponent::LockOnLostGraceTimerHandler()
{
	if (IsValidTarget(CurrentTarget))
	{
		GetWorld()->GetTimerManager().ClearTimer(LostGraceTimerHandle);
		LockOnState = ELockOnState::Locked;
		CurrentLockOnLostGraceTime = 0.0f;
		SetComponentTickEnabled(true);
		return;
	}
	
	if (CurrentLockOnLostGraceTime >= LockOnLostGraceTime)
	{
		ClearLockOn();
		return;
	}
	
	CurrentLockOnLostGraceTime += 0.1f;
}
