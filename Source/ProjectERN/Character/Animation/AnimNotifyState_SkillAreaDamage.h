// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_SkillAreaDamage.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;

/**
 * 
 */
UENUM(BlueprintType)
enum class ESkillAreaDamageOriginMode : uint8
{
	CharacterOffset UMETA(DisplayName = "Character Offset"),
	MeshSocket UMETA(DisplayName = "Mesh Socket"),
	WeaponHitbox UMETA(DisplayName = "Weapon Hitbox")
};

UCLASS()
class PROJECTERN_API UAnimNotifyState_SkillAreaDamage : public UAnimNotifyState
{
	GENERATED_BODY()
	
public:
	virtual void NotifyBegin(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		float TotalDuration,
		const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyTick(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		float FrameDeltaTime,
		const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override
	{
		return TEXT("Skill Area Damage");
	}
	
	// 대미지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WeaponSkill")
	float Damage = 30.f;

	// 반경
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WeaponSkill")
	float Radius = 80.f;
	
	// 대미지 배수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WeaponSkill")
	float DamageMultiplier = 1.f;

	// 적용 경직 수치
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WeaponSkill")
	float StaggerPower = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WeaponSkill")
	FVector Offset = FVector::ZeroVector;
	
	// 부착 소켓
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WeaponSkill")
	FName AttachSocketName = FName(TEXT("hand_r"));
	
	// 대미지 적용 Sphere의 적용 위치
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WeaponSkill")
	ESkillAreaDamageOriginMode OriginMode = ESkillAreaDamageOriginMode::WeaponHitbox;
	
	// 나이아가라
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WeaponSkill|VFX")
	TObjectPtr<UNiagaraSystem> NiagaraEffect;

	// 나이아가라 오프셋
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WeaponSkill|VFX")
	FVector NiagaraOffset = FVector(150.f, 0.f, 50.f);

private:
	TMap<TWeakObjectPtr<USkeletalMeshComponent>, TSet<TWeakObjectPtr<AActor>>> HitActorsByMesh;
	TMap<TWeakObjectPtr<USkeletalMeshComponent>, TWeakObjectPtr<UNiagaraComponent>> NiagaraComponentsByMesh;

	FVector GetDamageOrigin(const USkeletalMeshComponent* MeshComp) const;
	void ApplyDamageAtOrigin(USkeletalMeshComponent* MeshComp, const FVector& Origin);
	
	/** 
	 * 디버그 드로잉
	 */
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WeaponSkill|Debug")
	bool bDrawDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WeaponSkill|Debug")
	float DebugDrawTime = 0.1f;
	// --- 디버그용 ---

};
