// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ERNMinimapSubsystem.generated.h"

class AERNMinimapTargetPoint;
/**
 * 
 */
UCLASS()
class PROJECTERN_API UERNMinimapSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:
	DECLARE_MULTICAST_DELEGATE(FOnTargetsChanged);
	FOnTargetsChanged OnTargetsChanged;
	
	void RegisterTarget(AERNMinimapTargetPoint* Target);
	void UnregisterTarget(AERNMinimapTargetPoint* Target);
	void GetTargets(TArray<AERNMinimapTargetPoint*>& OutTargets) const;
	
private:
	int32 RemoveInvalidTargets();
	int32 RemoveTarget(const AERNMinimapTargetPoint* Target);
	bool ContainsTarget(const AERNMinimapTargetPoint* Target) const;
	
private:
	UPROPERTY()
	TArray<TWeakObjectPtr<AERNMinimapTargetPoint>> Targets;
};
