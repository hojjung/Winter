// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AI_Flee.h"
#include "AI_FleeJumper.generated.h"

/**
 * 
 */
UCLASS()
class WINTER2_API UAI_FleeJumper : public UAI_Flee
{
	GENERATED_BODY()

protected:
	float m_fJumpTimer;

protected:
	void Jump();

	virtual void OnFlee() override;
};

