#pragma once

#include "CoreMinimal.h"
#include "OpenCityCore.h"
#include "OpenCityVehicles.generated.h"

UCLASS()
class OPENCITY_API AOpenCityCar : public AOpenCityVehicle
{
    GENERATED_BODY()
public:
    AOpenCityCar();
};

UCLASS()
class OPENCITY_API AOpenCityMotorcycle : public AOpenCityVehicle
{
    GENERATED_BODY()
public:
    AOpenCityMotorcycle();
};

UCLASS()
class OPENCITY_API AOpenCityTruck : public AOpenCityVehicle
{
    GENERATED_BODY()
public:
    AOpenCityTruck();
};

UCLASS()
class OPENCITY_API AOpenCityHelicopter : public AOpenCityVehicle
{
    GENERATED_BODY()
public:
    AOpenCityHelicopter();
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
private:
    void ApplyLift(float Value);
};

UCLASS()
class OPENCITY_API AOpenCityAirplane : public AOpenCityVehicle
{
    GENERATED_BODY()
public:
    AOpenCityAirplane();
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
private:
    void ApplyPitch(float Value);
};

UCLASS()
class OPENCITY_API AOpenCityBoat : public AOpenCityVehicle
{
    GENERATED_BODY()
public:
    AOpenCityBoat();
};
