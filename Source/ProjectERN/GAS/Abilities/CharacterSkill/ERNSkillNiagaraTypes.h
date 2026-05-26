#pragma once

#include "CoreMinimal.h"
#include "ERNSkillNiagaraTypes.generated.h"

class UNiagaraSystem;

USTRUCT(BlueprintType)
struct FERNSkillAttachedNiagaraEffect
{
	GENERATED_BODY()

	// 출력할 Niagara System
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Niagara")
	TObjectPtr<UNiagaraSystem> NiagaraSystem = nullptr;

	// 중복 생성 방지 및 Stop 처리를 위한 고유 키
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Niagara")
	FName EffectKey = NAME_None;

	// 캐릭터 Mesh의 Bone 또는 Socket 이름
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Niagara", meta=(AnimNotifyBoneName="true"))
	FName AttachSocketName = NAME_None;

	// Socket/Bone 기준 상대 위치
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Niagara")
	FVector LocationOffset = FVector::ZeroVector;

	// Socket/Bone 기준 상대 회전
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Niagara")
	FRotator RotationOffset = FRotator::ZeroRotator;

	// Niagara 컴포넌트 상대 스케일
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Niagara")
	FVector Scale = FVector::OneVector;

	// true면 Stop 시 즉시 Destroy
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Niagara")
	bool bDestroyOnStop = false;
};