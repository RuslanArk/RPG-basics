// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TransitionVolume.generated.h"

UCLASS()
class SOMERPG_API ATransitionVolume : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATransitionVolume();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Transition")
	class UBoxComponent* TransitionVolume;

	class UBillboardComponent* Billboard;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition")
	FName TransitionLevelName;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION()
	virtual void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

};
