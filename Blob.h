#pragma once

#include "SoftBody.h"
#include <GLFW\glfw3.h>

#include <iostream>
#include <algorithm>
#include <sstream>

#include "Line.h"
#include "Helper.h"
#include "ShaderProgram.h"

enum MovementMode { averaging, stretch };

class Blob : public SoftBody
{

private:
	unsigned int sampleIndices[6] = { 0 };

public:
	btVector3 forward;
	float speed = 2.2f;
	MovementMode movementMode = MovementMode::stretch;

	Blob(
			btSoftBodyWorldInfo& softBodyWorldInfo,
			const btVector3& center, const btVector3& scale, int vertices);
	~Blob();

	void Move(int key, int action);

	void AddForce(const btVector3 &force);
	void AddForce(const btVector3 &force, int i);
	void AddForces(float magFwd, float magBack, float magLeft, float magRight);

	btVector3 GetCentroid();

	void DrawGizmos(ShaderProgram* shaderProgram);
};
