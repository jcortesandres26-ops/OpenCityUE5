#include "OpenCityCore.h"
#include "OpenCityComponents.h"
#include "OpenCityNetwork.h"
#include "OpenCityPresentation.h"
#include "OpenCityServices.h"
#include "OpenCityVehicles.h"
#include "OpenCityWorld.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/Canvas.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Subsystems/SubsystemCollection.h"

void UOpenCityGameSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    GenerateMissionCatalog();
    FactionReputation.Add(TEXT("Civilians"), 0);
    FactionReputation.Add(TEXT("Police"), 0);
    FactionReputation.Add(TEXT("Northside"), 0);
    FactionReputation.Add(TEXT("Southside"), 0);
    for (int32 Index = 0; Index < 12; ++Index)
    {
        FPropertyRecord Record;
        Record.Id = FName(*FString::Printf(TEXT("Property_%02d"), Index + 1));
        Record.Price = 10000 + Index * 7500;
        Record.DailyIncome = 100 + Index * 75;
        Properties.Add(Record);
    }
}

void UOpenCityGameSubsystem::GenerateMissionCatalog()
{
    if (!Missions.IsEmpty()) return;
    const TCHAR* Types[] = { TEXT("Story"), TEXT("Race"), TEXT("Delivery"), TEXT("Gang"), TEXT("Heist"), TEXT("Infiltration") };
    for (int32 Index = 0; Index < 120; ++Index)
    {
        FOpenCityMission Mission;
        Mission.Id = FName(*FString::Printf(TEXT("Mission_%03d"), Index + 1));
        Mission.Title = FText::FromString(FString::Printf(TEXT("%s Mission %03d"), Types[Index % 6], Index + 1));
        Mission.Description = FText::FromString(TEXT("Reach the checkpoint and complete the active objective."));
        Mission.Reward = 500 + Index * 125;
        Mission.State = Index < 3 ? EMissionState::Available : EMissionState::Locked;
        Mission.Checkpoint = FVector((Index % 12) * 2500.f, (Index / 12) * 2500.f, 100.f);
        Missions.Add(Mission);
    }
}

bool UOpenCityGameSubsystem::StartMission(FName MissionId)
{
    for (FOpenCityMission& Mission : Missions)
        if (Mission.Id == MissionId && Mission.State == EMissionState::Available)
        { Mission.State = EMissionState::Active; return true; }
    return false;
}

bool UOpenCityGameSubsystem::CompleteMission(FName MissionId)
{
    for (int32 Index = 0; Index < Missions.Num(); ++Index)
        if (Missions[Index].Id == MissionId && Missions[Index].State == EMissionState::Active)
        {
            Missions[Index].State = EMissionState::Completed;
            AddMoney(Missions[Index].Reward);
            if (Missions.IsValidIndex(Index + 3) && Missions[Index + 3].State == EMissionState::Locked)
                Missions[Index + 3].State = EMissionState::Available;
            return true;
        }
    return false;
}

void UOpenCityGameSubsystem::FailMission(FName MissionId)
{
    for (FOpenCityMission& Mission : Missions)
        if (Mission.Id == MissionId && Mission.State == EMissionState::Active)
        { Mission.State = EMissionState::Failed; return; }
}

void UOpenCityGameSubsystem::AddMoney(int32 Amount) { Money = FMath::Max(0, Money + Amount); }
bool UOpenCityGameSubsystem::SpendMoney(int32 Amount)
{
    if (Amount < 0 || Money < Amount) return false;
    Money -= Amount;
    return true;
}
void UOpenCityGameSubsystem::AddWantedLevel(int32 Amount) { WantedLevel = FMath::Clamp(WantedLevel + Amount, 0, 5); }
void UOpenCityGameSubsystem::ClearWantedLevel() { WantedLevel = 0; }

void UOpenCityGameSubsystem::AddInventoryItem(FName ItemId, int32 Quantity)
{
    if (Quantity <= 0) return;
    for (FInventoryItem& Item : Inventory)
        if (Item.Id == ItemId) { Item.Quantity += Quantity; return; }
    FInventoryItem NewItem; NewItem.Id = ItemId; NewItem.Quantity = Quantity; Inventory.Add(NewItem);
}

bool UOpenCityGameSubsystem::BuyProperty(FName PropertyId)
{
    for (FPropertyRecord& Property : Properties)
        if (Property.Id == PropertyId && !Property.bOwned && SpendMoney(Property.Price))
        { Property.bOwned = true; return true; }
    return false;
}

bool UOpenCityGameSubsystem::SaveGameToSlot(FString SlotName)
{
    UOpenCitySaveGame* Save = Cast<UOpenCitySaveGame>(UGameplayStatics::CreateSaveGameObject(UOpenCitySaveGame::StaticClass()));
    if (!Save) return false;
    Save->Money = Money;
    Save->WantedLevel = WantedLevel;
    Save->WorldTime = WorldTime;
    Save->Missions = Missions;
    Save->Inventory = Inventory;
    Save->Properties = Properties;
    Save->FactionReputation = FactionReputation;
    Save->TerritoryOwners = TerritoryOwners;
    if (const UOpenCityServiceSubsystem* Services = GetGameInstance()->GetSubsystem<UOpenCityServiceSubsystem>())
    {
        Save->Businesses = Services->Businesses;
        Save->Territories = Services->Territories;
    }
    if (const APlayerController* PC = GetWorld()->GetFirstPlayerController())
    {
        const APawn* Pawn = PC->GetPawn();
        if (Pawn)
        {
            Save->PlayerLocation = Pawn->GetActorLocation();
            Save->PlayerRotation = Pawn->GetActorRotation();
            const AOpenCityCharacter* Character = Cast<AOpenCityCharacter>(Pawn);
            if (!Character)
                if (const AOpenCityVehicle* Vehicle = Cast<AOpenCityVehicle>(Pawn)) Character = Vehicle->Driver;
            if (Character)
            {
                if (Character->HealthComponent)
                {
                    Save->Health = Character->HealthComponent->Health;
                    Save->Armor = Character->HealthComponent->Armor;
                }
                if (Character->InventoryComponent)
                {
                    Save->Inventory.Reset();
                    for (const FOpenCityInventoryStack& Stack : Character->InventoryComponent->Items)
                    {
                        FInventoryItem Item;
                        Item.Id = Stack.ItemId;
                        Item.Quantity = Stack.Quantity;
                        Save->Inventory.Add(Item);
                    }
                }
                if (Character->WeaponComponent)
                {
                    Save->CurrentWeaponIndex = Character->WeaponComponent->CurrentWeaponIndex;
                    Save->MagazineAmmo = Character->WeaponComponent->MagazineAmmo;
                    Save->ReserveAmmo = Character->WeaponComponent->ReserveAmmo;
                }
            }
        }
    }
    return UGameplayStatics::SaveGameToSlot(Save, SlotName, 0);
}

bool UOpenCityGameSubsystem::LoadGameFromSlot(FString SlotName)
{
    const UOpenCitySaveGame* Save = Cast<UOpenCitySaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
    if (!Save) return false;
    Money = Save->Money;
    Health = Save->Health;
    Armor = Save->Armor;
    WantedLevel = Save->WantedLevel;
    WorldTime = Save->WorldTime;
    Missions = Save->Missions;
    Inventory = Save->Inventory;
    Properties = Save->Properties;
    FactionReputation = Save->FactionReputation;
    TerritoryOwners = Save->TerritoryOwners;
    if (UOpenCityServiceSubsystem* Services = GetGameInstance()->GetSubsystem<UOpenCityServiceSubsystem>())
    {
        Services->Businesses = Save->Businesses;
        Services->Territories = Save->Territories;
    }
    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
    {
        APawn* Pawn = PC->GetPawn();
        if (Pawn)
        {
            Pawn->SetActorLocationAndRotation(Save->PlayerLocation, Save->PlayerRotation);
            AOpenCityCharacter* Character = Cast<AOpenCityCharacter>(Pawn);
            if (!Character)
                if (AOpenCityVehicle* Vehicle = Cast<AOpenCityVehicle>(Pawn)) Character = Vehicle->Driver;
            if (Character)
            {
                if (Character->HealthComponent)
                {
                    Character->HealthComponent->Health = Save->Health;
                    Character->HealthComponent->Armor = Save->Armor;
                }
                if (Character->InventoryComponent)
                {
                    Character->InventoryComponent->Items.Reset();
                    for (const FInventoryItem& Item : Save->Inventory)
                        Character->InventoryComponent->AddItem(Item.Id, Item.Quantity);
                }
                if (Character->WeaponComponent && Character->WeaponComponent->Arsenal.IsValidIndex(Save->CurrentWeaponIndex))
                {
                    Character->WeaponComponent->EquipWeapon(Save->CurrentWeaponIndex);
                    Character->WeaponComponent->MagazineAmmo = Save->MagazineAmmo;
                    Character->WeaponComponent->ReserveAmmo = Save->ReserveAmmo;
                }
            }
        }
    }
    return true;
}

AOpenCityCharacter::AOpenCityCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f);
    GetCharacterMovement()->AirControl = 0.35f;
    bUseControllerRotationYaw = false;

    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 420.f;
    CameraBoom->bUsePawnControlRotation = true;
    CameraBoom->bEnableCameraLag = true;
    CameraBoom->CameraLagSpeed = 12.f;
    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;

    BodyVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyVisual"));
    BodyVisual->SetupAttachment(GetCapsuleComponent());
    BodyVisual->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Capsule.Capsule")));
    BodyVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BodyVisual->SetRelativeScale3D(FVector(0.55f, 0.55f, 0.9f));
    BodyVisual->SetRelativeLocation(FVector(0.f, 0.f, -8.f));

    InteractionRange = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionRange"));
    InteractionRange->SetupAttachment(RootComponent);
    InteractionRange->SetSphereRadius(300.f);
    InteractionRange->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    InteractionRange->SetCollisionResponseToAllChannels(ECR_Ignore);
    InteractionRange->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

    HealthComponent = CreateDefaultSubobject<UOpenCityHealthComponent>(TEXT("HealthComponent"));
    InventoryComponent = CreateDefaultSubobject<UOpenCityInventoryComponent>(TEXT("InventoryComponent"));
    WeaponComponent = CreateDefaultSubobject<UOpenCityWeaponComponent>(TEXT("WeaponComponent"));
    MotionComponent = CreateDefaultSubobject<UOpenCityMotionComponent>(TEXT("MotionComponent"));
}

void AOpenCityCharacter::BeginPlay()
{
    Super::BeginPlay();
    if (HasAuthority())
    {
        InventoryComponent->AddItem(TEXT("Phone"), 1, 0.2f);
        InventoryComponent->AddItem(TEXT("Medkit"), 2, 0.7f);
    }
}

void AOpenCityCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (IsLocallyControlled()) FindNearbyVehicle();
}

void AOpenCityCharacter::SetupPlayerInputComponent(UInputComponent* Input)
{
    Super::SetupPlayerInputComponent(Input);
    Input->BindAxis(TEXT("MoveForward"), this, &AOpenCityCharacter::MoveForward);
    Input->BindAxis(TEXT("MoveRight"), this, &AOpenCityCharacter::MoveRight);
    Input->BindAxis(TEXT("Turn"), this, &APawn::AddControllerYawInput);
    Input->BindAxis(TEXT("LookUp"), this, &APawn::AddControllerPitchInput);
    Input->BindAction(TEXT("Jump"), IE_Pressed, this, &ACharacter::Jump);
    Input->BindAction(TEXT("Jump"), IE_Released, this, &ACharacter::StopJumping);
    Input->BindAction(TEXT("Sprint"), IE_Pressed, this, &AOpenCityCharacter::StartSprint);
    Input->BindAction(TEXT("Sprint"), IE_Released, this, &AOpenCityCharacter::StopSprint);
    Input->BindAction(TEXT("Crouch"), IE_Pressed, this, &AOpenCityCharacter::ToggleCrouch);
    Input->BindAction(TEXT("Interact"), IE_Pressed, this, &AOpenCityCharacter::EnterNearbyVehicle);
    Input->BindAction(TEXT("EnterVehicle"), IE_Pressed, this, &AOpenCityCharacter::EnterNearbyVehicle);
    Input->BindAction(TEXT("Fire"), IE_Pressed, this, &AOpenCityCharacter::FireWeapon);
    Input->BindAction(TEXT("Reload"), IE_Pressed, this, &AOpenCityCharacter::ReloadWeapon);
    Input->BindAction(TEXT("WeaponNext"), IE_Pressed, this, &AOpenCityCharacter::NextWeapon);
}

void AOpenCityCharacter::FireWeapon() { if (WeaponComponent) WeaponComponent->Fire(); }
void AOpenCityCharacter::ReloadWeapon() { if (WeaponComponent) WeaponComponent->Reload(); }
void AOpenCityCharacter::NextWeapon() { if (WeaponComponent) WeaponComponent->NextWeapon(); }

void AOpenCityCharacter::FindNearbyVehicle()
{
    TArray<AActor*> Actors;
    InteractionRange->GetOverlappingActors(Actors, AOpenCityVehicle::StaticClass());
    AOpenCityVehicle* BestVehicle = nullptr;
    float BestDistance = TNumericLimits<float>::Max();
    for (AActor* Actor : Actors)
    {
        const float Distance = FVector::DistSquared(GetActorLocation(), Actor->GetActorLocation());
        if (Distance < BestDistance)
        {
            BestDistance = Distance;
            BestVehicle = Cast<AOpenCityVehicle>(Actor);
        }
    }
    NearbyVehicle = BestVehicle;
}

void AOpenCityCharacter::MoveForward(float Value)
{
    if (Controller && !FMath::IsNearlyZero(Value))
    {
        const FRotator Yaw(0.f, Controller->GetControlRotation().Yaw, 0.f);
        AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), Value);
    }
}
void AOpenCityCharacter::MoveRight(float Value)
{
    if (Controller && !FMath::IsNearlyZero(Value))
    {
        const FRotator Yaw(0.f, Controller->GetControlRotation().Yaw, 0.f);
        AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), Value);
    }
}
void AOpenCityCharacter::StartSprint() { GetCharacterMovement()->MaxWalkSpeed = SprintSpeed; }
void AOpenCityCharacter::StopSprint() { GetCharacterMovement()->MaxWalkSpeed = WalkSpeed; }
void AOpenCityCharacter::ToggleCrouch() { bIsCrouched ? UnCrouch() : Crouch(); }
void AOpenCityCharacter::SetNearbyVehicle(AOpenCityVehicle* Vehicle) { NearbyVehicle = Vehicle; }
void AOpenCityCharacter::EnterNearbyVehicle() { if (NearbyVehicle) NearbyVehicle->EnterVehicle(this); }

AOpenCityVehicle::AOpenCityVehicle()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    SetReplicateMovement(true);
    VehicleBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VehicleBody"));
    SetRootComponent(VehicleBody);
    VehicleBody->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
    VehicleBody->SetRelativeScale3D(FVector(2.2f, 1.05f, 0.55f));
    VehicleBody->SetCollisionProfileName(TEXT("Vehicle"));
    Movement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("Movement"));
    Movement->MaxSpeed = 2600.f;
    Movement->Acceleration = 1800.f;
    Movement->Deceleration = 2400.f;
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 650.f;
    CameraBoom->SetRelativeLocation(FVector(0.f, 0.f, 220.f));
    CameraBoom->bEnableCameraLag = true;
    CameraBoom->CameraLagSpeed = 7.f;
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    HealthComponent = CreateDefaultSubobject<UOpenCityHealthComponent>(TEXT("HealthComponent"));
    HealthComponent->MaxHealth = 400.f;
    HealthComponent->Health = 400.f;
}

void AOpenCityVehicle::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (HasAuthority() && Driver && FMath::Abs(ThrottleInput) > 0.05f)
    {
        Fuel = FMath::Max(0.f, Fuel - FuelBurnPerSecond * FMath::Abs(ThrottleInput) * DeltaSeconds);
        if (Fuel <= 0.f) Movement->StopMovementImmediately();
    }
}

void AOpenCityVehicle::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AOpenCityVehicle, VehicleClass);
    DOREPLIFETIME(AOpenCityVehicle, Fuel);
    DOREPLIFETIME(AOpenCityVehicle, Driver);
}

void AOpenCityVehicle::SetupPlayerInputComponent(UInputComponent* Input)
{
    Super::SetupPlayerInputComponent(Input);
    Input->BindAxis(TEXT("VehicleThrottle"), this, &AOpenCityVehicle::ApplyThrottle);
    Input->BindAxis(TEXT("VehicleSteer"), this, &AOpenCityVehicle::ApplySteering);
    Input->BindAction(TEXT("Interact"), IE_Pressed, this, &AOpenCityVehicle::ExitVehicle);
    Input->BindAction(TEXT("EnterVehicle"), IE_Pressed, this, &AOpenCityVehicle::ExitVehicle);
}
void AOpenCityVehicle::ApplyThrottle(float Value)
{
    ThrottleInput = Fuel > 0.f ? Value : 0.f;
    Movement->AddInputVector(GetActorForwardVector() * ThrottleInput);
}
void AOpenCityVehicle::ApplySteering(float Value)
{
    const float SpeedFactor = FMath::Clamp(Movement->Velocity.Size() / 400.f, 0.2f, 1.f);
    if (!FMath::IsNearlyZero(Value)) AddActorLocalRotation(FRotator(0.f, Value * 75.f * SpeedFactor * GetWorld()->GetDeltaSeconds(), 0.f));
}
void AOpenCityVehicle::EnterVehicle(AOpenCityCharacter* NewDriver)
{
    if (!NewDriver || Driver || !HasAuthority()) return;
    APlayerController* PC = Cast<APlayerController>(NewDriver->GetController());
    if (!PC) return;
    Driver = NewDriver;
    NewDriver->SetActorHiddenInGame(true);
    NewDriver->SetActorEnableCollision(false);
    NewDriver->AttachToActor(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
    PC->Possess(this);
}
void AOpenCityVehicle::ExitVehicle()
{
    if (!Driver || !HasAuthority()) return;
    APlayerController* PC = Cast<APlayerController>(GetController());
    AOpenCityCharacter* ExitingDriver = Driver;
    Driver = nullptr;
    ExitingDriver->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    ExitingDriver->SetActorLocation(GetActorLocation() + GetActorRightVector() * 180.f + FVector(0.f, 0.f, 80.f));
    ExitingDriver->SetActorHiddenInGame(false);
    ExitingDriver->SetActorEnableCollision(true);
    if (PC) PC->Possess(ExitingDriver);
}
void AOpenCityVehicle::Refuel(float Amount)
{
    if (HasAuthority()) Fuel = FMath::Clamp(Fuel + Amount, 0.f, 100.f);
}

AOpenCityAIController::AOpenCityAIController()
{
    Perception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("Perception"));
    SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
    SightConfig->SightRadius = 2600.f;
    SightConfig->LoseSightRadius = 3200.f;
    SightConfig->PeripheralVisionAngleDegrees = 80.f;
    SightConfig->DetectionByAffiliation.bDetectEnemies = true;
    SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
    SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
    Perception->ConfigureSense(*SightConfig);
    Perception->SetDominantSense(SightConfig->GetSenseImplementation());
    Perception->OnPerceptionUpdated.AddDynamic(this, &AOpenCityAIController::HandlePerception);
}
void AOpenCityAIController::HandlePerception(const TArray<AActor*>& UpdatedActors)
{
    for (AActor* Actor : UpdatedActors)
    {
        FActorPerceptionBlueprintInfo Info;
        Perception->GetActorsPerception(Actor, Info);
        if (Info.LastSensedStimuli.ContainsByPredicate([](const FAIStimulus& Stimulus){ return Stimulus.WasSuccessfullySensed(); }))
        { SetFocus(Actor); MoveToActor(Actor, 250.f); return; }
    }
    ClearFocus(EAIFocusPriority::Gameplay);
}

AOpenCityNPC::AOpenCityNPC()
{
    PrimaryActorTick.bCanEverTick = true;
    SetActorTickInterval(2.f);
    bReplicates = true;
    AIControllerClass = AOpenCityAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
    GetCharacterMovement()->MaxWalkSpeed = 260.f;
    GetCharacterMovement()->bOrientRotationToMovement = true;

    BodyVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyVisual"));
    BodyVisual->SetupAttachment(GetCapsuleComponent());
    BodyVisual->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Capsule.Capsule")));
    BodyVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BodyVisual->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.85f));

    HealthComponent = CreateDefaultSubobject<UOpenCityHealthComponent>(TEXT("HealthComponent"));
    DialogueComponent = CreateDefaultSubobject<UOpenCityDialogueComponent>(TEXT("DialogueComponent"));
    RoutineComponent = CreateDefaultSubobject<UOpenCityRoutineComponent>(TEXT("RoutineComponent"));
    MotionComponent = CreateDefaultSubobject<UOpenCityMotionComponent>(TEXT("MotionComponent"));
}

void AOpenCityNPC::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!HasAuthority() || !RoutineComponent) return;
    if (const UOpenCityGameSubsystem* Systems = GetGameInstance()->GetSubsystem<UOpenCityGameSubsystem>())
        RoutineComponent->UpdateRoutine(Systems->WorldTime);
}

AOpenCityPoliceNPC::AOpenCityPoliceNPC()
{
    Faction = TEXT("Police");
    Attitude = EFactionAttitude::Hostile;
    GetCharacterMovement()->MaxWalkSpeed = 520.f;
}

AOpenCityDayNightController::AOpenCityDayNightController()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
}
void AOpenCityDayNightController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    UOpenCityGameSubsystem* Systems = GetGameInstance()->GetSubsystem<UOpenCityGameSubsystem>();
    if (!Systems) return;
    Systems->WorldTime = FMath::Fmod(Systems->WorldTime + DeltaSeconds * 24.f / FMath::Max(60.f, DayLengthMinutes * 60.f), 24.f);
    if (Sun) Sun->SetActorRotation(FRotator(Systems->WorldTime / 24.f * 360.f - 90.f, -35.f, 0.f));
}

void AOpenCityHUD::DrawHUD()
{
    Super::DrawHUD();
    if (!Canvas) return;
    const UOpenCityGameSubsystem* Systems = GetGameInstance()->GetSubsystem<UOpenCityGameSubsystem>();
    if (!Systems) return;

    float CurrentHealth = Systems->Health;
    float CurrentArmor = Systems->Armor;
    FString WeaponText = TEXT("UNARMED");
    APawn* Pawn = GetOwningPawn();
    AOpenCityCharacter* Character = Cast<AOpenCityCharacter>(Pawn);
    if (!Character)
        if (AOpenCityVehicle* Vehicle = Cast<AOpenCityVehicle>(Pawn)) Character = Vehicle->Driver;
    if (Character)
    {
        if (Character->HealthComponent)
        {
            CurrentHealth = Character->HealthComponent->Health;
            CurrentArmor = Character->HealthComponent->Armor;
        }
        if (Character->WeaponComponent)
        {
            const FOpenCityWeaponSpec Weapon = Character->WeaponComponent->GetCurrentWeapon();
            WeaponText = FString::Printf(TEXT("%s  %d/%d"), *Weapon.DisplayName.ToString(), Character->WeaponComponent->MagazineAmmo, Character->WeaponComponent->ReserveAmmo);
        }
    }

    DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.62f), 24.f, 24.f, 370.f, 142.f);
    DrawText(FString::Printf(TEXT("HEALTH %.0f   ARMOR %.0f"), CurrentHealth, CurrentArmor), FLinearColor::White, 45.f, 45.f, nullptr, 1.25f);
    DrawText(FString::Printf(TEXT("$ %d"), Systems->Money), FLinearColor(0.5f, 1.f, 0.2f), 45.f, 78.f, nullptr, 1.25f);
    DrawText(FString::Printf(TEXT("WANTED %d/5"), Systems->WantedLevel), FLinearColor::Yellow, 45.f, 111.f, nullptr, 1.1f);
    DrawText(WeaponText, FLinearColor(0.8f, 0.9f, 1.f), 45.f, 138.f, nullptr, 0.9f);

    for (const FOpenCityMission& Mission : Systems->Missions)
    {
        if (Mission.State == EMissionState::Active)
        {
            DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.62f), Canvas->ClipX * 0.5f - 260.f, 24.f, 520.f, 58.f);
            DrawText(Mission.Title.ToString(), FLinearColor::White, Canvas->ClipX * 0.5f - 230.f, 42.f, nullptr, 1.05f);
            break;
        }
    }

    const float MapSize = 190.f;
    const float MapX = Canvas->ClipX - MapSize - 28.f;
    const float MapY = Canvas->ClipY - MapSize - 28.f;
    DrawRect(FLinearColor(0.01f, 0.02f, 0.035f, 0.82f), MapX, MapY, MapSize, MapSize);
    DrawRect(FLinearColor(0.2f, 0.8f, 1.f), MapX + MapSize * 0.5f - 3.f, MapY + MapSize * 0.5f - 3.f, 6.f, 6.f);
    if (Pawn)
    {
        const FVector Origin = Pawn->GetActorLocation();
        constexpr float RadarRange = 6000.f;
        for (TActorIterator<AOpenCityPoliceNPC> It(GetWorld()); It; ++It)
        {
            const FVector Offset = It->GetActorLocation() - Origin;
            const float X = FMath::Clamp(Offset.Y / RadarRange, -1.f, 1.f) * MapSize * 0.48f;
            const float Y = FMath::Clamp(-Offset.X / RadarRange, -1.f, 1.f) * MapSize * 0.48f;
            DrawRect(FLinearColor::Red, MapX + MapSize * 0.5f + X - 2.f, MapY + MapSize * 0.5f + Y - 2.f, 4.f, 4.f);
        }
    }
}

AOpenCityGameMode::AOpenCityGameMode()
{
    DefaultPawnClass = AOpenCityCharacter::StaticClass();
    PlayerControllerClass = AOpenCityPlayerController::StaticClass();
    PlayerStateClass = AOpenCityPlayerState::StaticClass();
    GameStateClass = AOpenCityGameState::StaticClass();
    HUDClass = AOpenCityHUD::StaticClass();
    bUseSeamlessTravel = true;
}

void AOpenCityGameMode::BeginPlay()
{
    Super::BeginPlay();
    if (!HasAuthority()) return;

    GetWorld()->SpawnActor<AOpenCityWorldGenerator>(FVector::ZeroVector, FRotator::ZeroRotator);
    GetWorld()->SpawnActor<AOpenCityWorldDirector>(FVector::ZeroVector, FRotator::ZeroRotator);
    GetWorld()->SpawnActor<AOpenCityTrafficManager>(FVector::ZeroVector, FRotator::ZeroRotator);
    GetWorld()->SpawnActor<AOpenCityDayNightController>(FVector::ZeroVector, FRotator::ZeroRotator);
    GetWorld()->SpawnActor<AOpenCityMissionDirector>(FVector::ZeroVector, FRotator::ZeroRotator);
    GetWorld()->SpawnActor<AOpenCityPoliceDirector>(FVector::ZeroVector, FRotator::ZeroRotator);
    GetWorld()->SpawnActor<AOpenCityCinematicDirector>(FVector::ZeroVector, FRotator::ZeroRotator);

    const TArray<TSubclassOf<AOpenCityVehicle>> VehicleTypes =
    {
        AOpenCityCar::StaticClass(), AOpenCityCar::StaticClass(), AOpenCityMotorcycle::StaticClass(),
        AOpenCityTruck::StaticClass(), AOpenCityHelicopter::StaticClass(), AOpenCityAirplane::StaticClass(),
        AOpenCityBoat::StaticClass(), AOpenCityCar::StaticClass()
    };
    for (int32 Index = 0; Index < VehicleTypes.Num(); ++Index)
    {
        FVector Position((Index - 4) * 700.f, 550.f, 140.f);
        if (VehicleTypes[Index] == AOpenCityHelicopter::StaticClass()) Position = FVector(-7000.f, 7000.f, 450.f);
        if (VehicleTypes[Index] == AOpenCityAirplane::StaticClass()) Position = FVector(-9000.f, 9000.f, 220.f);
        if (VehicleTypes[Index] == AOpenCityBoat::StaticClass()) Position = FVector(9000.f, -9000.f, 80.f);
        GetWorld()->SpawnActor<AOpenCityVehicle>(VehicleTypes[Index], Position, FRotator::ZeroRotator);
    }

    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (PC && !PC->GetPawn())
    {
        if (AOpenCityCharacter* Character = GetWorld()->SpawnActor<AOpenCityCharacter>(FVector(0.f, 0.f, 180.f), FRotator::ZeroRotator))
            PC->Possess(Character);
    }
}
