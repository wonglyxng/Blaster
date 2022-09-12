// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterCharacter.h"

#include "Blaster/BlasterComponents/CombatComponent.h"
#include "Blaster/Weapon/Weapon.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"


ABlasterCharacter::ABlasterCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->TargetArmLength = 600.f;
	// 是否使Pawn控制旋转
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	// 是否使Pawn控制旋转
	FollowCamera->bUsePawnControlRotation = false;

	// 是否使用控制器旋转偏置控制角色转向
	bUseControllerRotationYaw = false;
	// 是否随运动自动转向
	GetCharacterMovement()->bOrientRotationToMovement = true;

	OverHeadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverHeadWidget"));
	OverHeadWidget->SetupAttachment(RootComponent);
	
	CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	CombatComponent->SetIsReplicated(true);

	// 是否能下蹲，蓝图默认true
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;

	// 忽略胶囊体和摄像机碰撞，避免两个角色靠近时镜头缩放
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	// 忽略网格体和摄像机碰撞，避免两个角色靠近时镜头缩放
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	
	// 调整转身速度，防止移动一段距离以后才完成转身
	GetCharacterMovement()->RotationRate = FRotator(0.f,0.f, 800.f);

	// 默认不播放原地转身动画
	TurningInSpaceState = ETurningInSpaceState::ETIS_NotTurning;

	// 网络刷新频率
	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;
	
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

void ABlasterCharacter::PlayFireMontage(bool bAiming)
{
	if (CombatComponent == nullptr || CombatComponent->EquippedWeapon == nullptr)
	{
		return;
	}

	// 获取当前动画实例，通过动画实例播放蒙太奇
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && FireWeaponMontage)
	{
		// 播放蒙太奇
		AnimInstance->Montage_Play(FireWeaponMontage);
		// 获取蒙太奇指定段名称
		const FName SectionName = bAiming ? FName(TEXT("RifleAim")) : FName(TEXT("RifleHip"));
		// 跳到指定段
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();

	// ENetRole LocalNetRole = GetLocalRole();
	// switch (LocalNetRole)
	// {
	// case ENetRole::ROLE_Authority:
	// 	GEngine->AddOnScreenDebugMessage(-1, 60.f, FColor::Red, TEXT("ABlasterCharacter::BeginPlay"));
	// 	break;
	// case ENetRole::ROLE_AutonomousProxy:
	// 	GEngine->AddOnScreenDebugMessage(-1, 60.f, FColor::Green, TEXT("ABlasterCharacter::BeginPlay"));
	// 	break;
	// case ENetRole::ROLE_SimulatedProxy:
	// 	GEngine->AddOnScreenDebugMessage(-1, 60.f, FColor::Yellow, TEXT("ABlasterCharacter::BeginPlay"));
	// 	break;
	// case ENetRole::ROLE_None:
	// 	GEngine->AddOnScreenDebugMessage(-1, 60.f, FColor::Blue, TEXT("ABlasterCharacter::BeginPlay"));
	// 	break;
	// }
}

void ABlasterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 每帧计算aim offset
	AimOffset(DeltaTime);
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

	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ABlasterCharacter::FireButtonPressed);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &ABlasterCharacter::FireButtonReleased);
	
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
	// ENetRole LocalNetRole = GetLocalRole();
	// switch (LocalNetRole)
	// {
	// case ENetRole::ROLE_Authority:
	// 	GEngine->AddOnScreenDebugMessage(-1, 60.f, FColor::Red, TEXT("ABlasterCharacter::EquipButtonPressed"));
	// 	break;
	// case ENetRole::ROLE_AutonomousProxy:
	// 	GEngine->AddOnScreenDebugMessage(-1, 60.f, FColor::Green, TEXT("ABlasterCharacter::EquipButtonPressed"));
	// 	break;
	// case ENetRole::ROLE_SimulatedProxy:
	// 	GEngine->AddOnScreenDebugMessage(-1, 60.f, FColor::Yellow, TEXT("ABlasterCharacter::EquipButtonPressed"));
	// 	break;
	// case ENetRole::ROLE_None:
	// 	GEngine->AddOnScreenDebugMessage(-1, 60.f, FColor::Blue, TEXT("ABlasterCharacter::EquipButtonPressed"));
	// 	break;
	// }
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

void ABlasterCharacter::AimOffset(float DeltaTime)
{
	// 如果没有装备武器，则不用计算瞄准偏移量直接返回
	if (!IsEquippedWeapon())
	{
		return;
	}

	FVector Velocity = GetVelocity();
	Velocity.Z = 0.f;
	const float Speed = Velocity.Size();
	const bool bIsInAir = GetCharacterMovement()->IsFalling();
	
	// 只有在不移动，不跳起的时候才计算瞄准偏移量
	if (Speed == 0.f && !bIsInAir)
	{
		const FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		const FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation);
		AO_Yaw = DeltaAimRotation.Yaw;
		// 如果不需要转动的时候，InterpAO_Yaw简单的等于AO_Yaw即可
		if (TurningInSpaceState == ETurningInSpaceState::ETIS_NotTurning)
		{
			InterpAO_Yaw = AO_Yaw;
		}
		// 使用控制器旋转偏置控制角色转向
		bUseControllerRotationYaw = true;
		// 原地转动
		TurningInSpace(DeltaTime);
	}
	// 移动或跳起不计算瞄准偏移量
	if (Speed > 0.f || bIsInAir)
	{
		StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		AO_Yaw = 0.f;
		bUseControllerRotationYaw = true;
		// 取消原地转动动画
		TurningInSpaceState = ETurningInSpaceState::ETIS_NotTurning;
	}
	// 计算瞄准俯仰量
	AO_Pitch = GetBaseAimRotation().Pitch;
	if(AO_Pitch > 90.f && !IsLocallyControlled())
	{
		// 将俯仰量[270,360)映射到[-90,0)
		const FVector2D InRange(270.f, 360.f);
		const FVector2D OutRange(-90.f, 0.f);
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	}
}

void ABlasterCharacter::Jump()
{
	if (bIsCrouched)
	{
		UnCrouch();
		return;
	}
	Super::Jump();
}

void ABlasterCharacter::FireButtonPressed()
{
	if (CombatComponent)
	{
		CombatComponent->FireButtonPressed(true);
	}
}

void ABlasterCharacter::FireButtonReleased()
{
	if (CombatComponent)
	{
		CombatComponent->FireButtonPressed(false);
	}
}

void ABlasterCharacter::TurningInSpace(float DeltaTime)
{
	//UE_LOG(LogTemp, Warning, TEXT("AO_Yaw: %f"), AO_Yaw);
	if (AO_Yaw > 90.f)
	{
		// 执行原地右转动画
		TurningInSpaceState = ETurningInSpaceState::ETIS_Right;
	}
	else if (AO_Yaw < -90.f)
	{
		// 执行原地左转动画
		TurningInSpaceState = ETurningInSpaceState::ETIS_Left;
	}
	// 如果需要转动动画的时候，将InterpAO_Yaw通过插值的方式归零
	if (TurningInSpaceState != ETurningInSpaceState::ETIS_NotTurning)
	{
		InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw, 0.f, DeltaTime, 4.f);
		//UE_LOG(LogTemp, Warning, TEXT("InterpAO_Yaw: %f"), InterpAO_Yaw);
		AO_Yaw = InterpAO_Yaw;
		if (FMath::Abs(AO_Yaw) < 15.f)
		{
			TurningInSpaceState = ETurningInSpaceState::ETIS_NotTurning;
			StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		}
	}
}

void ABlasterCharacter::ServerEquipButtonPressed_Implementation()
{
	// ENetRole LocalNetRole = GetLocalRole();
	// switch (LocalNetRole)
	// {
	// case ENetRole::ROLE_Authority:
	// 	GEngine->AddOnScreenDebugMessage(-1, 60.f, FColor::Red, TEXT("ABlasterCharacter::ServerEquipButtonPressed_Implementation"));
	// 	break;
	// case ENetRole::ROLE_AutonomousProxy:
	// 	GEngine->AddOnScreenDebugMessage(-1, 60.f, FColor::Green, TEXT("ABlasterCharacter::ServerEquipButtonPressed_Implementation"));
	// 	break;
	// case ENetRole::ROLE_SimulatedProxy:
	// 	GEngine->AddOnScreenDebugMessage(-1, 60.f, FColor::Yellow, TEXT("ABlasterCharacter::ServerEquipButtonPressed_Implementation"));
	// 	break;
	// case ENetRole::ROLE_None:
	// 	GEngine->AddOnScreenDebugMessage(-1, 60.f, FColor::Blue, TEXT("ABlasterCharacter::ServerEquipButtonPressed_Implementation"));
	// 	break;
	// }
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

AWeapon* ABlasterCharacter::GetEquippedWeapon()
{
	if (CombatComponent == nullptr)
	{
		return nullptr;
	}
	return CombatComponent->EquippedWeapon;
}

