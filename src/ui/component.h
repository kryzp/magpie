#ifndef UI_COMPONENT_H
#define UI_COMPONENT_H

#include "ui/animation.h"
#include "render/canvas.h"
#include "container/vector.h"

#include <memory>

namespace os {
struct InputState;
}

namespace rnd {
class Font;
}

namespace ui {

class Component;

struct RenderContext {
	rnd::Canvas *canvas = nullptr;
	rnd::Font *font = nullptr;
	const rnd::Image *white_pixel = nullptr;
};

class Constraint {
public:
	enum Kind {
		Kind_None,
		Kind_Value, //  . . . a fixed value
		Kind_Relative, // . . a fraction of the parent's size
		Kind_Centre, // . . . centred in the parent, plus an offset
		Kind_Fit, //  . . . . the component's preferred size
		Kind_Bottom, // . . . centred on the parent's top edge (y up)
	};

	Constraint();

	static Constraint Value(float value);
	static Constraint Relative(float fraction);
	static Constraint Centre();
	static Constraint Centre(float offset);
	static Constraint Fit();
	static Constraint Bottom();

	bool IsSet() const;

	float ConstrainX(const Component &c) const;
	float ConstrainY(const Component &c) const;
	float ConstrainWidth(const Component &c) const;
	float ConstrainHeight(const Component &c) const;
	float ConstrainRotation(const Component &c) const;

private:
	Constraint(Kind kind, float param);

	Kind kind;
	float param;
};

struct Constraints {
	Constraint x;
	Constraint y;
	Constraint width;
	Constraint height;
	Constraint rotation;

	static Constraints Values(float x, float y, float width, float height);
	static Constraints Fill();

	void Constrain(Component *component) const;

	Constraints &SetX(const Constraint &c);
	Constraints &SetY(const Constraint &c);
	Constraints &SetWidth(const Constraint &c);
	Constraints &SetHeight(const Constraint &c);
	Constraints &SetRotation(const Constraint &c);
};

class Component {
public:
	Component();
	virtual ~Component();

	Component(const Component &) = delete;
	Component &operator=(const Component &) = delete;

	virtual void Update(const os::InputState &input, float dt);
	virtual void Render(const os::InputState &input, const RenderContext &context);

	virtual float GetPreferredWidth() const;
	virtual float GetPreferredHeight() const;

	Component *AddComponent(Unique<Component> component, const Constraints &constraints);
	Unique<Component> RemoveComponent(Component *component);
	
	void Clear();

	bool IsMouseInSector(Vec2 mouse_position, float theta, float radius, int segments) const;
	bool IsMouseHovering(Vec2 mouse_position) const;

	void ToggleEnabled(bool enabled);
	bool IsEnabled() const;

	Animator &GetAnimator();

	float GetParentWidth() const;
	float GetParentHeight() const;

	float GetRenderX() const;
	float GetRenderY() const;
	float GetRenderWidth() const;
	float GetRenderHeight() const;
	float GetRenderRotation() const;
	float GetRenderRotationDeg() const;
	float GetRenderScale() const;
	float GetRenderAlpha() const;

	float GetX() const;
	void SetX(float x);

	float GetY() const;
	void SetY(float y);

	float GetWidth() const;
	void SetWidth(float width);

	float GetHeight() const;
	void SetHeight(float height);

	float GetRotation() const;
	void SetRotation(float rotation);

	float GetScale() const;
	void SetScale(float scale);

	float GetAlpha() const;
	void SetAlpha(float alpha);

protected:
	// when added under a parent.
	virtual void Init();

private:
	struct Child {
		Unique<Component> component;
		Constraints constraints;
	};

	Component *parent;
	Vector<Child> children;

	Animator animator;

	float x;
	float y;
	float width;
	float height;

	float rotation;
	float scale;

	float alpha;

	bool enabled;
};

}

#endif // UI_COMPONENT_H
