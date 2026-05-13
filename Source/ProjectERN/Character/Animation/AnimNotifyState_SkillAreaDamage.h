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
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WeaponSkill")
	float Damage = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WeaponSkill")
	float Radius = 80.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WeaponSkill")
	float DamageMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WeaponSkill")
	float StaggerPower = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WeaponSkill")
	FVector Offset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WeaponSkill")
	TObjectPtr<UNiagaraSystem> NiagaraEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WeaponSkill")
	FName AttachSocketName = FName(TEXT("hand_r"));

private:
	TMap<TWeakObjectPtr<USkeletalMeshComponent>, TSet<TWeakObjectPtr<AActor>>> HitActorsByMesh;
	TMap<TWeakObjectPtr<USkeletalMeshComponent>, TWeakObjectPtr<UNiagaraComponent>> NiagaraComponentsByMesh;

	FVector GetDamageOrigin(const USkeletalMeshComponent* MeshComp) const;
	void ApplyDamageAtOrigin(USkeletalMeshComponent* MeshComp, const FVector& Origin);

};
