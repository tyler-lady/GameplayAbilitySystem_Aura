// Copyright Druid Mechanics


#include "Actor/AuraEffectActor.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"

AAuraEffectActor::AAuraEffectActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>("SceneRoot"));
}

void AAuraEffectActor::BeginPlay()
{
	Super::BeginPlay();
	
}

void AAuraEffectActor::ApplyEffectToTarget(AActor* TargetActor, const FEffectType& EffectType)
{
	if (TargetActor->ActorHasTag(FName("Enemy")) && !bApplyEffectsToEnemies) return;
	
	UAbilitySystemComponent* TargetAbilitySystemComponent = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	if (!TargetAbilitySystemComponent) return;

	FGameplayEffectContextHandle EffectContextHandle = TargetAbilitySystemComponent->MakeEffectContext();
	EffectContextHandle.AddSourceObject(this);
	const FGameplayEffectSpecHandle EffectSpecHandle = TargetAbilitySystemComponent->MakeOutgoingSpec(EffectType.GameplayEffectClass,1.f, EffectContextHandle);
	const TSharedPtr<FActiveGameplayEffectHandle> ActiveEffectHandle = MakeShared<FActiveGameplayEffectHandle>(TargetAbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*EffectSpecHandle.Data));

	if ( EffectSpecHandle.Data->Def->DurationPolicy == EGameplayEffectDurationType::Infinite && EffectType.EffectRemovalPolicy == EEffectRemovalPolicy::RemoveOnEndOverlap )
	{
		ActiveEffectHandles.Add(ActiveEffectHandle, TargetAbilitySystemComponent);
	}
	else if (bDestroyOnEffectApplication && EffectSpecHandle.Data->Def->DurationPolicy == EGameplayEffectDurationType::Instant)
	{
		Destroy();
	} else if (EffectType.EffectRemovalPolicy != EEffectRemovalPolicy::RemoveOnEndOverlap) //This is make sure crystals are destroyed properly - may need to change
	{
		Destroy();
	}
}

void AAuraEffectActor::OnOverlap(AActor* TargetActor)
{
	if (TargetActor->ActorHasTag(FName("Enemy")) && !bApplyEffectsToEnemies) return;
	
	for ( auto& Effect : InstantEffects )
	{
		if ( Effect.EffectApplicationPolicy == EEffectApplicationPolicy::ApplyOnOverlap )
		{
			ApplyEffectToTarget(TargetActor, Effect);
		}
	}
}

void AAuraEffectActor::OnEndOverlap(AActor* TargetActor)
{
	if (TargetActor->ActorHasTag(FName("Enemy")) && !bApplyEffectsToEnemies) return;

	for ( auto& Effect : InstantEffects )
	{
		if ( Effect.EffectApplicationPolicy == EEffectApplicationPolicy::ApplyOnEndOverlap )
		{
			ApplyEffectToTarget(TargetActor, Effect);
		}
		else if ( Effect.EffectRemovalPolicy == EEffectRemovalPolicy::RemoveOnEndOverlap )
		{
			UAbilitySystemComponent* TargetAbilitySystemComponent = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
			if (!IsValid(TargetAbilitySystemComponent)) return;

			TArray<TSharedPtr<FActiveGameplayEffectHandle>> HandlesToRemove;

			for ( auto HandlePair : ActiveEffectHandles )
			{
				if ( TargetAbilitySystemComponent == HandlePair.Value )
				{
					TargetAbilitySystemComponent->RemoveActiveGameplayEffect(*HandlePair.Key);
					HandlesToRemove.Add(HandlePair.Key);
				}
			}
			for ( auto Handle : HandlesToRemove )
			{
				ActiveEffectHandles.FindAndRemoveChecked(Handle);
			}
		}
	}
}


