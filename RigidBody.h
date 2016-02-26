#pragma once

#include <btBulletDynamicsCommon.h>
#include "Mesh.h" 
#include <glm/gtc/type_ptr.hpp>

//enum RBShape { Box }; //TODO - add more shapes support (as needed)

class RigidBody
{

private:

	Mesh* mesh;

public:

	btRigidBody* rigidbody;
	glm::vec3 translation;
	glm::quat orientation;
	glm::vec3 scale;
	glm::vec4 color;
	float mass;

	RigidBody(Mesh* p_mesh, glm::vec3 p_translation, glm::quat p_orientation, glm::vec3 p_scale, glm::vec4 p_color, float p_mass = 0);
	~RigidBody();

	glm::mat4 GetModelMatrix();

	void Update();
	void Render();
};
