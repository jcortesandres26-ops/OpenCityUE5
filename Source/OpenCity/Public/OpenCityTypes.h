#pragma once

#include "CoreMinimal.h"
#include "OpenCityTypes.generated.h"

UENUM(BlueprintType)
enum class EOpenCityWeather : uint8
{
    Clear,
    Cloudy,
    Rain,
    Storm,
    Fog,
    Sandstorm
};

UENUM(BlueprintType)
enum class EOpenCityVehicleClass : uint8
{
    Car,
    Motorcycle,
    Truck,
    Helicopter,
    Airplane,
    Boat
};

UENUM(BlueprintType)
enum class EOpenCityDamageKind : uint8
{
    Impact,
    Bullet,
    Explosion,
    Fire,
    Drowning,
    Fall
};

UENUM(BlueprintType)
enum class EOpenCityRoutineTask : uint8
{
    Idle,
    Walk,
    Work,
    Drive,
    Shop,
    Eat,
    Sleep,
    Flee,
    Fight
};

UENUM(BlueprintType)
enum class EOpenCityEventType : uint8
{
    None,
    VehicleTheft,
    StreetFight,
    PoliceChase,
    Delivery,
    Race,
    Robbery,
    Rescue
};

USTRUCT(BlueprintType)
struct FOpenCityWeaponSpec
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Id = TEXT("Pistol");
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText DisplayName;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Damage = 24.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Range = 12000.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float FireInterval = 0.22f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 MagazineSize = 12;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 ReserveAmmo = 48;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bAutomatic = false;
};

USTRUCT(BlueprintType)
struct FOpenCityDialogueChoice
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Text;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName NextNode;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 MoneyCost = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName RequiredItem;
};

USTRUCT(BlueprintType)
struct FOpenCityDialogueNode
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Id;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Speaker;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Line;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FOpenCityDialogueChoice> Choices;
};

USTRUCT(BlueprintType)
struct FOpenCityRoutineEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite) float StartHour = 8.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float EndHour = 17.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EOpenCityRoutineTask Task = EOpenCityRoutineTask::Work;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector Destination = FVector::ZeroVector;
};

USTRUCT(BlueprintType)
struct FOpenCityBusinessData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FName Id;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) bool bOwned = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) int32 Price = 25000;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) int32 DailyIncome = 500;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) float Condition = 100.f;
};

USTRUCT(BlueprintType)
struct FOpenCityTerritoryData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FName Id;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FName OwnerFaction = TEXT("Neutral");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) float Influence = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector Center = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Radius = 5000.f;
};

USTRUCT(BlueprintType)
struct FOpenCityWorldEvent
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Id;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EOpenCityEventType Type = EOpenCityEventType::None;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector Location = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Reward = 250;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float TimeRemaining = 180.f;
};
