// Fill out your copyright notice in the Description page of Project Settings.

#include "Main.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Enemy.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "ItemStorage.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "MainPlayerController.h"
#include "SomeRPGSaveGame.h"
#include "Sound/SoundCue.h"
#include "Weapon.h"

// Sets default values
AMain::AMain()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	// Set size for the collision capsule
	GetCapsuleComponent()->SetCapsuleSize(45, 105);

	// Create Camera Boom (pulls towards the player if there's a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetRootComponent());
	CameraBoom->TargetArmLength = 600.f;		// Camera follows at this distance
	CameraBoom->bUsePawnControlRotation = true; // Rotate Arm based on controller
	// Create Follow Camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	// Attach the camera to the end of the boom and let the boom adjust
	// to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false;

	// Get our turn rates for input
	BaseTurnRate = 65.f;
	BaseLookUpRate = 65.f;

	// Don't rotate, when controller rotates
	// let it just affect the camera
	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;			// Character moves in the direction of movement...
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.f, 0.0f); // ...at this rotation rate.
	GetCharacterMovement()->JumpZVelocity = 650.f;
	GetCharacterMovement()->AirControl = 0.2f;

	MaxHealth = 100.f;
	Health = 65.f;
	MaxStamina = 150.f;
	Stamina = 120.f;
	Coins = 0;

	RunningSpeed = 650.f;
	SprintingSpeed = 950.f;

	bShiftKeyPressed = false;
	bLMBPressed = false;
	bESCPressed = false;

	// set the enums' default values
	MovementStatus = EMovementStatus::EMS_Normal;
	StaminaStatus = EStaminaStatus::ESS_Normal;

	StaminaDrainRate = 25.f;
	MinSprintStamina = 50.f;

	bAttacking = false;

	InterpSpeed = 15.f;
	bInterpToEnemy = false;

	bHasCombatTarget = false;

	bMovingForward = false;
	bMovingRight = false;
}

// Called when the game starts or when spawned
void AMain::BeginPlay()
{
	Super::BeginPlay();
	
	MainPlayerController = Cast<AMainPlayerController>(GetController());

	FString Map = GetWorld()->GetMapName();
	Map.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);

	if (Map != "SunTemple")
	{
		LoadGameNoSwitch();

		if (MainPlayerController)
		{
			MainPlayerController->GameModeOnly();
		}
	}	
}

// Called every frame
void AMain::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (MovementStatus == EMovementStatus::EMS_Dead) return;

	float DeltaStamina = StaminaDrainRate * DeltaTime;

	switch (StaminaStatus)
	{
	case EStaminaStatus::ESS_Normal:
		if (bShiftKeyPressed)
		{
			if (bMovingForward || bMovingRight)
			{
				SetMovementStatus(EMovementStatus::EMS_Sprinting);
				Stamina -= DeltaStamina;
			}
			
			if (Stamina <= MinSprintStamina)
			{
				SetStaminaStatus(EStaminaStatus::ESS_BelowMinimum);
			}
		}
		else
		{
			SetMovementStatus(EMovementStatus::EMS_Normal);
			Stamina += DeltaStamina;
			if ((Stamina + DeltaStamina) >= MaxStamina)
			{
				Stamina = MaxStamina;
			}
		}
		break;
	case EStaminaStatus::ESS_BelowMinimum:
		if (bShiftKeyPressed)
		{
			if (bMovingForward || bMovingRight)
			{
				SetMovementStatus(EMovementStatus::EMS_Sprinting);
				Stamina -= DeltaStamina;
			}
			
			if ((Stamina - DeltaStamina) <= 0.f)
			{
				Stamina = 0.f;
				SetMovementStatus(EMovementStatus::EMS_Normal);
				SetStaminaStatus(EStaminaStatus::ESS_Exhausted);
			}
		}
		else // Shift key released
		{
			SetMovementStatus(EMovementStatus::EMS_Normal);
			Stamina += DeltaStamina;
			if (Stamina + DeltaTime >= MinSprintStamina)
			{
				SetStaminaStatus(EStaminaStatus::ESS_Normal);
			}
		}
		break;
	case EStaminaStatus::ESS_Exhausted:
		SetMovementStatus(EMovementStatus::EMS_Normal);
		if (bShiftKeyPressed)
		{
			
		}
		else
		{
			SetStaminaStatus(EStaminaStatus::ESS_ExhaustedRecovering);
		}
		break;
	case EStaminaStatus::ESS_ExhaustedRecovering:
		SetMovementStatus(EMovementStatus::EMS_Normal);
		Stamina += DeltaStamina;
		if ((Stamina + DeltaStamina) >= MinSprintStamina)
		{
			SetStaminaStatus(EStaminaStatus::ESS_Normal);
		}
		break;
	default:;
	}

	if (bInterpToEnemy && CombatTarget)
	{
		FRotator LookAtYaw = GetLookAtRotationYaw(CombatTarget->GetActorLocation());
		FRotator InterpRotation = FMath::RInterpTo(GetActorRotation(), LookAtYaw, DeltaTime, InterpSpeed);

		SetActorRotation(InterpRotation);
	}

	if (CombatTarget)
	{
		CombatTargetLocation = CombatTarget->GetActorLocation();
		if (MainPlayerController)
		{
			MainPlayerController->EnemyLocation = CombatTargetLocation;
		}
	}
}

FRotator AMain::GetLookAtRotationYaw(FVector Target)
{
	FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), Target);
	FRotator LookAtRotationYaw(0.f, LookAtRotation.Yaw, 0.f);
	return LookAtRotationYaw;
}

// Called to bind functionality to input
void AMain::SetupPlayerInputComponent(UInputComponent *PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	check(PlayerInputComponent);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AMain::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &AMain::ShiftKeyDown);
	PlayerInputComponent->BindAction("Sprint", IE_Released, this, &AMain::ShiftKeyUp);

	PlayerInputComponent->BindAction("LMB", IE_Pressed, this, &AMain::LMBDown);
	PlayerInputComponent->BindAction("LMB", IE_Released, this, &AMain::LMBUp);

	PlayerInputComponent->BindAction("ESC", IE_Pressed, this, &AMain::ESCDown);
	PlayerInputComponent->BindAction("ESC", IE_Released, this, &AMain::ESCUp);

	PlayerInputComponent->BindAxis("MoveForward", this, &AMain::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AMain::MoveRight);

	PlayerInputComponent->BindAxis("Turn", this, &AMain::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &AMain::LookUp);
	PlayerInputComponent->BindAxis("TurnRate", this, &AMain::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AMain::LookUpAtRate);
}

bool AMain::CanMove(float Value)
{
	if (MainPlayerController)
	{
		return (Controller != nullptr) &&
			(Value != 0.0f) &&
			(!bAttacking) &&
			(MovementStatus != EMovementStatus::EMS_Dead) &&
			(!MainPlayerController->bPauseMenuVisible);
	}

	return false;
}

void AMain::Turn(float Value)
{
	if (CanMove(Value))
	{
		AddControllerYawInput(Value);
	}
}

void AMain::LookUp(float Value)
{
	if (CanMove(Value))
	{
		AddControllerPitchInput(Value);
	}
}

void AMain::MoveForward(float Value)
{
	bMovingForward = false;
	if (CanMove(Value))
	{
		bMovingForward = true;
		// GetControlRotation - gives you the direction where controller is facing this frame
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);
		// FRotationMatrix is a method of getting a vector in the direction
		// based of a Rotator (sets X as forward direction for this particular rotator)
		// it accessing X,Y,Z axis for this particular Rotator we have (YawRotation)
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void AMain::MoveRight(float Value)
{
	bMovingRight = false;
	if (CanMove(Value))
	{
		bMovingRight = true;

		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);

		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		AddMovementInput(Direction, Value);
	}
}

void AMain::TurnAtRate(float Rate)
{
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AMain::LookUpAtRate(float Rate)
{
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AMain::DecrementHealth(float Amount)
{
	
}

void AMain::IncrementCoins(int32 Amount)
{
	Coins += Amount;
}

void AMain::IncrementHealth(float Amount)
{
	if (Health + Amount >= MaxHealth)
	{
		Health = MaxHealth;
	}
	else
	{
		Health += Amount;
	}
}

void AMain::Die()
{
	if (MovementStatus == EMovementStatus::EMS_Dead) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && CombatMontage)
	{
		AnimInstance->Montage_Play(CombatMontage, 1.0f);
		AnimInstance->Montage_JumpToSection(FName("Death"));
	}
	SetMovementStatus(EMovementStatus::EMS_Dead);	
}

void AMain::DeathEnd()
{
	GetMesh()->bPauseAnims = true;
	GetMesh()->bNoSkeletonUpdate = true;
}

void AMain::Jump()
{
	if (MainPlayerController)
		if (MainPlayerController->bPauseMenuVisible) return;

	if (MovementStatus != EMovementStatus::EMS_Dead)
	{
		Super::Jump();
	}
}

void AMain::SetMovementStatus(EMovementStatus Status)
{
	MovementStatus = Status;
	if (MovementStatus == EMovementStatus::EMS_Sprinting)
	{
		GetCharacterMovement()->MaxWalkSpeed = SprintingSpeed;
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed = RunningSpeed;
	}
}

void AMain::LMBDown()
{
	bLMBPressed = true;

	if (MovementStatus == EMovementStatus::EMS_Dead) return;

	if (MainPlayerController)
		if (MainPlayerController->bPauseMenuVisible) return;

	if (ActiveOverlapingItem)
	{
		AWeapon* Weapon = Cast<AWeapon>(ActiveOverlapingItem);

		if (Weapon)
		{
			Weapon->Equip(this);
			SetActiveOverlapingItem(nullptr);
		}
	}
	else if (EquippedWeapon)
	{
		Attack();
	}
}

void AMain::LMBUp()
{
	bLMBPressed = false;
}

void AMain::ESCDown()
{
	bESCPressed = true;

	if (MainPlayerController)
	{
		MainPlayerController->TogglePauseMenu();
	}
}

void AMain::ESCUp()
{
	bESCPressed = false;
}

void AMain::ShiftKeyDown()
{
	bShiftKeyPressed = true;
}

void AMain::ShiftKeyUp()
{
	bShiftKeyPressed = false;
}

void AMain::ShowPickupsLocations()
{
	for (FVector &Location : PickupsLocations)
	{
		UKismetSystemLibrary::DrawDebugSphere(this, Location, 25.f, 8, FLinearColor::Green, 10.f, 0.5f);
	}
}

void AMain::SetEquippedWeapon(AWeapon* WeaponToSet)
{
	if (EquippedWeapon)
	{
		EquippedWeapon->Destroy();
	}

	EquippedWeapon = WeaponToSet;
}

void AMain::Attack()
{
	if (!bAttacking && MovementStatus != EMovementStatus::EMS_Dead)
	{
		bAttacking = true;
		SetInterpToEnemy(true);

		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance && CombatMontage)
		{
			int32 Section = FMath::RandRange(0, 1);
			switch (Section)
			{
			case 0:
				AnimInstance->Montage_Play(CombatMontage, 2.f);
				AnimInstance->Montage_JumpToSection(FName("Attack_1"), CombatMontage);
				break;
			case 1:
				AnimInstance->Montage_Play(CombatMontage, 1.7f);
				AnimInstance->Montage_JumpToSection(FName("Attack_2"), CombatMontage);
				break;
			default:;
			}			
		}
	}	
}

void AMain::AttackEnd()
{
	bAttacking = false;
	SetInterpToEnemy(false);
	if (bLMBPressed)
	{
		Attack();
	}
}

void AMain::PlaySwingSound()
{
	if (EquippedWeapon->SwingSound)
	{
		UGameplayStatics::PlaySound2D(this, EquippedWeapon->SwingSound);
	}
}

void AMain::SetInterpToEnemy(bool Interp)
{
	bInterpToEnemy = Interp;
}

float AMain::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{	
	if (Health - DamageAmount <= 0.f)
	{
		Health -= DamageAmount;
		Die();

		if (DamageCauser)
		{
			AEnemy* Enemy = Cast<AEnemy>(DamageCauser);
			if (Enemy)
			{
				Enemy->bHasValidTarget = false;
			}			
		}
	}
	else
	{
		Health -= DamageAmount;
	}

	return DamageAmount;
}

void AMain::UpdateCombatTarget()
{
	TArray<AActor*> OverlappingActors;
	GetOverlappingActors(OverlappingActors, EnemyFilter);

	if (OverlappingActors.Num() == 0)
	{
		if (MainPlayerController)
		{
			MainPlayerController->RemoveEnemyHealthBar();
		}
		return;
	}

	AEnemy* ClosestEnemy = Cast<AEnemy>(OverlappingActors[0]);

	if (ClosestEnemy)
	{
		FVector MainLocation = GetActorLocation();
		float MinDistance = (ClosestEnemy->GetActorLocation() - MainLocation).Size();

		for (auto Actor : OverlappingActors)
		{
			AEnemy* Enemy = Cast<AEnemy>(Actor);
			if (Enemy)
			{
				float DistanceToActor = (Enemy->GetActorLocation() - MainLocation).Size();
				if (DistanceToActor < MinDistance)
				{
					MinDistance = DistanceToActor;
					ClosestEnemy = Enemy;
				}
			}
		}

		if (MainPlayerController)
		{
			MainPlayerController->DisplayEnemyHealthBar();
		}
		SetCombatTarget(ClosestEnemy);
		bHasCombatTarget = true;
	}
}

void AMain::SwitchLevel(FName NextLevel)
{
	UWorld* World = GetWorld();
	if (World)
	{
		FString CurrentLevel = World->GetMapName();
		FName CurrentLevelName(CurrentLevel); // maybe need to be derefferenced

		if (CurrentLevelName != NextLevel)
		{
			UGameplayStatics::OpenLevel(World, NextLevel);
		}
	}
}

void AMain::SaveGame()
{
	USomeRPGSaveGame* SaveGameInstance = Cast<USomeRPGSaveGame>(UGameplayStatics::CreateSaveGameObject(USomeRPGSaveGame::StaticClass()));

	SaveGameInstance->CharacterStats.Health = Health;
	SaveGameInstance->CharacterStats.MaxHealth = MaxHealth;
	SaveGameInstance->CharacterStats.Stamina = Stamina;
	SaveGameInstance->CharacterStats.MaxStamina = MaxStamina;
	SaveGameInstance->CharacterStats.Coins = Coins;

	FString MapName = GetWorld()->GetMapName();
	MapName.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);

	SaveGameInstance->CharacterStats.LevelName = MapName;

	if (EquippedWeapon)
	{
		SaveGameInstance->CharacterStats.WeaponName = EquippedWeapon->Name;
	}

	SaveGameInstance->CharacterStats.PlayerLocation = GetActorLocation();
	SaveGameInstance->CharacterStats.PlayerRotation = GetActorRotation();

	UGameplayStatics::SaveGameToSlot(SaveGameInstance, SaveGameInstance->PlayerName, SaveGameInstance->PlayerIndex);
}

void AMain::LoadGame(bool SetPosition)
{
	USomeRPGSaveGame* LoadGameInstance = Cast<USomeRPGSaveGame>(UGameplayStatics::CreateSaveGameObject(USomeRPGSaveGame::StaticClass()));

	LoadGameInstance = Cast<USomeRPGSaveGame>(UGameplayStatics::LoadGameFromSlot(LoadGameInstance->PlayerName, LoadGameInstance->PlayerIndex));

	Health = LoadGameInstance->CharacterStats.Health;
	MaxHealth = LoadGameInstance->CharacterStats.MaxHealth;
	Stamina = LoadGameInstance->CharacterStats.Stamina;
	MaxStamina = LoadGameInstance->CharacterStats.MaxStamina;
	Coins = LoadGameInstance->CharacterStats.Coins;

	if (WeaponStorage)
	{
		AItemStorage* Weapons = GetWorld()->SpawnActor<AItemStorage>(WeaponStorage);
		if (Weapons)
		{
			FString WeaponName = LoadGameInstance->CharacterStats.WeaponName;

			if (Weapons->WeaponMap.Contains(WeaponName))
			{
				AWeapon* WeaponToEquip = GetWorld()->SpawnActor<AWeapon>(Weapons->WeaponMap[WeaponName]);

				WeaponToEquip->Equip(this);
			}			
		}
	}

	if (SetPosition)
	{
		SetActorLocation(LoadGameInstance->CharacterStats.PlayerLocation);
		SetActorRotation(LoadGameInstance->CharacterStats.PlayerRotation);
	}

	SetMovementStatus(EMovementStatus::EMS_Normal);

	GetMesh()->bPauseAnims = false;
	GetMesh()->bNoSkeletonUpdate = false;

	if (LoadGameInstance->CharacterStats.LevelName != TEXT(""))
	{
		FName LevelName(*LoadGameInstance->CharacterStats.LevelName);

		SwitchLevel(LevelName);
	}
}

void AMain::LoadGameNoSwitch()
{
	USomeRPGSaveGame* LoadGameInstance = Cast<USomeRPGSaveGame>(UGameplayStatics::CreateSaveGameObject(USomeRPGSaveGame::StaticClass()));

	LoadGameInstance = Cast<USomeRPGSaveGame>(UGameplayStatics::LoadGameFromSlot(LoadGameInstance->PlayerName, LoadGameInstance->PlayerIndex));

	Health = LoadGameInstance->CharacterStats.Health;
	MaxHealth = LoadGameInstance->CharacterStats.MaxHealth;
	Stamina = LoadGameInstance->CharacterStats.Stamina;
	MaxStamina = LoadGameInstance->CharacterStats.MaxStamina;
	Coins = LoadGameInstance->CharacterStats.Coins;

	if (WeaponStorage)
	{
		AItemStorage* Weapons = GetWorld()->SpawnActor<AItemStorage>(WeaponStorage);
		if (Weapons)
		{
			FString WeaponName = LoadGameInstance->CharacterStats.WeaponName;

			if (Weapons->WeaponMap.Contains(WeaponName))
			{
				AWeapon* WeaponToEquip = GetWorld()->SpawnActor<AWeapon>(Weapons->WeaponMap[WeaponName]);

				WeaponToEquip->Equip(this);
			}
		}
	}

	SetMovementStatus(EMovementStatus::EMS_Normal);

	GetMesh()->bPauseAnims = false;
	GetMesh()->bNoSkeletonUpdate = false;
}