// Copyright (c) 2025 UnForge. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Async/Async.h"               
#include "Async/TaskGraphInterfaces.h" 
#include "Misc/EngineVersionComparison.h"
#include "TimerManager.h" 

FORCEINLINE void SAL_RunOnGameThread(TFunction<void()> Fn)
{
#if ENGINE_MAJOR_VERSION >= 5
	AsyncTask(ENamedThreads::GameThread, MoveTemp(Fn));
#else
	FFunctionGraphTask::CreateAndDispatchWhenReady(
		MoveTemp(Fn), TStatId(), nullptr, ENamedThreads::GameThread);
#endif
}
