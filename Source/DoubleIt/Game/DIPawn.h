#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "DIPawn.generated.h"

class UCameraComponent;

/** Static orthographic camera looking down on the ring. */
UCLASS()
class DOUBLEIT_API ADIPawn : public APawn
{
	GENERATED_BODY()

public:
	ADIPawn();

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCameraComponent> Camera;
};
