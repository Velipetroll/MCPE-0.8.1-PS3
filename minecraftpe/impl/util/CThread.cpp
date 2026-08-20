#include <util/CThread.hpp>
#include <unistd.h>
#include <stdio.h> // Para printf

CThread::CThread(void* (*func)(void*), void* args) {
	printf("\n==================================\n"); fflush(stdout);
	printf("[CThread] INICIANDO NUEVO HILO...\n"); fflush(stdout);
	this->function = func;

	#ifdef EVILOREOBROKEPTHREADS
	pthread_mutex_init(&this->lock, 0);
	pthread_mutex_lock(&this->lock);
	#endif

	pthread_attr_init(&this->field_C);

	// 1. Lo marcamos como JOINABLE (0) en vez de DETACHED (1) para que la consola
	// no crashee cuando Minecraft llame a CThread::join() al salir del mundo.
	pthread_attr_setdetachstate(&this->field_C, 0);

	// 2. [VITAL] ¡INCREMENTAMOS EL STACK DEL HILO A 4 MB!
	// Esto evita el Stack Overflow que causa el "std::bad_alloc" al generar terreno.
	pthread_attr_setstacksize(&this->field_C, 4 * 1024 * 1024);

	printf("[CThread] Ejecutando pthread_create con Stack de 4MB...\n"); fflush(stdout);
	int res = pthread_create(&this->field_8, &this->field_C, this->function, args);

	if (res == 0) {
		printf("[CThread] EXITO: Hilo corriendo perfectamente.\n"); fflush(stdout);
	} else {
		printf("[CThread] ERROR FATAL: pthread_create fallo con codigo %d\n", res); fflush(stdout);
	}
	printf("==================================\n\n"); fflush(stdout);
}

int32_t CThread::join() {
	printf("[CThread] Esperando a que el hilo termine (join)...\n"); fflush(stdout);
	return pthread_join(this->field_8, 0);
}

int32_t CThread::sleep(uint32_t a2) {
	return usleep(1000 * a2);
}

CThread::~CThread() {
	printf("[CThread] Destruyendo hilo...\n"); fflush(stdout);
	#ifdef EVILOREOBROKEPTHREADS
	pthread_mutex_unlock(&this->lock);
	#endif

	// Limpieza segura de memoria
	pthread_join(this->field_8, 0);
	pthread_attr_destroy(&this->field_C);

	#ifdef EVILOREOBROKEPTHREADS
	pthread_mutex_destroy(&this->lock);
	#endif
}
