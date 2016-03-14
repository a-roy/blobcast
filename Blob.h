#pragma once

#include "SoftBody.h"
#include <GLFW/glfw3.h>

#include <iostream>
#include <algorithm>
#include <sstream>

#include "BlobInput.h"
#include "Line.h"
#include "Helper.h"
#include "ShaderProgram.h"

enum MovementMode { averaging, stretch };

class Blob : public SoftBody
{

private:
	btSoftBody::Node *sampleNodes[6] = { NULL };
	btVector3 centroid;
	btScalar radius;

public:
	btVector3 forward;
	float speed;
	MovementMode movementMode = MovementMode::stretch;

	Blob(
			btSoftBodyWorldInfo& softBodyWorldInfo,
			const btVector3& center, btScalar scale, int vertices);
	~Blob();

	void Update();
	void Move(int key, int action);

	void AddForce(const btVector3 &force);
	void AddForces(AggregateInput inputs);
	void AddForce(const btVector3 &force, int i);
	void AddForces(float magFwd, float magBack, float magLeft, float magRight);

	void ComputeCentroid();
	btVector3 GetCentroid();

	void DrawGizmos(ShaderProgram* shaderProgram);
};
