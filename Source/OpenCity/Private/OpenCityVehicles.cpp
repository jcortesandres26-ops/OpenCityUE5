#include "OpenCityVehicles.h"
#include "OpenCityComponents.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/FloatingPawnMovement.h"

AOpenCityCar::AOpenCityCar()
{
    VehicleClass = EOpenCityVehicleClass::Car;
    Movement->MaxSpeed = 3000.f;
    FuelBurnPerSecond = 0.32f;
    VehicleBody->SetRelativeScale3D(FVector(2.2f, 1.05f, 0.55f));
}

AOpenCityMotorcycle::AOpenCityMotorcycle()
{
    VehicleClass = EOpenCityVehicleClass::Motorcycle;
    Movement->MaxSpeed = 3600.f;
    FuelBurnPerSecond = 0.2f;
    VehicleBody->SetRelativeScale3D(FVector(1.5f, 0.42f, 0.65f));
}

AOpenCityTruck::AOpenCityTruck()
{
    VehicleClass = EOpenCityVehicleClass::Truck;
    Movement->MaxSpeed = 2100.f;
    Movement->Acceleration = 1000.f;
    FuelBurnPerSecond = 0.55f;
    VehicleBody->SetRelativeScale3D(FVector(3.8f, 1.25f, 1.2f));
    HealthComponent->MaxHealth = 800.f;
    HealthComponent->Health = 800.f;
}

AOpenCityHelicopter::AOpenCityHelicopter()
{
    VehicleClass = EOpenCityVehicleClass::Helicopter;
    Movement->MaxSpeed = 4200.f;
    FuelBurnPerSecond = 0.8f;
    VehicleBody->SetRelativeScale3D(FVector(3.2f, 1.25f, 0.9f));
}

void AOpenCityHelicopter::SetupPlayerInputComponent(UInputComponent* Input)
{
    Super::SetupPlayerInputComponent(Input);
    Input->BindAxis(TEXT("VehicleVertical"), this, &AOpenCityHelicopter::ApplyLift);
}

void AOpenCityHelicopter::ApplyLift(float Value)
{
    if (Fuel > 0.f) Movement->AddInputVector(FVector::UpVector * Value);
}

AOpenCityAirplane::AOpenCityAirplane()
{
    VehicleClass = EOpenCityVehicleClass::Airplane;
    Movement->MaxSpeed = 6500.f;
    Movement->Acceleration = 2200.f;
    FuelBurnPerSecond = 1.1f;
    VehicleBody->SetRelativeScale3D(FVector(4.5f, 5.5f, 0.65f));
}

void AOpenCityAirplane::SetupPlayerInputComponent(UInputComponent* Input)
{
    Super::SetupPlayerInputComponent(Input);
    Input->BindAxis(TEXT("VehicleVertical"), this, &AOpenCityAirplane::ApplyPitch);
}

void AOpenCityAirplane::ApplyPitch(float Value)
{
    AddActorLocalRotation(FRotator(Value * 32.f * GetWorld()->GetDeltaSeconds(), 0.f, 0.f));
    if (Fuel > 0.f) Movement->AddInputVector(GetActorUpVector() * Value * 0.35f);
}

AOpenCityBoat::AOpenCityBoat()
{
    VehicleClass = EOpenCityVehicleClass::Boat;
    Movement->MaxSpeed = 2400.f;
    FuelBurnPerSecond = 0.42f;
    VehicleBody->SetRelativeScale3D(FVector(3.4f, 1.3f, 0.55f));
}
