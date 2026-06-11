// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/Enemy/AI/Tasks/BTTask_LevitateBehavior.h"
#include "AIController.h"
#include "Character/Enemy/ERNEnemyCharacter.h"
#include "Character/Enemy/ERNBossCharacter.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AbilitySystemComponent.h"
#include "GAS/ERNGameplayTags.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"

UBTTask_LevitateBehavior::UBTTask_LevitateBehavior()
{
	NodeName = TEXT("Levitate Behavior");
	bNotifyTick = true;
	// 페이즈/시작 위치 등 진행 상태를 멤버에 저장하므로 BT 컴포넌트마다 노드 인스턴스 분리
	bCreateNodeInstance = true;
}

EBTNodeResult::Type UBTTask_LevitateBehavior::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
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

	UCharacterMovementComponent* Movement = Enemy->GetCharacterMovement();
	if (!Movement)
	{
		return EBTNodeResult::Failed;
	}

	// 타겟 방향으로 회전
	AIController->SetFocus(Target);

	// 진행 상태 초기화
	CachedEnemy = Enemy;
	StartLocation = Enemy->GetActorLocation();
	Phase = ELevitatePhase::Rise;
	bMontageFinished = (Montage == nullptr);
	HoverRemaining = ExtraHoverTime;

	// 중력 OFF — 수동 Z 보간 (서버 권위, 위치는 리플리케이션으로 클라 동기화)
	Movement->StopMovementImmediately();
	Movement->SetMovementMode(MOVE_Flying);

	// 상승 시작과 동시에 몽타주 재생 + 서버 AnimInstance에 종료 델리게이트 바인딩
	if (Montage)
	{
		Enemy->Multicast_PlayAttackMontage(Montage);

		if (UAnimInstance* AnimInstance = Enemy->GetMesh() ? Enemy->GetMesh()->GetAnimInstance() : nullptr)
		{
			FOnMontageEnded EndDelegate;
			EndDelegate.BindUObject(this, &UBTTask_LevitateBehavior::OnMontageEnded);
			AnimInstance->Montage_SetEndDelegate(EndDelegate, Montage);
		}
		else
		{
			// 종료 감지 불가 폴백 — 정점 도달 후 ExtraHoverTime만 체류
			bMontageFinished = true;
		}
	}

	// Focus 해제
	AIController->ClearFocus(EAIFocusPriority::Gameplay);

	return EBTNodeResult::InProgress;
}

void UBTTask_LevitateBehavior::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AERNEnemyCharacter* Enemy = CachedEnemy.Get();
	if (!Enemy)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	switch (Phase)
	{
	case ELevitatePhase::Rise:
	{
		const float TargetZ = StartLocation.Z + LevitateHeight;
		const float Step = RiseSpeed * DeltaSeconds;
		const float RemainingZ = TargetZ - Enemy->GetActorLocation().Z;

		FHitResult Hit;
		Enemy->AddActorWorldOffset(FVector(0.f, 0.f, FMath::Min(Step, RemainingZ)), true, &Hit);

		// 정점 도달 또는 천장에 막히면 공중 정지
		if (Hit.bBlockingHit || Enemy->GetActorLocation().Z >= TargetZ - KINDA_SMALL_NUMBER)
		{
			Phase = ELevitatePhase::Hover;
		}
		break;
	}

	case ELevitatePhase::Hover:
	{
		// 몽타주 종료 후 ExtraHoverTime만큼 추가 체류 후 하강
		if (bMontageFinished)
		{
			HoverRemaining -= DeltaSeconds;
			if (HoverRemaining <= 0.f)
			{
				Phase = ELevitatePhase::Descend;
			}
		}
		break;
	}

	case ELevitatePhase::Descend:
	{
		const float Step = DescendSpeed * DeltaSeconds;

		FHitResult Hit;
		Enemy->AddActorWorldOffset(FVector(0.f, 0.f, -Step), true, &Hit);

		// 바닥에 닿거나(스윕 차단) 시작 높이까지 내려오면 착지
		if (Hit.bBlockingHit || Enemy->GetActorLocation().Z <= StartLocation.Z + KINDA_SMALL_NUMBER)
		{
			RestoreMovement();
			Phase = ELevitatePhase::None;
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		}
		break;
	}

	default:
		break;
	}
}

EBTNodeResult::Type UBTTask_LevitateBehavior::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// 몽타주 정지 + 이동 모드 복원 (공중에 떠있는 채로 남지 않게)
	if (AERNEnemyCharacter* Enemy = CachedEnemy.Get())
	{
		if (Montage)
		{
			if (UAnimInstance* AnimInstance = Enemy->GetMesh() ? Enemy->GetMesh()->GetAnimInstance() : nullptr)
			{
				AnimInstance->Montage_Stop(0.2f, Montage);
			}
		}
	}

	RestoreMovement();
	Phase = ELevitatePhase::None;

	return EBTNodeResult::Aborted;
}

void UBTTask_LevitateBehavior::OnMontageEnded(UAnimMontage* EndedMontage, bool bInterrupted)
{
	bMontageFinished = true;
}

void UBTTask_LevitateBehavior::RestoreMovement()
{
	AERNEnemyCharacter* Enemy = CachedEnemy.Get();
	UCharacterMovementComponent* Movement = Enemy ? Enemy->GetCharacterMovement() : nullptr;
	if (Movement)
	{
		Movement->StopMovementImmediately();
		// Walking으로 직접 복원하지 않고 Falling으로 두면 공중에 남은 잔여 높이는 중력이 정리
		Movement->SetMovementMode(MOVE_Falling);
	}
}
