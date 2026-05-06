// Copyright Epic Games, Inc. All Rights Reserved.

#include "Combat/Weapons/ERNRangedWeapon.h"
#include "Combat/Projectile/ERNProjectileBase.h"
#include "Components/ArrowComponent.h"
#include "NiagaraFunctionLibrary.h"

AERNRangedWeapon::AERNRangedWeapon()
{
	// MuzzlePoint: 스태틱 메시는 소켓 미지원이므로 ArrowComponent로 발사 위치를 표시
	// 에디터에서 WeaponMesh 자식으로 위치/회전 조정
	MuzzlePoint = CreateDefaultSubobject<UArrowComponent>(TEXT("MuzzlePoint"));
	// MuzzlePoint->SetupAttachment(WeaponMesh);
	MuzzlePoint->SetupAttachment(SceneRoot);
	MuzzlePoint->ArrowColor = FColor::Red;
	MuzzlePoint->bHiddenInGame = true;
}

void AERNRangedWeapon::SpawnProjectile(bool bIsHeavyAttack)
{
	// 공격 종류에 따라 투사체 클래스 선택
	// 강공격 클래스가 미설정이면 약공격 클래스로 폴백
	TSubclassOf<AERNProjectileBase> SelectedProjectile = bIsHeavyAttack && HeavyProjectileClass
		? HeavyProjectileClass
		: LightProjectileClass;

	if (!SelectedProjectile || !GetOwner())
	{
		return;
	}

	// 발사 위치: MuzzlePoint → WeaponMesh 소켓 → 무기 위치 순으로 결정
	// 발사 방향: 캐릭터 전방
	FVector SpawnLocation = GetActorLocation();
	FRotator SpawnRotation = GetOwner()->GetActorRotation();

	if (MuzzlePoint)
	{
		SpawnLocation = MuzzlePoint->GetComponentLocation();
	}
	else if (WeaponMesh && WeaponMesh->DoesSocketExist(MuzzleSocketName))
	{
		SpawnLocation = WeaponMesh->GetSocketLocation(MuzzleSocketName);
	}
	else if (SkeletalWeaponMesh && SkeletalWeaponMesh->DoesSocketExist(MuzzleSocketName))
	{
		SpawnLocation = SkeletalWeaponMesh->GetSocketLocation(MuzzleSocketName);
	}
	
	// 캐릭터 전방 방향
	SpawnRotation = GetOwner()->GetActorForwardVector().Rotation();

	// 공격 종류에 따라 이펙트 선택 (강공격 이펙트 미설정 시 기본 이펙트 사용)
	UNiagaraSystem* SelectedEffect = bIsHeavyAttack && HeavyMuzzleEffect ? HeavyMuzzleEffect : LightMuzzleEffect;
	if (SelectedEffect)
	{
		Multicast_PlayMuzzleEffect(SpawnLocation, SpawnRotation, SelectedEffect);
	}

	// 투사체 스폰
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.Instigator = Cast<APawn>(GetOwner());
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	GetWorld()->SpawnActor<AERNProjectileBase>(SelectedProjectile, SpawnLocation, SpawnRotation, SpawnParams);
}

void AERNRangedWeapon::Multicast_PlayMuzzleEffect_Implementation(FVector Location, FRotator Rotation, UNiagaraSystem* Effect)
{
	if (Effect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), Effect, Location, Rotation);
	}
}
