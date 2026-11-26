#include "ui/component.h"
#include "core/maths.h"

#include <cmath>

ui::Constraint::Constraint()
	: kind(Kind_None)
	, param(0.f)
{
}

ui::Constraint::Constraint(Kind kind_, float param_)
	: kind(kind_)
	, param(param_)
{
}

ui::Constraint ui::Constraint::Value(float value)
{
	return Constraint(Kind_Value, value);
}

ui::Constraint ui::Constraint::Relative(float fraction)
{
	return Constraint(Kind_Relative, fraction);
}

ui::Constraint ui::Constraint::Centre()
{
	return Constraint(Kind_Centre, 0.f);
}

ui::Constraint ui::Constraint::Centre(float offset)
{
	return Constraint(Kind_Centre, offset);
}

ui::Constraint ui::Constraint::Fit()
{
	return Constraint(Kind_Fit, 0.f);
}

ui::Constraint ui::Constraint::Bottom()
{
	return Constraint(Kind_Bottom, 0.f);
}

bool ui::Constraint::IsSet() const
{
	return kind != Kind_None;
}

float ui::Constraint::ConstrainX(const Component &c) const
{
	switch (kind) {
		case Kind_Value:
			return param;

		case Kind_Relative:
			return c.GetParentWidth() * param;

		case Kind_Centre:
			return (c.GetParentWidth() - c.GetWidth()) * 0.5f + param;

		default:
			return c.GetX();
	}
}

float ui::Constraint::ConstrainY(const Component &c) const
{
	switch (kind) {
		case Kind_Value:
			return param;

		case Kind_Relative:
			return c.GetParentHeight() * param;

		case Kind_Centre:
			return (c.GetParentHeight() - c.GetHeight()) * 0.5f + param;

		case Kind_Bottom:
			return c.GetParentHeight() - c.GetHeight() * 0.5f;

		default:
			return c.GetY();
	}
}

float ui::Constraint::ConstrainWidth(const Component &c) const
{
	switch (kind) {
		case Kind_Value:
			return param;

		case Kind_Relative:
			return c.GetParentWidth() * param;

		case Kind_Fit:
			return c.GetPreferredWidth();

		default:
			return c.GetWidth();
	}
}

float ui::Constraint::ConstrainHeight(const Component &c) const
{
	switch (kind) {
		case Kind_Value:
			return param;

		case Kind_Relative:
			return c.GetParentHeight() * param;

		case Kind_Fit:
			return c.GetPreferredHeight();

		default:
			return c.GetHeight();
	}
}

float ui::Constraint::ConstrainRotation(const Component &c) const
{
	switch (kind) {
		case Kind_Value:
			return param;

		case Kind_Relative:
			return c.GetRotation() * param;

		default:
			return c.GetRotation();
	}
}

ui::Constraints ui::Constraints::Values(float x, float y, float width, float height)
{
	Constraints c;
	c.x = Constraint::Value(x);
	c.y = Constraint::Value(y);
	c.width = Constraint::Value(width);
	c.height = Constraint::Value(height);

	return c;
}

ui::Constraints ui::Constraints::Fill()
{
	Constraints c;
	c.x = Constraint::Value(0.f);
	c.y = Constraint::Value(0.f);
	c.width = Constraint::Relative(1.f);
	c.height = Constraint::Relative(1.f);

	return c;
}

void ui::Constraints::Constrain(Component *component) const
{
	if (width.IsSet())
		component->SetWidth(width.ConstrainWidth(*component));

	if (height.IsSet())
		component->SetHeight(height.ConstrainHeight(*component));

	if (x.IsSet())
		component->SetX(x.ConstrainX(*component));

	if (y.IsSet())
		component->SetY(y.ConstrainY(*component));

	if (rotation.IsSet())
		component->SetRotation(rotation.ConstrainRotation(*component));
}

ui::Constraints &ui::Constraints::SetX(const Constraint &c)
{
	x = c;
	return *this;
}

ui::Constraints &ui::Constraints::SetY(const Constraint &c)
{
	y = c;
	return *this;
}

ui::Constraints &ui::Constraints::SetWidth(const Constraint &c)
{
	width = c;
	return *this;
}

ui::Constraints &ui::Constraints::SetHeight(const Constraint &c)
{
	height = c;
	return *this;
}

ui::Constraints &ui::Constraints::SetRotation(const Constraint &c)
{
	rotation = c;
	return *this;
}

ui::Component::Component()
	: parent(nullptr)
	, x(0.f)
	, y(0.f)
	, width(0.f)
	, height(0.f)
	, rotation(0.f)
	, scale(1.f)
	, alpha(1.f)
	, enabled(false)
{
}

ui::Component::~Component()
{
}

void ui::Component::Init()
{
}

void ui::Component::Update(const os::InputState &input, float dt)
{
	for (Child &child : children) {
		Component *component = child.component.get();

		if (!component->enabled)
			continue;

		child.constraints.Constrain(component);

		component->Update(input, dt);
		component->animator.Update(dt);
	}
}

void ui::Component::Render(const os::InputState &input, const RenderContext &context)
{
	for (Child &child : children) {
		Component *component = child.component.get();

		if (!component->enabled)
			continue;

		if (component->GetRenderAlpha() <= 0.f)
			continue;

		component->Render(input, context);
	}
}

float ui::Component::GetPreferredWidth() const
{
	return 0.f;
}

float ui::Component::GetPreferredHeight() const
{
	return 0.f;
}

ui::Component *ui::Component::AddComponent(Unique<Component> component, const Constraints &constraints)
{
	Component *c = component.get();

	c->parent = this;
	c->Init();

	Child child;
	child.component = std::move(component);
	child.constraints = constraints;
	children.push_back(std::move(child));

	constraints.Constrain(c);
	c->ToggleEnabled(true);

	return c;
}

Unique<ui::Component> ui::Component::RemoveComponent(Component *component)
{
	for (auto it = children.begin(); it != children.end(); ++it) {
		if (it->component.get() != component)
			continue;

		Unique<Component> removed = std::move(it->component);
		children.erase(it);

		removed->parent = nullptr;
		return removed;
	}

	return nullptr;
}

void ui::Component::Clear()
{
	children.clear();
}

bool ui::Component::IsMouseInSector(Vec2 mouse_position, float theta, float radius, int segments) const
{
	float cx = parent->GetRenderX() + parent->GetRenderWidth() * 0.5f;
	float cy = parent->GetRenderY() + parent->GetRenderHeight() * 0.5f;

	float dx = mouse_position.x - cx;
	float dy = mouse_position.y - cy;

	float dist_sq = dx * dx + dy * dy;

	float inner = radius;
	float outer = radius + GetRenderHeight();

	if (dist_sq < inner * inner || dist_sq > outer * outer)
		return false;

	float diff = std::atan2(dy, dx) - theta;
	diff -= maths::PI2f * std::floor((diff + maths::PIf) / maths::PI2f);

	return std::abs(diff) <= maths::PIf / static_cast<float>(segments);
}

bool ui::Component::IsMouseHovering(Vec2 mouse_position) const
{
	float w = GetRenderWidth();
	float h = GetRenderHeight();

	float dx = mouse_position.x - (GetRenderX() + w * 0.5f);
	float dy = mouse_position.y - (GetRenderY() + h * 0.5f);

	float rot = -GetRenderRotation();

	float cos = std::cos(rot);
	float sin = std::sin(rot);

	float local_x = dx * cos - dy * sin;
	float local_y = dx * sin + dy * cos;

	return std::abs(local_x) <= w * 0.5f && std::abs(local_y) <= h * 0.5f;
}

void ui::Component::ToggleEnabled(bool enabled_)
{
	enabled = enabled_;
}

bool ui::Component::IsEnabled() const
{
	return enabled;
}

ui::Animator &ui::Component::GetAnimator()
{
	return animator;
}

float ui::Component::GetParentWidth() const
{
	if (!parent)
		return 0.f;

	return parent->width;
}

float ui::Component::GetParentHeight() const
{
	if (!parent)
		return 0.f;

	return parent->height;
}

float ui::Component::GetRenderX() const
{
	float result = x + animator.GetAnimationState().x;

	if (parent)
		result += parent->GetRenderX();

	return result;
}

float ui::Component::GetRenderY() const
{
	float result = y + animator.GetAnimationState().y;

	if (parent)
		result += parent->GetRenderY();

	return result;
}

float ui::Component::GetRenderWidth() const
{
	return width + animator.GetAnimationState().width;
}

float ui::Component::GetRenderHeight() const
{
	return height + animator.GetAnimationState().height;
}

float ui::Component::GetRenderRotation() const
{
	return rotation + animator.GetAnimationState().rotation;
}

float ui::Component::GetRenderRotationDeg() const
{
	return GetRenderRotation() * maths::RAD_TO_DEGf;
}

float ui::Component::GetRenderScale() const
{
	return scale * animator.GetAnimationState().scale;
}

float ui::Component::GetRenderAlpha() const
{
	float result = alpha * animator.GetAnimationState().alpha;

	if (parent)
		result *= parent->GetRenderAlpha();

	return result;
}

float ui::Component::GetX() const
{
	return x;
}

void ui::Component::SetX(float x_)
{
	x = x_;
}

float ui::Component::GetY() const
{
	return y;
}

void ui::Component::SetY(float y_)
{
	y = y_;
}

float ui::Component::GetWidth() const
{
	return width;
}

void ui::Component::SetWidth(float width_)
{
	width = width_;
}

float ui::Component::GetHeight() const
{
	return height;
}

void ui::Component::SetHeight(float height_)
{
	height = height_;
}

float ui::Component::GetRotation() const
{
	return rotation;
}

void ui::Component::SetRotation(float rotation_)
{
	rotation = rotation_;
}

float ui::Component::GetScale() const
{
	return scale;
}

void ui::Component::SetScale(float scale_)
{
	scale = scale_;
}

float ui::Component::GetAlpha() const
{
	return alpha;
}

void ui::Component::SetAlpha(float alpha_)
{
	alpha = alpha_;
}
