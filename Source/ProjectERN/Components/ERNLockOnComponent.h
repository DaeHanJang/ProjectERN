// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ERNLockOnComponent.generated.h"

class UCameraComponent;
class USphereComponent;

UENUM(BlueprintType)
enum class ELockOnState : uint8
{
	None      UMETA(DisplayName="None"), 
	Searching UMETA(DisplayName="Searching"), 
	Lost      UMETA(DisplayName="Lost"), 
	Locked    UMETA(DisplayName="Locked")
};

UENUM(BlueprintType)
enum class ELockOnForwardSource : uint8
{
	OwnerActor      UMETA(DisplayName="OwnerActor"), 
	Controller      UMETA(DisplayName="Controller"), 
	CameraComponent UMETA(DisplayName="CameraComponent")
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECTERN_API UERNLockOnComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UERNLockOnComponent();
	
	FORCEINLINE AActor* GetCurrentTarget() const { return CurrentTarget; }
	bool IsLockedOn() const;
	bool IsLockOnActive() const;
	FRotator GetDesiredRotationToTarget() const;
	
	void Initialize(USphereComponent* InDetectionSphere, UCameraComponent* InCameraComponent = nullptr);
	
	bool ToggleLockOn();
	bool TryLockOn();
	void ClearLockOn();
	
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	AActor* FindBaseTarget() const;
	
	FVector GetSourceLocation() const;
	FVector GetForwardVector() const;
	bool HasLineOfSightToTarget(AActor* Target) const;
	
	bool IsValidTarget(AActor* Target) const;
	
	void StartLockOnLostGraceTimer();
	void LockOnLostGraceTimerHandler();
	
private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LookOn|Sphere", meta=(AllowPrivateAccess="true"))
	float SphereRadius = 2000.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LookOn|Sphere", meta=(AllowPrivateAccess="true"))
	TArray<TSubclassOf<AActor>> TargetActorClasses;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LookOn|DotProduct", meta=(AllowPrivateAccess="true"))
	ELockOnForwardSource LockOnForwardSource = ELockOnForwardSource::CameraComponent;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LookOn|DotProduct", meta=(AllowPrivateAccess="true"))
	float Degree = 45.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LookOn|DotProduct", meta=(AllowPrivateAccess="true"))
	float LockOnLostGraceTime = 2.0f;
	
	UPROPERTY()
	ELockOnState LockOnState = ELockOnState::None;
	
	UPROPERTY()
	TObjectPtr<USphereComponent> DetectionSphere;
	
	UPROPERTY()
	TObjectPtr<UCameraComponent> CameraComponent;
	
	UPROPERTY(Transient)
	TObjectPtr<AActor> CurrentTarget;
	
	FTimerHandle LostGraceTimerHandle;
	float CurrentLockOnLostGraceTime = 0.0f;
	
};
