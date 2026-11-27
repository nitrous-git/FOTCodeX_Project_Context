#pragma once

#include "AIController.h"
#include "CombatEnemyAIController.generated.h"

/** Native movement and Blackboard gates for the standard enemy controller Blueprint. */
UCLASS()
class FOTCODEX_11142026_API ACombatEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	virtual bool RunBehaviorTree(UBehaviorTree* BTAsset) override;
	virtual FPathFollowingRequestResult MoveTo(const FAIMoveRequest& MoveRequest, FNavPathSharedPtr* OutPath = nullptr) override;
	virtual FAIRequestID RequestMove(const FAIMoveRequest& MoveRequest, FNavPathSharedPtr Path) override;

protected:
	virtual bool InitializeBlackboard(UBlackboardComponent& BlackboardComp, UBlackboardData& BlackboardAsset) override;
};
