#pragma once

struct Vector {
	float x = 0, y = 0, z = 0;
};
float NormalizeVectorValue(float vectorValue);
Vector ClampVector(Vector &vector);