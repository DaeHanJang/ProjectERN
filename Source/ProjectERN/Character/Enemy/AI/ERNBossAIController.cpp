// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/Enemy/AI/ERNBossAIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Character/Player/ProjectERNCharacter.h"

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
		if (Stimulus.WasSuccessfullySensed())
		{
			// 시야에 들어온 플레이어 어그로 추가
			AddAggro(Player, 10.0f);

			// 타겟이 없으면 설정
			AActor* CurrentTarget = Cast<AActor>(Blackboard->GetValueAsObject(TEXT("TargetActor")));
			if (!CurrentTarget)
			{
				SetTarget(Player);
			}

			UE_LOG(LogTemp, Log, TEXT("[Boss %s] Player detected: %s"), *GetName(), *Player->GetName());
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

	UE_LOG(LogTemp, Log, TEXT("[Boss %s] Switched to BehaviorTree: %s"), *GetName(), *NewBehaviorTree->GetName());
}

void AERNBossAIController::SetTarget(AActor* NewTarget)
{
	if (Blackboard)
	{
		Blackboard->SetValueAsObject(TEXT("TargetActor"), NewTarget);
	}
}

void AERNBossAIController::AddAggro(AActor* Target, float AggroAmount)
{
	if (!Target)
	{
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

	// 어그로가 가장 높은 타겟으로 전환
	AActor* HighestAggroTarget = GetHighestAggroTarget();
	if (HighestAggroTarget)
	{
		SetTarget(HighestAggroTarget);
	}
}

AActor* AERNBossAIController::GetHighestAggroTarget() const
{
	AActor* HighestTarget = nullptr;
	float HighestAggro = 0.0f;

	for (const auto& Pair : AggroTable)
	{
		if (Pair.Value > HighestAggro && IsValid(Pair.Key))
		{
			HighestAggro = Pair.Value;
			HighestTarget = Pair.Key;
		}
	}

	return HighestTarget;
}

void AERNBossAIController::DecayAggro()
{
	TArray<AActor*> ToRemove;

	for (auto& Pair : AggroTable)
	{
		Pair.Value -= AggroDecayRate;

		if (Pair.Value <= 0.0f || !IsValid(Pair.Key))
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
	}
}
