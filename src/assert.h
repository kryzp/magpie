
#define Assert(s) do{if(!(s)){*(int*)0=0;}}while(0)
#define DebugLog(m, ...) do{printf((m "\n"),##__VA_ARGS__);}while(0)
#define DebugLogCrash(m, ...) do{DebugLog(m,##__VA_ARGS__);Assert(0);}while(0)
