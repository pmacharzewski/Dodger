
#include "DodgerHUD.h"

#include "Engine/Canvas.h"

void ADodgerHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas)
	{
		return;
	}
	
	{ // Draw crosshair
		const FVector2D Center(Canvas->ClipX * 0.5f, Canvas->ClipY * 0.5f);
	
		constexpr float CrosshairSize = 10.0f;
		constexpr float CrosshairThickness = 2.0f;
		const FLinearColor CrosshairColor = FLinearColor::White;
	
		DrawLine(
			Center.X - CrosshairSize, 
			Center.Y, 
			Center.X + CrosshairSize, 
			Center.Y, 
			CrosshairColor, 
			CrosshairThickness
		);
	
		DrawLine(
			Center.X, 
			Center.Y - CrosshairSize, 
			Center.X, 
			Center.Y + CrosshairSize, 
			CrosshairColor, 
			CrosshairThickness
		);
	}
}
