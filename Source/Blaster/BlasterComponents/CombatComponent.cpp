// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatComponent.h"

#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/Weapon/Weapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	BaseWalkSpeed = 600.f;
	AimWalkSpeed = 460.f;
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	if (Character)
	{
		// 设置移动最大速度
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
	}
}

void UCombatComponent::SetAiming(bool bIsAiming)
{
	bAiming = bIsAiming; //服务器执行会有延迟，所以客户端先模拟，然后服务器再覆盖，不会有延迟感觉
	ServerSetAiming(bIsAiming);
	if (Character)
	{
		// 设置移动最大速度
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	if (EquippedWeapon && Character)
	{
		// 设置不随运动自动转向
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		// 设置使用控制器旋转偏置控制角色转向
		Character->bUseControllerRotationYaw = true;
	}
}

void UCombatComponent::FireButtonPressed(bool bPressed)
{
	bFireButtonPressed = bPressed;
	if (EquippedWeapon == nullptr)
	{
		return;
	}
	if (Character && bFireButtonPressed)
	{
		// 播放射击蒙太奇
		Character->PlayFireMontage(bAiming);
		// 调用武器射击接口
		EquippedWeapon->Fire();
	}
}

void UCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)
{
	bAiming = bIsAiming;
	if (Character)
	{
		// 设置服务端移动最大速度
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, bAiming);
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	ENetRole LocalNetRole = WeaponToEquip->GetLocalRole();
	switch (LocalNetRole)
	{
	case ENetRole::ROLE_Authority:
		GEngine->AddOnScreenDebugMessage(-1, 60.f, FColor::Red, TEXT("UCombatComponent::EquipWeapon"));
		break;
	case ENetRole::ROLE_AutonomousProxy:
		GEngine->AddOnScreenDebugMessage(-1, 60.f, FColor::Green, TEXT("UCombatComponent::EquipWeapon"));
		break;
	case ENetRole::ROLE_SimulatedProxy:
		GEngine->AddOnScreenDebugMessage(-1, 60.f, FColor::Yellow, TEXT("UCombatComponent::EquipWeapon"));
		break;
	case ENetRole::ROLE_None:
		GEngine->AddOnScreenDebugMessage(-1, 60.f, FColor::Blue, TEXT("UCombatComponent::EquipWeapon"));
		break;
	}
	if (Character == nullptr || WeaponToEquip == nullptr)
	{
		return;
	}

	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	EquippedWeapon->SetOwner(Character);
	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName(TEXT("RightHandSocket")));
	if (HandSocket)
	{
		HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());
	}
	//EquippedWeapon->ShowPickupWidget(false);
	//EquippedWeapon->GetAreaSphere()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 设置不随运动自动转向
	Character->GetCharacterMovement()->bOrientRotationToMovement = false;
	// 设置使用控制器旋转偏置控制角色转向
	Character->bUseControllerRotationYaw = true;
}

