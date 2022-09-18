// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileWeapon.h"

#include "Engine/SkeletalMeshSocket.h"
#include "Projectile.h"

void AProjectileWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
	if (MuzzleFlashSocket)
	{
		// 获取枪口插槽位置的转换，即在这个位置生成子弹
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		// 生成子弹的初始位置
		FVector TargetLocation = SocketTransform.GetLocation();
		
		// 从枪口插槽到命中目标的位置
		FVector ToTarget = HitTarget - SocketTransform.GetLocation();
		// 生成子弹的初始旋转角
		FRotator TargetRotation = ToTarget.Rotation();

		// 生成子弹的发起者
		APawn* InstigatorPawn = Cast<APawn>(GetOwner());
		if (ProjectileClass && InstigatorPawn)
		{
			FActorSpawnParameters SpawnParameters;
			SpawnParameters.Owner = GetOwner(); // 生成子弹的拥有者
			SpawnParameters.Instigator = InstigatorPawn; // 生成子弹的发起者
			
			UWorld* World = GetWorld();
			if (World)
			{
				World->SpawnActor<AProjectile>(
					ProjectileClass, // 生成的子弹类
					TargetLocation, // 生成子弹的初始位置
					TargetRotation, // 生成子弹的初始旋转角
					SpawnParameters
					);
			}
		}
	}
}
