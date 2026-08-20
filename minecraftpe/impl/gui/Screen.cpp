#include <gui/Screen.hpp>
#include <Minecraft.hpp>
#include <gui/GuiElement.hpp>
#include <gui/buttons/Button.hpp>
#include <rendering/Textures.hpp>
#include <rendering/Tesselator.hpp>
#include <rendering/states/DisableState.hpp>
#include <unigl.h>
#include <math/Mth.hpp>
#include <util/Color4.hpp>
#include <input/Mouse.hpp>
#include <sound/SoundEngine.hpp>
#include <input/Keyboard.hpp>
#include <utils.h>
#include <math.h>

char_t* panorama_images[] = {
	(char_t*)"gui/background/panorama_0.png",
	(char_t*)"gui/background/panorama_1.png",
	(char_t*)"gui/background/panorama_2.png",
	(char_t*)"gui/background/panorama_3.png",
	(char_t*)"gui/background/panorama_4.png",
	(char_t*)"gui/background/panorama_5.png"
};

// ESTADO GLOBAL SEGURO
static int32_t g_selectedButtonIndex = -1;
static Screen* g_lastActiveScreen = nullptr;

// FILTRO ESTRICTO: Solo botones visibles, activos y con tamaño real dentro del área de pantalla
static inline bool isValidButton(Button* b, int screenW, int screenH) {
	if (!b) return false;
	if (!b->visible || !b->active) return false;
	if (b->width <= 4 || b->height <= 4) return false;
	if (b->posX + b->width <= 0 || b->posY + b->height <= 0) return false;
	if (b->posX >= screenW || b->posY >= screenH) return false;
	return true;
}

Screen::Screen(){
	this->width = 1;
	this->height = 1;
	this->field_C = 0;
	this->minecraft = 0;
	this->field_20 = 0;
	this->field_24 = 0;
	this->field_28 = 0;
	this->field_44 = 0;
	this->font = 0;
	this->lastPressedButton = 0;
}

void Screen::updateTabButtonSelection(){
	if(!this->minecraft->useTouchscreen()){
		for(size_t i = 0; i < this->field_2C.size(); ++i) {
			Button* b = this->field_2C[i];
			b->text = ((int32_t)i - this->field_44) == 0;
		}
	}
}

void Screen::init(struct Minecraft* mc, int32_t w, int32_t h){
	this->minecraft = mc;
	this->height = h;
	this->font = mc->font;
	this->width = w;
	g_selectedButtonIndex = -1;
	g_lastActiveScreen = this;
	this->init();
	this->setupPositions();
	this->updateTabButtonSelection();
}

void Screen::setSize(int32_t w, int32_t h){
	this->width = w;
	this->height = h;
	this->setupPositions();
}

void Screen::render(int32_t x, int32_t y, float a3){
	if(this->supppressedBySubWindow()){
		for(GuiElement** start = this->elements.data(); start != (this->elements.data()+this->elements.size()); ++start){
			(*start)->topRender(this->minecraft, x, y);
		}
	}else{
		for(GuiElement** start = this->elements.data(); start != (this->elements.data()+this->elements.size()); ++start){
			(*start)->render(this->minecraft, x, y);
		}

		for(size_t i = 0; i < this->buttons.size(); ++i){
			Button* b = this->buttons[i];
			if(!b->isOverrideScreenRendering()){
				b->render(this->minecraft, x, y);
			}
		}
	}

	// DIBUJAR EL RECUADRO BLANCO PULSANTE SOBRE EL BOTÓN SELECCIONADO
	this->renderSelectionBox();
}

void Screen::renderSelectionBox() {
	if(this->buttons.empty()) return;

	if(g_lastActiveScreen != this) {
		g_lastActiveScreen = this;
		g_selectedButtonIndex = -1;
	}

	int total = (int)this->buttons.size();

	// Verificar si el botón actualmente seleccionado es válido
	bool currentValid = false;
	if(g_selectedButtonIndex >= 0 && g_selectedButtonIndex < total) {
		if(isValidButton(this->buttons[g_selectedButtonIndex], this->width, this->height)) {
			currentValid = true;
		}
	}

	// Si no es válido, buscar el primer botón válido
	if(!currentValid) {
		g_selectedButtonIndex = -1;
		for(int i = 0; i < total; ++i) {
			if(isValidButton(this->buttons[i], this->width, this->height)) {
				g_selectedButtonIndex = i;
				break;
			}
		}
	}

	if(g_selectedButtonIndex < 0 || g_selectedButtonIndex >= total) return;

	Button* b = this->buttons[g_selectedButtonIndex];
	if(!isValidButton(b, this->width, this->height)) return;

	// ONDA SENOIDAL: Efecto loop continuo iluminándose y oscureciéndose
	double timeS = getTimeS();
	float pulse = 0.55f + 0.45f * sinf((float)(timeS * 6.5));
	int alphaBorder = (int)(pulse * 255.0f);
	if(alphaBorder < 60) alphaBorder = 60;
	if(alphaBorder > 255) alphaBorder = 255;

	int alphaFill = (int)(pulse * 35.0f);

	int colorBorder = (alphaBorder << 24) | 0x00FFFFFF;
	int colorFill   = (alphaFill << 24)   | 0x00FFFFFF;

	int x1 = b->posX - 2;
	int y1 = b->posY - 2;
	int x2 = b->posX + b->width + 2;
	int y2 = b->posY + b->height + 2;

	this->drawRect(x1, y1, x2, y2, colorBorder, 2);
	this->fill(x1, y1, x2, y2, colorFill);
}

void Screen::navigateDirection(int dir) {
	if(this->buttons.empty()) return;

	int total = (int)this->buttons.size();
	int cur = g_selectedButtonIndex;

	if(cur < 0 || cur >= total || !isValidButton(this->buttons[cur], this->width, this->height)) {
		for(int i = 0; i < total; ++i) {
			if(isValidButton(this->buttons[i], this->width, this->height)) {
				g_selectedButtonIndex = i;
				return;
			}
		}
		return;
	}

	Button* curBtn = this->buttons[cur];
	float curCX = (float)curBtn->posX + (float)curBtn->width * 0.5f;
	float curCY = (float)curBtn->posY + (float)curBtn->height * 0.5f;

	int bestIndex = -1;
	float bestScore = 1e9f;

	// 1. BÚSQUEDA DIRECCIONAL GEOMÉTRICA
	for(int i = 0; i < total; ++i) {
		if(i == cur) continue;
		Button* b = this->buttons[i];
		if(!isValidButton(b, this->width, this->height)) continue;

		float bCX = (float)b->posX + (float)b->width * 0.5f;
		float bCY = (float)b->posY + (float)b->height * 0.5f;

		float dx = bCX - curCX;
		float dy = bCY - curCY;

		float forwardDist = 0.0f;
		float sideDist = 0.0f;
		bool inDir = false;

		switch(dir) {
			case 0: // ARRIBA
				forwardDist = -dy;
				sideDist = fabsf(dx);
				inDir = (dy < -2.0f);
				break;
			case 1: // ABAJO
				forwardDist = dy;
				sideDist = fabsf(dx);
				inDir = (dy > 2.0f);
				break;
			case 2: // IZQUIERDA
				forwardDist = -dx;
				sideDist = fabsf(dy);
				inDir = (dx < -2.0f);
				break;
			case 3: // DERECHA
				forwardDist = dx;
				sideDist = fabsf(dy);
				inDir = (dx > 2.0f);
				break;
		}

		if(inDir) {
			float score = forwardDist + sideDist * 1.3f;
			if(score < bestScore) {
				bestScore = score;
				bestIndex = i;
			}
		}
	}

	// 2. WRAP-AROUND CIRCULAR ENTRE BOTONES VÁLIDOS
	if(bestIndex == -1) {
		int step = (dir == 1 || dir == 3) ? 1 : -1;
		int candidate = cur;
		for(int i = 0; i < total; ++i) {
			candidate = (candidate + step + total) % total;
			if(isValidButton(this->buttons[candidate], this->width, this->height)) {
				bestIndex = candidate;
				break;
			}
		}
	}

	if(bestIndex != -1 && bestIndex != cur) {
		g_selectedButtonIndex = bestIndex;
		if(this->minecraft && this->minecraft->soundEngine) {
			this->minecraft->soundEngine->playUI("random.click", 0.6f, 1.2f);
		}
	}
}

void Screen::triggerSelectedButton() {
	if(this->buttons.empty()) return;
	if(g_selectedButtonIndex < 0 || g_selectedButtonIndex >= (int32_t)this->buttons.size()) return;

	Button* b = this->buttons[g_selectedButtonIndex];
	if(!isValidButton(b, this->width, this->height)) return;

	Minecraft* mc = this->minecraft;

	if(mc) {
		mc->field_D14 = 1; // Escudo protector contra Use-After-Free
	}

	int cx = b->posX + b->width / 2;
	int cy = b->posY + b->height / 2;

	// 1. Simular clic de ratón/táctil en las coordenadas centrales (para listas de mundos y contenedores)
	this->mouseClicked(cx, cy, 1);

	// 2. Ejecución directa del botón
	b->setPressed();
	if(mc && mc->soundEngine) {
		mc->soundEngine->playUI("random.click", 1.0f, 1.0f);
	}
	this->buttonClicked(b);
	b->released(cx, cy);

	this->mouseReleased(cx, cy, 1);

	if(mc) {
		mc->field_D14 = 0;
		if(mc->field_D15) {
			mc->setScreen(mc->field_D18);
			mc->field_D18 = nullptr;
			mc->field_D15 = 0;
			g_selectedButtonIndex = -1;
		}
	}
}

void Screen::init(){}

void Screen::setupPositions(void){
	for(GuiElement** start = this->elements.data(); start != (this->elements.data()+this->elements.size()); ++start){
		(*start)->setupPositions();
	}
}

void Screen::updateEvents(){
	if(!this->field_C){
		while(Mouse::next()){
			this->mouseEvent();
		}

		while ( (size_t)(Keyboard::_index + 1) < Keyboard::_inputs.size() )
		{
			++Keyboard::_index;
			this->keyboardEvent();
		}
		while ( (size_t)(Keyboard::_textIndex + 1) < Keyboard::_inputText.size() )
		{
			++Keyboard::_textIndex;
			this->keyboardTextEvent();
		}
	}
}

void Screen::mouseEvent(void) {
	MouseAction* ma = Mouse::getEvent();
	if(ma->isButton()) {
		bool_t b = Mouse::getEventButtonState();
		int32_t v5 = this->width * ma->field_0 / this->minecraft->field_1C;
		int32_t v6 = ma->field_2 * this->height / this->minecraft->field_20;
		int32_t evb = Mouse::getEventButton();
		if(b){
			this->mouseClicked(v5, v6-1, evb);
		}else{
			this->mouseReleased(v5, v6-1, evb);
		}
	}
}

void Screen::keyboardEvent(){
	if(Keyboard::_inputs[Keyboard::_index].field_0) {
		this->keyPressed(Keyboard::_inputs[Keyboard::_index].field_4);
	}
}

void Screen::keyboardTextEvent(){
	this->keyboardNewChar(Keyboard::_inputText[Keyboard::_textIndex].field_0, Keyboard::_inputText[Keyboard::_textIndex].field_4);
}

bool_t Screen::handleBackEvent(bool_t a2){
	GuiElement* v5;
	GuiElement** start = this->elements.data();
	do{
		if(start == (this->elements.data()+this->elements.size())) return 0;
		v5 = *(start++);
	}while(!v5->backPressed(this->minecraft, a2));
	return 1;
}

void Screen::tick(){
	for(GuiElement** start = this->elements.data(); start != (this->elements.data()+this->elements.size()); ++start){
		(*start)->tick(this->minecraft);
	}
}

void Screen::removed(){}

void Screen::renderBackground(int32_t a2){
	if(this->renderGameBehind()){
		this->fill(0, 0, this->width, this->height, 0x7f000000);
	}else{
		this->renderDirtBackground(a2);
	}
}

void Screen::renderDirtBackground(int32_t a2){
	this->minecraft->texturesPtr->loadAndBindTexture("gui/background.png");
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	float v5 = a2;
	Tesselator::instance.begin(4);
	Tesselator::instance.color(0x404040);
	Tesselator::instance.vertexUV(0.0f, (float)this->height, 0.0f, 0.0f, v5 + (float)this->height*0.03125f);
	Tesselator::instance.vertexUV((float)this->width, (float)this->height, 0.0f, (float)this->width*0.03125f, v5 + (float)this->height*0.03125f);
	float v6 = v5+0.0f;
	Tesselator::instance.vertexUV((float)this->width, 0.0f, 0.0f, (float)this->width*0.03125f, v6);
	Tesselator::instance.vertexUV(0.0f, 0.0f, 0.0f, 0.0f, v6);
	Tesselator::instance.draw(1);
}

float dword_D6E05C20 = 0;

void Screen::renderMenuBackground(float a2){
	dword_D6E05C20 += this->minecraft->field_D34 * 30.0f;

	glDisable(0x0B71);
	glDisable(0x0B44);
	glDisable(0x0BE2);
	glEnable(0x0DE1);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	glMatrixMode(0x1701);
	glPushMatrix();
	glLoadIdentity();
	gluPerspective(120.0f, 1.0f, 0.05f, 10.0f);

	glMatrixMode(0x1700u);
	glPushMatrix();
	glLoadIdentity();
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	glRotatef(180.0f, 1.0f, 0.0f, 0.0f);
	glRotatef(Mth::sin((float)(a2 + dword_D6E05C20) / 400.0f) + 20.0f, 1.0f, 0.0f, 0.0f);
	glRotatef(-(float)((float)(a2 + dword_D6E05C20) * 0.1f), 0.0f, 1.0f, 0.0f);

	int32_t v8 = 0;
	do{
		float v9 = 0.0f, v10 = 0.0f, v11 = 0.0f;
		glPushMatrix();
		switch(v8){
			case 1: v9 = 90.0f;  v10 = 0.0f; v11 = 1.0f; break;
			case 2: v9 = 180.0f; v10 = 0.0f; v11 = 1.0f; break;
			case 3: v9 = -90.0f; v10 = 0.0f; v11 = 1.0f; break;
			case 4: v9 = 90.0f;  v11 = 0.0f; v10 = 1.0f; break;
			case 5: v9 = -90.0f; v11 = 0.0f; v10 = 1.0f; break;
		}
		if(v8 != 0) glRotatef(v9, v10, v11, 0.0f);

		char_t* texture = panorama_images[v8++];
		int32_t tex_id = this->minecraft->texturesPtr->loadTexture(texture, 1, 1);
		if(tex_id){
			glBindTexture(0x0DE1, tex_id);
			this->minecraft->texturesPtr->currentTexture = tex_id;
		}

		Tesselator::instance.begin(4);
		Tesselator::instance.vertexUV(-1.0f, -1.0f, 1.0f, 0.0f, 0.0f);
		Tesselator::instance.vertexUV( 1.0f, -1.0f, 1.0f, 1.0f, 0.0f);
		Tesselator::instance.vertexUV( 1.0f,  1.0f, 1.0f, 1.0f, 1.0f);
		Tesselator::instance.vertexUV(-1.0f,  1.0f, 1.0f, 0.0f, 1.0f);
		Tesselator::instance.draw(1);

		glPopMatrix();
	}while(v8 != 6);

	glMatrixMode(0x1701u);
	glPopMatrix();
	glMatrixMode(0x1700u);
	glPopMatrix();

	glDisable(0x0B71);
	glDisable(0x0B44);
	glEnable(0x0BE2);
	glBlendFunc(0x0302, 0x0303);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}

bool_t Screen::renderGameBehind(){
	return this->minecraft->options.graphics;
}

bool_t Screen::hasClippingArea(struct IntRectangle&){
	return 0;
}

bool_t Screen::isPauseScreen(){
	return 1;
}

bool_t Screen::isErrorScreen(){
	return 0;
}

bool_t Screen::isInGameScreen(){
	return 1;
}

bool_t Screen::closeOnPlayerHurt(){
	return 0;
}

void Screen::confirmResult(bool_t, int32_t){}

void Screen::lostFocus(){
}

void Screen::toGUICoordinate(int32_t& x, int32_t& y){
	x = this->width*x / this->minecraft->field_1C;
	y = this->height*y / this->minecraft->field_20 - 1;
}

void Screen::feedMCOEvent(MCOEvent){}

bool_t Screen::supppressedBySubWindow(){
	int32_t v3 = 0;
	for(GuiElement** start = this->elements.data(); start != (this->elements.data()+this->elements.size()); ++start){
		if((*start)->suppressOtherGUI()) v3 = 1;
	}
	return v3;
}

void Screen::onTextBoxUpdated(int32_t){}
void Screen::onMojangConnectorStatus(MojangConnectionStatus){}

void Screen::setTextboxText(const std::string& a2){
	for(auto&& e : this->elements){
		if(e->suppressOtherGUI()){
			e->setTextboxText(a2);
		}
	}
}

void Screen::onInternetUpdate(){}
void Screen::buttonClicked(struct Button*){}

void Screen::mouseClicked(int32_t a2, int32_t a3, int32_t a4) {
	GuiElement** elements = this->elements.data();
	if(this->supppressedBySubWindow()) {
		while(elements != &this->elements.back()) {
			GuiElement* el = *elements++;
			if(el->suppressOtherGUI()) {
				el->focusuedMouseClicked(this->minecraft, a2, a3, a4);
			}
		}
	} else {
		while(elements != (this->elements.data() + this->elements.size())) {
			GuiElement* el = *elements++;
			el->mouseClicked(this->minecraft, a2, a3, a4);
		}

		if(a4 == 1) {
			for(size_t i = 0; i < this->buttons.size(); ++i) {
				Button* b = this->buttons[i];
				if(b->active) {
					if(b->clicked(this->minecraft, a2, a3)) {
						b->setPressed();
						this->lastPressedButton = b;
					}
				}
			}
		}
	}
}

void Screen::mouseReleased(int32_t a2, int32_t a3, int32_t a4) {
	GuiElement** elements = this->elements.data();
	if(this->supppressedBySubWindow()) {
		while(elements != &this->elements.back()) {
			GuiElement* el = *elements++;
			if(el->suppressOtherGUI()) {
				el->focusuedMouseReleased(this->minecraft, a2, a3, a4);
			}
		}
	} else {
		while(elements != (this->elements.data() + this->elements.size())) {
			GuiElement* el = *elements++;
			el->mouseReleased(this->minecraft, a2, a3, a4);
		}

		if(this->lastPressedButton) {
			if(a4 == 1) {
				for(size_t i = 0; i < this->buttons.size(); ++i) {
					Button* b = this->buttons[i];
					if(this->lastPressedButton == b) {
						if(this->lastPressedButton->clicked(this->minecraft, a2, a3)) {
							this->buttonClicked(this->lastPressedButton);
							this->minecraft->soundEngine->playUI("random.click", 1.0f, 1.0f);
							this->lastPressedButton->released(a2, a3);
						}
					}
				}
				this->lastPressedButton = 0;
			}
		}
	}
}

void Screen::keyPressed(int32_t a2) {
	for(auto&& e: this->elements) {
		e->keyPressed(this->minecraft, a2);
	}
}

void Screen::keyboardNewChar(const std::string& a2, bool_t a3) {
	for(auto&& e: this->elements) {
		if(e->suppressOtherGUI()) {
			e->keyboardNewChar(this->minecraft, a2, a3);
		}
	}
}

Screen::~Screen() {}
