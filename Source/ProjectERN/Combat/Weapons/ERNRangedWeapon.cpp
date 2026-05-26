// Copyright Epic Games, Inc. All Rights Reserved.

#include "Combat/Weapons/ERNRangedWeapon.h"
#include "Combat/Projectile/ERNProjectileBase.h"
#include "Components/ArrowComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GAS/Abilities/CharacterSkill/ERNProjectileSpawnTypes.h"
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

void AERNRangedWeapon::SpawnProjectile(ACharacter* OwnerCharacter, USkeletalMeshComponent* SourceMesh)
{
	// 투사체 클래스 적용 했을때만 동작
	if (!ProjectileSpawnData.bUseProjectile || !ProjectileSpawnData.ProjectileClass)
	{
		return;
	}

	// 서버에서만 적용
	ACharacter* Character = OwnerCharacter ? OwnerCharacter : Cast<ACharacter>(GetOwner());
	if (!Character || !Character->HasAuthority() || !GetWorld())
	{
		return;
	}
	
	// 투사체 Transform 적용 (Default: 캐릭터 앞쪽 + BP로 입력한 Distance)
	FTransform SpawnTransform(
		Character->GetActorRotation(),
		Character->GetActorLocation()
		+ Character->GetActorForwardVector() * ProjectileSpawnData.CharacterForwardDistance,
		FVector::OneVector);

	// 투사체 스폰 지점 설정
	switch (ProjectileSpawnData.SpawnSource)
	{
		// 무기(Staff)의 Muzzle Point를 이용
	case EERNProjectileSpawnSource::WeaponMuzzle:
		{
			FVector Location;
			FRotator Rotation;
			if (GetDefaultProjectileSpawnTransform(Location, Rotation))
			{
				SpawnTransform = FTransform(Rotation, Location, FVector::OneVector);
			}
		}
		break;

		// 캐릭터의 소켓 이용
	case EERNProjectileSpawnSource::CharacterSocket:
		if (SourceMesh &&
			ProjectileSpawnData.CharacterSocketName != NAME_None &&
			SourceMesh->DoesSocketExist(ProjectileSpawnData.CharacterSocketName))
		{
			SpawnTransform = SourceMesh->GetSocketTransform(ProjectileSpawnData.CharacterSocketName);
		}
		break;

		// 캐릭터 앞쪽 소환
	case EERNProjectileSpawnSource::CharacterForward:
		// 무기의 HitBox 이용
	case EERNProjectileSpawnSource::WeaponHitbox:
		// 그 외
	default:
		break;
	}

	// 스폰 지점 + Offset 적용 (BP에서 Location Offset 적용)
	SpawnTransform.SetLocation(SpawnTransform.TransformPosition(ProjectileSpawnData.SpawnOffset));

	// Rotation 적용 (BP에서 사용 여부 설정 가능)
	const FRotator SpawnRotation = ProjectileSpawnData.bUseSourceRotation
		? SpawnTransform.GetRotation().Rotator()
		: Character->GetActorForwardVector().Rotation();

	// 투사체 소환
	SpawnProjectileAtTransform(
		ProjectileSpawnData.ProjectileClass,
		SpawnTransform.GetLocation(),
		SpawnRotation,
		ProjectileSpawnData.MuzzleEffect.Get());
}

void AERNRangedWeapon::SpawnProjectileAtTransform(TSubclassOf<AERNProjectileBase> ProjectileClass,
                                                  FVector SpawnLocation, 
                                                  FRotator SpawnRotation, 
                                                  UNiagaraSystem* MuzzleEffect)
{
	AActor* OwnerActor = GetOwner();
	// 서버에서만 투사체 생성
	if (!ProjectileClass || !OwnerActor || !OwnerActor->HasAuthority() || !GetWorld())
	{
		return;
	}

	// 이펙트 적용
	if (MuzzleEffect)
	{
		Multicast_PlayMuzzleEffect(SpawnLocation, SpawnRotation, MuzzleEffect);
	}

	// 스폰 옵션 설정
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = OwnerActor;
	SpawnParams.Instigator = Cast<APawn>(OwnerActor);
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// 옵션 적용하여 투사체 스폰
	GetWorld()->SpawnActor<AERNProjectileBase>(
		ProjectileClass,
		SpawnLocation,
		SpawnRotation,
		SpawnParams);
}

bool AERNRangedWeapon::GetDefaultProjectileSpawnTransform(FVector& OutLocation, FRotator& OutRotation) const
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return false;
	}

	// 발사 위치: MuzzlePoint → WeaponMesh 소켓 → 무기 위치 순으로 결정
	OutLocation = GetActorLocation();
	
	if (MuzzlePoint)
	{
		OutLocation = MuzzlePoint->GetComponentLocation();
	}
	else if (WeaponMesh && WeaponMesh->DoesSocketExist(MuzzleSocketName))
	{
		OutLocation = WeaponMesh->GetSocketLocation(MuzzleSocketName);
	}
	else if (SkeletalWeaponMesh && SkeletalWeaponMesh->DoesSocketExist(MuzzleSocketName))
	{
		OutLocation = SkeletalWeaponMesh->GetSocketLocation(MuzzleSocketName);
	}

	OutRotation = OwnerActor->GetActorForwardVector().Rotation();
	return true;
}

void AERNRangedWeapon::Multicast_PlayMuzzleEffect_Implementation(FVector Location, FRotator Rotation, UNiagaraSystem* Effect)
{
	if (Effect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), Effect, Location, Rotation);
	}
}
