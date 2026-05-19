// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/Abilities/ERNGA_WeaponSkill.h"
#include "GAS/Abilities/WeaponSkill/ERNWeaponSkillTypes.h"
#include "ERNGA_WeaponSkill_Instant.generated.h"

class AERNProjectileBase;
class UNiagaraSystem;
class UNiagaraComponent;

// 선택에 따라 노출될 변수 조절하기 위한 구조체(범위 공격)
USTRUCT(BlueprintType)
struct FERNWeaponSkillAreaDamageData
{
	GENERATED_BODY()

	// 범위 공격 사용 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AreaDamage")
	bool bUseAreaDamage = false;

	// 범위 공격 대미지
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AreaDamage", meta=(EditCondition="bUseAreaDamage", EditConditionHides))
	float BaseDamage = 30.f;

	// 대미지 배수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AreaDamage", meta=(EditCondition="bUseAreaDamage", EditConditionHides))
	float DamageMultiplier = 1.f;

	// 공격 범위
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AreaDamage", meta=(EditCondition="bUseAreaDamage", EditConditionHides))
	float DamageRadius = 120.f;

	// 경직도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AreaDamage", meta=(EditCondition="bUseAreaDamage", EditConditionHides))
	float StaggerPower = 0.f;

	// 범위 판정 위치 지정
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AreaDamage", meta=(EditCondition="bUseAreaDamage", EditConditionHides))
	EWeaponSkillAreaOriginMode OriginMode = EWeaponSkillAreaOriginMode::WeaponHitbox;

	// 범위 판정 위치 : MeshSocket | 소켓 이름 지정
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AreaDamage", meta=(EditCondition="bUseAreaDamage && OriginMode == EWeaponSkillAreaOriginMode::MeshSocket", EditConditionHides))
	FName MeshSocketName = FName(TEXT("hand_r"));

	// 범위 판정 오프셋
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AreaDamage", meta=(EditCondition="bUseAreaDamage", EditConditionHides))
	FVector OriginOffset = FVector::ZeroVector;

	// 스킬 나이아가라 효과
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AreaDamage|VFX", meta=(EditCondition="bUseAreaDamage", EditConditionHides))
	TObjectPtr<UNiagaraSystem> AreaEffect;

	// 효과 오프셋
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AreaDamage|VFX", meta=(EditCondition="bUseAreaDamage", EditConditionHides))
	FVector AreaEffectOffset = FVector(150.f, 0.f, 50.f);

	// 범위 판정 디버그 스피어 사용 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AreaDamage|Debug", meta=(EditCondition="bUseAreaDamage", EditConditionHides))
	bool bDrawDebug = false;

	// 디버그 사용 : 노출 시간 설정
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AreaDamage|Debug", meta=(EditCondition="bUseAreaDamage && bDrawDebug", EditConditionHides))
	float DebugDrawTime = 0.1f;
};

// 선택에 따라 노출될 변수 조절하기 위한 구조체(투사체)
USTRUCT(BlueprintType)
struct FERNWeaponSkillProjectileData
{
	GENERATED_BODY()

	// 투사체 사용 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Projectile")
	bool bUseProjectile = false;

	// 투사체 클래스 지정
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Projectile", meta=(EditCondition="bUseProjectile", EditConditionHides))
	TSubclassOf<class AERNProjectileBase> ProjectileClass;

	// 투사체 스폰 위치 지정
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Projectile", meta=(EditCondition="bUseProjectile", EditConditionHides))
	EWeaponSkillProjectileSpawnSource SpawnSource = EWeaponSkillProjectileSpawnSource::WeaponMuzzle;

	// 투사체 스폰 위치 : Character Socket | 소켓 이름 지정
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Projectile", meta=(EditCondition="bUseProjectile && SpawnSource == EWeaponSkillProjectileSpawnSource::CharacterSocket", EditConditionHides))
	FName CharacterSocketName = FName(TEXT("hand_r"));

	// 투사체 스폰 위치 : Character Forward | 스폰 거리
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Projectile", meta=(EditCondition="bUseProjectile && SpawnSource == EWeaponSkillProjectileSpawnSource::CharacterForward", EditConditionHides))
	float CharacterForwardDistance = 100.f;

	// 투사체 스폰 오프셋
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Projectile", meta=(EditCondition="bUseProjectile", EditConditionHides))
	FVector SpawnOffset = FVector::ZeroVector;

	// 소환 위치의 Rotation 사용 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Projectile", meta=(EditCondition="bUseProjectile", EditConditionHides))
	bool bUseSourceRotation = false;
};
/**
 * 
 */
UCLASS()
class PROJECTERN_API UERNGA_WeaponSkill_Instant : public UERNGA_WeaponSkill
{
	GENERATED_BODY()
	
public:
	UERNGA_WeaponSkill_Instant();

	// NotifyState 범위 데미지 적용
	void BeginAreaDamage(USkeletalMeshComponent* MeshComp);
	void TickAreaDamage(USkeletalMeshComponent* MeshComp);
	void EndAreaDamage(USkeletalMeshComponent* MeshComp);

	// Notify 투사체 발사
	void FireProjectileFromNotify(USkeletalMeshComponent* MeshComp);

protected:
	// 범위 대미지 데이터
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="WeaponSkill|AreaDamage")
	FERNWeaponSkillAreaDamageData AreaDamageData;

	// 투사체 데이터
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="WeaponSkill|Projectile")
	FERNWeaponSkillProjectileData ProjectileData;

private:
	// MeshComp별로 이번 AreaDamage 구간에서 이미 피격된 Actor 목록을 저장한다. (중복 타격 방지)
	TMap<TWeakObjectPtr<USkeletalMeshComponent>, TSet<TWeakObjectPtr<AActor>>> HitActorsByMesh;
	TMap<TWeakObjectPtr<USkeletalMeshComponent>, TWeakObjectPtr<UNiagaraComponent>> AreaEffectsByMesh;

	FVector GetAreaDamageOrigin(USkeletalMeshComponent* MeshComp) const;
	bool GetProjectileSpawnTransform(USkeletalMeshComponent* MeshComp, FTransform& OutTransform) const;

	// 범위 대미지 적용
	void ApplyAreaDamage(USkeletalMeshComponent* MeshComp, const FVector& Origin);
	// 범위 대미지 계산
	float CalculateAreaDamage(AActor* OwnerActor) const;
	// 무기 공격력 받아오기
	float GetWeaponBaseDamage(AActor* OwnerActor) const;
	// 캐릭터 공격력 받아오기
	float GetCharacterAttackPower(AActor* OwnerActor) const;
};
