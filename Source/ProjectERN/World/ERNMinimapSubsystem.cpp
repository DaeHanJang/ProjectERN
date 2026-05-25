// Fill out your copyright notice in the Description page of Project Settings.

#include "World/ERNMinimapSubsystem.h"
#include "World/ERNMinimapTargetPoint.h"

void UERNMinimapSubsystem::RegisterTarget(AERNMinimapTargetPoint* Target)
{
	if (IsValid(Target) == false)
	{
		return;
	}
	
	const int32 RemovedCount = RemoveInvalidTargets();
	
	// 중복 등록 방지
	if (ContainsTarget(Target))
	{
		if (RemovedCount > 0)
		{
			OnTargetsChanged.Broadcast();
		}
		
		return;
	}
	
	Targets.Add(Target);
	OnTargetsChanged.Broadcast();
}

void UERNMinimapSubsystem::UnregisterTarget(AERNMinimapTargetPoint* Target)
{
	int32 RemovedCount = RemoveInvalidTargets();
	RemovedCount += RemoveTarget(Target);
	
	if (RemovedCount > 0)
	{
		OnTargetsChanged.Broadcast();
	}
}

void UERNMinimapSubsystem::GetTargets(TArray<AERNMinimapTargetPoint*>& OutTargets) const
{
	OutTargets.Reset();
	
	for (const TWeakObjectPtr<AERNMinimapTargetPoint>& WeakTarget : Targets)
	{
		if (AERNMinimapTargetPoint* Target = WeakTarget.Get())
		{
			OutTargets.Add(Target);
		}
	}
}

void UERNMinimapSubsystem::NotifyTargetsChanged() const
{
	OnTargetsChanged.Broadcast();
}

int32 UERNMinimapSubsystem::RemoveInvalidTargets()
{
	int32 RemovedCount = 0;
	
	for (int32 Index = Targets.Num() - 1; Index >= 0; --Index)
	{
		if (Targets[Index].IsValid() == false)
		{
			Targets.RemoveAt(Index);
			RemovedCount++;
		}
	}
	
	return RemovedCount;
}

int32 UERNMinimapSubsystem::RemoveTarget(const AERNMinimapTargetPoint* Target)
{
	if (Target == nullptr)
	{
		return 0;
	}
	
	int32 RemovedCount = 0;
	
	for (int32 Index = Targets.Num() - 1; Index >= 0; --Index)
	{
		if (Targets[Index].Get() == Target)
		{
			Targets.RemoveAt(Index);
			++RemovedCount;
		}
	}
	
	return RemovedCount;
}

bool UERNMinimapSubsystem::ContainsTarget(const AERNMinimapTargetPoint* Target) const
{
	if (Target == nullptr)
	{
		return false;
	}
	
	for (const TWeakObjectPtr<AERNMinimapTargetPoint>& WeakTarget : Targets)
	{
		if (WeakTarget.Get() == Target)
		{
			return true;
		}
	}
	
	return false;
}
