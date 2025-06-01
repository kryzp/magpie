#pragma once

#include <glm/glm.hpp>

#include "math/colour.h"

#include "shadow_map_atlas.h"

namespace mgp
{
	class Image;

	constexpr static unsigned MAX_POINT_LIGHTS = 16;

	struct GPU_PointLight
	{
		glm::vec4 position; // [x,y,z]: position, [w]: unused
		glm::vec4 colour; // [x,y,z]: colour, [w]: 0/1 depending on shadows
		glm::vec4 atlasRegion; // [x,y]: top left, [z,w]: width, height
		glm::mat4 lightSpaceMatrix;
	};

	using LightId = unsigned;

	class Light
	{
		friend class LightHandle;

	public:
		enum LightType
		{
			TYPE_POINT
		};

		Light();
		~Light();

		LightType getType() const;
		void setType(LightType type);

		const Colour &getColour() const;
		void setColour(const Colour &colour);

		bool isShadowCaster() const;
		void toggleShadows(bool enabled);

		const ShadowMapAtlas::AtlasRegion &getShadowAtlasRegion() const;
		void setShadowAtlasRegion(const ShadowMapAtlas::AtlasRegion &region);

		float getFalloff() const;
		void setFalloff(float falloff);

		float getIntensity() const;
		void setIntensity(float intensity);

		const glm::vec3 &getDirection() const;
		void setDirection(const glm::vec3 &direction);

		const glm::vec3 &getPosition() const;
		void setPosition(const glm::vec3 &position);

	private:
		LightType m_type;

		Colour m_colour;

		bool m_shadowsEnabled;
		ShadowMapAtlas::AtlasRegion m_shadowAtlasRegion;

		float m_intensity;
		float m_falloff;

		glm::vec3 m_direction;
		glm::vec3 m_position;
	};

	/*
	template <typename T, typename TID = uint32_t>
	class Handle
	{
	public:
		Handle();
		Handle(const T *object);
		Handle(TID id);
		Handle(const Handle &other);
		Handle(Handle &&other) noexcept;
		Handle &operator = (const Handle &other);
		Handle &operator = (Handle &&other) noexcept;

		~Handle() = default;

		bool isValid() const;
		operator bool () const;

		TID id() const;
		operator TID () const;

		T *get();
		const T *get() const;

		T *operator -> ();
		const T *operator -> () const;

		bool operator == (const Handle& other) const;
		bool operator != (const Handle& other) const;

	private:
		TID m_id;
	};
	*/
}
