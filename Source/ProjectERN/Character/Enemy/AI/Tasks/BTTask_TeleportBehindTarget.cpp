// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/Enemy/AI/Tasks/BTTask_TeleportBehindTarget.h"
#include "AIController.h"
#include "Character/Enemy/ERNEnemyCharacter.h"
#include "Character/Enemy/ERNBossCharacter.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AbilitySystemComponent.h"
#include "GAS/ERNGameplayTags.h"

UBTTask_TeleportBehindTarget::UBTTask_TeleportBehindTarget()
{
	NodeName = TEXT("Teleport Behind Target");
	bNotifyTick = false;
}

EBTNodeResult::Type UBTTask_TeleportBehindTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
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

	// 서버에서만 이동 (위치는 자동 리플리케이트)
	if (!Enemy->HasAuthority())
	{
		return EBTNodeResult::Failed;
	}

	AActor* Target = Cast<AActor>(OwnerComp.GetBlackboardComponent()->GetValueAsObject(TEXT("TargetActor")));
	if (!Target)
	{
		return EBTNodeResult::Failed;
	}

	// 페이즈 전환 중이면 중단
	AERNBossCharacter* Boss = Cast<AERNBossCharacter>(Enemy);
	if (Boss && Boss->bIsTransitioningPhase)
	{
		return EBTNodeResult::Failed;
	}

	// 경직 상태면 중단
	UAbilitySystemComponent* ASC = Enemy->GetAbilitySystemComponent();
	if (ASC && ASC->HasMatchingGameplayTag(TAG_State_Stagger))
	{
		return EBTNodeResult::Failed;
	}

	// 보스→타겟 수평 방향을 기준 축으로
	FVector Forward = (Enemy->GetActorLocation() - Target->GetActorLocation());
	Forward.Z = 0.f;
	Forward = Forward.GetSafeNormal();

	// 보스가 타겟과 거의 같은 수평 위치면 폴백 (degenerate 방지)
	if (Forward.IsNearlyZero())
	{
		Forward = Enemy->GetActorForwardVector().GetSafeNormal2D();
	}

	const FVector Right = FVector::CrossProduct(FVector::UpVector, Forward);

	FVector DestLocation = Target->GetActorLocation()
		+ Forward * TeleportOffset.X
		+ Right   * TeleportOffset.Y
		+ FVector::UpVector * TeleportOffset.Z;

	// 바닥 스냅 (수직 라인트레이스)
	if (bSnapToGround)
	{
		FHitResult Hit;
		const FVector TraceStart = DestLocation + FVector(0.f, 0.f, GroundTraceHalfHeight);
		const FVector TraceEnd = DestLocation - FVector(0.f, 0.f, GroundTraceHalfHeight);

		FCollisionQueryParams Params;
		Params.AddIgnoredActor(Enemy);
		Params.AddIgnoredActor(Target);

		if (Enemy->GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params))
		{
			DestLocation.Z = Hit.ImpactPoint.Z + Enemy->GetDefaultHalfHeight();
		}
	}

	// 텔레포트 전 시작 위치 캡처 (번개 잔상 시작점)
	const FVector StartLocation = Enemy->GetActorLocation();

	// 텔레포트 (충돌 보정 포함)
	const bool bMoved = Enemy->TeleportTo(DestLocation, Enemy->GetActorRotation());
	if (!bMoved)
	{
		return EBTNodeResult::Failed;
	}

	// 타겟 바라보게 회전
	AIController->SetFocus(Target);

	// 번개 잔상 (시작→도착 빔, 모든 클라 동기화) — 보스만
	if (Boss)
	{
		Boss->Multicast_PlayTeleportTrail(StartLocation, Enemy->GetActorLocation());
	}

	UE_LOG(LogTemp, Log, TEXT("[%s] Teleported behind %s"), *Enemy->GetName(), *Target->GetName());
	return EBTNodeResult::Succeeded;
}
