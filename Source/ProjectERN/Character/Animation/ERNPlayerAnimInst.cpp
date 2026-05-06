// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Animation/ERNPlayerAnimInst.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GAS/ERNGameplayTags.h"

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
	
	// 속도 계산 (멀티플레이에서 Velocity는 서버 동기화됨)
	Velocity = MovementComp->Velocity;
	Speed = Velocity.Size();
	
	// 낙하 속도 계산
	if (bIsInAir)
	{
		FallSpeed = FMath::Max(0.f, -Velocity.Z);
	}
	
	// 이동 방향 계산 (캐릭터의 회전 기준으로 상대적 방향)
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
