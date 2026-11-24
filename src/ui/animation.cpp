#include "ui/animation.h"

#include <algorithm>

ui::Transition::Transition()
{
}

void ui::Transition::Apply(float t, AnimationState *out) const
{
	if (x)
		out->x += x(t);

	if (y)
		out->y += y(t);

	if (width)
		out->width += width(t);

	if (height)
		out->height += height(t);

	if (rotation)
		out->rotation += rotation(t);

	if (scale)
		out->scale *= scale(t);

	if (alpha)
		out->alpha *= alpha(t);
}

ui::Transition &ui::Transition::XDriver(const Driver &driver)
{
	x = driver;
	return *this;
}

ui::Transition &ui::Transition::YDriver(const Driver &driver)
{
	y = driver;
	return *this;
}

ui::Transition &ui::Transition::WidthDriver(const Driver &driver)
{
	width = driver;
	return *this;
}

ui::Transition &ui::Transition::HeightDriver(const Driver &driver)
{
	height = driver;
	return *this;
}

ui::Transition &ui::Transition::RotationDriver(const Driver &driver)
{
	rotation = driver;
	return *this;
}

ui::Transition &ui::Transition::ScaleDriver(const Driver &driver)
{
	scale = driver;
	return *this;
}

ui::Transition &ui::Transition::AlphaDriver(const Driver &driver)
{
	alpha = driver;
	return *this;
}

ui::Animator::Animator()
{
}

void ui::Animator::AddChannel(const String &name, float rate, const Transition &transition)
{
	Channel c;
	c.transition = transition;
	c.rate = rate;
	c.progress = 0.f;
	c.target = 0.f;

	channels[name] = c;
}

void ui::Animator::SetTarget(const String &name, bool active)
{
	auto it = channels.find(name);

	if (it == channels.end())
		return;

	if (active)
		it->second.target = 1.f;
	else
		it->second.target = 0.f;
}

void ui::Animator::Update(float dt)
{
	state = {};

	for (auto &pair : channels) {
		Channel &c = pair.second;
		float step = c.rate * dt;

		if (c.progress < c.target)
			c.progress = std::min(c.progress + step, c.target);
		else
			c.progress = std::max(c.progress - step, c.target);

		c.transition.Apply(c.progress, &state);
	}
}

ui::AnimationState &ui::Animator::GetAnimationState()
{
	return state;
}

const ui::AnimationState &ui::Animator::GetAnimationState() const
{
	return state;
}
