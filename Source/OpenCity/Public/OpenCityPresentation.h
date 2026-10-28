#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "OpenCityPresentation.generated.h"

class USoundBase;
class UAudioComponent;

UCLASS(ClassGroup=(OpenCity), meta=(BlueprintSpawnableComponent))
class OPENCITY_API UOpenCityAudioDirectorComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UOpenCityAudioDirectorComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<USoundBase> CityAmbience;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<USoundBase> CombatMusic;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<USoundBase> MissionMusic;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UAudioComponent> ActiveAudio;

    UFUNCTION(BlueprintCallable) void PlayCityAmbience();
    UFUNCTION(BlueprintCallable) void PlayCombatMusic();
    UFUNCTION(BlueprintCallable) void PlayMissionMusic();
    UFUNCTION(BlueprintCallable) void StopMusic(float FadeSeconds = 1.f);
private:
    void Play(USoundBase* Sound, float Volume);
};

UCLASS()
class OPENCITY_API AOpenCityCinematicDirector : public AActor
{
    GENERATED_BODY()
public:
    AOpenCityCinematicDirector();
    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(BlueprintReadOnly) bool bPlaying = false;
    UPROPERTY(BlueprintReadOnly) TObjectPtr<AActor> CameraTarget;
    UPROPERTY(BlueprintReadOnly) float RemainingTime = 0.f;

    UFUNCTION(BlueprintCallable) void PlayShot(AActor* Target, float Duration = 4.f, float BlendTime = 0.75f);
    UFUNCTION(BlueprintCallable) void StopShot(float BlendTime = 0.5f);
};

UCLASS(ClassGroup=(OpenCity), meta=(BlueprintSpawnableComponent))
class OPENCITY_API UOpenCityMotionComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UOpenCityMotionComponent();
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite) float BobHeight = 3.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float BobSpeed = 8.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float LeanDegrees = 8.f;
private:
    FVector StartLocation = FVector::ZeroVector;
    FVector PreviousLocation = FVector::ZeroVector;
    float Phase = 0.f;
};
