// Copyright (c) 2025 UnForge. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Async/Async.h"               // UE5.x provides AsyncTask + ENamedThreads here
#include "Async/TaskGraphInterfaces.h" // FFunctionGraphTask (belt-and-suspenders)

FORCEINLINE void SAL_RunOnGameThread(TFunction<void()> Fn)
{
#if ENGINE_MAJOR_VERSION >= 5
	// Available in UE 5.0+
	AsyncTask(ENamedThreads::GameThread, MoveTemp(Fn));
#else
	FFunctionGraphTask::CreateAndDispatchWhenReady(
		MoveTemp(Fn), TStatId(), nullptr, ENamedThreads::GameThread);
#endif
}
