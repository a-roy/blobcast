#pragma once

#include <btBulletDynamicsCommon.h>
#include "Mesh.h" 
#include <glm/gtc/type_ptr.hpp>
#include "Helper.h"""

//enum RBShape { Box }; //TODO - add more shapes support (as needed)

class RigidBody
{

private:

	Mesh* mesh;
	
public:

	btRigidBody* rigidbody;
	glm::vec3 globalScale;
	glm::vec4 color;
	glm::vec4 trueColor;
	float mass;

	RigidBody(Mesh* p_mesh, glm::vec3 p_translation, glm::quat p_orientation, 
		glm::vec3 p_scale, glm::vec4 p_color, float p_mass = 0);
	~RigidBody();

	glm::mat4 GetModelMatrix();

	void Render();

	glm::quat GetOrientation() 
	{
		return convert(rigidbody->getOrientation());
	}
	glm::vec3 GetTranslation() 
	{
		return convert(rigidbody->getWorldTransform().getOrigin());
	}
};
