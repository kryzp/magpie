// idk if this works just a quick draft

#define PARTICLE_COUNT 1

struct PushConstants
{
	float deltaTime;
};

[[vk::push_constant]]
PushConstants pushConstants;

struct ParameterUBO
{
	float4x4 projMatrix;
	float4x4 inverseProjMatrix;
	float4x4 inverseViewMatrix;
	float4x4 inversePrevViewMatrix;
	float4x4 prevViewMatrix;
};

ConstantBuffer<ParameterUBO> frameData : register(b0);

struct Particle
{
	float3 position;
	float _padding0;
	float3 velocity;
	float _padding1;
};

RWStructuredBuffer<Particle> particles : register(u1); // we can just use one buffer since particles don't influence each other

Texture2D motionTexture : register(t2);
Texture2D normalTexture : register(t3);
Texture2D depthTexture : register(t4);

SamplerState motionTextureSampler : register(s2);
SamplerState normalTextureSampler : register(s3);
SamplerState depthTextureSampler : register(s4);

static const float3 GRAVITY = float3(0.0, -3.0, 0.0);
static const float TOUCH_DISTANCE = 0.1;

float2 toScreenPosition(float3 worldPosition, float4x4 vm)
{
	float4 clipSpacePos = mul(frameData.projMatrix, mul(vm, float4(worldPosition, 1.0)));
	float3 ndcPosition = clipSpacePos.xyz / clipSpacePos.w;
	return 0.5*ndcPosition.xy + 0.5;
}

float3 toWorldPosition(float2 screenPosition, float4x4 ivm)
{
	float depth = depthTexture.Sample(depthTextureSampler, float2(screenPosition.x, 1.0 - screenPosition.y)).r;
	float4 coord = mul(ivm, mul(frameData.inverseProjMatrix, float4(2.0 * screenPosition - 1.0, depth, 1.0)));
	return coord.xyz / coord.w;
}

float distanceFromSurface(float3 position)
{
	float2 screenPosition = toScreenPosition(position, frameData.prevViewMatrix);
	
	float3 normal = normalTexture.Sample(normalTextureSampler, float2(screenPosition.x, 1.0 - screenPosition.y)).xyz;
	normal = 2.0 * normal - 1.0;
	normal = normalize(normal);
	
	float3 projectedSurfacePosition = toWorldPosition(screenPosition, frameData.inversePrevViewMatrix);
	return abs(dot(normal, position - projectedSurfacePosition));
}

[numthreads(PARTICLE_COUNT, 1, 1)]
void main(uint3 GIID : SV_DispatchThreadID)
{
	uint idx = GIID.x;
	
	Particle particle = particles[idx];

	float3 position = particle.position;
	float2 screenPosition = toScreenPosition(position, frameData.prevViewMatrix);

	bool onSurface = distanceFromSurface(position) <= TOUCH_DISTANCE;

	if (onSurface)
	{
		float2 naiveMotion = motionTexture.Sample(motionTextureSampler, float2(screenPosition.x, 1.0 - screenPosition.y)).xy;
		float2 naiveScreenPosition = screenPosition + naiveMotion;

		float2 correctionMotion = motionTexture.Sample(motionTextureSampler, float2(naiveScreenPosition.x, 1.0 - naiveScreenPosition.y)).xy;
		float2 newScreenPosition = screenPosition + correctionMotion;

		float3 newPosition = toWorldPosition(newScreenPosition, frameData.inverseViewMatrix);
		float3 curPosition = position;

		particle.position = newPosition;
		particle.velocity = (newPosition - curPosition) / pushConstants.deltaTime;
	}
	else
	{
		particle.position += particle.velocity * pushConstants.deltaTime + 0.5 * GRAVITY * pushConstants.deltaTime * pushConstants.deltaTime;
		particle.velocity += GRAVITY * pushConstants.deltaTime;
	}

	particles[idx] = particle;
}
