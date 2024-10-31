// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapons/LyraGameplayAbility_MeleeWeapon.h"
#include "Weapons/LyraMeleeWeaponInstance.h"
#include "NativeGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "LyraLogChannels.h"
#include "Character/LyraCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Teams/LyraTeamSubsystem.h"
#include "Weapons/LyraWeaponStateComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/LyraGameplayAbilityTargetData_SingleTargetHit.h"
#include "DrawDebugHelpers.h"

ULyraGameplayAbility_MeleeWeapon::ULyraGameplayAbility_MeleeWeapon(const FObjectInitializer& ObjectInitializer)
{
    
}

ULyraMeleeWeaponInstance* ULyraGameplayAbility_MeleeWeapon::GetWeaponInstance() const
{
    return Cast<ULyraMeleeWeaponInstance>(GetAssociatedEquipment());
}

bool ULyraGameplayAbility_MeleeWeapon::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
    bool bResult = Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);

	if (bResult)
	{
		if (GetWeaponInstance() == nullptr)
		{
			UE_LOG(LogLyraAbilitySystem, Error, TEXT("Weapon ability %s cannot be activated because there is no associated ranged weapon (equipment instance=%s but needs to be derived from %s)"),
				*GetPathName(),
				*GetPathNameSafe(GetAssociatedEquipment()),
				*ULyraMeleeWeaponInstance::StaticClass()->GetName());
			bResult = false;
		}
	}

	return bResult;
}

void ULyraGameplayAbility_MeleeWeapon::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) {
	// Bind target data callback
	UAbilitySystemComponent* MyAbilityComponent = CurrentActorInfo->AbilitySystemComponent.Get();
	check(MyAbilityComponent);

	OnTargetDataReadyCallbackDelegateHandle = MyAbilityComponent->AbilityTargetDataSetDelegate(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey()).AddUObject(this, &ThisClass::OnTargetDataReadyCallback);

	// Update the last firing time
	ULyraMeleeWeaponInstance* WeaponData = GetWeaponInstance();
	check(WeaponData);
	WeaponData->UpdateFiringTime();

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void ULyraGameplayAbility_MeleeWeapon::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (IsEndAbilityValid(Handle, ActorInfo))
	{
		if (ScopeLockCount > 0)
		{
			WaitingToExecute.Add(FPostLockDelegate::CreateUObject(this, &ThisClass::EndAbility, Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled));
			return;
		}

		UAbilitySystemComponent* MyAbilityComponent = CurrentActorInfo->AbilitySystemComponent.Get();
		check(MyAbilityComponent);

		// When ability ends, consume target data and remove delegate
		MyAbilityComponent->AbilityTargetDataSetDelegate(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey()).Remove(OnTargetDataReadyCallbackDelegateHandle);
		MyAbilityComponent->ConsumeClientReplicatedTargetData(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey());

		Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	}
}

void ULyraGameplayAbility_MeleeWeapon::OnTargetDataReadyCallback(const FGameplayAbilityTargetDataHandle& InData, FGameplayTag ApplicationTag)
{
	UAbilitySystemComponent* abilityComponent = CurrentActorInfo->AbilitySystemComponent.Get();
	check(abilityComponent);

	if (const FGameplayAbilitySpec* abilitySpec = abilityComponent->FindAbilitySpecFromHandle(CurrentSpecHandle))
	{
		FScopedPredictionWindow	ScopedPrediction(abilityComponent);

		// Take ownership of the target data to make sure no callbacks into game code invalidate it out from under us
		FGameplayAbilityTargetDataHandle LocalTargetDataHandle(MoveTemp(const_cast<FGameplayAbilityTargetDataHandle&>(InData)));

		const bool bShouldNotifyServer = CurrentActorInfo->IsLocallyControlled() && !CurrentActorInfo->IsNetAuthority();
		if (bShouldNotifyServer)
		{
			abilityComponent->CallServerSetReplicatedTargetData(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey(), LocalTargetDataHandle, ApplicationTag, abilityComponent->ScopedPredictionKey);
		}

		const bool bIsTargetDataValid = true;

		bool bProjectileWeapon = false;

#if WITH_SERVER_CODE
		if (!bProjectileWeapon)
		{
			if (AController* Controller = GetControllerFromActorInfo())
			{
				if (Controller->GetLocalRole() == ROLE_Authority)
				{
					// Confirm hit markers
					if (ULyraWeaponStateComponent* WeaponStateComponent = Controller->FindComponentByClass<ULyraWeaponStateComponent>())
					{
						TArray<uint8> HitReplaces;
						for (uint8 i = 0; (i < LocalTargetDataHandle.Num()) && (i < 255); ++i)
						{
							if (FGameplayAbilityTargetData_SingleTargetHit* SingleTargetHit = static_cast<FGameplayAbilityTargetData_SingleTargetHit*>(LocalTargetDataHandle.Get(i)))
							{
								if (SingleTargetHit->bHitReplaced)
								{
									HitReplaces.Add(i);
								}
							}
						}

						WeaponStateComponent->ClientConfirmTargetData(LocalTargetDataHandle.UniqueId, bIsTargetDataValid, HitReplaces);
					}

				}
			}
		}
#endif //WITH_SERVER_CODE


		// See if we still have ammo
		if (bIsTargetDataValid && CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
		{

			// Let the blueprint do stuff like apply effects to the targets
			OnMeleeWeaponTargetDataReady(LocalTargetDataHandle);
		}
		else
		{
			UE_LOG(LogLyraAbilitySystem, Warning, TEXT("Weapon ability %s failed to commit (bIsTargetDataValid=%d)"), *GetPathName(), bIsTargetDataValid ? 1 : 0);
			K2_EndAbility();
		}
	}

	// We've processed the data
	abilityComponent->ConsumeClientReplicatedTargetData(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey());
}

void ULyraGameplayAbility_MeleeWeapon::DoHitDetection(OUT TArray<FHitResult>& outHits)
{
	APawn* const pawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	ULyraMeleeWeaponInstance* weapon = GetWeaponInstance();

	// Is everything valid and locally controlled?
	if (pawn && pawn->IsLocallyControlled() && weapon) {
		ALyraCharacter* character = GetLyraCharacterFromActorInfo();
		FVector start, end;
		// Values used in standard melee capsule trace
		FCollisionShape capsule = FCollisionShape::MakeCapsule(weapon->GetMaxHitRange()/2.f, 48);

		// Get pos of bat (in the right hand)
		start = character->GetMesh()->GetSocketLocation(FName("hand_r")) + (character->GetActorForwardVector() * (weapon->GetMaxHitRange()));
		end = (character->GetActorForwardVector() * weapon->GetMaxHitRange()) + start;

		//UE_LOG(LogLyraAbilitySystem, Warning, TEXT("Range %f"), weapon->GetMaxHitRange());

		// Only check for pawns
		if (GetWorld()->SweepMultiByChannel(outHits, start, end, FQuat::Identity, ECollisionChannel::ECC_Pawn, capsule)) {
			for (FHitResult& hitResult : outHits) {
				AActor* actor = hitResult.GetActor();

				// Check the object is mobile and a pawn (very likely it's a character if true, need to test further though) 
				if (actor->GetRootComponent()->Mobility == EComponentMobility::Movable && Cast<APawn>(actor)) {
					ULyraTeamSubsystem* sys = GetWorld()->GetSubsystem<ULyraTeamSubsystem>();
					if (sys->CompareTeams(actor, GetLyraCharacterFromActorInfo()) == ELyraTeamComparison::DifferentTeams) {
						// The SkeletalMeshComponent is at index 1, theres most likely an eaiser way of finding this but I could'nt find any
						ALyraCharacter* enemy  = Cast<ALyraCharacter>(actor);
						if (enemy) {
							//UE_LOG(LogLyra, Warning, TEXT("Casted"));
							enemy->AddImpulse(character->GetActorForwardVector() * weapon->GetHitForce());
						}
						//UE_LOG(LogLyraAbilitySystem, Warning, TEXT("Hit %s"), *actor->GetName());
					}
				}

			}
		}
		//DrawDebugCapsule(GetWorld(), start, 48, weapon->GetMaxHitRange(), FQuat::Identity, FColor::Red);
	}
}

void ULyraGameplayAbility_MeleeWeapon::StartAttackCheck()
{
	check(CurrentActorInfo);

	AActor* avatarActor = CurrentActorInfo->AvatarActor.Get();
	check(avatarActor);

	AController* controller = GetControllerFromActorInfo();
	check(controller);
	ULyraWeaponStateComponent* weaponStateComponent = controller->FindComponentByClass<ULyraWeaponStateComponent>();

	TArray<FHitResult> foundHits;

	DoHitDetection(foundHits);

	// Create target data to pass into the blueprint
	FGameplayAbilityTargetDataHandle targetData;
	targetData.UniqueId = weaponStateComponent ? weaponStateComponent->GetUnconfirmedServerSideHitMarkerCount() : 0;
	
	if (foundHits.Num() > 0) {
		for (const FHitResult& found : foundHits) {
			FLyraGameplayAbilityTargetData_SingleTargetHit* target = new FLyraGameplayAbilityTargetData_SingleTargetHit();
			target->HitResult = found;
			target->CartridgeID = 0;

			targetData.Add(target);
		}
	}

	weaponStateComponent->AddUnconfirmedServerSideHitMarkers(targetData, foundHits);

	OnTargetDataReadyCallback(targetData, FGameplayTag());
}
