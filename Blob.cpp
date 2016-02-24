#include "Blob.h"

Blob::Blob(btSoftBody* p_softBody)
	: SoftBody(p_softBody)
{
	forward = btVector3(0, 0, 1);
	invSize = 1.0f / softbody->m_nodes.size();
}

Blob::~Blob()
{

}

void Blob::Move(int key, int action)
{
	if (key == GLFW_KEY_UP && (action == GLFW_PRESS || action == GLFW_REPEAT))
		AddForce(btVector3(0, 0, 1));
	if (key == GLFW_KEY_DOWN && (action == GLFW_PRESS || action == GLFW_REPEAT))
		AddForce(btVector3(0, 0, -1));
	if (key == GLFW_KEY_LEFT && (action == GLFW_PRESS || action == GLFW_REPEAT))
		AddForce(btVector3(1, 0, 0));
	if (key == GLFW_KEY_RIGHT && (action == GLFW_PRESS || action == GLFW_REPEAT))
		AddForce(btVector3(-1, 0, 0));
	if (key == GLFW_KEY_SPACE && (action == GLFW_PRESS || action == GLFW_REPEAT))
		AddForce(btVector3(0, 1, 0));
}

void Blob::AddForce(const btVector3 &force)
{
	if (force.length() > 0.0f)
		softbody->addForce(force * speed);
}

void Blob::AddForce(const btVector3 &force, int i)
{
	if (force.length() > 0.0f)
		softbody->addForce(force * speed, i);
}

void Blob::AddForces(float magFwd, float magBack, float magLeft, float magRight)
{
	btVector3 right = forward.cross(btVector3(0, 1, 0));

	btVector3 L = forward.rotate(btVector3(0, 1, 0), -glm::quarter_pi<float>()); //assuming that forward is always on XZ plane
	btVector3 R = forward.rotate(btVector3(0, 1, 0), glm::quarter_pi<float>());

	for (int i = 0; i < softbody->m_nodes.size(); i++)
	{
		btVector3 blobSpaceNode = softbody->m_nodes[i].m_x - GetCentroid();

		if (blobSpaceNode.dot(L) > 0)
			if (blobSpaceNode.dot(R) > 0)
				AddForce(-right * magLeft, i);
			else
				AddForce(forward * magFwd, i);
		else
			if (blobSpaceNode.dot(R) > 0)
				AddForce(-forward * magBack, i);
			else
				AddForce(right * magRight, i);
	}
}

btVector3 Blob::GetCentroid()
{
	btVector3 centroid = btVector3(0, 0, 0);
	for (int i = 0; i < softbody->m_nodes.size(); i++)
		centroid += softbody->m_nodes[i].m_x;
	centroid *= invSize;

	return centroid;
}

void Blob::DrawGizmos(ShaderProgram* shaderProgram)
{
	glm::vec3 L = convert(&forward.rotate(btVector3(0, 1, 0), -glm::quarter_pi<float>()));
	glm::vec3 R = convert(&forward.rotate(btVector3(0, 1, 0), glm::quarter_pi<float>()));
	glm::vec3 BL = -R;
	glm::vec3 BR = -L;

	glm::vec3 c = convert(&GetCentroid());

	Line l(c, c + L * 10.0f);
	Line r(c, c + R * 10.0f);
	Line bl(c, c + BL * 10.0f);
	Line br(c, c + BR * 10.0f);
	Line fwd(c, c + convert(&forward) * 10.0f);

	l.Render();
	r.Render();
	bl.Render();
	br.Render();

	shaderProgram->SetUniform("uColor", glm::vec4(1, 0, 0, 1));
	fwd.Render();
}