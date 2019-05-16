
#pragma once

#include "intrin.h"
#include "synchapi.h"


class CNetSpinLock
{
public:
	CNetSpinLock(long InSpinTimeInMilliSeconds = 1)
		: LockValue(0)
		, SleepInMilliSeconds(InSpinTimeInMilliSeconds)
	{
	}

	~CNetSpinLock()
	{
		// A spin lock should never be destroyed while others are blocked on it!
		Unlock();
	}

	/** Locks the primitive so no one can proceed past a block point */
	void Lock()
	{
		_InterlockedExchange(&LockValue, 1);
	}

	/** Unlocks the primitive so other threads can proceed past a block point */
	void Unlock()
	{
		_InterlockedExchange(&LockValue, 0);
	}

	void EnforceLock()
	{
		while (_InterlockedCompareExchange(&LockValue, 1, 0) != 0)
		{
			Sleep(SleepInMilliSeconds);
		}
	}

	/** Blocks this thread until the primitive is unlocked */
	void BlockUntilUnlocked() const
	{
		while (LockValue != 0)
		{
			Sleep(SleepInMilliSeconds);
		}
	}

	/** Queries the state of the spin lock */
	bool IsLocked() const
	{
		return LockValue != 0;
	}

private:
	/** Thread safe lock value */
	volatile long LockValue;

	/** The time spent sleeping between queries while blocked */
	long SleepInMilliSeconds;
};
