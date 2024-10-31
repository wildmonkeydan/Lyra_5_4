// Fill out your copyright notice in the Description page of Project Settings.


#include "LyraMeleeWeaponInstance.h"
#include "Physics/PhysicalMaterialWithTags.h"

ULyraMeleeWeaponInstance::ULyraMeleeWeaponInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void ULyraMeleeWeaponInstance::PostLoad()
{
	Super::PostLoad();
}

#if WITH_EDITOR
void ULyraMeleeWeaponInstance::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	UpdateDebugVisualization();
}

void ULyraMeleeWeaponInstance::UpdateDebugVisualization()
{
	// Will leave blank for now
}
#endif
void ULyraMeleeWeaponInstance::Tick(float DeltaSeconds)
{
	
}

void ULyraMeleeWeaponInstance::OnEquipped()
{
	Super::OnEquipped();
}

void ULyraMeleeWeaponInstance::OnUnequipped()
{
	Super::OnUnequipped();
}

float ULyraMeleeWeaponInstance::GetDistanceAttenuation(float Distance, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags) const
{
	return Distance < MaxHitRange ? Distance : MaxHitRange;
}

float ULyraMeleeWeaponInstance::GetPhysicalMaterialAttenuation(const UPhysicalMaterial* PhysicalMaterial, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags) const
{
	float CombinedMultiplier = 1.0f;
	if (const UPhysicalMaterialWithTags* PhysMatWithTags = Cast<const UPhysicalMaterialWithTags>(PhysicalMaterial))
	{
		for (const FGameplayTag MaterialTag : PhysMatWithTags->Tags)
		{
			if (const float* pTagMultiplier = MaterialDamageMultiplier.Find(MaterialTag))
			{
				CombinedMultiplier *= *pTagMultiplier;
			}
		}
	}

	return CombinedMultiplier;
}
