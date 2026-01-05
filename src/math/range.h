#pragma once

struct Range {
	float min;
	float max;

	Range()
		: min(0.0f)
		, max(0.0f)
	{
	}

	Range(float min, float max)
		: min(min)
		, max(max)
	{
	}

	float mean() const
	{
		return (min + max) * 0.5f;
	}

	float deviation() const
	{
		return (max - min) * 0.5f;
	}

	float random() const
	{
		return 0.f;
		//return Game::get_singleton()->get_random().next_single(min, max);
	}
};
