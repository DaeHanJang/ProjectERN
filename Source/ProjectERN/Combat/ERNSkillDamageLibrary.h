#pragma once

#include "CoreMinimal.h"
#include "Combat/ERNSkillDamageTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ERNSkillDamageLibrary.generated.h"

UENUM(BlueprintType)
enum class EERNSkillHitResult : uint8	// 대미지 유형
{
	None UMETA(DisplayName="None"),
	Damage UMETA(DisplayName="Damage"),	// 적 공격
	Revive UMETA(DisplayName="Revive")	// 아군 구출 공격
};

UCLASS()
class PROJECTERN_API UERNSkillDamageLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// 적용 대미지 계산
	UFUNCTION(BlueprintPure, Category="ERN|SkillDamage")
	static float CalculateSkillDamage(AActor* SourceActor, const FERNSkillDamageData& DamageData);

	// ASC에서 AttackPower 반환
	UFUNCTION(BlueprintPure, Category="ERN|SkillDamage")
	static float GetSourceAttackPower(AActor* SourceActor);

	// ASC에서 AttackPowerBonus 반환
	UFUNCTION(BlueprintPure, Category="ERN|SkillDamage")
	static float GetSourceAttackPowerBonus(AActor* SourceActor);

	// 무기 대미지 반환
	UFUNCTION(BlueprintPure, Category="ERN|SkillDamage")
	static float GetSourceWeaponDamage(AActor* SourceActor);

	// 스킬 공격 적용
	UFUNCTION(BlueprintCallable, Category="ERN|SkillDamage")
	static EERNSkillHitResult ApplySkillHit(
		AActor* TargetActor,
		AActor* SourceActor,
		AController* InstigatorController,
		const FERNSkillDamageData& DamageData,
		const FVector& HitOrigin);
};
