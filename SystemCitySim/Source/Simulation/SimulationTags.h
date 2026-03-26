
#pragma once
#include "GameplayTagContainer.h"

struct FSimTags
{
    static FGameplayTag Need_Hunger;
    static FGameplayTag Need_Energy;
    static FGameplayTag Need_Social;
    static FGameplayTag Need_Caffeine;

    static FGameplayTag Type_Home;
    static FGameplayTag Type_Office;
    static FGameplayTag Type_Cafe;
    static FGameplayTag Type_Shop;
    static FGameplayTag Type_Park;
    static FGameplayTag Type_Bar;
    static FGameplayTag Type_Industrial;

    static void Init()
    {
        Need_Hunger = FGameplayTag::RequestGameplayTag("Need.Hunger");
        Need_Energy = FGameplayTag::RequestGameplayTag("Need.Energy");
        Need_Social = FGameplayTag::RequestGameplayTag("Need.Social");
        Need_Caffeine = FGameplayTag::RequestGameplayTag("Need.Caffeine");

        Type_Home = FGameplayTag::RequestGameplayTag("Type.Home");
        Type_Office = FGameplayTag::RequestGameplayTag("Type.Office");
        Type_Cafe = FGameplayTag::RequestGameplayTag("Type.Cafe");
        Type_Shop = FGameplayTag::RequestGameplayTag("Type.Shop");
        Type_Park = FGameplayTag::RequestGameplayTag("Type.Park");
        Type_Bar = FGameplayTag::RequestGameplayTag("Type.Bar");
        Type_Industrial = FGameplayTag::RequestGameplayTag("Type.Industrial");
    }
};

