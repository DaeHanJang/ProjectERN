// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/Enemy/AI/ERNMobAIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISense_Sight.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "Character/Enemy/ERNEnemyCharacter.h"
#include "Character/Enemy/ERNMobCharacter.h"

AERNMobAIController::AERNMobAIController()
{
	// AI Perception Component 생성
	UAIPerceptionComponent* PerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComp"));
	SetPerceptionComponent(*PerceptionComp);

	// Sight Config 설정
	UAISenseConfig_Sight* SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = 1500.0f;
	SightConfig->LoseSightRadius = 2000.0f;
	SightConfig->PeripheralVisionAngleDegrees = 90.0f;
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = false;

	PerceptionComp->ConfigureSense(*SightConfig);
	PerceptionComp->SetDominantSense(SightConfig->GetSenseImplementation());
}

void AERNMobAIController::BeginPlay()
{
	Super::BeginPlay();

	// UPROPERTY 값으로 SightConfig 업데이트
	if (UAIPerceptionComponent* PerceptionComp = GetPerceptionComponent())
	{
		FAISenseID SightSenseID = UAISense::GetSenseID<UAISense_Sight>();
		if (UAISenseConfig_Sight* SightConfig = Cast<UAISenseConfig_Sight>(
			PerceptionComp->GetSenseConfig(SightSenseID)))
		{
			SightConfig->SightRadius = SightRadius;
			SightConfig->LoseSightRadius = LoseSightRadius;
			SightConfig->PeripheralVisionAngleDegrees = DefaultVisionAngle;
			PerceptionComp->RequestStimuliListenerUpdate();
		}
	}

	SetupPerception();

	// Leash 체크 타이머 시작 (loop)
	GetWorldTimerManager().SetTimer(
		LeashCheckTimerHandle,
		this,
		&AERNMobAIController::CheckLeashDistance,
		LeashCheckInterval,
		true);
}

void AERNMobAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (BehaviorTreeAsset)
	{
		RunBehaviorTree(BehaviorTreeAsset);
	}
}

void AERNMobAIController::SetupPerception()
{
	UAIPerceptionComponent* PerceptionComp = GetPerceptionComponent();
	if (!PerceptionComp)
	{
		return;
	}

	// 타겟 감지 델리게이트 바인딩
	PerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &AERNMobAIController::OnTargetDetected);
}

void AERNMobAIController::OnTargetDetected(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Blackboard)
	{
		return;
	}

	AProjectERNCharacter* Player = Cast<AProjectERNCharacter>(Actor);
	if (Player)
	{
		AERNEnemyCharacter* SelfEnemy = Cast<AERNEnemyCharacter>(GetPawn());

		if (Stimulus.WasSuccessfullySensed())
		{
			// 플레이어가 죽어있는 상태면 감지 대상에서 제외
			if (!Player->IsAlive())
			{
				if (Cast<AActor>(Blackboard->GetValueAsObject(TEXT("TargetActor"))) == Player)
				{
					SetTarget(nullptr);
				}

				if (SelfEnemy)
				{
					Player->NotifyLostBy(SelfEnemy);
				}

				return;
			}

			SetTarget(Player);

			// 비전투 해제 알림
			if (SelfEnemy)
			{
				Player->NotifyDetectedBy(SelfEnemy);
			}
		}
		else
		{
			AActor* CurrentTarget = Cast<AActor>(Blackboard->GetValueAsObject(TEXT("TargetActor")));
			if (CurrentTarget == Player)
			{
				Blackboard->ClearValue(TEXT("TargetActor"));
				SetCombatVision(false);
				UE_LOG(LogTemp, Log, TEXT("[%s] Player lost: %s"), *GetName(), *Player->GetName());
			}

			// 비전투 진입 그레이스 타이머 트리거
			if (SelfEnemy)
			{
				Player->NotifyLostBy(SelfEnemy);
			}
		}
	}
}

void AERNMobAIController::SetTarget(AActor* NewTarget)
{
	if (!Blackboard)
	{
		return;
	}

	// 살아있는 대상만 Target으로 지정
	if (const AProjectERNCharacter* Player = Cast<AProjectERNCharacter>(NewTarget))
	{
		if (!Player->IsAlive())
		{
			NewTarget = nullptr;
		}
	}
	
	if (NewTarget)
	{
		Blackboard->SetValueAsObject(TEXT("TargetActor"), NewTarget);
		SetCombatVision(true);
		UE_LOG(LogTemp, Log, TEXT("[%s] Target set: %s"), *GetName(), *NewTarget->GetName());
	}
	else
	{
		Blackboard->ClearValue(TEXT("TargetActor"));
		SetCombatVision(false);
	}
}

TArray<AActor*> AERNMobAIController::GetPerceivedPlayers()
{
	TArray<AActor*> Result;

	UAIPerceptionComponent* PerceptionComp = GetPerceptionComponent();
	if (!PerceptionComp) return Result;

	TArray<AActor*> Perceived;
	PerceptionComp->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), Perceived);

	for (AActor* Actor : Perceived)
	{
		AProjectERNCharacter* Player = Cast<AProjectERNCharacter>(Actor);
		// 살아있는 대상만 적용
		if (Player && Player->IsAlive())
		{
			Result.Add(Actor);
		}
	}

	return Result;
}

void AERNMobAIController::SetCombatVision(bool bInCombat)
{
	if (bIsInCombatMode == bInCombat)
	{
		return;
	}

	bIsInCombatMode = bInCombat;

	UAIPerceptionComponent* PerceptionComp = GetPerceptionComponent();
	if (!PerceptionComp)
	{
		return;
	}

	FAISenseID SightSenseID = UAISense::GetSenseID<UAISense_Sight>();
	UAISenseConfig_Sight* SightConfig = Cast<UAISenseConfig_Sight>(
		PerceptionComp->GetSenseConfig(SightSenseID));

	if (SightConfig)
	{
		SightConfig->PeripheralVisionAngleDegrees = bInCombat ? CombatVisionAngle : DefaultVisionAngle;
		PerceptionComp->RequestStimuliListenerUpdate();

		UE_LOG(LogTemp, Log, TEXT("[%s] Vision changed to %.1f degrees"),
			*GetName(), SightConfig->PeripheralVisionAngleDegrees);
	}
}

void AERNMobAIController::CheckLeashDistance()
{
	if (!Blackboard)
	{
		return;
	}

	AERNMobCharacter* Mob = Cast<AERNMobCharacter>(GetPawn());
	if (!Mob || Mob->IsDead())
	{
		return;
	}

	// 타겟이 없으면 체크 불필요
	AActor* CurrentTarget = Cast<AActor>(Blackboard->GetValueAsObject(TEXT("TargetActor")));
	if (!CurrentTarget)
	{
		return;
	}
	
	// 플레이어가 죽으면 타겟팅 취소
	if (AProjectERNCharacter* Player = Cast<AProjectERNCharacter>(CurrentTarget))
	{
		if (!Player->IsAlive())
		{
			if (AERNEnemyCharacter* SelfEnemy = Cast<AERNEnemyCharacter>(GetPawn()))
			{
				Player->NotifyLostBy(SelfEnemy);
			}

			SetTarget(nullptr);
			return;
		}
	}

	// 스폰 지점에서 자기 위치 거리 측정
	const float DistFromSpawn = FVector::Dist(
		Mob->GetActorLocation(),
		Mob->SpawnLocation);

	// LeashDistance 초과 시 타겟 해제 + 귀환 모드
	if (DistFromSpawn > Mob->LeashDistance)
	{
		// 블랙보드 타겟 클리어 → BT가 추격 멈춤
		Blackboard->ClearValue(TEXT("TargetActor"));

		// 시야각 평상시(90도)로 복귀
		SetCombatVision(false);

		// 플레이어에게 감지 상실 통지 → DetectingEnemies에서 제거 → 그레이스 타이머 시작
		if (AProjectERNCharacter* Player = Cast<AProjectERNCharacter>(CurrentTarget))
		{
			Player->NotifyLostBy(Mob);
		}

		// 귀환 모드 ON (BT의 귀환 로직 트리거)
		Mob->bIsReturning = true;

		UE_LOG(LogTemp, Log,
			TEXT("[%s] Leash exceeded (%.0f > %.0f) - releasing target & returning"),
			*GetName(), DistFromSpawn, Mob->LeashDistance);
	}
}
