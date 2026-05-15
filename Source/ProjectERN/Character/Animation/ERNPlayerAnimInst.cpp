// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Animation/ERNPlayerAnimInst.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GAS/ERNGameplayTags.h"
#include "Subsystem/ERNCutsceneSubsystem.h"
#include "Character/ERNCharacterBase.h"

void UERNPlayerAnimInst::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	
	// 소유자 캐릭터 가져오기 (PawnOwner는 애니메이션 인스턴스가 붙은 캐릭터)
	ACharacter* Char = Cast<ACharacter>(TryGetPawnOwner());
	if (!Char)
	{
		bIsAttacking = false;
		return;  // 캐릭터가 없으면 업데이트하지 않음
	}
	
	UAbilitySystemComponent* ASC =
	UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Char);

	bIsAttacking = ASC && ASC->HasMatchingGameplayTag(TAG_State_Combat_Attacking);
	
	// MovementComponent 가져오기
	UCharacterMovementComponent* MovementComp = Char->GetCharacterMovement();
	if (!MovementComp)
	{
		return;
	}
	
	// 공중 확인 (점프/낙하)
	bIsInAir = MovementComp->IsFalling();
	
	// 가속 확인
	bIsAccelerating = MovementComp->GetCurrentAcceleration().Size() > 0.f;
	
	// 컷신 재생 중인지 확인
	bool bInCutscene = false;
	if (UGameInstance* GI = Cast<UGameInstance>(Char->GetWorld()->GetGameInstance()))
	{
		if (UERNCutsceneSubsystem* CutsceneSubsystem = GI->GetSubsystem<UERNCutsceneSubsystem>())
		{
			bInCutscene = CutsceneSubsystem->IsPlayingCutscene();
		}
	}

	// 컷신 → 일반 플레이 전환 시 위치 추적 리셋 (튀는 현상 방지)
	if (bWasInCutscene && !bInCutscene)
	{
		bPreviousLocationValid = false;
	}
	bWasInCutscene = bInCutscene;

	// 컷신 중: 서버에서 리플리케이트된 CutsceneSpeed 사용
	if (bInCutscene)
	{
		if (AERNCharacterBase* ERNChar = Cast<AERNCharacterBase>(Char))
		{
			Speed = ERNChar->CutsceneSpeed;
			// 컷신 중에는 캐릭터가 바라보는 방향으로 전진 (Direction = 0)
			Velocity = FVector::ZeroVector;
			Direction = 0.f;
		}
	}
	else
	{
		// 일반 플레이: 위치 기반 속도 계산 (시퀀서 Transform Track 지원)
		FVector CurrentLocation = Char->GetActorLocation();

		if (bPreviousLocationValid && DeltaSeconds > 0.f)
		{
			// 위치 변화량으로 속도 계산
			FVector PositionDelta = CurrentLocation - PreviousLocation;
			float CalculatedSpeed = PositionDelta.Size() / DeltaSeconds;

			// 텔레포트 감지 (비정상적으로 큰 속도는 무시)
			constexpr float MaxReasonableSpeed = 2000.f;
			if (CalculatedSpeed > MaxReasonableSpeed)
			{
				CalculatedSpeed = 0.f;
				PositionDelta = FVector::ZeroVector;
			}

			// 미세 떨림 방지 (threshold 이하면 정지 처리)
			constexpr float SpeedThreshold = 20.f;
			if (CalculatedSpeed < SpeedThreshold)
			{
				CalculatedSpeed = 0.f;
				PositionDelta = FVector::ZeroVector;
			}

			Speed = CalculatedSpeed;
			Velocity = PositionDelta / DeltaSeconds;
		}
		else
		{
			// 첫 프레임: MovementComponent Velocity 사용
			Velocity = MovementComp->Velocity;
			Speed = Velocity.Size();
			bPreviousLocationValid = true;
		}

		PreviousLocation = CurrentLocation;
	}

	// 낙하 속도 계산
	if (bIsInAir)
	{
		FallSpeed = FMath::Max(0.f, -MovementComp->Velocity.Z);
	}

	// 이동 방향 계산 (캐릭터의 회전 기준으로 상대적 방향)
	// 컷신 중에는 위에서 이미 Direction = 0으로 설정됨
	if (!bInCutscene)
	{
		if (Speed > 0.f)
		{
			FRotator CharRotation = Char->GetActorRotation();
			FRotator VelocityRotation = Velocity.Rotation();
			Direction = FMath::UnwindDegrees(VelocityRotation.Yaw - CharRotation.Yaw);
		}
		else
		{
			// 정지 시 방향 0
			Direction = 0.f;
		}
	}
}
