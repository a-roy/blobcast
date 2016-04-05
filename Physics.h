#pragma once
#include <btBulletDynamicsCommon.h>
#include <BulletSoftBody/btSoftBody.h>
#include <BulletSoftBody/btSoftRigidDynamicsWorld.h>
#include <BulletSoftBody/btDefaultSoftBodySolver.h>
#include <BulletSoftBody/btSoftBodyHelpers.h>
#include <BulletSoftBody/btSoftBodyRigidBodyCollisionConfiguration.h>

#include <glm/glm.hpp>

#include <iostream>
#include "config.h"
#include "Blob.h"
//#include "GameObject.h"

class Physics
{
public:
	static Blob *blob;
	static btSoftRigidDynamicsWorld *dynamicsWorld;
	static btCollisionDispatcher *dispatcher;
	static btBroadphaseInterface *broadphase;
	static btSequentialImpulseConstraintSolver *solver;
	static btSoftBodyRigidBodyCollisionConfiguration *collisionConfiguration;
	static btSoftBodySolver *softBodySolver;

	static btSoftBodyWorldInfo softBodyWorldInfo;

	static bool bStepPhysics;
	static bool bShowBulletDebug;

	static void Init();
	static void Cleanup();

	static void CreateBlob(glm::vec4 color = glm::vec4(0,1,0,1));

	struct ContactResultCallback : 
		public btCollisionWorld::ContactResultCallback
	{
		ContactResultCallback(bool* ptr) : context(ptr) {}

		btScalar addSingleResult(btManifoldPoint& cp,
			const btCollisionObjectWrapper* colObj0Wrap,
			int partId0,
			int index0,
			const btCollisionObjectWrapper* colObj1Wrap,
			int partId1,
			int index1);

		bool* context;
	};

	static bool BroadphaseCheck(btCollisionObject* obj1,
		btCollisionObject* obj2);
	static bool NarrowphaseCheck(btSoftBody* sb, btCollisionObject* rb);
	static bool NarrowphaseCheck(btCollisionObject* rb, btSoftBody* sb);

	static glm::vec3 InverseDynamics(glm::vec3 currentPosition, 
		glm::vec3 desiredPosition, glm::vec3 currentVelocity, float mass,
		float deltaTime)
	{
		//desiredPosition = currentPosition + requiredVelocity * dt
		glm::vec3 requiredVelocity = 
			(desiredPosition - currentPosition) / deltaTime; 

		//requiredVelocity = currentVelocity + requiredAccel * dt;																	  
		glm::vec3 requiredAccel = (requiredVelocity - currentVelocity) 
			/ deltaTime;
		
		//F=ma
		return mass * requiredAccel;
	}
};
