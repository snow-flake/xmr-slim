#pragma once

class executor;

namespace xmrstak
{

struct globalStates;

struct environment
{
	static inline environment& inst(environment* init = nullptr)
	{
		static environment* env = nullptr;

		if(env == nullptr)
		{
			if(init == nullptr)
				env = new environment;
			else
				env = init;
		}

		return *env;
	}

	environment()
	{
	}

	globalStates* pglobalStates = nullptr;
	executor* pExecutor = nullptr;
};

} // namepsace xmrstak
