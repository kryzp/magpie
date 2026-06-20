#ifndef ENTITY_MODULE_EMOTION_H
#define ENTITY_MODULE_EMOTION_H

typedef struct EM_EmotionPersonality EM_EmotionPersonality;
struct EM_EmotionPersonality
{
	f32 courage;
};

typedef struct EM_EmotionSensors EM_EmotionSensors;
struct EM_EmotionSensors
{
	f32 confidence;
	f32 worry;
	f32 fear;
	f32 disgust;
};

typedef struct EM_EmotionState EM_EmotionState;
struct EM_EmotionState
{
	f32 adrenaline;
};

EM_EmotionState EM_EmotionEval(const EM_EmotionPersonality *personality,
							   const EM_EmotionSensors *sensors);

#endif // ENTITY_MODULE_EMOTION_H
