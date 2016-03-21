#pragma once

#include "Entity.h"

class Platform : public Entity
{
public:
	Path motion;

	Platform(Mesh* p_mesh, Shape p_shapeType, glm::vec3 p_translation,
		glm::quat p_orientation, glm::vec3 p_scale, glm::vec4 p_color,
		float p_mass = 0)
		: Entity(p_mesh, p_shapeType, p_translation, p_orientation,
			p_scale, p_color, p_mass)
	{

	}

	Platform(Platform& p) :
		Platform(Mesh::CreateCubeWithNormals(),
			p.shapeType,
			p.GetTranslation(),
			p.GetOrientation(),
			p.GetScale(),
			p.trueColor,
			p.mass) {}

	void Update(float deltaTime)
	{
		if (!motion.Points.empty())
		{
			if (motion.Enabled)
			{
				if (motion.Step())
				{
					/*rigidbody->translate(
						convert(motion.GetPosition() - GetTranslation()));
					rigidbody->setLinearVelocity(btVector3(0, 0, 0));*/

					rigidbody->applyCentralForce(convert(
						Physics::InverseDynamics(GetTranslation(),
						motion.GetPosition(),
						convert(rigidbody->getLinearVelocity()),
						mass, deltaTime)));
				}
			}
		}
	}
};