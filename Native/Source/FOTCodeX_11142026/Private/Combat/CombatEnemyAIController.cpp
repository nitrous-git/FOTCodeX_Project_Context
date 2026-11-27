#include "Combat/CombatEnemyAIController.h"

#include "Combat/CombatActionRestrictionComponent.h"
#include "Navigation/PathFollowingComponent.h"

bool ACombatEnemyAIController::RunBehaviorTree(UBehaviorTree* BTAsset)
{
	UCombatActionRestrictionComponent::SynchronizeBlackboard(GetPawn());
	const bool bStarted = Super::RunBehaviorTree(BTAsset);
	UCombatActionRestrictionComponent::SynchronizeBlackboard(GetPawn());
	return bStarted;
}

bool ACombatEnemyAIController::InitializeBlackboard(UBlackboardComponent& BlackboardComp, UBlackboardData& BlackboardAsset)
{
	const bool bInitialized = Super::InitializeBlackboard(BlackboardComp, BlackboardAsset);
	if (bInitialized)
	{
		UCombatActionRestrictionComponent::SynchronizeBlackboard(GetPawn(), &BlackboardComp);
	}
	return bInitialized;
}

FPathFollowingRequestResult ACombatEnemyAIController::MoveTo(const FAIMoveRequest& MoveRequest, FNavPathSharedPtr* OutPath)
{
	if (UCombatActionRestrictionComponent::IsActorMovementRestricted(GetPawn()))
	{
		if (OutPath)
		{
			OutPath->Reset();
		}
		return {};
	}
	return Super::MoveTo(MoveRequest, OutPath);
}

FAIRequestID ACombatEnemyAIController::RequestMove(const FAIMoveRequest& MoveRequest, FNavPathSharedPtr Path)
{
	return UCombatActionRestrictionComponent::IsActorMovementRestricted(GetPawn())
		? FAIRequestID::InvalidRequest : Super::RequestMove(MoveRequest, MoveTemp(Path));
}
