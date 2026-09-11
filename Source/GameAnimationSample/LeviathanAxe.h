// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "LeviathanAxe.generated.h"

UCLASS()
class GAMEANIMATIONSAMPLE_API ALeviathanAxe : public AActor
{
    GENERATED_BODY()

public:
    // Sets default values for this actor's properties
    ALeviathanAxe();

protected:
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

public:
    // Called every frame
    virtual void Tick(float DeltaTime) override;

    UFUNCTION(BlueprintCallable, Category = "Axe|Throw")
    void ThrowAxe(const FVector& InThrowDirection);
    void FindStickPoint();
    void UpdateThrowMovement(float DeltaTime);
    void StickAxe(const FHitResult& HitResult);
    void UpdateSpin(float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Axe|Recall")
    void RecallAxe();

    void UpdateRecallMovement(float DeltaTime);

    void FinishRecall();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Axe|Throw")
    float ThrowSpeed = 2000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Axe|Throw")
    FVector ThrowDirection = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Axe|Throw")
    bool bManualThrow = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Axe|Throw")
    bool bIsThrown = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Axe|Recall")
    bool bManualRecall = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Axe|Recall")
    float RecallSpeed = 2500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Axe|Spin")
    bool bShouldSpin = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Axe|Spin")
    float SpinSpeed = 1000.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Axe")
    ACharacter* CharacterRef = nullptr;

    UPROPERTY()
    TObjectPtr<USceneComponent> StickPoint = nullptr;

    UFUNCTION(BlueprintImplementableEvent, Category = "Axe|Recall")
    void OnAxeEquipped();
};
