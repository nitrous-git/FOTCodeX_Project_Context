#pragma once

#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_WaitForCombatRestriction.generated.h"

/** The observing decorator aborts this task when combat is allowed again. */
UCLASS()
class FOTCODEX_11142026_API UBTTask_WaitForCombatRestriction : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_WaitForCombatRestriction();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
