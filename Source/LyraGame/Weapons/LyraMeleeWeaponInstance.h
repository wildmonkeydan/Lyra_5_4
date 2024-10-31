// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

//#include "CoreMinimal.h"
#include "LyraWeaponInstance.h"
#include "AbilitySystem/LyraAbilitySourceInterface.h"
#include "Weapons/LyraWeaponInstance.h"
#include "LyraMeleeWeaponInstance.generated.h"

/**
 * ULyraMeleeWeaponInstance
 * 
 * A peice of equipment representing a melee weapon spawned and applied to a pawn. Built off of ULyraRangedWeaponInstance
 */
UCLASS()
class LYRAGAME_API ULyraMeleeWeaponInstance : public ULyraWeaponInstance, public ILyraAbilitySourceInterface
{
	GENERATED_BODY()

public:
	ULyraMeleeWeaponInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	virtual void PostLoad() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;

	void UpdateDebugVisualization();
#endif

protected:

	// Max distance another player can be hit by the weapon
	UPROPERTY(EditAnywhere, Category="Weapon Config")
	float MaxHitRange = 125.f;

	// Max distance the player can lunge toward the enemy
	UPROPERTY(EditAnywhere, Category="Weapon Config")
	float MaxLungeRange = 1000.f;

	// Force at which the player lunges
	UPROPERTY(EditAnywhere, Category="Weapon Config")
	float LungeForce = 50.f;

	// Force which will be applied to the enemy on hit
	UPROPERTY(EditAnywhere, Category="Weapon Config")
	float HitForce = 10000.f;

	// List of special tags that affect how damage is dealt
	// These tags will be compared to tags in the physical material of the thing being hit
	// If more than one tag is present, the multipliers will be combined multiplicatively
	UPROPERTY(EditAnywhere, Category = "Weapon Config")
	TMap<FGameplayTag, float> MaterialDamageMultiplier;
private:
	// Time since weapon was last used in world time
	float LastUseTime = 0.f; 

public:
	void Tick(float DeltaSeconds);

	//~ULyraEquipmentInstance interface
	virtual void OnEquipped();
	virtual void OnUnequipped();
	//~End of ULyraEquipmentInstance interface

	float GetMaxHitRange() {
		return MaxHitRange;
	}

	float GetMaxLungeRange() {
		return MaxLungeRange;
	}

	float GetHitForce() {
		return HitForce;
	}

	//~ILyraAbilitySourceInterface interface
	virtual float GetDistanceAttenuation(float Distance, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr) const override;
	virtual float GetPhysicalMaterialAttenuation(const UPhysicalMaterial* PhysicalMaterial, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr) const override;
	//~End of ILyraAbilitySourceInterface interface
};
