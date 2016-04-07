#include "Blob.h"
#include <imgui.h>
#include "imgui_impl_glfw.h"
#include "Timer.h"

Blob::Blob(
		btSoftBodyWorldInfo& softBodyWorldInfo,
		const btVector3& center, btScalar r, int vertices) :
	SoftBody(btSoftBodyHelpers::CreateEllipsoid(
				softBodyWorldInfo, center, btVector3(1, 1, 1) * r, vertices)),
	centroid(center),
	radius(r),
	speed(5500.f / vertices)
{
	forward = btVector3(0, 0, -1);
	btVector3 scale = btVector3(1, 1, 1) * r;
	btVector3 points[6] = {
		btVector3(center[0], center[1] - scale[1], center[2]),
		btVector3(center[0], center[1] + scale[1], center[2]),
		btVector3(center[0] - scale[0], center[1], center[2]),
		btVector3(center[0] + scale[0], center[1], center[2]),
		btVector3(center[0], center[1], center[2] - scale[2]),
		btVector3(center[0], center[1], center[2] + scale[2]) };
	for (unsigned int i = 0, n = softbody->m_nodes.size(); i < n; i++)
	{
		btSoftBody::Node *node = &softbody->m_nodes[i];
		for (unsigned int j = 0; j < 6; j++)
		{
			if (sampleNodes[j] == NULL)
				sampleNodes[j] = node;
			else if (btDistance2(node->m_x, points[j]) <
					btDistance2(sampleNodes[j]->m_x, points[j]))
			{
				sampleNodes[j] = node;
			}
		}
	}

	softbody->m_materials[0]->m_kLST = 0.1;
	softbody->m_cfg.kDF = 1;
	softbody->m_cfg.kDG = 0.008;
	softbody->m_cfg.kDP = 0.001;
	softbody->m_cfg.kPR = 2500;
	softbody->setTotalMass(30, true);

	softbody->m_cfg.collisions |= btSoftBody::fCollision::CL_RS +
		btSoftBody::fCollision::RVSmask,	///Rigid versus soft mask
		btSoftBody::fCollision::SDF_RS,	///SDF based rigid vs soft
		btSoftBody::fCollision::CL_RS; ///Cluster vs convex rigid vs soft;
	//class btSoftRididCollisionAlgorithm;
	//class btSoftRigidDynamicsWorld;

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
		AddForce(btVector3(0, 0, -1));
	if (key == GLFW_KEY_DOWN && (action == GLFW_PRESS || action == GLFW_REPEAT))
		AddForce(btVector3(0, 0, 1));
	if (key == GLFW_KEY_LEFT && (action == GLFW_PRESS || action == GLFW_REPEAT))
		AddForce(btVector3(-1, 0, 0));
	if (key == GLFW_KEY_RIGHT && (action == GLFW_PRESS || action == GLFW_REPEAT))
		AddForce(btVector3(1, 0, 0));
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

void Blob::AddForces(const AggregateInput& inputs)
{
	float total = (float)inputs.TotalCount;
	if (total == 0.0f)
		total = 1.0f;
	float magFwd = (float)inputs.FCount / total,
		  magBack = (float)inputs.BCount / total,
		  magRight = (float)inputs.RCount / total,
		  magLeft = (float)inputs.LCount / total,
		  magFR = (float)inputs.FRCount / total,
		  magFL = (float)inputs.FLCount / total,
		  magBR = (float)inputs.BRCount / total,
		  magBL = (float)inputs.BLCount / total;
	btVector3 right = forward.cross(btVector3(0, 1, 0));
	btVector3 fwdright = (forward + right) * SIMDSQRT12;
	btVector3 fwdleft = (forward - right) * SIMDSQRT12;
#pragma loop(hint_parallel(0))
#pragma loop(ivdep)
	for (int i = 0, n = softbody->m_nodes.size(); i < n; i++)
	{
		btVector3 blobSpaceDir =
			((softbody->m_nodes[i].m_x - centroid) *
			 btVector3(1, 0, 1)).normalized();
		btScalar magnitude = btMin(
			btPow(btMax(btDot(blobSpaceDir, forward), 0.f), 2) * magFwd +
			btPow(btMax(btDot(blobSpaceDir, -forward), 0.f), 2) * magBack +
			btPow(btMax(btDot(blobSpaceDir, right), 0.f), 2) * magRight +
			btPow(btMax(btDot(blobSpaceDir, -right), 0.f), 2) * magLeft +
			btPow(btMax(btDot(blobSpaceDir, fwdright), 0.f), 2) * magFR +
			btPow(btMax(btDot(blobSpaceDir, fwdleft), 0.f), 2) * magFL +
			btPow(btMax(btDot(blobSpaceDir, -fwdleft), 0.f), 2) * magBR +
			btPow(btMax(btDot(blobSpaceDir, -fwdright), 0.f), 2) * magBL,
			1.0f);
		btVector3 force = blobSpaceDir * magnitude;
		AddForce(force, i);
	}

	btScalar amplitude(0.2 / speed);
	btScalar bounce = btSin(Timer::currentFrame) * amplitude;
	AddForce(btVector3(0, bounce, 0));
}

void Blob::ComputeCentroid()
{
	centroid = btVector3(0, 0, 0);
	for (int i = 0; i < 6; i++)
		centroid += sampleNodes[i]->m_x;
	centroid *= (1.f/6.f);
}

btVector3 Blob::GetCentroid()
{
	return centroid;
}

void Blob::DrawGizmos(ShaderProgram* shaderProgram)
{
	glm::vec3 L = convert(forward.rotate(btVector3(0, 1, 0), -glm::quarter_pi<float>()));
	glm::vec3 R = convert(forward.rotate(btVector3(0, 1, 0), glm::quarter_pi<float>()));
	glm::vec3 BL = -R;
	glm::vec3 BR = -L;

	glm::vec3 c = convert(GetCentroid());

	Line l(c, c + L * 10.0f);
	Line r(c, c + R * 10.0f);
	Line bl(c, c + BL * 10.0f);
	Line br(c, c + BR * 10.0f);
	Line fwd(c, c + convert(forward) * 10.0f);

	shaderProgram->Use([&](){
		l.Render();
		r.Render();
		bl.Render();
		br.Render();
	});

	(*shaderProgram)["uColor"] = glm::vec4(1, 0, 0, 1);
	shaderProgram->Use([&](){
		fwd.Render();
	});
}

void Blob::Gui()
{
	ImGui::SetNextWindowSize(ImVec2(400, 500), ImGuiSetCond_FirstUseEver);
	ImGui::Begin("Blob Edtior");
	ImGui::SliderFloat("Rigid Contacts Hardness [0,1]",
		&softbody->m_cfg.kCHR, 0.0f, 1.0f);
	ImGui::SliderFloat("Dynamic Friction Coefficient [0,1]",
		&softbody->m_cfg.kDF, 0.0f, 1.0f);
	ImGui::InputFloat("Pressure coefficient [-inf,+inf]",
		&softbody->m_cfg.kPR, 1.0f, 100.0f);
	ImGui::InputFloat("Volume conservation coefficient [0, +inf]",
		&softbody->m_cfg.kVC, 1.0f, 100.0f);
	ImGui::InputFloat("Drag coefficient [0, +inf]",
		&softbody->m_cfg.kDG, 1e-4f, 100.0f);
	ImGui::SliderFloat("Damping coefficient [0,1]",
		&softbody->m_cfg.kDP, 0.0f, 1.0f);
	ImGui::InputFloat("Lift coefficient [0,+inf]",
		&softbody->m_cfg.kLF, 1.0f, 100.0f);
	ImGui::SliderFloat("Pose matching coefficient [0,1]",
		&softbody->m_cfg.kMT, 0.0f, 1.0f);

	//softbody->m_cfg.maxvolume
	//softbody->setTotalMass

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	ImGui::InputFloat("Movement force", &speed, 0.1f, 100.0f);

	static float vec3[3] = { 0.f, 0.f, 0.f };
	if (ImGui::InputFloat3("", vec3))
		ImGui::SameLine();
	if (ImGui::SmallButton("Set Position"))
		softbody->translate(
			btVector3(vec3[0], vec3[1], vec3[2]) - centroid);

	ImGui::End();
}
