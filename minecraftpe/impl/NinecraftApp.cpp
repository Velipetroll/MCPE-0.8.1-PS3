#include <I18n.hpp>
#include <NinecraftApp.hpp>
#include <cpputils.hpp>
#include <entity/LocalPlayer.hpp>
#include <entity/MobCategory.hpp>
#include <gui/Screen.hpp>
#include <input/Mouse.hpp>
#include <item/Item.hpp>
#include <level/Level.hpp>
#include <level/biome/Biome.hpp>
#include <level/storage/ExternalFileLevelStorageSource.hpp>
#include <math/Mth.hpp>
#include <network/RakNetInstance.hpp>
#include <network/ServerSideNetworkHandler.hpp>
#include <rendering/Font.hpp>
#include <rendering/GLBufferPool.hpp>
#include <rendering/GameRenderer.hpp>
#include <rendering/LevelRenderer.hpp>
#include <rendering/ParticleEngine.hpp>
#include <rendering/PerfRenderer.hpp>
#include <rendering/Tesselator.hpp>
#include <rendering/Textures.hpp>
#include <rendering/textures/FireTexture.hpp>
#include <rendering/textures/LavaSideTexture.hpp>
#include <rendering/textures/LavaTexture.hpp>
#include <rendering/textures/WaterSideTexture.hpp>
#include <rendering/textures/WaterTexture.hpp>
#include <tile/Tile.hpp>
#include <tile/entity/TileEntity.hpp>
#include <tile/material/Material.hpp>
#include <unigl.h>
#include <input/Multitouch.hpp>
#include <AppPlatform.hpp>
#include <stdio.h>

std::shared_ptr<TextureAtlas> NinecraftApp::_itemsTextureAtlas;
std::shared_ptr<TextureAtlas> NinecraftApp::_terrainTextureAtlas;
bool NinecraftApp::_hasInitedStatics = 0;

NinecraftApp::NinecraftApp(){
	this->field_D48___ = 1;
	this->field_D4C = 0;
	this->field_D50 = 0;
	this->field_D64 = 0;
}

std::shared_ptr<TextureAtlas> NinecraftApp::getTextureAtlas(TextureAtlasId atlas){
	if(atlas) return std::shared_ptr<TextureAtlas>(NinecraftApp::_itemsTextureAtlas);
	return std::shared_ptr<TextureAtlas>(NinecraftApp::_terrainTextureAtlas);
}
void NinecraftApp::handleBackNoReturn(void){
	if(this->field_CF4){
		if(this->currentScreen){
			this->currentScreen->handleBackEvent(this->field_CF4);
		}
	}
}
void NinecraftApp::initGLStates(){
	glEnable(0xBE2u);
	glEnable(0xB71u);
	glEnable(0xB44u);
	glEnable(0xDE1u);
	glDisable(0xB60u);
	glDisable(0xB50u);
	glDisable(0xBC0u);
	glDisable(0xB90u);
	glDisable(0x8037u);
	glDisable(0xB50u);
	glDepthFunc(GL_LEQUAL);
	#ifdef USEGLES
	glDepthRangef(0, 1.0);
	#else
	glDepthRange(0, 1.0);
	#endif
	glAlphaFunc(0x204u, 0.5);
	glCullFace(0x405u);
	glShadeModel(0x1D00u);
	glEnableClientState(0x8074u);
	glHint(0xC50u, 0x1101u);
	glHint(0xC54u, 0x1101u);
	glBlendFunc(0x302u, 0x303u);
	glDepthMask(1u);
	glStencilFunc(0x202u, 0, 0xFFu);
	glStencilMask(0xFFu);
	glLightModelf(0xB52u, 0.0);
	this->powerVR = AppPlatform::_singleton->isPowerVR();
}

void NinecraftApp::restartServer(){
	Level* levelPtr;
	ServerSideNetworkHandler* serverSideNetworkHandler;

	levelPtr = this->level;
	if(levelPtr) {
		for(int32_t i = levelPtr->playersMaybe.size() - 1; i >= 0; --i) {
			Player* player = levelPtr->playersMaybe.back();
			this->level->removePlayer(player);
			levelPtr->playersMaybe.pop_back();
		}
		this->rakNetInstance->resetIsBroken();
		this->gui.addMessage("server", "This server has restarted!", 200);
		this->hostMultiplayer(19132);
		if ( this->serverSideNetworkHandler )
		{
			serverSideNetworkHandler->levelGenerated(this->level);
		}
	}
}
void NinecraftApp::updateStats(){}
NinecraftApp::~NinecraftApp(void){ this->teardown(); }

bool_t NinecraftApp::onLowMemory(void){
	if(glBufferPool.unusedBuffers.empty()) return 0;
	while(glBufferPool.unusedBuffers.back() != glBufferPool.unusedBuffers.front()){
		uint32_t s = glBufferPool.unusedBuffers.at(0);
		glDeleteBuffers(1, &s);
		glBufferPool.unusedBuffers.pop_front();
	}
	return 1;
}
void NinecraftApp::onAppResumed(void){
	this->initGLStates();
	Tesselator::instance.init();
	Minecraft::onAppResumed();
}

void NinecraftApp::update(void){
	if(!this->some_std_vec.empty()){
		for(size_t v2 = 0; v2 < this->some_std_vec.size(); ++v2){
			this->handleBackNoReturn();
		}
		this->some_std_vec.clear();
	}
	++this->field_D4C;
	Multitouch::commit();

	Minecraft::update();

	Mouse::reset2();
	if(this->level){
		if(this->rakNetInstance->isProbablyBroken()){
			if(this->rakNetInstance->isServer()){
				this->restartServer();
			}
		}
	}
	this->updateStats();
}

bool_t NinecraftApp::handleBack(bool_t a2){
	if(!this->field_CF4){
		if(this->level){
			if(!a2) {
				if(!this->currentScreen) {
					this->pauseGame(1);
					return 0;
				}
				if(!this->currentScreen->handleBackEvent(0)) {
					if(this->player->currentContainer) {
						this->player->closeContainer();
					} else {
						this->setScreen(0);
					}
				}
			}
		}else{
			if(this->currentScreen){
				this->currentScreen->handleBackEvent(a2);
			}
		}
	}
	return 1;
}
void NinecraftApp::handleBack(void){
	this->some_std_vec.push_back(1);
}

void NinecraftApp::init(void){
	printf("[NinecraftApp] Configurando directorio base seguro...\n"); fflush(stdout);
	this->field_D00 = this->dataPathMaybe;

	printf("[NinecraftApp] Inicializando tablas matematicas...\n"); fflush(stdout);
	int32_t x = 0;
	do{
		float v7 = sin((float)x / 10430.0);
		Mth::_sin[x] = v7;
		++x;
	}while(x != 65536);

	if(!NinecraftApp::_hasInitedStatics){
		printf("[NinecraftApp] Cargando atlas de texturas...\n"); fflush(stdout);
		std::string v40 = "images/";
		NinecraftApp::_hasInitedStatics = 1;

		NinecraftApp::_terrainTextureAtlas = std::shared_ptr<TextureAtlas>(new TextureAtlas(v40+"terrain.meta"));
		NinecraftApp::_itemsTextureAtlas = std::shared_ptr<TextureAtlas>(new TextureAtlas(v40+"items.meta"));

		NinecraftApp::_terrainTextureAtlas->load(this);
		NinecraftApp::_itemsTextureAtlas->load(this);

		printf("[NinecraftApp] Inicializando Bloques e Items...\n"); fflush(stdout);
		Material::initMaterials();
		MobCategory::initMobCategories();
		Tile::initTiles(NinecraftApp::_terrainTextureAtlas);
		Item::initItems(NinecraftApp::_itemsTextureAtlas);
		Biome::initBiomes();
		TileEntity::initTileEntities();
	}

	printf("[NinecraftApp] initGLStates()...\n"); fflush(stdout);
	this->initGLStates();

	printf("[NinecraftApp] Tesselator::instance.init()...\n"); fflush(stdout);
	Tesselator::instance.init();

	printf("[NinecraftApp] Cargando idioma...\n"); fflush(stdout);
	I18n::loadLanguage(AppPlatform::_singleton, "en_US");

	printf("[NinecraftApp] Llamando a Minecraft::init()...\n"); fflush(stdout);
	Minecraft::init();

	printf("[NinecraftApp] Configurando almacenamiento...\n"); fflush(stdout);
	this->levelStorageSource = new ExternalFileLevelStorageSource(this->dataPathMaybe, this->field_CC4);
	this->field_CFC = 0;

	printf("[NinecraftApp] Creando texturas dinamicas...\n"); fflush(stdout);
	this->texturesPtr = new Textures(&this->options, AppPlatform::_singleton);
	this->texturesPtr->addDynamicTexture(new FireTexture());
	this->texturesPtr->addDynamicTexture(new WaterTexture(*NinecraftApp::_terrainTextureAtlas->getTextureItem("still_water")->getUV(0)));
	this->texturesPtr->addDynamicTexture(new WaterSideTexture());
	this->texturesPtr->addDynamicTexture(new LavaTexture());
	this->texturesPtr->addDynamicTexture(new LavaSideTexture(*NinecraftApp::_terrainTextureAtlas->getTextureItem("flowing_lava")->getUV(0)));
	this->gui.texturesLoaded(this->texturesPtr);
	this->field_190 = 0;

	printf("[NinecraftApp] Creando GameRenderer...\n"); fflush(stdout);
	this->levelRenderer = new LevelRenderer(this, std::shared_ptr<TextureAtlas>(NinecraftApp::_terrainTextureAtlas));
	this->gameRenderer = new GameRenderer(this);
	this->particleEngine = new ParticleEngine(this->level, this->texturesPtr);
	this->font = new Font(AppPlatform::_singleton, &this->options, "font/default8.png", this->texturesPtr);
	this->perfRenderer = new PerfRenderer(this, this->font);
	this->checkGLError("Init complete");

	printf("[NinecraftApp] Validando version...\n"); fflush(stdout);
	this->updateStatusUserAttributes();
	this->options.validateVersion();

	printf("[NinecraftApp] Seteando menu inicial...\n"); fflush(stdout);
	this->screenChooser.setScreen(START_MENU_SCREEN);
	printf("[NinecraftApp] EXITO TOTAL EN INIT\n"); fflush(stdout);
}

void NinecraftApp::teardown(void){
	Minecraft::teardown();
}
