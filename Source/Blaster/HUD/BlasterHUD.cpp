// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterHUD.h"

void ABlasterHUD::DrawHUD()
{
	Super::DrawHUD();
	if (GEngine && GEngine->GameViewport)
	{
		// 获取屏幕尺寸
		FVector2D ViewportSize;
		GEngine->GameViewport->GetViewportSize(ViewportSize);
		// 取屏幕中心
		FVector2D ViewportCenter(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);
		
		// 绘制十字准星中心
		if (HudPackage.CrosshairsCenter)
		{
			DrawCrosshair(HudPackage.CrosshairsCenter, ViewportCenter);
		}
		// 绘制十字准星左部
		if (HudPackage.CrosshairsLeft)
		{
			DrawCrosshair(HudPackage.CrosshairsLeft, ViewportCenter);
		}
		// 绘制十字准星右部
		if (HudPackage.CrosshairsRight)
		{
			DrawCrosshair(HudPackage.CrosshairsRight, ViewportCenter);
		}
		// 绘制十字准星上部
		if (HudPackage.CrosshairsTop)
		{
			DrawCrosshair(HudPackage.CrosshairsTop, ViewportCenter);
		}
		// 绘制十字准星上部
		if (HudPackage.CrosshairsBottom)
		{
			DrawCrosshair(HudPackage.CrosshairsBottom, ViewportCenter);
		}
	}
}

void ABlasterHUD::DrawCrosshair(UTexture2D* Texture, FVector2D ViewportCenter)
{
	const float TextureWidth = Texture->GetSizeX();
	const float TextureHeight = Texture->GetSizeY();
	const FVector2D TextureDrawPoint(ViewportCenter.X - TextureWidth / 2.f, ViewportCenter.Y - TextureHeight / 2.f);

	DrawTexture(
		Texture,
		TextureDrawPoint.X,
		TextureDrawPoint.Y,
		TextureWidth,
		TextureHeight,
		0.f,
		0.f,
		1.f,
		1.f,
		FLinearColor::White
		);
}
