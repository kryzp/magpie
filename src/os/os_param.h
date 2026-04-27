#ifndef OS_PARAM_H
#define OS_PARAM_H

// The total amount of memory
// allocated to the App.
#define OS_PROCESS_MEMORY              Gigabytes(12)

// The total amount of memory
// allocated to the Platform Layer.
#define OS_LAYER_MEMORY                Megabytes(512)

#define OS_TOTAL_MEMORY                (OS_PROCESS_MEMORY + OS_LAYER_MEMORY)

#define OS_ENGINE_NAME                "Magpie"
#define OS_DEFAULT_WINDOW_TITLE       "Magpie Demo"

#define OS_DEFAULT_WINDOW_WIDTH        1280
#define OS_DEFAULT_WINDOW_HEIGHT       720

#define OS_APP_VERSION_MAJOR           0
#define OS_APP_VERSION_MINOR           1
#define OS_APP_VERSION_PATCH           0

#define OS_ENGINE_VERSION_MAJOR        0
#define OS_ENGINE_VERSION_MINOR        1
#define OS_ENGINE_VERSION_PATCH        0

#endif // OS_PARAM_H
