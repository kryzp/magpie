#include "ui/manager.h"

ui::Manager::Manager()
{
	canvas.ToggleEnabled(true);
}

void ui::Manager::Update(const os::InputState &input, float dt)
{
	canvas.Update(input, dt);
}

void ui::Manager::Render(const os::InputState &input, const RenderContext &context)
{
	canvas.Render(input, context);
}

void ui::Manager::Clear()
{
	canvas.Clear();
}

void ui::Manager::Resize(float width, float height)
{
	canvas.SetX(0.f);
	canvas.SetY(0.f);
	canvas.SetWidth(width);
	canvas.SetHeight(height);
}

ui::Component *ui::Manager::AddMenu(Unique<Component> element)
{
	return AddElement(std::move(element), Constraints::Fill());
}

ui::Component *ui::Manager::AddElement(Unique<Component> element, const Constraints &constraints)
{
	return canvas.AddComponent(std::move(element), constraints);
}

ui::Component *ui::Manager::GetCanvas()
{
	return &canvas;
}
