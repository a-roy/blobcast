#pragma once

#include <GL/glew.h>
#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>
#include "ShaderProgram.h"
#include "Line.h"
#include "Helper.h"

class BulletDebugDrawer_DeprecatedOpenGL : public btIDebugDraw{

public:

	int m;

	void SetMatrices(glm::mat4 pViewMatrix, glm::mat4 pProjectionMatrix)
	{
		glUseProgram(0); // Use Fixed-function pipeline (no shaders)
		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf(&pViewMatrix[0][0]);
		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf(&pProjectionMatrix[0][0]);
	}

	virtual void drawLine(const btVector3& from,const btVector3& to,const btVector3& color)
	{
		glColor3f(color.x(), color.y(), color.z());
		glBegin(GL_LINES);
			glVertex3f(from.x(), from.y(), from.z());
			glVertex3f(to.x(), to.y(), to.z());
		glEnd();
	}

	virtual void drawContactPoint(const btVector3 &,const btVector3 &,btScalar,int,const btVector3 &){}
	virtual void reportErrorWarning(const char *){}
	virtual void draw3dText(const btVector3 &,const char *){}
	
	virtual void setDebugMode(int p){ m = p;}
	int getDebugMode(void) const {return 3;}
	
};

//class BulletDebugDrawer : public btIDebugDraw {
//
//public:
//
//	int m;
//	ShaderProgram *debugdrawShaderProgram;
//	Line *l;
//
//	void SetMatrices(glm::mat4 pViewMatrix, glm::mat4 pProjectionMatrix)
//	{
//		glm::mat4 mvpMatrix = pProjectionMatrix * pViewMatrix;
//		debugdrawShaderProgram->SetUniform("uMVPMatrix", mvpMatrix);
//	}
//
//	virtual void drawLine(const btVector3& from, const btVector3& to, const btVector3& color)
//	{
//		debugdrawShaderProgram->SetUniform("uColor", glm::vec4(color.getX(), color.getY(), color.getZ(), 1));
//		l->Update(glm::vec3(from.getX(), from.getY(), from.getZ()), glm::vec3(to.getX(), to.getY(), to.getZ()));
//		l->Render();
//	}
//
//	virtual void drawContactPoint(const btVector3 &, const btVector3 &, btScalar, int, const btVector3 &) {}
//	virtual void reportErrorWarning(const char *) {}
//	virtual void draw3dText(const btVector3 &, const char *) {}
//
//	virtual void setDebugMode(int p) { m = p; }
//	int getDebugMode(void) const { return 3; }
//
//};