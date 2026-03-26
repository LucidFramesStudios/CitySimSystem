#pragma once
#include "CoreMinimal.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSimulation, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogAgentDebug, Log, All);

#define SIM_LOG_ERROR(AgentID, Format, ...) UE_LOG(LogSimulation, Error, TEXT("[Agent %d] " Format), AgentID, ##__VA_ARGS__)
#define SIM_LOG_WARNING(AgentID, Format, ...) UE_LOG(LogSimulation, Warning, TEXT("[Agent %d] " Format), AgentID, ##__VA_ARGS__)
#define SIM_LOG_INFO(AgentID, Format, ...) UE_LOG(LogSimulation, Log, TEXT("[Agent %d] " Format), AgentID, ##__VA_ARGS__)
#define SIM_LOG_VERBOSE(AgentID, Format, ...) UE_LOG(LogSimulation, Verbose, TEXT("[Agent %d] " Format), AgentID, ##__VA_ARGS__)