#ifndef ENTITY_SCENE_LAYER_H
#define ENTITY_SCENE_LAYER_H

/*
 * A scene layer can literally be anything.
 *
 * It's just a way of "grouping" together entities for
 * more involved manipulation of the world.
 *
 * So for instance you have an "overworld" layer, but
 * if a scripted scene happens (e.g: a car chase)
 * then you also have another "car_chase_road" layer
 * which you specifically activate then and maybe
 * do some stuff to "overworld" to "pacify" it while
 * the car chase event happens.
 *
 * Each entity lives on a specific layer, and if it
 * is / isn't on it determines if it gets ticked
 * and / or rendered if that layer is active or not.
 *
 * TODO: Maybe rename to "SceneGroup" or "SceneBucket" ?
 */

typedef struct E_SceneLayer E_SceneLayer;
struct E_SceneLayer
{
	String8 name;
	b32 active;
	b32 visible;
};

#endif // ENTITY_SCENE_LAYER_H
