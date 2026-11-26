#include "Combat/Status/CombatStatusEffectDefinition.h"

TConstArrayView<FName> UCombatStatusEffectDefinition::GetKnownStatusTypes()
{
	static const FName Types[] = { FName(TEXT("Fire")), FName(TEXT("Bleed")), FName(TEXT("Lightning")) };
	return Types;
}

TArray<FString> UCombatStatusEffectDefinition::GetStatusTypes() const
{
	TArray<FString> Options;
	for (const FName Type : GetKnownStatusTypes())
	{
		Options.Add(Type.ToString());
	}
	return Options;
}
