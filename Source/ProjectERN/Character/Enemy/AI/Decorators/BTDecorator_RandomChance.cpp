// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/Enemy/AI/Decorators/BTDecorator_RandomChance.h"

UBTDecorator_RandomChance::UBTDecorator_RandomChance()
{
	NodeName = TEXT("Random Chance");
	Chance = 50.0f;
}

bool UBTDecorator_RandomChance::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const float RandomValue = FMath::FRand() * 100.0f;
	return RandomValue <= Chance;
}
