// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterCharacter.h"

#include "Blaster/BlasterComponents/CombatComponent.h"
#include "Blaster/Weapon/Weapon.h"
#include "Camera/CameraComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Net/UnrealNetwork.h"


ABlasterCharacter::ABlasterCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->TargetArmLength = 600.f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	OverHeadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverHeadWidget"));
	OverHeadWidget->SetupAttachment(RootComponent);
	
	CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	CombatComponent->SetIsReplicated(true);
	
}

void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ABlasterCharacter, OverlappingWeapon, COND_OwnerOnly);
}

void ABlasterCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (CombatComponent)
	{
		CombatComponent->Character = this;
	}
	
}

void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();

	ENetRole LocalNetRole = GetLocalRole();
	switch (LocalNetRole)
	{
	case ENetRole::ROLE_Authority:
		GEngine->AddOnScreenDebugMessage(-1, 60.f, FColor::Red, TEXT("ABlasterCharacter::BeginPlay"));
		break;
	case ENetRole::ROLE_AutonomousProxy:
		GEngine->AddOnScreenDebugMessage(-1, 60.f, FColor::Green, TEXT("ABlasterCharacter::BeginPlay"));
		break;
	case ENetRole::ROLE_SimulatedProxy:
		GEngine->AddOnScreenDebugMessage(-1, 60.f, FColor::Yellow, TEXT("ABlasterCharacter::BeginPlay"));
		break;
	case ENetRole::ROLE_None:
		GEngine->AddOnScreenDebugMessage(-1, 60.f, FColor::Blue, TEXT("ABlasterCharacter::BeginPlay"));
		break;
	}
}

void ABlasterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ABlasterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ABlasterCharacter::Jump);

	PlayerInputComponent->BindAxis("MoveForward", this, &ABlasterCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ABlasterCharacter::MoveRight);
	PlayerInputComponent->BindAxis("Turn", this, &ABlasterCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &ABlasterCharacter::LookUp);

	PlayerInputComponent->BindAction("Equip", IE_Pressed, this, &ABlasterCharacter::EquipButtonPressed);
	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &ABlasterCharacter::CrouchButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Pressed, this, &ABlasterCharacter::AimButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Released, this, &ABlasterCharacter::AimButtonReleased);
	
}

void ABlasterCharacter::MoveForward(float Value)
{
	if (Controller != nullptr && Value != 0.f)
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X));
		AddMovementInput(Direction, Value);
	}
}

void ABlasterCharacter::MoveRight(float Value)
{
	if (Controller != nullptr && Value != 0.f)
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y));
		AddMovementInput(Direction, Value);
	}
}

void ABlasterCharacter::Turn(float Value)
{
	AddControllerYawInput(Value);
}

void ABlasterCharacter::LookUp(float Value)
{
	AddControllerPitchInput(Value);
}

void ABlasterCharacter::EquipButtonPressed()
{
	ENetRole LocalNetRole = GetLocalRole();
	switch (LocalNetRole)
	{
	case ENetRole::ROLE_Authority:
		GEngine->AddOnScreenDebugMessage(-1, 60.f, FColor::Red, TEXT("ABlasterCharacter::EquipButtonPressed"));
		break;
	case ENetRole::ROLE_AutonomousProxy:
		GEngine->AddOnScreenDebugMessage(-1, 60.f, FColor::Green, TEXT("ABlasterCharacter::EquipButtonPressed"));
		break;
	case ENetRole::ROLE_SimulatedProxy:
		GEngine->AddOnScreenDebugMessage(-1, 60.f, FColor::Yellow, TEXT("ABlasterCharacter::EquipButtonPressed"));
		break;
	case ENetRole::ROLE_None:
		GEngine->AddOnScreenDebugMessage(-1, 60.f, FColor::Blue, TEXT("ABlasterCharacter::EquipButtonPressed"));
		break;
	}
	if (CombatComponent && OverlappingWeapon)
	{
		if (HasAuthority())
		{
			CombatComponent->EquipWeapon(OverlappingWeapon);
		}
		else
		{
			ServerEquipButtonPressed();
		}
	}
}

void ABlasterCharacter::CrouchButtonPressed()
{
	if (bIsCrouched)
	{
		UnCrouch();
		return;
	}
	Crouch();
}

void ABlasterCharacter::AimButtonPressed()
{
	if (CombatComponent)
	{
		CombatComponent->SetAiming(true);
	}
}

void ABlasterCharacter::AimButtonReleased()
{
	if (CombatComponent)
	{
		CombatComponent->SetAiming(false);
	}
}

void ABlasterCharacter::ServerEquipButtonPressed_Implementation()
{
	ENetRole LocalNetRole = GetLocalRole();
	switch (LocalNetRole)
	{
	case ENetRole::ROLE_Authority:
		GEngine->AddOnScreenDebugMessage(-1, 60.f, FColor::Red, TEXT("ABlasterCharacter::ServerEquipButtonPressed_Implementation"));
		break;
	case ENetRole::ROLE_AutonomousProxy:
		GEngine->AddOnScreenDebugMessage(-1, 60.f, FColor::Green, TEXT("ABlasterCharacter::ServerEquipButtonPressed_Implementation"));
		break;
	case ENetRole::ROLE_SimulatedProxy:
		GEngine->AddOnScreenDebugMessage(-1, 60.f, FColor::Yellow, TEXT("ABlasterCharacter::ServerEquipButtonPressed_Implementation"));
		break;
	case ENetRole::ROLE_None:
		GEngine->AddOnScreenDebugMessage(-1, 60.f, FColor::Blue, TEXT("ABlasterCharacter::ServerEquipButtonPressed_Implementation"));
		break;
	}
	if (CombatComponent)
	{
		CombatComponent->EquipWeapon(OverlappingWeapon);
	}
}

void ABlasterCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{
	//todo: 如果物品触发区域有重叠，将只能显示其中一个，其他的无法显示问题如何解决
	
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(true);
	}
	if (LastWeapon)
	{
		LastWeapon->ShowPickupWidget(false);
	}
}

void ABlasterCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	if (IsLocallyControlled() && OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(false);
	}
	OverlappingWeapon = Weapon;
	if (IsLocallyControlled() && OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(true);
	}
}

bool ABlasterCharacter::IsEquippedWeapon()
{
	return CombatComponent && CombatComponent->EquippedWeapon;
}

bool ABlasterCharacter::IsAiming()
{
	return CombatComponent && CombatComponent->bAiming;
}

