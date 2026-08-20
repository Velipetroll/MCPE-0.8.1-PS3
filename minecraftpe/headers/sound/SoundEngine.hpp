#pragma once
#include <_types.h>

// Subimos estas inclusiones para que nuestro motor PS3 conozca 'SoundDesc'
#include <util/Random.hpp>
#include <sound/SoundRepository.hpp>

// ==========================================
// RUTA PARA PS3 (Stub de audio silencioso)
// ==========================================
#if defined(__PS3__)

struct SoundSystemPS3 {
	SoundSystemPS3() {}
	virtual ~SoundSystemPS3() {}

	// Funciones que el .cpp exige para no crashear
	inline void playAt(const SoundDesc& desc, float x, float y, float z, float vol, float pitch) {}
	inline void setListenerAngle(float angle) {}
};
#define SS_SUPER_CLASS SoundSystemPS3

// ==========================================
// RUTA ORIGINAL (Windows, Linux, Android)
// ==========================================
#elif defined(__WIN32__)
#include <sound/SoundSystemDirectSound.hpp>
#define SS_SUPER_CLASS SoundSystemDirectSound
#elif not defined(ANDROID) and defined(__linux__)
#include <sound/SoundSystemAL.hpp>
#define SS_SUPER_CLASS SoundSystemAL
#else
#include <sound/SoundSystemSL.hpp>
#define SS_SUPER_CLASS SoundSystemSL
#endif

struct SoundEngine : public SS_SUPER_CLASS{
	struct Options* options;
	int32_t field_40;
	Random randomInstance;
	float field_A14;
	float field_A18;
	float field_A1C;
	float field_A20;
	float field_A24;
	SoundRepository sounds;
	struct Minecraft* minecraft;

	SoundEngine(float);
	float _getVolumeMult(float, float, float);
	void destroy(void);
	virtual void enable(bool_t);
	void init(struct Minecraft*, struct Options*);
	void play(const std::string&, float, float, float, float, float);
	void playUI(const std::string&, float, float);
	void update(struct Mob*, float);
	void updateOptions(void);
	~SoundEngine();
};
