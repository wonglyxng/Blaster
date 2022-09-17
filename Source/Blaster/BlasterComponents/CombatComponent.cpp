// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatComponent.h"

#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/Weapon/Weapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
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

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	FHitResult TraceHitResult;
	TraceUnderCrossHairs(TraceHitResult);
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, bAiming);
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
	if (bFireButtonPressed)
	{
		ServerFire();
	}
}

void UCombatComponent::TraceUnderCrossHairs(FHitResult& TraceHitResult)
{
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	// 十字准星位置在屏幕中心
	FVector2D CrossHairLocation(ViewportSize.X/2.f, ViewportSize.Y/2.f);
	
	FVector CrossHairWorldPosition;
	FVector CrossHairWorldDirection;
	// 将2d屏幕空间位置转换为3d世界空间位置
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		// 获取PlayerController对象
		UGameplayStatics::GetPlayerController(this, 0),
		CrossHairLocation,
		CrossHairWorldPosition,
		CrossHairWorldDirection
		);

	if (bScreenToWorld)
	{
		// 指定射线检测的射线起点和终点
		const FVector Start = CrossHairWorldPosition;
		const FVector End = Start + CrossHairWorldDirection * TRACE_LENGTH;

		// 射线检测，命中一个可见对象即返回碰撞信息
		GetWorld()->LineTraceSingleByChannel(TraceHitResult,Start,End,ECollisionChannel::ECC_Visibility);
		if (!TraceHitResult.bBlockingHit)
		{
			TraceHitResult.ImpactPoint = End;
		}
		else
		{
			DrawDebugSphere(GetWorld(),TraceHitResult.ImpactPoint,12.f,12,FColor::Red);
		}
	}
}

void UCombatComponent::ServerFire_Implementation()
{
	MulticastFire();
}

void UCombatComponent::MulticastFire_Implementation()
{
	if (EquippedWeapon == nullptr) return;
	if (Character)
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

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	// ENetRole LocalNetRole = WeaponToEquip->GetLocalRole();
	// switch (LocalNetRole)
	// {
	// case ENetRole::ROLE_Authority:
	// 	GEngine->AddOnScreenDebugMessage(-1, 60.f, FColor::Red, TEXT("UCombatComponent::EquipWeapon"));
	// 	break;
	// case ENetRole::ROLE_AutonomousProxy:
	// 	GEngine->AddOnScreenDebugMessage(-1, 60.f, FColor::Green, TEXT("UCombatComponent::EquipWeapon"));
	// 	break;
	// case ENetRole::ROLE_SimulatedProxy:
	// 	GEngine->AddOnScreenDebugMessage(-1, 60.f, FColor::Yellow, TEXT("UCombatComponent::EquipWeapon"));
	// 	break;
	// case ENetRole::ROLE_None:
	// 	GEngine->AddOnScreenDebugMessage(-1, 60.f, FColor::Blue, TEXT("UCombatComponent::EquipWeapon"));
	// 	break;
	// }
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

