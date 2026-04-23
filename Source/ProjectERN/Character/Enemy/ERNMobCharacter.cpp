// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/Enemy/ERNMobCharacter.h"
#include "Character/Enemy/AI/ERNMobAIController.h"
#include "BehaviorTree/BlackboardComponent.h"

AERNMobCharacter::AERNMobCharacter()
{
	// 일반 몬스터용 AI Controller 클래스 설정
	AIControllerClass = AERNMobAIController::StaticClass();
}

void AERNMobCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 스폰 위치 저장 (귀환용)
	SpawnLocation = GetActorLocation();
}

float AERNMobCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	// 피격 시 공격자를 타겟으로 설정 (어그로)
	if (ActualDamage > 0.0f && HasAuthority())
	{
		if (AERNMobAIController* AIC = Cast<AERNMobAIController>(GetController()))
		{
			// 현재 타겟이 없을 때만 설정 (기존 타겟 유지)
			if (UBlackboardComponent* BB = AIC->GetBlackboardComp())
			{
				AActor* CurrentTarget = Cast<AActor>(BB->GetValueAsObject(TEXT("TargetActor")));
				if (!CurrentTarget)
				{
					// DamageCauser의 Owner(플레이어)를 타겟으로 설정
					AActor* Attacker = DamageCauser ? DamageCauser->GetOwner() : nullptr;
					if (!Attacker)
					{
						Attacker = DamageCauser;
					}

					if (Attacker)
					{
						AIC->SetTarget(Attacker);  // SetTarget으로 시야각도 변경
						bIsReturning = false;
					}
				}
			}
		}
	}

	return ActualDamage;
}
