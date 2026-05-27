// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/Enemy/ERNMobCharacter.h"
#include "Character/Enemy/AI/ERNMobAIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Character/Player/ProjectERNCharacter.h"

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
					// 어그로 대상은 항상 Pawn으로 잡아야 BT의 MoveTo/공격이 정상 동작
					// 플레이어 본인, 무기, 투사체 한번에 처리
					AActor* Attacker = nullptr;
					if (EventInstigator)
					{
						Attacker = EventInstigator->GetPawn();
					}
					if (!Attacker && Cast<APawn>(DamageCauser))
					{
						Attacker = DamageCauser;
					}
					if (!Attacker && DamageCauser)
					{
						Attacker = DamageCauser->GetOwner();
					}

					if (Attacker)
					{
						AIC->SetTarget(Attacker);  // SetTarget으로 시야각도 변경
						bIsReturning = false;

						// 시야 밖 원거리 피격도 비전투 해제되도록 명시 알림
						if (AProjectERNCharacter* Player = Cast<AProjectERNCharacter>(Attacker))
						{
							Player->NotifyDetectedBy(this);
						}

						// 주변 동료 몹들도 같은 공격자에게 어그로 전파
						AlertNearbyMobs(Attacker);
					}
				}
			}
		}
	}

	return ActualDamage;
}

void AERNMobCharacter::AlertNearbyMobs(AActor* Attacker)
{
	if (!bAlertNearbyMobs || !Attacker || !HasAuthority())
	{
		return;
	}

	TArray<AActor*> IgnoreActors;
	IgnoreActors.Add(this);

	UClass* FilterClass = bAlertOnlySameType ? GetClass() : AERNMobCharacter::StaticClass();

	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	TArray<AActor*> Found;
	UKismetSystemLibrary::SphereOverlapActors(
		this,
		GetActorLocation(),
		AlertRadius,
		ObjectTypes,
		FilterClass,
		IgnoreActors,
		Found);

	for (AActor* HitActor : Found)
	{
		AERNMobCharacter* Mob = Cast<AERNMobCharacter>(HitActor);
		if (!Mob || Mob->IsDead())
		{
			continue;
		}

		AERNMobAIController* AIC = Cast<AERNMobAIController>(Mob->GetController());
		if (!AIC)
		{
			continue;
		}

		UBlackboardComponent* BB = AIC->GetBlackboardComp();
		if (!BB)
		{
			continue;
		}

		// 이미 타겟 잡고 있는 몹은 기존 어그로 유지
		if (BB->GetValueAsObject(TEXT("TargetActor")))
		{
			continue;
		}

		AIC->SetTarget(Attacker);
		Mob->bIsReturning = false;

		// 전파받은 몹도 플레이어에게 감지 알림
		if (AProjectERNCharacter* Player = Cast<AProjectERNCharacter>(Attacker))
		{
			Player->NotifyDetectedBy(Mob);
		}
	}
}
