#include <network/mco/MojangConnector.hpp>
#include <util/ThreadCollection.hpp>
#include <network/mco/LoginInformation.hpp>
#include <Minecraft.hpp>
#include <network/mco/MCOParser.hpp>
#include <network/RestService.hpp>
#include <util/Common.hpp>
#include <network/mco/MCOPayloadPacker.hpp>
#include <AppPlatform.hpp> // VITAL PARA EVITAR CRASHEOS DE PUNTERO
#include <stdio.h> // Para logs

// Ocultamos la libreria de criptografia a la PS3
#ifndef __PS3__
#include <oaes_lib.h>
#endif

#include <string.h>
#include <util/Base64.hpp>

#ifndef AUTH_SERVER
#define AUTH_SERVER "https://authserver.mojang.com"
#endif

#ifndef PEOAPI_SERVER
#define PEOAPI_SERVER "https://peoapi.minecraft.net"
#endif

MojangConnector::MojangConnector(Minecraft* minecraft) {
	printf("[MojangConnector] Constructor invocado...\n"); fflush(stdout);
	this->serverCreationEnabled = 0;
	this->serviceEnabled = 0;
	this->random = std::shared_ptr<Random>(new Random());

	printf("[MojangConnector] Obteniendo LoginInformation...\n"); fflush(stdout);
	// REEMPLAZO VITAL: Usamos AppPlatform_ps3 directamente en lugar del contexto de Android Nulo
	this->loginInformation = std::make_shared<LoginInformation>(AppPlatform::_singleton->getLoginInformation());

	this->minecraft = minecraft;

	// ==========================================
	// LOBOTOMIZACION PARA PS3
	// Evitamos instanciar hilos (ThreadCollection) y servicios REST que causen uso de sockets
	// ==========================================
	#ifdef __PS3__
	printf("[MojangConnector] Aplicando Lobotomia de Red para PS3...\n"); fflush(stdout);
	this->threadCollection = nullptr;
	this->mcoParser = nullptr;
	this->accountService = nullptr;
	this->mcoService = nullptr;
	this->gameVersionNet = "0.8.1";
	#else
	this->threadCollection = std::shared_ptr<ThreadCollection>(new ThreadCollection(4));
	this->mcoParser = std::shared_ptr<MCOParser>(new MCOParser());
	this->accountService = std::shared_ptr<RestService>(new RestService(AUTH_SERVER));
	this->mcoService = std::shared_ptr<RestService>(new RestService(PEOAPI_SERVER));
	this->gameVersionNet = Common::getGameVersionStringNet();
	this->mcoService->setCookieData("version", this->gameVersionNet);
	#endif

	// STATUS_0 significa Desconectado/Inactivo
	this->status = STATUS_0;
	printf("[MojangConnector] Constructor completado con exito.\n"); fflush(stdout);
}

void MojangConnector::clearLoginInformation() {
	this->setLoginInformation(LoginInformation());
}
std::shared_ptr<RestService> MojangConnector::getAccountSercice() {
	return this->accountService;
}
MojangConnectionStatus MojangConnector::getConnectionStatus() {
	return this->status;
}

std::string MojangConnector::getEncryptedJoinDataString(long long a3, const std::string& a4, const std::string& a5) {
	// ==========================================
	// RUTA PARA PS3 (Sin Criptografia)
	// ==========================================
	#ifdef __PS3__
	return "";
	// ==========================================
	// RUTA ORIGINAL (Android / PC)
	// ==========================================
	#else
	MCOPayloadPacker v7(*this->random);
	std::string v8 = v7.writeBitStream(a3, a4);
	OAES_CTX* ctx = oaes_alloc();
	oaes_set_option(ctx, OAES_OPTION_ECB, 0);
	oaes_key_import_data(ctx, (const uint8_t*)a5.c_str(), a5.size());
	char v12[512];
	memset(v12, 0, sizeof(v12));
	size_t v10 = 512;
	oaes_encrypt(ctx, (const uint8_t*)v8.c_str(), v8.size(), (uint8_t*)v12, &v10);
	std::string v11(&v12[32], v10 - 32);
	return Base64::base64Encode(v11);
	#endif
}

std::string* MojangConnector::getJoinMCOPayload() {
	return &this->joinMCOPayload;
}
std::shared_ptr<LoginInformation> MojangConnector::getLoginInformation() {
	return this->loginInformation;
}
std::shared_ptr<MCOParser> MojangConnector::getMCOParser() {
	return this->mcoParser;
}
std::shared_ptr<std::unordered_map<long long, MCOServerListItem>> MojangConnector::getMCOServerList() {
	return this->serverList;
}
std::shared_ptr<RestService> MojangConnector::getMCOSercice() {
	return this->mcoService;
}
std::string* MojangConnector::getServerKey() {
	return &this->serverKey;
}
std::shared_ptr<ThreadCollection> MojangConnector::getThreadCollection() {
	return this->threadCollection;
}
bool_t MojangConnector::isMCOCreateServersEnabled() {
	return this->status == STATUS_2 && this->serverCreationEnabled;
}
bool_t MojangConnector::isServiceEnabled() {
	return this->serviceEnabled;
}
void MojangConnector::setLoginInformation(const LoginInformation& a2) {
}
void MojangConnector::setMCOCreateServersEnabled(bool_t a2) {
	this->serverCreationEnabled = a2;
}
void MojangConnector::setMCOServerList(std::shared_ptr<std::unordered_map<long long, MCOServerListItem>> a2) {
	this->serverList = a2;
}
void MojangConnector::setMCOServiceEnabled(bool_t a2) {
	this->serviceEnabled = a2;
}
void MojangConnector::setPayload(const std::string& a2) {
	this->joinMCOPayload = a2;
}
void MojangConnector::setServerKey(const std::string&) {
}
void MojangConnector::setStatus(MojangConnectionStatus a2){
}
void MojangConnector::updateUIThread() {
	// Seguro anti-crasheo por lobotomizacion
	if (this->threadCollection) {
		this->threadCollection->processUIThread();
	}
}
std::string MojangConnector::urlEncode(std::string a2) {
	return "";
}
MojangConnector::~MojangConnector() {
}
