// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/Enemy/AI/ERNMobAIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISense_Sight.h"
#include "Character/Player/ProjectERNCharacter.h"

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
		if (Stimulus.WasSuccessfullySensed())
		{
			SetTarget(Player);
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
		}
	}
}

void AERNMobAIController::SetTarget(AActor* NewTarget)
{
	if (!Blackboard)
	{
		return;
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
