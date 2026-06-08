// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/Enemy/ERNWatcherMonster.h"
#include "Components/SphereComponent.h"
#include "Components/DecalComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"

AERNWatcherMonster::AERNWatcherMonster()
{
	PrimaryActorTick.bCanEverTick = true;

	// 체력바, 드롭 등 AERNEnemyCharacter의 기본 기능 중 불필요한 부분 비활성화
	InitialMaxHealth = 999999.0f; // 안 죽도록
	bIsImmortal = true;           // 무적 처리
	bExcludeFromCombatStats = true;
	BasicRewordGold = 0;
	InstancePortalSpawnChance = 0.0f;

	// 중력을 무시하고 비행 모드로 설정하여 공중에 배치할 수 있도록 처리
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->GravityScale = 0.0f;
		GetCharacterMovement()->DefaultLandMovementMode = MOVE_Flying;
	}
	if (GetCapsuleComponent())
	{
		GetCapsuleComponent()->SetEnableGravity(false);
	}

	// 1. 감시 반경 콜리전 (오버랩만 체크)
	SurveillanceCollision = CreateDefaultSubobject<USphereComponent>(TEXT("SurveillanceCollision"));
	SurveillanceCollision->SetupAttachment(RootComponent);
	SurveillanceCollision->SetSphereRadius(SurveillanceRadius);
	SurveillanceCollision->SetGenerateOverlapEvents(true);
	SurveillanceCollision->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

	// 2. 바닥에 그려지는 데칼 (점점 커지는 CCTV 연출용)
	SurveillanceDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("SurveillanceDecal"));
	SurveillanceDecal->SetupAttachment(RootComponent);
	SurveillanceDecal->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f)); // 바닥 방향을 보도록 세팅

	// 3. 허공에 렌더링되는 3D 볼륨 메쉬 (데칼과 함께 쓰거나 선택 가능)
	SurveillanceMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SurveillanceMesh"));
	SurveillanceMesh->SetupAttachment(RootComponent);
	SurveillanceMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 시각용이므로 충돌 제거
}

void AERNWatcherMonster::BeginPlay()
{
	Super::BeginPlay();

	// 콜리전 크기 초기화 (블루프린트에서 수정한 값 적용)
	SurveillanceCollision->SetSphereRadius(SurveillanceRadius);

	// 게임 시작 시 확실하게 비행 모드로 전환
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	}

	InitialLocation = GetActorLocation();
	InitialRightVector = GetActorRightVector();
	
	InitialDecalScale = SurveillanceDecal->GetRelativeScale3D();
	InitialMeshScale = SurveillanceMesh->GetRelativeScale3D();

	// 오버랩 델리게이트 바인딩
	SurveillanceCollision->OnComponentBeginOverlap.AddDynamic(this, &AERNWatcherMonster::OnSurveillanceBeginOverlap);
	SurveillanceCollision->OnComponentEndOverlap.AddDynamic(this, &AERNWatcherMonster::OnSurveillanceEndOverlap);
}

void AERNWatcherMonster::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// --- 1. 감시망 시각화(스윕 연출) 및 판정 On/Off ---
	CurrentCycleTime += DeltaTime;
	if (CurrentCycleTime > SurveillanceCycleTime)
	{
		CurrentCycleTime = fmod(CurrentCycleTime, SurveillanceCycleTime);
	}

	// 0 ~ 1 사이로 부드럽게 커졌다가 작아지는 사인파 (0부터 시작해 중간에 1이 되고 다시 0으로)
	float Alpha = FMath::Clamp(FMath::Sin((CurrentCycleTime / SurveillanceCycleTime) * PI), 0.0f, 1.0f);

	// 데칼과 메쉬의 크기를 0% ~ 100% 사이로 조절하여 켜지고 꺼지는 듯한 역동적인 연출
	SurveillanceDecal->SetRelativeScale3D(InitialDecalScale * Alpha);
	SurveillanceMesh->SetRelativeScale3D(InitialMeshScale * Alpha);

	// 시각적으로 크기가 일정 이상일 때만 실제 감시 기능 활성화
	bIsSurveillanceActive = (Alpha >= SurveillanceActiveThreshold);

	// --- 2. 이동 및 회전 기믹 ---
	if (bShouldRotate)
	{
		// 제자리 회전 (Sweep 효과처럼 회전)
		AddActorLocalRotation(FRotator(0.0f, RotationSpeed * DeltaTime, 0.0f));
	}

	if (bShouldMoveLeftRight)
	{
		// 좌우 이동 (몬스터가 회전하더라도 이동 축은 최초 배치된 좌/우 방향을 고정으로 사용)
		float MoveOffset = FMath::Sin((CurrentCycleTime / SurveillanceCycleTime) * PI * 2.0f * MovementSpeed) * MovementDistance;
		FVector NewLocation = InitialLocation + (InitialRightVector * MoveOffset);
		SetActorLocation(NewLocation);
	}

	// --- 3. 플레이어 감시 각도 판정 ---
	if (bIsSurveillanceActive && OverlappingPlayers.Num() > 0)
	{
		// 텔레포트 시 SetActorLocation으로 인해 OnSurveillanceEndOverlap이 즉시 호출되어 
		// 원본 배열(OverlappingPlayers)이 수정될 수 있으므로 복사본으로 순회합니다.
		TArray<AActor*> PlayersToCheck = OverlappingPlayers;
		for (AActor* PlayerActor : PlayersToCheck)
		{
			if (IsValid(PlayerActor))
			{
				// 배열에서 이미 제거되었다면 스킵
				if (!OverlappingPlayers.Contains(PlayerActor)) continue;

				// 액터 간 방향 벡터 (높이 차이는 무시하고 평면 각도만 계산)
				FVector DirToPlayer = (PlayerActor->GetActorLocation() - GetActorLocation()).GetSafeNormal();
				DirToPlayer.Z = 0.0f;
				DirToPlayer.Normalize();
				
				FVector ForwardVec = GetActorForwardVector();
				ForwardVec.Z = 0.0f;
				ForwardVec.Normalize();

				// 내적(Dot Product)을 통해 두 벡터 사이의 각도 계산
				float DotResult = FVector::DotProduct(ForwardVec, DirToPlayer);
				float AngleRad = FMath::Acos(DotResult);
				float AngleDeg = FMath::RadiansToDegrees(AngleRad);

				// 부채꼴 시야각 이내에 들어왔는지 확인
				if (AngleDeg <= SurveillanceAngle)
				{
					// 감시망에 걸렸으므로 순간이동 처리!
					// 텔레포트에 성공하면 EndOverlap이 트리거되어 원본 배열에서 안전하게 자동 제거됩니다.
					TeleportPlayer(PlayerActor);
				}
			}
			else
			{
				OverlappingPlayers.Remove(PlayerActor); // 유효하지 않은 액터는 지움
			}
		}
	}
}

void AERNWatcherMonster::OnSurveillanceBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor && OtherActor != this)
	{
		// 플레이어인지 확인 (컨트롤러 여부로 판단)
		APawn* OverlappedPawn = Cast<APawn>(OtherActor);
		if (OverlappedPawn && OverlappedPawn->IsPlayerControlled())
		{
			OverlappingPlayers.AddUnique(OtherActor);
		}
	}
}

void AERNWatcherMonster::OnSurveillanceEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor && OverlappingPlayers.Contains(OtherActor))
	{
		OverlappingPlayers.Remove(OtherActor);
	}
}

void AERNWatcherMonster::TeleportPlayer(AActor* PlayerActor)
{
	if (!IsValid(PlayerActor)) return;

	if (TeleportPointClass)
	{
		TArray<AActor*> FoundPoints;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), TeleportPointClass, FoundPoints);

		if (FoundPoints.Num() > 0)
		{
			// 첫 번째 찾은 포인트로 플레이어 강제 순간이동
			AActor* TargetPoint = FoundPoints[0];
			PlayerActor->SetActorLocation(TargetPoint->GetActorLocation());
		}
	}
}
