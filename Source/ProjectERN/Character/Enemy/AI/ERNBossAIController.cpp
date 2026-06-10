// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/Enemy/AI/ERNBossAIController.h"
#include "Character/Enemy/ERNBossCharacter.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

AERNBossAIController::AERNBossAIController()
{
	// AI Perception Component 생성
	UAIPerceptionComponent* PerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComp"));
	SetPerceptionComponent(*PerceptionComp);

	// Sight Config 설정
	UAISenseConfig_Sight* SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = 6000.0f;
	SightConfig->LoseSightRadius = 8000.0f;
	SightConfig->PeripheralVisionAngleDegrees = 180.0f;  // 보스는 전방위 감지
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = false;

	PerceptionComp->ConfigureSense(*SightConfig);
	PerceptionComp->SetDominantSense(SightConfig->GetSenseImplementation());
}

void AERNBossAIController::BeginPlay()
{
	Super::BeginPlay();

	SetupPerception();

	// 어그로 감소 타이머 시작 (1초마다)
	GetWorldTimerManager().SetTimer(
		AggroDecayTimerHandle,
		this,
		&AERNBossAIController::DecayAggro,
		1.0f,
		true
	);

	// 시야 어그로 누적 타이머 시작 (1초마다)
	GetWorldTimerManager().SetTimer(
		SightAggroTimerHandle,
		this,
		&AERNBossAIController::TickSightAggro,
		1.0f,
		true
	);
}

void AERNBossAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (DefaultBehaviorTree)
	{
		RunBehaviorTree(DefaultBehaviorTree);
		CurrentBehaviorTree = DefaultBehaviorTree;
	}
}

void AERNBossAIController::SetupPerception()
{
	UAIPerceptionComponent* PerceptionComp = GetPerceptionComponent();
	if (!PerceptionComp)
	{
		return;
	}

	PerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &AERNBossAIController::OnTargetDetected);
}

void AERNBossAIController::OnTargetDetected(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Blackboard)
	{
		return;
	}

	AProjectERNCharacter* Player = Cast<AProjectERNCharacter>(Actor);
	if (Player)
	{
		AERNBossCharacter* Boss = Cast<AERNBossCharacter>(GetPawn());

		if (Stimulus.WasSuccessfullySensed())
		{
			// 플레이어 생사 확인 후 감지 방지
			if (!Player->IsAlive())
			{
				RemoveAggroTarget(Player);

				if (Boss)
				{
					Player->NotifyLostBy(Boss);
				}

				return;
			}
			
			// 시야 진입: 어그로 추가 + 추적 리스트 등록
			AddAggro(Player, 10.0f);
			PerceivedPlayers.AddUnique(Player);

			// 타겟이 없으면 설정
			AActor* CurrentTarget = Cast<AActor>(Blackboard->GetValueAsObject(TEXT("TargetActor")));
			if (!CurrentTarget)
			{
				SetTarget(Player);
			}

			// 보스 체력바 표시 + BGM 재생 (첫 감지 시)
			if (Boss)
			{
				Boss->ShowHealthBarToAllPlayers();
				Boss->Multicast_PlayBossBGM();

				// 비전투 해제 알림
				Player->NotifyDetectedBy(Boss);
			}
		}
		else
		{
			// 시야 이탈: 추적 리스트에서 제거
			PerceivedPlayers.Remove(Player);

			// 비전투 진입 그레이스 타이머 트리거
			if (Boss)
			{
				Player->NotifyLostBy(Boss);
			}
		}
	}
}

void AERNBossAIController::SwitchBehaviorTree(UBehaviorTree* NewBehaviorTree)
{
	if (!NewBehaviorTree || NewBehaviorTree == CurrentBehaviorTree)
	{
		return;
	}

	// 기존 BT 중지
	UBehaviorTreeComponent* BTComp = Cast<UBehaviorTreeComponent>(BrainComponent);
	if (BTComp)
	{
		BTComp->StopTree();
	}

	// 새 BT 시작
	RunBehaviorTree(NewBehaviorTree);
	CurrentBehaviorTree = NewBehaviorTree;
}

void AERNBossAIController::SetTarget(AActor* NewTarget)
{
	if (Blackboard)
	{
		AActor* OldTarget = Cast<AActor>(Blackboard->GetValueAsObject(TEXT("TargetActor")));

		// 타겟 설정 방지 방어코드
		if (NewTarget && !IsValidAggroTarget(NewTarget))
		{
			NewTarget = nullptr;
		}
		
		// 타겟이 실제로 변경될 때만 시간 기록
		if (NewTarget != OldTarget)
		{
			CurrentTargetStartTime = GetWorld()->GetTimeSeconds();
			CachedCurrentTarget = NewTarget;
		}

		Blackboard->SetValueAsObject(TEXT("TargetActor"), NewTarget);
	}
}

void AERNBossAIController::AddAggro(AActor* Target, float AggroAmount)
{
	if (!Target)
	{
		return;
	}
	
	// 유효하지 않은 대상이면 어그로 타겟에서 제거
	if (!IsValidAggroTarget(Target))
	{
		RemoveAggroTarget(Target);
		return;
	}

	if (float* ExistingAggro = AggroTable.Find(Target))
	{
		*ExistingAggro += AggroAmount;
	}
	else
	{
		AggroTable.Add(Target, AggroAmount);
	}

	// 어그로가 가장 높은 타겟으로 전환 (최소 락 시간 체크)
	AActor* HighestAggroTarget = GetHighestAggroTarget();
	AActor* CurrentTarget = Blackboard ? Cast<AActor>(Blackboard->GetValueAsObject(TEXT("TargetActor"))) : nullptr;

	if (HighestAggroTarget)
	{
		// 타겟이 없거나, 같은 타겟이거나, 락 시간이 지났으면 변경 허용
		if (!CurrentTarget || HighestAggroTarget == CurrentTarget || CanChangeTarget())
		{
			SetTarget(HighestAggroTarget);
		}
	}
}

AActor* AERNBossAIController::GetHighestAggroTarget() const
{
	AActor* HighestTarget = nullptr;
	float HighestAggro = 0.0f;

	for (const auto& Pair : AggroTable)
	{
		// if (Pair.Value > HighestAggro && IsValid(Pair.Key))
		if (Pair.Value > HighestAggro && IsValidAggroTarget(Pair.Key))
		{
			HighestAggro = Pair.Value;
			HighestTarget = Pair.Key;
		}
	}

	return HighestTarget;
}

void AERNBossAIController::TickSightAggro()
{
	// 최대 락 시간 초과 체크
	CheckForceTargetChange();

	if (SightAggroPerSecond > 0.f)
	{
		// 시야 안 플레이어들에게 매초 어그로 누적 (거리 배율 적용)
		for (int32 i = PerceivedPlayers.Num() - 1; i >= 0; --i)
		{
			AActor* Player = PerceivedPlayers[i];
			
			/*
			if (!IsValid(Player))
			{
				PerceivedPlayers.RemoveAt(i);
				continue;
			}
			*/
			// 유효 대상 판단 확인 수정
			if (!IsValidAggroTarget(Player))
			{
				RemoveAggroTarget(Player);
				continue;
			}

			// 거리에 따른 배율 계산
			const float DistanceMultiplier = GetDistanceAggroMultiplier(Player);
			const float AggroToAdd = SightAggroPerSecond * DistanceMultiplier;
			AddAggro(Player, AggroToAdd);
		}
	}
}

void AERNBossAIController::DecayAggro()
{
	TArray<AActor*> ToRemove;

	for (auto& Pair : AggroTable)
	{
		Pair.Value -= AggroDecayRate;

		// if (Pair.Value <= 0.0f || !IsValid(Pair.Key))
		if (Pair.Value <= 0.0f || !IsValidAggroTarget(Pair.Key))
		{
			ToRemove.Add(Pair.Key);
		}
	}

	// 0 이하인 어그로 제거
	for (AActor* Actor : ToRemove)
	{
		AggroTable.Remove(Actor);
	}

	// 어그로 테이블이 비면 타겟 해제
	if (AggroTable.Num() == 0 && Blackboard)
	{
		Blackboard->ClearValue(TEXT("TargetActor"));
		CurrentTargetStartTime = 0.f;
		CachedCurrentTarget.Reset();
	}
}

float AERNBossAIController::GetDistanceAggroMultiplier(AActor* Target) const
{
	if (!Target || !GetPawn())
	{
		return 1.0f;
	}

	const float Distance = FVector::Dist(GetPawn()->GetActorLocation(), Target->GetActorLocation());

	// 가까우면 높은 배율, 멀면 낮은 배율 (선형 보간)
	if (Distance <= NearDistanceThreshold)
	{
		return NearAggroMultiplier;
	}
	else if (Distance >= FarDistanceThreshold)
	{
		return FarAggroMultiplier;
	}
	else
	{
		// 중간 거리: 선형 보간
		const float Alpha = (Distance - NearDistanceThreshold) / (FarDistanceThreshold - NearDistanceThreshold);
		return FMath::Lerp(NearAggroMultiplier, FarAggroMultiplier, Alpha);
	}
}

bool AERNBossAIController::CanChangeTarget() const
{
	if (!GetWorld())
	{
		return true;
	}

	const float ElapsedTime = GetWorld()->GetTimeSeconds() - CurrentTargetStartTime;
	return ElapsedTime >= MinTargetLockTime;
}

void AERNBossAIController::CheckForceTargetChange()
{
	if (!GetWorld() || !Blackboard)
	{
		return;
	}

	AActor* CurrentTarget = Cast<AActor>(Blackboard->GetValueAsObject(TEXT("TargetActor")));
	if (!CurrentTarget)
	{
		return;
	}

	const float ElapsedTime = GetWorld()->GetTimeSeconds() - CurrentTargetStartTime;

	// 최대 락 시간 초과 시 강제 타겟 변경
	if (ElapsedTime >= MaxTargetLockTime)
	{
		AActor* NextTarget = GetSecondHighestAggroTarget();
		if (NextTarget && NextTarget != CurrentTarget)
		{
			SetTarget(NextTarget);
		}
	}
}

AActor* AERNBossAIController::GetSecondHighestAggroTarget() const
{
	AActor* CurrentTarget = Blackboard ? Cast<AActor>(Blackboard->GetValueAsObject(TEXT("TargetActor"))) : nullptr;

	AActor* SecondTarget = nullptr;
	float SecondHighestAggro = 0.0f;

	for (const auto& Pair : AggroTable)
	{
		// 현재 타겟 제외
		if (Pair.Key == CurrentTarget)
		{
			continue;
		}

		// if (Pair.Value > SecondHighestAggro && IsValid(Pair.Key))
		if (Pair.Value > SecondHighestAggro && IsValidAggroTarget(Pair.Key))
		{
			SecondHighestAggro = Pair.Value;
			SecondTarget = Pair.Key;
		}
	}

	return SecondTarget;
}

bool AERNBossAIController::IsValidAggroTarget(AActor* Target) const
{
	// 플레이어 캐릭터가 살아있는지 확인
	const AProjectERNCharacter* Player = Cast<AProjectERNCharacter>(Target);
	return Player && Player->IsAlive();
}

void AERNBossAIController::ReacquireTargetsAfterRevive()
{
	UWorld* World = GetWorld();
	if (!World || !Blackboard)
	{
		return;
	}

	AERNBossCharacter* Boss = Cast<AERNBossCharacter>(GetPawn());

	// 살아있는 전체 플레이어를 다시 인식 목록에 등록 + 어그로 부여 (AddAggro가 타겟 없으면 SetTarget까지 처리)
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		AProjectERNCharacter* Player = PC ? Cast<AProjectERNCharacter>(PC->GetPawn()) : nullptr;
		if (!IsValidAggroTarget(Player))
		{
			continue;
		}

		PerceivedPlayers.AddUnique(Player);
		AddAggro(Player, 10.0f);

		// 비전투 상태 해제 알림 (시야 최초 감지와 동일 처리)
		if (Boss)
		{
			Player->NotifyDetectedBy(Boss);
		}
	}
}

void AERNBossAIController::RemoveAggroTarget(AActor* Target)
{
	if (!Target)
	{
		return;
	}
	
	AggroTable.Remove(Target);
	PerceivedPlayers.Remove(Target);

	if (Blackboard && Blackboard->GetValueAsObject(TEXT("TargetActor")) == Target)
	{
		Blackboard->ClearValue(TEXT("TargetActor"));
		CurrentTargetStartTime = 0.f;
		CachedCurrentTarget.Reset();
	}
}
