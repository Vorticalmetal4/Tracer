// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "TracerCharacter.generated.h"

class UCharacterMovementComponent;
class UInputComponent;
class USkeletalMeshComponent;
class USceneComponent;
class UCameraComponent;
class UAnimMontage;
class USoundBase;

struct RetrocessData
{
	FVector2D Movement;
	FVector2D Rotation;
	float Health;
	int Ammo;
};

UCLASS(config=Game)
class ATracerCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Pawn mesh: 1st person view (arms; seen only by self) */
	UPROPERTY(VisibleDefaultsOnly, Category=Mesh)
	USkeletalMeshComponent* Mesh1P;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FirstPersonCameraComponent;

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	class UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	class UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	class UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* FirstAbilityAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* SecondAbilityAction;

	
public:
	ATracerCharacter();

protected:
	virtual void BeginPlay();

public:
		
	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* LookAction;

	/** Bool for AnimBP to switch to another animation set */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Weapon)
	bool bHasRifle;

	/** Setter to set the bool */
	UFUNCTION(BlueprintCallable, Category = Weapon)
	void SetHasRifle(bool bNewHasRifle);

	/** Getter for the bool */
	UFUNCTION(BlueprintCallable, Category = Weapon)
	bool GetHasRifle();

protected:
	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	virtual void Tick(float DeltaTime);

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	// End of APawn interface

public:
	/** Returns Mesh1P subobject **/
	USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }
	/** Returns FirstPersonCameraComponent subobject **/
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

protected:
	UPROPERTY(BlueprintReadOnly)
	int Ammo;

	UPROPERTY(BlueprintReadOnly)
	float Health;

	UFUNCTION(BlueprintImplementableEvent)
	void AddDash();

	UFUNCTION(BlueprintImplementableEvent)
	void DiscountDash();

	UFUNCTION(BlueprintImplementableEvent)
	void Regressing(bool isRegressing);

private:

	float ImpulseReloadTime;
	float ImpulseReloadTimeRemaining;
	float CooldownFirstAbilitie;
	float CooldownRemainingFirstAbilitie;
	float DashTime;
	float CurrentDashTime;

	float TimeRegisterData;
	float TimeTillRegisterData;

	bool DashActivated;
	bool HasMove;
	bool HasRotate;
	bool CanMove;
	bool bIsRegressing;

	int16 ImpulsesRemaining;
	int16 MaxImpulses;
	int16 RetrocessPointsNumber;
	int16 RetrocessIterator;

	FVector LaunchVector;
	FVector2D AuxMovement;
	FVector2D AuxRotation;

	UCharacterMovementComponent* CharacterMovement;

	RetrocessData RetrocessPoints[240];

	void FirstAbility();
	void SecondAbility();
	void UpdateRetrocessData();

	void Retrocess();
};

