#include <gui/Gui.hpp>
#include <Config.hpp>
#include <Minecraft.hpp>
#include <entity/LocalPlayer.hpp>
#include <entity/player/gamemode/GameMode.hpp>
#include <input/IInputHolder.hpp>
#include <input/Mouse.hpp>
#include <inventory/Inventory.hpp>
#include <item/Item.hpp>
#include <level/Level.hpp>
#include <math.h>
#include <math/Mth.hpp>
#include <rendering/Font.hpp>
#include <rendering/Tesselator.hpp>
#include <rendering/Textures.hpp>
#include <rendering/entity/ItemRenderer.hpp>
#include <rendering/states/DisableState.hpp>
#include <sstream>
#include <string.h>
#include <tile/material/Material.hpp>
#include <util/GuiMessage.hpp>
#include <util/IntRectangle.hpp>
#include <util/OffsetPosTranslator.hpp>
#include <utils.h>
#include <stdio.h>

float Gui::InvGuiScale = 0.333333f;
float Gui::GuiScale;
float Gui::DropTicks = 40.0f;

Gui::Gui(Minecraft* mc) {
	this->field_10 = 0;
	this->field_18 = 240;
	this->field_14 = "";
	this->minecraftInst = mc;
	this->field_9FC = 0;
	this->field_A00 = 2;
	this->field_A04 = "";
	this->field_A08 = 0;
	this->field_A0C = 0;
	this->field_A10 = 1;
	this->field_A18 = -1;
	this->invUpdated = 1;
	this->field_A20 = -1;
	this->field_A28 = 0;
	this->slotsAmount = 4;
	this->field_A80 = -1;
	this->tipMessage = "";
	this->field_A84 = -1;
	this->field_A8C = 0;
	this->field_A94 = 0;
	if (AppPlatform::_singleton) {
		AppPlatform::_singleton->listeners.emplace(1.0f, this);
	}
}

Gui::~Gui() {
	if (AppPlatform::_singleton) {
		AppPlatform::_singleton->removeListener(this);
	}
}

void Gui::onAppSuspended() {
}

void Gui::onConfigChanged(const Config& a2) {
	AppPlatform* v5 = nullptr;
	if (this->minecraftInst) {
		v5 = this->minecraftInst->platform();
	}
	if (!v5) {
		v5 = AppPlatform::_singleton;
	}

	float ppm = v5 ? v5->getPixelsPerMillimeter() : 10.0f;
	float v6 = (float)(ppm * 12.0f) * 0.3333f;
	if(v6 >= 40.0f) {
		v6 = 40.0f;
	}
	float v7 = v6 * 0.95f;
	float v8 = 6.2832f / 24.0f;

	Tesselator::instance.begin(7, 96);

	int v4 = 0;
	do {
		int v9 = v4++;
		float v10 = (float)v9 * v8;
		float v11 = Mth::cos(v10);
		float v12 = Mth::cos(v10 + v8);
		float v13 = Mth::sin(v10);
		float v14 = v12;
		float v15 = Mth::sin(v10 + v8);
		Tesselator::instance.vertexUV(v7 * v11, v7 * v13, 0.0f, 0.0f, 1.0f);
		Tesselator::instance.vertexUV(v7 * v14, v7 * v15, 0.0f, 1.0f, 1.0f);
		Tesselator::instance.vertexUV(v6 * v14, v6 * v15, 0.0f, 1.0f, 0.0f);
		Tesselator::instance.vertexUV(v6 * v11, v6 * v13, 0.0f, 0.0f, 0.0f);
	} while(v4 != 24);
	this->outerBreakRingMesh = Tesselator::instance.end();

	int v16 = 0;
	Tesselator::instance.begin(6, 26);
	Tesselator::instance.vertex(0.0f, 0.0f, 0.0f);
	do {
		int v17 = v16--;
		float v18 = (float)v17 * v8;
		float v20 = Mth::cos(v18);
		Tesselator::instance.vertex(v7 * v20, v7 * Mth::sin(v18), 0.0f);
	} while(v16 != -25);
	this->innerBreakRingMesh = Tesselator::instance.end();

	Minecraft* mc = a2.mc ? a2.mc : this->minecraftInst;
	int sp = 0;
	if (mc) {
		sp = mc->useTouchscreen();
	}

	if(sp && mc) {
		int v22 = 6;
		if(mc->options.useJoypad) {
			v22 = 6;
		} else {
			v22 = 6;
			if(a2.field_0 > 480) {
				int v23 = 0, v24 = 0, v25 = 0;
				do {
					this->getSlotPos(0, v23, v25);
					this->getSlotPos(v22, v24, v25);
					if((float)((float)((float)a2.field_0 - (float)((float)(v24 - v23) * a2.guiScale)) * a2.field_1C) < 80.0f) {
						break;
					}
					++v22;
				} while(v22 != 8);
			}
		}
		this->slotsAmount = v22;
	} else {
		this->slotsAmount = 9;
	}
	this->field_18 = a2.field_10;
}

int32_t Gui::itemCountItoa(char_t* buf, int32_t n) {
	if(!buf) return 0;
	if(n < 0) return 0;
	if(n > 9) {
		if(n > 99) {
			strcpy(buf, "99+");
			return 3;
		}
		*buf = n / 10 + 48;
		buf[2] = 0;
		buf[1] = n + 48 - 10 * (n / 10);
		return 2;
	} else {
		*buf = n + 48;
		buf[1] = 0;
		return 1;
	}
}

void Gui::addMessage(const std::string& a2, const std::string& a3, int32_t a4) {
	if(this->minecraftInst && this->minecraftInst->font) {
		GuiMessage v13(a2, a3, a4);
		this->chatMessages.emplace(this->chatMessages.begin(), v13);

		if(!this->minecraftInst->isOnlineClient() && v13.field_8.length() > 0 && v13.field_8[0] == '/') {
			std::string cmd = v13.field_8.substr(1);
			std::string v10 = cmd == "" ? "Error: no command provided" : "Error: Command "+cmd+" not found";

			this->chatMessages.emplace(this->chatMessages.begin(), GuiMessage("server", v10, 200));
		}

		while(this->chatMessages.size() > 30) {
			this->chatMessages.pop_back();
		}
	}
}

float Gui::cubeSmoothStep(float a2, float a3, float a4) {
	return (float)(a2 * a2) * (float)(3.0 - (float)(a2 + a2));
}

void Gui::displayClientMessage(const std::string& a2) {
	this->addMessage("", a2, 200);
}

void Gui::flashSlot(int32_t a2) {
	this->field_A18 = a2;
	this->field_A20 = getTimeS();
}

float Gui::floorAlignToScreenPixel(float a1) {
	return (float)(int32_t)(float)(a1 * Gui::GuiScale) * Gui::InvGuiScale;
}

int32_t Gui::getNumSlots() {
	return this->slotsAmount;
}

RectangleArea Gui::getRectangleArea(int32_t a3) {
	if (!this->minecraftInst) return RectangleArea(0, 0.0f, 0.0f, 0.0f, 0.0f);

	int32_t v6 = this->minecraftInst->field_1C / 2;
	int32_t slots = this->getNumSlots();
	float v8 = (float)v6 + 2.0f;
	Minecraft* minecraftInst = this->minecraftInst;
	float v10 = (float)((float)(10 * slots + 3) + 1.0f) * Gui::GuiScale;
	float v11 = Gui::GuiScale * 25.0f;
	float v12;

	if(a3 < 0) {
		v12 = (float)minecraftInst->field_20;
		return RectangleArea(1, 0, v12 - v11, (float)(v8 + v10) + 2, v12);
	}
	if(!a3) {
		int32_t v14 = minecraftInst->field_20;
		v12 = (float)v14;
		return RectangleArea(1, v8 - v10, v12 - v11, (float)(v8 + v10) + 2, v12);
	}
	v12 = (float)minecraftInst->field_20;
	int32_t v13 = minecraftInst->field_1C;
	return RectangleArea(1, v8 - v10, v12 - v11, v13, v12);
}

int32_t Gui::getSlotIdAt(int32_t a2, int32_t a3) {
	if (!this->minecraftInst) return -1;

	int32_t v5 = (int32_t)(float)((float)this->minecraftInst->field_1C * Gui::InvGuiScale);
	int32_t v6 = (int32_t)(float)((float)this->minecraftInst->field_20 * Gui::InvGuiScale);
	int32_t v8 = (int32_t)(float)((float)a3 * Gui::InvGuiScale);
	if(v8 < v6 - 19) return -1;
	if(v8 > v6) return -1;

	int32_t v7 = (int32_t)(float)((float)a2 * Gui::InvGuiScale);
	int32_t v10 = v7 - (v5 / 2 + 2 - 10 * this->getNumSlots());
	if(v10 < 0) return -1;

	int32_t v9 = v10 / 20;
	if(v10 / 20 >= this->getNumSlots()) return -1;

	return v9;
}

int32_t Gui::getSlotPos(int32_t slot, int32_t& x, int32_t& y) {
	if (!this->minecraftInst) return 0;

	Minecraft* minecraftInst = this->minecraftInst;
	int32_t v8 = (int32_t)(float)((float)minecraftInst->field_20 * Gui::InvGuiScale);
	int32_t v9 = (int32_t)(float)((float)minecraftInst->field_1C * Gui::InvGuiScale);
	int32_t result = v9 / 2 - 10 * this->getNumSlots();
	x = result + 20 * slot;
	y = v8 - 22;
	return result;
}

void Gui::handleClick(int32_t a2, int32_t a3, int32_t a4) {
	if (!this->minecraftInst) return;

	if(a2 == 1) {
		int32_t SlotIdAt = this->getSlotIdAt(a3, a4);
		if(SlotIdAt != -1) {
			bool_t v6 = SlotIdAt == this->getNumSlots() - 1;
			Minecraft* minecraftInst = this->minecraftInst;
			if(v6) {
				Screen* currentScreen = minecraftInst->currentScreen;
				ScreenId v10 = currentScreen ? ScreenId::NONE_SCREEN : ScreenId::INVENTORY_SCREEN;
				minecraftInst->screenChooser.setScreen(v10);
			} else if (minecraftInst->player && minecraftInst->player->inventory) {
				minecraftInst->player->inventory->selectSlot(SlotIdAt);
				this->resetItemNameOverlay();
			}
		}
	}
}

void Gui::handleKeyPressed(int32_t a2) {
	if (!this->minecraftInst || !this->minecraftInst->player) return;

	Inventory* inventory;
	int32_t selectedSlot;
	int32_t v5;
	int32_t v6;

	switch(a2) {
		case 99:
			inventory = this->minecraftInst->player->inventory;
			if (!inventory) return;
			selectedSlot = inventory->selectedSlot;
		if(selectedSlot <= 0) return;
		v5 = selectedSlot - 1;
		LABEL_7:
		inventory->selectedSlot = v5;
		return;
		case 4:
			if (!this->minecraftInst->player->inventory) return;
			v6 = this->minecraftInst->player->inventory->selectedSlot;
		if(v6 >= this->getNumSlots() - 2) return;
		inventory = this->minecraftInst->player->inventory;
		v5 = inventory->selectedSlot + 1;
		goto LABEL_7;
		case 100:
			this->minecraftInst->screenChooser.setScreen(ScreenId::INVENTORY_SCREEN);
			break;
	}
}

void Gui::inventoryUpdated() {
	this->invUpdated = 1;
}

bool_t Gui::isInside(int32_t x, int32_t y) {
	return this->getSlotIdAt(x, y) != -1;
}

OffsetPosTranslator _spawnPos;
char_t _D6E05AAC[264];

void Gui::onLevelGenerated() {
	if(this->minecraftInst && this->minecraftInst->level) {
		TilePos res = this->minecraftInst->level->getSharedSpawnPos();
		_spawnPos.y = -res.y;
		_spawnPos.z = -res.z;
		_spawnPos.x = -res.x;
	}
}

void Gui::postError(int32_t a2) {
	static std::set<int> _D6E05BAC;
	if(_D6E05BAC.find(a2) != _D6E05BAC.end()) {
		_D6E05BAC.insert(a2);

		std::stringstream v11;
		v11 << "Something went wrong! (errcode ";
		v11 << a2;
		v11 << ")\n";
		this->addMessage("error", v11.str(), 200);
	}
}

void Gui::render(float a2, bool_t a3, int32_t a4, int32_t a5) {
	if(!this->minecraftInst) return;

	Minecraft* minecraftInst = this->minecraftInst;
	if(minecraftInst->level && minecraftInst->player) {
		DisableState v25(0xB71);
		Font* font = minecraftInst->font;
		int32_t v10 = minecraftInst->useTouchscreen();
		int32_t v12 = minecraftInst->field_20;
		float v13 = Gui::InvGuiScale;
		float v14 = (float)minecraftInst->field_1C * Gui::InvGuiScale;
		this->zLayer = -90.0f;
		int32_t v15 = (int32_t)v14;
		int32_t v16 = (int32_t)(float)((float)v12 * v13);

		if(!minecraftInst->currentScreen) {
			this->renderProgressIndicator(v10, v15, v16, a2);
		}
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		if(minecraftInst->gameMode && minecraftInst->gameMode->canHurtPlayer() && minecraftInst->texturesPtr) {
			minecraftInst->texturesPtr->loadAndBindTexture("gui/icons.png");
			Tesselator::instance.beginOverride();
			Tesselator::instance.colorABGR(-1);
			this->renderHearts();
			this->renderBubbles();
			Tesselator::instance.endOverrideAndDraw();
		}
		if(minecraftInst->player->getSleepTimer() > 0) {
			this->renderSleepAnimation(v15, v16);
		}
		if(!minecraftInst->currentScreen) {
			this->renderToolBar(a2, 0.65f);
		}
		float v18 = this->field_A8C;
		if(v18 > 0.0f && font) {
			float v20 = this->field_A90;
			float v21 = (float)minecraftInst->field_1C / Gui::GuiScale;
			float v22 = v18 / 20.0f;
			float v23 = (float)((float)(minecraftInst->field_20 / 2) / Gui::GuiScale) + 20.0f;
			if(v22 >= 1.0f) v22 = 1.0f;

			int32_t v24 = Color4(1, 1, 1, v22 * 0.85f).toARGB();
			font->drawShadow(this->tipMessage, (float)(v21 * 0.5f) - (float)(v20 * 0.5f), v23, v24);
		}
		if(font) {
			this->renderChatMessages(v15, v16, 0xAu, this->field_A94, font);
			if(!minecraftInst->currentScreen) {
				this->renderOnSelectItemNameText(v15, font, v16 - 19);
			}
		}
	}
}

void Gui::renderBubbles() {
	if (!this->minecraftInst || !this->minecraftInst->player) return;

	if(this->minecraftInst->player->isUnderLiquid(Material::water)) {
		int32_t relatedToBubbleRendering = this->minecraftInst->player->air;
		int32_t v3 = (int32_t)ceilf((float)((float)(relatedToBubbleRendering - 2) * 10.0f) / 300.0f);
		int32_t v4 = relatedToBubbleRendering;
		int32_t v5 = 0;
		int32_t v7 = (int32_t)ceilf((float)((float)v4 * 10.0f) / 300.0f);
		while(v5 < v7) {
			int32_t v6 = (v5 < v3) ? 16 : 25;
			this->blit(8 * v5++ + 2, 12, v6, 18, 9, 9, 0, 0);
		}
	}
}

void Gui::renderChatMessages(int32_t a2, int32_t a3, uint32_t a4, bool_t a5, struct Font* a6) {
	if(!a5 && a6) {
		int32_t v7 = 0;
		int32_t totalMsgs = (int32_t)this->chatMessages.size();
		if(totalMsgs > 0) {
			for (int32_t v12 = totalMsgs - 1; v12 >= 0; --v12) {
				if(v12 < (int32_t)this->chatMessages.size() && this->chatMessages[v12].field_0 < this->chatMessages[v12].field_4) {
					++v7;
				}
			}
			int32_t v16 = 0;
			for (int32_t v10 = (int32_t)this->chatMessages.size() - 1; v10 >= 0; --v10) {
				if (v10 >= (int32_t)this->chatMessages.size()) continue;
				GuiMessage* str = &this->chatMessages[v10];
				if(str->field_0 < str->field_4) {
					float v20 = (float)(1.0f - (float)((float)str->field_0 / (float)str->field_4)) * 10.0f;
					if(v20 < 0) v20 = 0;
					else if(v20 > 1) v20 = 1;
					int v21 = (int)(float)((float)(v20 * v20) * 255.0f);
					if(v21 > 0) {
						if(v7 <= 10) {
							++v16;
							this->fill(2.0f, (float)(9 * v16 + 18) - 1.0f, (float)this->field_18 + 2.0f, (float)(9 * v16 + 18) + 8.0f, (int32_t)((v21 >> 1) << 24));
							Color4* v22 = (str->field_8.length() > 0 && str->field_8[0] == '/') ? &Color4::GREY : &Color4::WHITE;
							Color4 v27(v22->r, v22->g, v22->b, 0);
							a6->drawShadow(str->field_10, 2.0f, (float)(9 * v16 + 18), v27.toARGB() + (v21 << 24));
						} else {
							--v7;
						}
					}
				}
			}
		}
	}
}

void Gui::renderDebugInfo(void) {
	if (!this->minecraftInst || !this->minecraftInst->player || !this->minecraftInst->font) return;

	LocalPlayer* player = this->minecraftInst->player;
	float v3 = player->posY - player->ridingHeight;
	float posX = player->posX;
	float posZ = player->posZ;
	float a2 = posX;
	float a3 = v3;
	float a4 = posZ;

	_spawnPos.to(a2, a3, a4);
	snprintf(_D6E05AAC, sizeof(_D6E05AAC), "pos: %3.1f, %3.1f, %3.1f\n", a2, a3, a4);
	Tesselator::instance.beginOverride();
	Tesselator::instance.scale2d(Gui::InvGuiScale, Gui::InvGuiScale);
	this->minecraftInst->font->draw(_D6E05AAC, 2.0f, 2.0f, 0xFFFFFF);
	Tesselator::instance.resetScale();
	Tesselator::instance.endOverrideAndDraw();
}

void Gui::renderHearts() {
	if (!this->minecraftInst || !this->minecraftInst->player) return;

	LocalPlayer* player = this->minecraftInst->player;
	int32_t field_DC = player->field_DC;
	int32_t v4 = (field_DC <= 9) ? 0 : ((field_DC / 3) & 1);
	int32_t v5 = 1;
	int32_t health = player->health;
	int32_t prevHealthMaybe = player->prevHealthMaybe;

	this->randomInst = Random(312871 * this->field_9FC);
	int32_t v7 = player->getArmorValue();

	do {
		if(v7 > 0) {
			int32_t v8 = 4 * v5 + 82;
			if(v5 >= v7) {
				if(v5 == v7) this->blit(v8, 2, 52, 9, 9, 9, 0, 0);
				else if(v5 > v7) this->blit(v8, 2, 16, 9, 9, 9, 0, 0);
			} else {
				this->blit(v8, 2, 34, 9, 9, 9, 0, 0);
			}
		}
		int32_t v9 = (health > 4) ? 2 : ((this->randomInst.genrand_int32() & 1) + 1);
		this->blit(4 * v5 - 2, v9, 9 * v4 + 16, 0, 9, 9, 0, 0);
		if(v4) {
			if(v5 >= prevHealthMaybe) {
				if(v5 == prevHealthMaybe) this->blit(4 * v5 - 2, v9, 79, 0, 9, 9, 0, 0);
			} else {
				this->blit(4 * v5 - 2, v9, 70, 0, 9, 9, 0, 0);
			}
		}
		if(v5 >= health) {
			if(v5 == health) this->blit(4 * v5 - 2, v9, 61, 0, 9, 9, 0, 0);
		} else {
			this->blit(4 * v5 - 2, v9, 52, 0, 9, 9, 0, 0);
		}
		v5 += 2;
	} while(v5 != 21);
}

void Gui::renderOnSelectItemNameText(int32_t a2, struct Font* a3, int32_t a4) {
	if (!this->minecraftInst || !this->minecraftInst->player || !this->minecraftInst->player->inventory || !a3) return;

	if(this->field_A00 < 1.0f) {
		ItemInstance* sel = this->minecraftInst->player->inventory->getSelected();
		if(sel) {
			int32_t v10 = a3->width(sel->getName());
			int32_t v11;
			if(this->field_A00 <= 0.75f) {
				v11 = 255;
			} else {
				v11 = (int32_t)(float)(this->cubeSmoothStep((float)(0.25f - (float)(this->field_A00 - 0.75f)) * 4.0f, 0.0f, 1.0f) * 255.0f);
				if(!v11) return;
			}

			a3->drawShadow(sel->getName(), (float)(a2 / 2 - v10 / 2), (float)(a4 - 22), (v11 << 24) + 0xFFFFFF);
		}
	}
}

void Gui::renderProgressIndicator(bool_t a2, int32_t a3, int32_t a4, float a5) {
	if (!this->minecraftInst || !this->minecraftInst->player || !this->minecraftInst->texturesPtr) return;

	bool_t v11 = 0;
	bool_t v12 = 0;
	ItemInstance* Selected = this->minecraftInst->player->inventory->getSelected();
	if(Selected) {
		Item* itemClass = Selected->itemClass;
		v11 = (itemClass == Item::bow);
		if (this->minecraftInst->player->getUseItem()) {
			v12 = (itemClass == this->minecraftInst->player->getUseItem()->itemClass);
		}
	}

	if(!a2 || this->minecraftInst->options.useJoypad) {
		LABEL_8:
		this->minecraftInst->texturesPtr->loadAndBindTexture("gui/icons.png");
		glBlendFunc(0x307u, 0x301u);
		this->blit(a3 / 2 - 8, a4 / 2 - 8, 0, 0, 16, 16, 0, 0);
		glBlendFunc(0x302u, 0x303u);
		return;
	}
	if(v11) {
		if(!v12) return;
		goto LABEL_8;
	}

	float v15 = 1.0f;
	IInputHolder* inputHolder = this->minecraftInst->inputHolder;
	if (!inputHolder) return;

	float v17 = inputHolder->field_C;
	GameMode* gameMode = this->minecraftInst->gameMode;
	if (!gameMode) return;

	float v19 = gameMode->field_8;
	if(v17 > 1.0f) {
		LABEL_12:
		if(v19 <= 0.0f && v17 > 0.0f) {
			v17 = v15;
			LABEL_15:
			DisableState v27(3553);
			float v20;
			if(this->minecraftInst->selectedObject.hitType == 2) {
				float v21 = v17 * 0.4f;
				v20 = (v21 < 0.4f) ? v21 : 0.4f;
			} else {
				v20 = v17 * 0.8f;
			}
			glColor4f(1.0f, 1.0f, 1.0f, v20);
			IInputHolder* v22 = this->minecraftInst->inputHolder;
			float v23 = Gui::InvGuiScale * v22->mouseX;
			float v24 = Gui::InvGuiScale * v22->mouseY;
			glTranslatef(v23, v24, 0.0f);
			this->outerBreakRingMesh.render();
			glTranslatef(-v23, -v24, 0.0f);
			return;
		}
		goto LABEL_21;
	}
	if(v17 <= 0.0f) {
		v15 = 0.0f;
		goto LABEL_12;
	}
	if(v19 <= 0.0f) goto LABEL_15;

	v15 = inputHolder->field_C;
	LABEL_21:
	if(v19 <= 0.0f) return;

	float v25 = gameMode->field_4;
	DisableState v27(3553);
	glPushMatrix();
	glColor4f(1.0f, 1.0f, 1.0f, v15 * 0.8f);
	glTranslatef(Gui::InvGuiScale * this->minecraftInst->inputHolder->mouseX, Gui::InvGuiScale * this->minecraftInst->inputHolder->mouseY, 0.0f);
	this->outerBreakRingMesh.render();
	float v26 = (float)((float)(v25 + (float)((float)(v19 - v25) * a5)) * 0.5f) + 0.5f;
	glScalef(v26, v26, 1.0f);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glBlendFunc(0x307u, 0x301u);
	this->innerBreakRingMesh.render();
	glBlendFunc(0x302u, 0x303u);
	glPopMatrix();
}

void Gui::renderSleepAnimation(int32_t a2, int32_t a3) {
	if (!this->minecraftInst || !this->minecraftInst->player) return;

	int32_t v6 = this->minecraftInst->player->getSleepTimer();
	float v7 = (float)v6 / 100.0f;
	if(v7 > 1.0f) v7 = 1.0f - (float)(v6 - 100) / 10.0f;
	this->fill(0, 0, a2, a3, ((int32_t)(float)(v7 * 220.0f) << 24) | 0x101020);
}

void Gui::renderSlot(int32_t a2, int32_t a3, int32_t a4, float a5) {
	if (!this->minecraftInst || !this->minecraftInst->player || !this->minecraftInst->player->inventory || !this->minecraftInst->texturesPtr) return;

	LocalPlayer* player = this->minecraftInst->player;
	if(player->abilities.instabuild) {
		a2 += 9;
	}
	ItemInstance* v9 = player->inventory->getItem(a2);
	if(v9) {
		Item* itemClass = v9->itemClass;
		int32_t v12 = (itemClass && itemClass->field_10) ? itemClass->getAnimationFrameFor(this->minecraftInst->player) : 0;
		ItemRenderer::renderGuiItemNew(this->minecraftInst->texturesPtr, v9, v12, (float)a3, (float)a4, 1.0f, 1.0f, 1.0f);
	}
}

void Gui::renderSlotText(const struct ItemInstance* a2, float a3, float a4, bool_t a5, bool_t a6) {
	if (!a2 || !this->minecraftInst || !this->minecraftInst->font) return;

	int count = a2->count;
	if(count > 1) {
		char_t v15[4] = {0, 0, 0, 0};
		if(a5) {
			Gui::itemCountItoa(v15, count);
		} else {
			v15[0] = -99;
		}
		int v11 = a2->count;
		Font* font = this->minecraftInst->font;
		int v13 = (v11 > 0) ? 0xFFCCCCCC : 0x60CCCCCC;
		if(a6) {
			font->drawShadow(v15, a3, a4, v13);
		} else {
			font->draw(v15, a3, a4, v13);
		}
	}
}

void Gui::renderToolBar(float a2, float a3) {
	if (!this->minecraftInst || !this->minecraftInst->player || !this->minecraftInst->player->inventory || !this->minecraftInst->texturesPtr) return;

	Minecraft* minecraftInst = this->minecraftInst;
	int32_t v7 = (int32_t)(float)((float)minecraftInst->field_1C * Gui::InvGuiScale);
	int32_t v8 = (int32_t)(float)((float)minecraftInst->field_20 * Gui::InvGuiScale);
	int32_t v9 = v8 - 19;
	glColor4f(1.0f, 1.0f, 1.0f, a3);
	this->minecraftInst->texturesPtr->loadAndBindTexture("gui/gui.png");
	Inventory* inventory = this->minecraftInst->player->inventory;
	int32_t sp = 0, sp2 = 0;
	this->getSlotPos(0, sp, sp2);
	float v12 = (float)((float)sp + 3.0f) + 1.0f;
	int32_t v13 = 20 * this->getNumSlots();
	this->blit(sp, sp2, 0, 0, v13, 22, 0, 0);
	this->blit(sp + v13, sp2, 180, 0, 2, 22, 0, 0);

	if(this->field_A84 >= 0) {
		if(inventory->getItem(this->field_A84)) {
			int32_t v35 = sp + 3 + 20 * this->field_A84;
			float v36 = this->field_A80;
			if(v36 >= 3.0f) {
				glColor4f(0.0f, 1.0f, 0.0f, a3);
			}
			this->fill(v35, v8 - 3 - (int32_t)(float)((float)((float)(v36 + a2) * 17.0f) / 40.0f), v35 + 16, v8 - 3, 0x8000FF00);
		}
	}

	this->blit(sp - 1 + 20 * inventory->selectedSlot, sp2 - 1, 0, 22, 24, 22, 0, 0);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	if(this->field_A18 >= 0) {
		float v14 = getTimeS() - this->field_A20;
		if(v14 <= 0.2f) {
			int32_t v15 = -10 * this->getNumSlots() + v7 / 2 + 20 * this->field_A18;
			float v16 = Mth::cos(v14 * 62.8f);
			this->fill(v15 + 2, v8 - 19, v15 + 18, v8 - 3, ((int32_t)(float)(81.0f - (float)(v16 * 80.0f)) << 24) + 0xFFFFFF);
		} else {
			this->field_A18 = -1;
		}
	}

	float v17 = v12;
	int32_t v18 = 0;
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_DEPTH_TEST);
	while(v18 < this->getNumSlots() - 1) {
		int32_t v19 = v18++;
		this->renderSlot(v19, (int32_t)v17, v8 - 20, a2);
		v17 = v17 + 20.0f;
	}
	this->invUpdated = 0;
	this->minecraftInst->texturesPtr->loadAndBindTexture("gui/gui.png");
	int32_t NumSlots = this->getNumSlots();
	int32_t v23 = v8 - 13;
	int32_t v24 = 0;
	this->blit(10 * NumSlots + v7 / 2 - 16, v23, 228, 248, 14, 4, 28, 8);
	{
		DisableState v39(0xDE1);
		Tesselator::instance.beginOverride();
		float v25 = v12 - 1.0f;
		while(v24 < this->getNumSlots() - 1) {
			int32_t v26 = v24++;
			ItemRenderer::renderGuiItemDecorations(this->minecraftInst->player->inventory->getItem(v26), v25, (float)v9);
			v25 = v25 + 20.0f;
		}
		Tesselator::instance.endOverrideAndDraw();
	}
	glPushMatrix();
	glScalef(Gui::InvGuiScale + Gui::InvGuiScale, Gui::InvGuiScale + Gui::InvGuiScale, 1.0f);
	float v29 = Gui::GuiScale;
	Tesselator::instance.beginOverride();
	if(this->minecraftInst->gameMode && this->minecraftInst->gameMode->isSurvivalType()) {
		int32_t v31 = 0;
		float v32 = v29 * 0.5f;
		while(v31 < this->getNumSlots() - 1) {
			ItemInstance* v34 = this->minecraftInst->player->inventory->getItem(v31);
			if(v34 && v34->count >= 0) {
				this->renderSlotText(v34, v32 * v12, (float)(v32 * (float)v9) + 1.0f, 1, 1);
			}
			v12 = v12 + 20.0f;
			++v31;
		}
	}

	if (this->minecraftInst->texturesPtr) {
		this->minecraftInst->texturesPtr->loadAndBindTexture("font/default8.png");
	}
	Tesselator::instance.endOverrideAndDraw();
	glPopMatrix();
}

void Gui::renderVignette(float a2, int32_t a3, int32_t a4) {
	if (!this->minecraftInst || !this->minecraftInst->texturesPtr) return;

	float v7 = 1.0f - a2;
	if(v7 < 0.0f) v7 = 0.0f;
	else if(v7 > 1.0f) v7 = 1.0f;

	float* v8 = &this->field_A10;
	float v9 = this->field_A10;
	DisableState v13(2929);
	this->field_A10 = v9 + (float)((float)(v7 - v9) * 0.01f);
	glBlendFunc(0, 0x301u);
	glDepthMask(0);
	glColor4f(*v8, *v8, *v8, 1.0f);

	Textures* texturesPtr = this->minecraftInst->texturesPtr;
	texturesPtr->loadAndBindTexture("misc/vignette.png");
	Tesselator::instance.begin(4);
	float v11 = (float)a4;
	Tesselator::instance.vertexUV(0.0f, v11, -90.0f, 0.0f, 1.0f);
	float v12 = (float)a3;
	Tesselator::instance.vertexUV(v12, v11, -90.0f, 1.0f, 1.0f);
	Tesselator::instance.vertexUV(v12, 0.0f, -90.0f, 1.0f, 0.0f);
	Tesselator::instance.vertexUV(0.0f, 0.0f, -90.0f, 0.0f, 0.0f);
	Tesselator::instance.draw(1);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glDepthMask(1u);
	glBlendFunc(0x302u, 0x303u);
}

void Gui::resetItemNameOverlay() {
	this->field_A00 = 0;
}

void Gui::setNowPlaying(const std::string& a2) {
	this->field_A04 = "Now playing: " + a2;
	this->field_A08 = 60;
	this->field_A0C = 1;
}

void Gui::setScissorRect(const IntRectangle& a2) {
	if (!this->minecraftInst) return;
	glScissor((uint32_t)(float)(Gui::GuiScale * (float)a2.minX), this->minecraftInst->field_20 - (uint32_t)(float)(Gui::GuiScale * (float)(a2.height + a2.minY)), (uint32_t)(float)(Gui::GuiScale * (float)a2.width), (uint32_t)(float)(Gui::GuiScale * (float)a2.height));
}

void Gui::showTipMessage(const std::string& a2) {
	this->tipMessage = a2;
	this->field_A8C = 40;
	if (this->minecraftInst && this->minecraftInst->font) {
		this->field_A90 = this->minecraftInst->font->getPixelLength(a2);
	}
}

void Gui::texturesLoaded(struct Textures*) {
}

void Gui::tick() {
	int32_t v2 = this->field_A08;
	if(v2 > 0) this->field_A08 = v2 - 1;
	float v3 = this->field_A8C;
	if(v3 > 0) this->field_A8C = v3 - 1;
	++this->field_9FC;
	float v4 = this->field_A00;
	if(v4 < 2) this->field_A00 = v4 + 0.05f;
	for(size_t i = 0; i < this->chatMessages.size(); ++i) {
		++this->chatMessages[i].field_0;
	}
	if(this->minecraftInst && !this->minecraftInst->isCreativeMode()) {
		this->tickItemDrop();
	}
}

char_t _D6E05A98;
void Gui::tickItemDrop(void) {
	if (!this->minecraftInst || !this->minecraftInst->player || !this->minecraftInst->player->inventory) return;

	_D6E05A98 = 0;
	if(Mouse::isButtonDown(1)) {
		int32_t v2 = Mouse::getX();
		int32_t v3 = Mouse::getY();
		int32_t SlotIdAt = this->getSlotIdAt(v2, v3);
		if(SlotIdAt >= 0 && SlotIdAt < this->getNumSlots() - 1) {
			if(SlotIdAt != this->field_A84 || this->minecraftInst->currentScreen) {
				this->field_A80 = 0.0f;
				this->field_A84 = SlotIdAt;
			}
			_D6E05A98 = 1;
			float v5 = this->field_A80 + 1.0f;
			this->field_A80 = v5;
			if(v5 >= 40.0f) {
				this->minecraftInst->player->inventory->dropSlot(SlotIdAt, 0, 0);
				Minecraft* minecraftInst = this->minecraftInst;
				Level* levelPtr = minecraftInst->level;
				Entity* player = (Entity*)minecraftInst->player;
				if (levelPtr && player) {
					levelPtr->playSound(player, "random.pop", 0.3f, 1.0f);
				}
				_D6E05A98 = 0;
			}
		}
	}
	if(!_D6E05A98) {
		this->field_A84 = -1;
		this->field_A80 = -1.0f;
	}
}
