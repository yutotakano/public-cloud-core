#include <pthread.h>

#define RESET "\x1B[0m"
#define RED     "\033[1m\033[31m"
#define GREEN   "\033[1m\033[32m"
#define YELLOW  "\033[1m\033[33m"
#define BLUE    "\033[1m\033[34m"
#define CYAN	"\033[1m\033[36m"

#define printOK(...)	printf("[%sOK%s](%s:%d): ", GREEN, RESET, __FILE__, __LINE__); printf(__VA_ARGS__);
#define printWarning(...)	printf("[%sWARNING%s](%s:%d): ", YELLOW, RESET, __FILE__, __LINE__); printf(__VA_ARGS__);
#define printError(...) printf("[%sERROR%s](%s:%d): ", RED, RESET, __FILE__, __LINE__); printf(__VA_ARGS__);
#define printInfo(...) printf("[%sINFO%s](%s:%d): ", BLUE, RESET, __FILE__, __LINE__); printf(__VA_ARGS__);
#define printThread(...) printf("[%sThread ID: %d%s](%s:%d): ", CYAN, (int)pthread_self(), RESET, __FILE__, __LINE__); printf(__VA_ARGS__);
