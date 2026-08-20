#include <input/Controller.hpp>
#include <math.h>

float Controller::stickValuesX[2] = {0.0f, 0.0f};
float Controller::stickValuesY[2] = {0.0f, 0.0f};
bool_t Controller::isTouchedValues[2] = {0, 0};

void Controller::feed(int32_t a2, int32_t a3, float a4, float a5) {
	if(Controller::isValidStick(a2)) {
		int32_t idx = a2 - 1;
		Controller::isTouchedValues[idx] = (a3 != 0);
		Controller::stickValuesX[idx] = a4;
		Controller::stickValuesY[idx] = a5;
	}
}

float Controller::getTransformedX(int32_t a2, float a3, float a4, bool_t a5) {
	if(Controller::isValidStick(a2)) {
		return Controller::linearTransform(Controller::stickValuesX[a2 - 1], a3, a4, a5);
	}
	return 0.0f;
}

float Controller::getTransformedY(int32_t a2, float a3, float a4, bool_t a5) {
	if(Controller::isValidStick(a2)) {
		return Controller::linearTransform(Controller::stickValuesY[a2 - 1], a3, a4, a5);
	}
	return 0.0f;
}

float Controller::getX(int32_t a1) {
	if(Controller::isValidStick(a1)) return Controller::stickValuesX[a1 - 1];
	return 0.0f;
}

float Controller::getY(int32_t a1) {
	if(Controller::isValidStick(a1)) return Controller::stickValuesY[a1 - 1];
	return 0.0f;
}

bool_t Controller::isTouched(int32_t a1) {
	if(Controller::isValidStick(a1)) return Controller::isTouchedValues[a1 - 1];
	return 0;
}

bool_t Controller::isValidStick(int32_t a1) {
	return (uint32_t)(a1 - 1) <= 1;
}

float Controller::linearTransform(float a2, float a3, float a4, bool_t a5) {
	float v4 = (a2 < 0.0f) ? -a3 : a3;
	if(fabsf(v4) >= fabsf(a2)) {
		return 0.0f;
	}
	float v5 = (a2 - v4) * a4;
	if(a5 && fabsf(v5) > 1.0f) {
		return (v5 < 0.0f) ? -1.0f : 1.0f;
	}
	return v5;
}
