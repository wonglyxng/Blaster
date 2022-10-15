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

		float SpreadScaled = CrosshairSpreadMax * HudPackage.CrosshairSpread;
		
		// 绘制十字准星中心
		if (HudPackage.CrosshairsCenter)
		{
			FVector2D Spread(0.f, 0.f);
			DrawCrosshair(HudPackage.CrosshairsCenter, ViewportCenter, Spread, HudPackage.CrosshairColor);
		}
		// 绘制十字准星左部
		if (HudPackage.CrosshairsLeft)
		{
			FVector2D Spread(-SpreadScaled, 0.f);
			DrawCrosshair(HudPackage.CrosshairsLeft, ViewportCenter, Spread, HudPackage.CrosshairColor);
		}
		// 绘制十字准星右部
		if (HudPackage.CrosshairsRight)
		{
			FVector2D Spread(SpreadScaled, 0.f);
			DrawCrosshair(HudPackage.CrosshairsRight, ViewportCenter, Spread, HudPackage.CrosshairColor);
		}
		// 绘制十字准星上部
		if (HudPackage.CrosshairsTop)
		{
			FVector2D Spread(0.f, -SpreadScaled);
			DrawCrosshair(HudPackage.CrosshairsTop, ViewportCenter, Spread, HudPackage.CrosshairColor);
		}
		// 绘制十字准星上部
		if (HudPackage.CrosshairsBottom)
		{
			FVector2D Spread(0.f, SpreadScaled);
			DrawCrosshair(HudPackage.CrosshairsBottom, ViewportCenter, Spread, HudPackage.CrosshairColor);
		}
	}
}

void ABlasterHUD::DrawCrosshair(UTexture2D* Texture, FVector2D ViewportCenter, FVector2D Spread, FLinearColor Color)
{
	const float TextureWidth = Texture->GetSizeX();
	const float TextureHeight = Texture->GetSizeY();
	
	const FVector2D TextureDrawPoint(
		ViewportCenter.X - TextureWidth / 2.f + Spread.X,
		ViewportCenter.Y - TextureHeight / 2.f + Spread.Y
		);

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
		Color
		);
}
