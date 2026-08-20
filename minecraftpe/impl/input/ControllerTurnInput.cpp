#include <input/ControllerTurnInput.hpp>
#include <input/Controller.hpp>

ControllerTurnInput::ControllerTurnInput() {
	this->curTime = -1.0;
	this->field_20 = 0;
	this->field_18 = 0.0;
	this->field_1C = 0.0;
	this->field_10 = 2;
	this->field_14 = 2;
}

ControllerTurnInput::~ControllerTurnInput() {
}

Vec3 ControllerTurnInput::getTurnDelta() {
	float rx = Controller::getX(2);
	float ry = Controller::getY(2);

	// Sensibilidad de giro de la camara
	float sens = 8.5f;
	return Vec3(rx * sens, -ry * sens, 0.0f);
}
