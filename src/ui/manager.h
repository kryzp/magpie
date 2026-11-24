#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include "ui/component.h"

namespace ui {

// todo: is Root a better term?
class Manager {
public:
	Manager();

	void Update(const os::InputState &input, float dt);
	void Render(const os::InputState &input, const RenderContext &context);

	void Clear();

	void Resize(float width, float height);

	Component *AddMenu(Unique<Component> element);
	Component *AddElement(Unique<Component> element, const Constraints &constraints);

	Component *GetCanvas();

private:
	Component canvas;
};

}

#endif // UI_MANAGER_H
