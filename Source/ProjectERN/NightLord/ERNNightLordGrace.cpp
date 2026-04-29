// Fill out your copyright notice in the Description page of Project Settings.


#include "NightLord/ERNNightLordGrace.h"
#include "Character/ERNCharacterBase.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GAS/ERNAttributeSet.h"

AERNNightLordGrace::AERNNightLordGrace()
{
	PrimaryActorTick.bCanEverTick = false;
	
	// GraceMesh 설정
	GraceMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GraceMesh"));
	RootComponent = GraceMesh;
	
	// InteractionComponent 설정
	InteractionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionComponent"));
	InteractionComponent->SetupAttachment(RootComponent);
	InteractionComponent->SetSphereRadius(300.f);
	InteractionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void AERNNightLordGrace::BeginPlay()
{
	Super::BeginPlay();
	
	InteractionComponent->OnComponentBeginOverlap.AddDynamic(this, &AERNNightLordGrace::OnSphereBeginOverlap);
	
}

void AERNNightLordGrace::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// 플레이어 캐릭터를 불러옴 (몬스터와 구분시키기 위해 ProjectERNCharacter에 있는 플레이어 엑터를 불러옴)
	AProjectERNCharacter* PlayerCharacter = Cast<AProjectERNCharacter>(OtherActor); 
	
	// 플레이어 캐릭터라면
	if (PlayerCharacter)
	{
		// 회복 함수 호출
		RestoreAttributes(PlayerCharacter);
	}
}

void AERNNightLordGrace::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	//추후 UI를 숨기는 로직과 상호작용 해제와 관련된 로직 추가 예정
}

void AERNNightLordGrace::RestoreAttributes(AProjectERNCharacter* TargetCharacter)
{
	// 캐릭터가 맞다면 캐릭터가 가지고 있는 체력, 마나 등이 들어 있는 어트리뷰트 셋을 가져옴  
	UERNAttributeSet* AttributeSet = TargetCharacter->GetAttributeSet();
	if (!AttributeSet) return; 
	// 어트리뷰트셋이 없다면 리턴
	
	// AttributeSet에 있는 체력,마나,스테미나를 게터로 불러온 Max변수의 값에 맞춰서 셋
	AttributeSet->SetHealth(AttributeSet->GetMaxHealth());
	AttributeSet->SetMana(AttributeSet->GetMaxMana());
	AttributeSet->SetStamina(AttributeSet->GetMaxStamina());
	
	//TODO 디버그로그
	UE_LOG(LogTemp, Warning, TEXT("%s: 화톳불에 의해 모든 스탯이 회복되었습니다.!"), *TargetCharacter->GetName());
}
