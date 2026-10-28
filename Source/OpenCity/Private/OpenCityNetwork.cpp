#include "OpenCityNetwork.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"
#include "OpenCityCore.h"
#include "OpenCityServices.h"

AOpenCityPlayerState::AOpenCityPlayerState()
{
    bReplicates = true;
}

void AOpenCityPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AOpenCityPlayerState, Money);
    DOREPLIFETIME(AOpenCityPlayerState, WantedLevel);
    DOREPLIFETIME(AOpenCityPlayerState, Faction);
    DOREPLIFETIME(AOpenCityPlayerState, Reputation);
}

AOpenCityGameState::AOpenCityGameState()
{
    bReplicates = true;
}

void AOpenCityGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AOpenCityGameState, WorldHour);
    DOREPLIFETIME(AOpenCityGameState, Weather);
    DOREPLIFETIME(AOpenCityGameState, WorldSeed);
}

AOpenCityPlayerController::AOpenCityPlayerController()
{
    bShowMouseCursor = false;
}

void AOpenCityPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    InputComponent->BindAction(TEXT("PauseGame"), IE_Pressed, this, &AOpenCityPlayerController::TogglePauseMenu);
    InputComponent->BindAction(TEXT("Map"), IE_Pressed, this, &AOpenCityPlayerController::ToggleWorldMap);
    InputComponent->BindAction(TEXT("Inventory"), IE_Pressed, this, &AOpenCityPlayerController::ToggleInventory);
    InputComponent->BindAction(TEXT("QuickSave"), IE_Pressed, this, &AOpenCityPlayerController::QuickSave);
    InputComponent->BindAction(TEXT("QuickLoad"), IE_Pressed, this, &AOpenCityPlayerController::QuickLoad);
}

void AOpenCityPlayerController::TogglePauseMenu()
{
    const bool bPause = !IsPaused();
    SetPause(bPause);
    bShowMouseCursor = bPause;
    if (bPause) SetInputMode(FInputModeGameAndUI());
    else SetInputMode(FInputModeGameOnly());
}

void AOpenCityPlayerController::ToggleWorldMap()
{
    bMapOpen = !bMapOpen;
    bShowMouseCursor = bMapOpen || bInventoryOpen;
    if (bMapOpen || bInventoryOpen) SetInputMode(FInputModeGameAndUI());
    else SetInputMode(FInputModeGameOnly());
}

void AOpenCityPlayerController::ToggleInventory()
{
    bInventoryOpen = !bInventoryOpen;
    bShowMouseCursor = bMapOpen || bInventoryOpen;
    if (bMapOpen || bInventoryOpen) SetInputMode(FInputModeGameAndUI());
    else SetInputMode(FInputModeGameOnly());
}

void AOpenCityPlayerController::QuickSave()
{
    if (UOpenCityGameSubsystem* Game = GetGameInstance()->GetSubsystem<UOpenCityGameSubsystem>()) Game->SaveGameToSlot(TEXT("QuickSave"));
}

void AOpenCityPlayerController::QuickLoad()
{
    if (UOpenCityGameSubsystem* Game = GetGameInstance()->GetSubsystem<UOpenCityGameSubsystem>()) Game->LoadGameFromSlot(TEXT("QuickSave"));
}

void AOpenCityPlayerController::ServerReportCrime_Implementation(int32 Severity)
{
    for (TActorIterator<AOpenCityPoliceDirector> It(GetWorld()); It; ++It)
    {
        It->ReportCrime(GetPawn(), FMath::Clamp(Severity, 1, 5));
        break;
    }
}