// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/Enemy/AI/Tasks/BTTask_RandomBehavior.h"
#include "AIController.h"
#include "Character/Enemy/ERNEnemyCharacter.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AbilitySystemComponent.h"
#include "GAS/ERNGameplayTags.h"
#include "MotionWarpingComponent.h"
#include "Character/Enemy/ERNBossCharacter.h"

UBTTask_RandomBehavior::UBTTask_RandomBehavior()
{
	NodeName = TEXT("Random Behavior");
	bNotifyTick = false;
}

EBTNodeResult::Type UBTTask_RandomBehavior::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		return EBTNodeResult::Failed;
	}

	AERNEnemyCharacter* Enemy = Cast<AERNEnemyCharacter>(AIController->GetPawn());
	if (!Enemy)
	{
		return EBTNodeResult::Failed;
	}

	AActor* Target = Cast<AActor>(OwnerComp.GetBlackboardComponent()->GetValueAsObject(TEXT("TargetActor")));
	if (!Target)
	{
		return EBTNodeResult::Failed;
	}
	
	// 보스 페이즈 전환 중일시 공격 x
	if (AERNBossCharacter* Boss = Cast<AERNBossCharacter>(Enemy))
	{
		if (Boss->bIsTransitioningPhase)
		{
			return EBTNodeResult::Failed;
		}
	}

	// 경직 상태면 공격 중단
	if (UAbilitySystemComponent* ASC = Enemy->GetAbilitySystemComponent())
	{
		if (ASC->HasMatchingGameplayTag(TAG_State_Stagger))
		{
			return EBTNodeResult::Failed;
		}
	}

	// 타겟 방향으로 회전
	AIController->SetFocus(Target);

	// 모션 워핑 타겟 설정
	if (UMotionWarpingComponent* MotionWarping = Enemy->FindComponentByClass<UMotionWarpingComponent>())
	{
		MotionWarping->AddOrUpdateWarpTargetFromLocation(TEXT("AttackTarget"), Target->GetActorLocation());
	}

	// 가중치 기반 랜덤 몽타주 선택 및 재생
	UAnimMontage* SelectedMontage = SelectRandomMontage();
	if (SelectedMontage)
	{
		// 몽타주 길이를 블랙보드에 저장
		float MontageLength = SelectedMontage->GetPlayLength() - 0.15f;
		OwnerComp.GetBlackboardComponent()->SetValueAsFloat(TEXT("MontageDuration"), MontageLength);

		Enemy->Multicast_PlayAttackMontage(SelectedMontage);
		
		// Focus 해제
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
		
		UE_LOG(LogTemp, Log, TEXT("[%s] Random behavior: %s (%.2fs) on %s"),
			*Enemy->GetName(), *SelectedMontage->GetName(), MontageLength, *Target->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] No valid montage in WeightedMontages array"), *Enemy->GetName());
		return EBTNodeResult::Failed;
	}

	return EBTNodeResult::Succeeded;
}

UAnimMontage* UBTTask_RandomBehavior::SelectRandomMontage() const
{
	if (WeightedMontages.Num() == 0)
	{
		return nullptr;
	}

	// 총 가중치 계산
	float TotalWeight = 0.0f;
	for (const FWeightedMontage& WeightedMontage : WeightedMontages)
	{
		if (WeightedMontage.Montage && WeightedMontage.Weight > 0.0f)
		{
			TotalWeight += WeightedMontage.Weight;
		}
	}

	if (TotalWeight <= 0.0f)
	{
		return nullptr;
	}

	// 랜덤 값 생성 (0 ~ TotalWeight)
	float RandomValue = FMath::FRandRange(0.0f, TotalWeight);

	// 가중치 누적 합으로 선택
	float CumulativeWeight = 0.0f;
	for (const FWeightedMontage& WeightedMontage : WeightedMontages)
	{
		if (WeightedMontage.Montage && WeightedMontage.Weight > 0.0f)
		{
			CumulativeWeight += WeightedMontage.Weight;
			if (RandomValue <= CumulativeWeight)
			{
				return WeightedMontage.Montage;
			}
		}
	}

	// 폴백 (첫 번째 유효한 몽타주)
	for (const FWeightedMontage& WeightedMontage : WeightedMontages)
	{
		if (WeightedMontage.Montage)
		{
			return WeightedMontage.Montage;
		}
	}

	return nullptr;
}
