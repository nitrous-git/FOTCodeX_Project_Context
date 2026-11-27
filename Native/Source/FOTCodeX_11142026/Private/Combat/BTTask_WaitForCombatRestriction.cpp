#include "Combat/BTTask_WaitForCombatRestriction.h"

UBTTask_WaitForCombatRestriction::UBTTask_WaitForCombatRestriction()
{
	NodeName = TEXT("Wait for Combat Restriction");
}

EBTNodeResult::Type UBTTask_WaitForCombatRestriction::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	return EBTNodeResult::InProgress;
}
