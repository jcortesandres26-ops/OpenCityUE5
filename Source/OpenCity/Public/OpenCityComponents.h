#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "OpenCityTypes.h"
#include "OpenCityComponents.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOpenCityHealthChanged, float, Health, float, Armor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOpenCityDeath, AActor*, Victim);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOpenCityWeaponChanged, FName, WeaponId, int32, Ammo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOpenCityDialogueChanged, FName, NodeId);

USTRUCT(BlueprintType)
struct FOpenCityInventoryStack
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FName ItemId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) int32 Quantity = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) float WeightEach = 0.f;
};

UCLASS(ClassGroup=(OpenCity), meta=(BlueprintSpawnableComponent))
class OPENCITY_API UOpenCityHealthComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UOpenCityHealthComponent();
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_Vitals) float MaxHealth = 100.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_Vitals) float Health = 100.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_Vitals) float Armor = 0.f;
    UPROPERTY(BlueprintAssignable) FOpenCityHealthChanged OnHealthChanged;
    UPROPERTY(BlueprintAssignable) FOpenCityDeath OnDeath;

    UFUNCTION(BlueprintCallable) float ApplyOpenCityDamage(float Amount, EOpenCityDamageKind Kind, AActor* InstigatorActor);
    UFUNCTION(BlueprintCallable) void Heal(float Amount);
    UFUNCTION(BlueprintCallable) void AddArmor(float Amount);
    UFUNCTION(BlueprintPure) bool IsAlive() const { return Health > 0.f; }

private:
    UFUNCTION() void OnRep_Vitals();
};

UCLASS(ClassGroup=(OpenCity), meta=(BlueprintSpawnableComponent))
class OPENCITY_API UOpenCityInventoryComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UOpenCityInventoryComponent();
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated) float MaxWeight = 40.f;
    UPROPERTY(BlueprintReadOnly, Replicated) TArray<FOpenCityInventoryStack> Items;

    UFUNCTION(BlueprintCallable) bool AddItem(FName ItemId, int32 Quantity, float WeightEach = 0.f);
    UFUNCTION(BlueprintCallable) bool RemoveItem(FName ItemId, int32 Quantity);
    UFUNCTION(BlueprintPure) int32 GetItemCount(FName ItemId) const;
    UFUNCTION(BlueprintPure) float GetCurrentWeight() const;
};

UCLASS(ClassGroup=(OpenCity), meta=(BlueprintSpawnableComponent))
class OPENCITY_API UOpenCityWeaponComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UOpenCityWeaponComponent();
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FOpenCityWeaponSpec> Arsenal;
    UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_WeaponState) int32 CurrentWeaponIndex = 0;
    UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_WeaponState) int32 MagazineAmmo = 0;
    UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_WeaponState) int32 ReserveAmmo = 0;
    UPROPERTY(BlueprintAssignable) FOpenCityWeaponChanged OnWeaponChanged;

    UFUNCTION(BlueprintCallable) bool Fire();
    UFUNCTION(BlueprintCallable) bool Reload();
    UFUNCTION(BlueprintCallable) bool EquipWeapon(int32 Index);
    UFUNCTION(BlueprintCallable) void NextWeapon();
    UFUNCTION(BlueprintPure) FOpenCityWeaponSpec GetCurrentWeapon() const;

private:
    double LastFireTime = -1000.0;
    UFUNCTION() void OnRep_WeaponState();
};

UCLASS(ClassGroup=(OpenCity), meta=(BlueprintSpawnableComponent))
class OPENCITY_API UOpenCityDialogueComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FOpenCityDialogueNode> Nodes;
    UPROPERTY(BlueprintReadOnly) FName CurrentNode;
    UPROPERTY(BlueprintAssignable) FOpenCityDialogueChanged OnDialogueChanged;

    UFUNCTION(BlueprintCallable) bool StartDialogue(FName NodeId);
    UFUNCTION(BlueprintCallable) bool Choose(int32 ChoiceIndex);
    UFUNCTION(BlueprintPure) FOpenCityDialogueNode GetCurrentDialogue() const;
};

UCLASS(ClassGroup=(OpenCity), meta=(BlueprintSpawnableComponent))
class OPENCITY_API UOpenCityRoutineComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UOpenCityRoutineComponent();
    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FOpenCityRoutineEntry> Schedule;
    UPROPERTY(BlueprintReadOnly) EOpenCityRoutineTask CurrentTask = EOpenCityRoutineTask::Idle;
    UPROPERTY(BlueprintReadOnly) FVector CurrentDestination = FVector::ZeroVector;

    UFUNCTION(BlueprintCallable) void UpdateRoutine(float WorldHour);
};
