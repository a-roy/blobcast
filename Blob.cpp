#include "Blob.h"

Blob::Blob(
		btSoftBodyWorldInfo& softBodyWorldInfo,
		const btVector3& center, const btVector3& scale, int vertices) :
	SoftBody(btSoftBodyHelpers::CreateEllipsoid(
				softBodyWorldInfo, center, scale, vertices)),
	speed(1120.f / vertices),
	centroid(center)
{
	forward = btVector3(0, 0, 1);
	btVector3 points[6] = {
		btVector3(center[0], center[1] - scale[1], center[2]),
		btVector3(center[0], center[1] + scale[1], center[2]),
		btVector3(center[0] - scale[0], center[1], center[2]),
		btVector3(center[0] + scale[0], center[1], center[2]),
		btVector3(center[0], center[1], center[2] - scale[2]),
		btVector3(center[0], center[1], center[2] + scale[2]) };
	for (unsigned int i = 0, n = softbody->m_nodes.size(); i < n; i++)
	{
		for (unsigned int j = 0; j < 6; j++)
		{
			if (btDistance2(softbody->m_nodes[i].m_x, points[j]) <
				btDistance2(softbody->m_nodes[sampleIndices[j]].m_x, points[j]))
			{
				sampleIndices[j] = i;
			}
		}
	}

	softbody->m_materials[0]->m_kLST = 0.1;
	softbody->m_cfg.kDF = 1;
	softbody->m_cfg.kDP = 0.001;
	softbody->m_cfg.kPR = 2500;
	softbody->setTotalMass(30, true);
}

Blob::~Blob()
{

}

void Blob::Update()
{
	SoftBody::Update();
	ComputeCentroid();
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

void Blob::ComputeCentroid()
{
	centroid = btVector3(0, 0, 0);
	for (int i = 0; i < 6; i++)
		centroid += softbody->m_nodes[sampleIndices[i]].m_x;
	centroid *= (1.f/6.f);
}

btVector3 Blob::GetCentroid()
{
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
