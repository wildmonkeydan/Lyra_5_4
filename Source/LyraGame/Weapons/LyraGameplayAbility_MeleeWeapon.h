// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Equipment/LyraGameplayAbility_FromEquipment.h"
#include "LyraGameplayAbility_MeleeWeapon.generated.h"

class ULyraMeleeWeaponInstance;
class APawn;
class UObject;
struct FCollisionQueryParams;
struct FFrame;
struct FGameplayAbilityActorInfo;
struct FGameplayEventData;
struct FGameplayTag;
struct FGameplayTagContainer;

/**
 * 
 */
UCLASS()
class ULyraGameplayAbility_MeleeWeapon : public ULyraGameplayAbility_FromEquipment
{
	GENERATED_BODY()
public:
	ULyraGameplayAbility_MeleeWeapon(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category="Lyra|Ability")
	ULyraMeleeWeaponInstance* GetWeaponInstance() const;

	//~UGameplayAbility interface
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	//~End of UGameplayAbility interface
protected:
	void OnTargetDataReadyCallback(const FGameplayAbilityTargetDataHandle& InData, FGameplayTag ApplicationTag);

	void DoHitDetection(OUT TArray<FHitResult>& outHits);

	// Call in the GameAbility blueprint when it is activated, starts to check for a hit
	UFUNCTION(BlueprintCallable)
	void StartAttackCheck();

	UFUNCTION(BlueprintImplementableEvent)
	void OnMeleeWeaponTargetDataReady(const FGameplayAbilityTargetDataHandle& TargetData);
private:
	FDelegateHandle OnTargetDataReadyCallbackDelegateHandle;
};