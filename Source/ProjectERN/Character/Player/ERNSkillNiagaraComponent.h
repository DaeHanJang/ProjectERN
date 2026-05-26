// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GAS/Abilities/CharacterSkill/ERNSkillNiagaraTypes.h"
#include "ERNSkillNiagaraComponent.generated.h"

class UNiagaraComponent;
class USkeletalMeshComponent;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECTERN_API UERNSkillNiagaraComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UERNSkillNiagaraComponent();

	// 스킬 시작 시 호출
	void StartEffects(const TArray<FERNSkillAttachedNiagaraEffect>& Effects);

	// 스킬 종료 시 호출
	void StopEffects(const TArray<FERNSkillAttachedNiagaraEffect>& Effects);

	// 특정 EffectKey만 정지하고 싶을 때 사용
	void StopEffectByKey(FName EffectKey, bool bDestroyOnStop = false);

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// 클라이언트가 스킬을 시작했을 때 서버로 요청
	UFUNCTION(Server, Reliable)
	void ServerStartEffects(const TArray<FERNSkillAttachedNiagaraEffect>& Effects);

	// 클라이언트가 스킬을 종료했을 때 서버로 요청
	UFUNCTION(Server, Reliable)
	void ServerStopEffects(const TArray<FERNSkillAttachedNiagaraEffect>& Effects);

	UFUNCTION(Server, Reliable)
	void ServerStopEffectByKey(FName EffectKey, bool bDestroyOnStop);

	// 서버가 모든 클라이언트에 Niagara 시작을 전파
	UFUNCTION(NetMulticast, Reliable)
	void MulticastStartEffects(const TArray<FERNSkillAttachedNiagaraEffect>& Effects);

	// 서버가 모든 클라이언트에 Niagara 정지를 전파
	UFUNCTION(NetMulticast, Reliable)
	void MulticastStopEffects(const TArray<FERNSkillAttachedNiagaraEffect>& Effects);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastStopEffectByKey(FName EffectKey, bool bDestroyOnStop);

	// 실제 로컬 Niagara 생성
	void SpawnEffect_Local(const FERNSkillAttachedNiagaraEffect& EffectData);

	// 실제 로컬 Niagara 정지
	void StopEffect_Local(const FERNSkillAttachedNiagaraEffect& EffectData);
	void StopEffectByKey_Local(FName EffectKey, bool bDestroyOnStop);
	void StopAllEffects_Local();

	FName ResolveEffectKey(const FERNSkillAttachedNiagaraEffect& EffectData) const;
	USkeletalMeshComponent* GetOwnerMesh() const;
	bool CanSendServerRpc() const;

	// 현재 재생 중인 Niagara 컴포넌트 목록
	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UNiagaraComponent>> ActiveEffects;
};
