#include <utils.h>
#include <_types.h>
#include <sys/time.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <stdint.h>

time_t startedAtSec = 0;
static uint64_t start_time_ms = 0;
static int time_initialized = 0;

// Obtiene los milisegundos reales del sistema en 64 bits para evitar overflow
static uint64_t get_current_ms() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (uint64_t)tv.tv_sec * 1000ULL + (uint64_t)tv.tv_usec / 1000ULL;
}

double getTimeS() {
	if (!time_initialized) {
		start_time_ms = get_current_ms();
		time_initialized = 1;
	}
	// Devuelve los segundos transcurridos desde que se inicio el juego
	return (double)(get_current_ms() - start_time_ms) / 1000.0;
}

time_t getEpochTimeS() {
	return time(0);
}

int32_t getTimeMs() {
	if (!time_initialized) {
		start_time_ms = get_current_ms();
		time_initialized = 1;
	}
	// Devuelve los milisegundos desde el inicio.
	// Un int32_t soporta 24 dias de juego continuo antes de crashear.
	return (int32_t)(get_current_ms() - start_time_ms);
}

// Por si alguna parte del código (Minecraft.cpp) usa getRawTimeS indirectamente
int32_t getRawTimeS() {
	return (int32_t)time(0);
}

int32_t getRemainingFileSize(FILE* file){
	if(file){
		int32_t cur = ftell(file);
		fseek(file, 0, SEEK_END);
		int32_t end = ftell(file);
		fseek(file, cur, SEEK_SET);
		return end - cur;
	}
	return 0;
}

void sleepMs(int32_t a1) {
	usleep(1000 * a1);
}
