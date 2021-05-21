// Fill out your copyright notice in the Description page of Project Settings.


#include "TransitionVolume.h"
#include "Components/BillboardComponent.h"
#include "Components/BoxComponent.h"
#include "Main.h"

// Sets default values
ATransitionVolume::ATransitionVolume()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	TransitionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TransitionVolume"));
	RootComponent = TransitionVolume;

	Billboard = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard"));
	Billboard->SetupAttachment(GetRootComponent());

	TransitionLevelName = "SunTemple";
}

// Called when the game starts or when spawned
void ATransitionVolume::BeginPlay()
{
	Super::BeginPlay();

	TransitionVolume->OnComponentBeginOverlap.AddDynamic(this, &ATransitionVolume::OnOverlapBegin);
	
}

// Called every frame
void ATransitionVolume::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ATransitionVolume::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor)
	{
		AMain* Main = Cast<AMain>(OtherActor);
		if (Main)
		{
			Main->SwitchLevel(TransitionLevelName);
		}
	}
}