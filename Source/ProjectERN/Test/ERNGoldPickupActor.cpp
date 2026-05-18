// 테스트용 골드 지급 액터 구현

#include "Test/ERNGoldPickupActor.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "GAS/ERNAttributeSet.h"
#include "AbilitySystemComponent.h"

AERNGoldPickupActor::AERNGoldPickupActor()
{
	PrimaryActorTick.bCanEverTick = false;

	// 루트 컴포넌트: 오버랩 감지용 Sphere
	OverlapSphere = CreateDefaultSubobject<USphereComponent>(TEXT("OverlapSphere"));
	OverlapSphere->SetSphereRadius(150.f);
	OverlapSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	OverlapSphere->SetGenerateOverlapEvents(true);
	SetRootComponent(OverlapSphere);

	// 시각적 메쉬 (기본 구체, 에디터에서 변경 가능)
	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetupAttachment(OverlapSphere);
	MeshComp->SetRelativeScale3D(FVector(0.5f));
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 기본 구체 메쉬 할당
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		MeshComp->SetStaticMesh(SphereMesh.Object);
	}

	// 기본 골드색 머티리얼은 에디터에서 직접 설정 가능
}

void AERNGoldPickupActor::BeginPlay()
{
	Super::BeginPlay();

	// 서버에서만 오버랩 감지 (골드 지급은 서버 권한)
	if (HasAuthority())
	{
		OverlapSphere->OnComponentBeginOverlap.AddDynamic(this, &AERNGoldPickupActor::OnOverlapBegin);
	}
}

void AERNGoldPickupActor::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority()) return;
	if (!bReusable && bAlreadyUsed) return;

	// 오버랩한 액터가 플레이어 캐릭터인지 확인
	AProjectERNCharacter* PlayerChar = Cast<AProjectERNCharacter>(OtherActor);
	if (!PlayerChar) return;

	// AttributeSet에서 골드를 가져와 추가
	UERNAttributeSet* AttributeSet = PlayerChar->GetAttributeSet();
	if (!AttributeSet) return;

	float CurrentGold = AttributeSet->GetGold();
	AttributeSet->SetGold(CurrentGold + GoldAmount);

	UE_LOG(LogTemp, Warning, TEXT("[GoldPickup] %s에게 %d 골드 지급! (현재 골드: %d → %d)"),
		*PlayerChar->GetName(), GoldAmount,
		FMath::FloorToInt(CurrentGold),
		FMath::FloorToInt(CurrentGold + GoldAmount));

	if (!bReusable)
	{
		bAlreadyUsed = true;
	}
}
