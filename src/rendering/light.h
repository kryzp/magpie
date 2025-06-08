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
		glm::vec4 position; // [x,y,z]: position
		glm::vec4 colour; // [x,y,z]: colour, [w]: intensity
		glm::vec4 attenuation; // [x]*dist^2 + [y]*dist + [z], [w]: has shadows? 0/1
//		glm::mat4 lightSpaceMatrix;
//		glm::uvec4 shadowMap_id;
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

		Light() = default;
		~Light() = default;

		LightType getType() const { return m_type; }
		void setType(LightType type) { m_type = type; }

		const Colour &getColour() const { return m_colour; }
		void setColour(const Colour &colour) { m_colour = colour; }

		bool isShadowCaster() const { return m_shadowsEnabled; }
		void toggleShadows(bool enabled) { m_shadowsEnabled = enabled; }

//		const ShadowMapAtlas::AtlasRegion &getShadowAtlasRegion() const;
//		void setShadowAtlasRegion(const ShadowMapAtlas::AtlasRegion &region);

		float getFalloff() const { return m_falloff; }
		void setFalloff(float falloff) { m_falloff = falloff; }

		float getIntensity() const { return m_intensity; }
		void setIntensity(float intensity) { m_intensity = intensity; }

		const glm::vec3 &getDirection() const { return m_direction; }
		void setDirection(const glm::vec3 &direction) { m_direction = direction; }

		const glm::vec3 &getPosition() const { return m_position; }
		void setPosition(const glm::vec3 &position) { m_position = position; }

	private:
		LightType m_type;

		Colour m_colour;

		bool m_shadowsEnabled;
//		ShadowMapAtlas::AtlasRegion m_shadowAtlasRegion;

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
