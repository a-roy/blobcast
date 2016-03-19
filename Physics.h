#pragma once
#include <btBulletDynamicsCommon.h>
#include <BulletSoftBody/btSoftBody.h>
#include <BulletSoftBody/btSoftRigidDynamicsWorld.h>
#include <BulletSoftBody/btDefaultSoftBodySolver.h>
#include <BulletSoftBody/btSoftBodyHelpers.h>
#include <BulletSoftBody/btSoftBodyRigidBodyCollisionConfiguration.h>

#include <glm/glm.hpp>

#include <iostream>

class Physics
{
public:
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

	//Works between rigid bodies only!
	struct ContactResultCallback : 
		public btCollisionWorld::ContactResultCallback
	{
		btScalar addSingleResult(btManifoldPoint& cp,
			const btCollisionObjectWrapper* colObj0Wrap,
			int partId0,
			int index0,
			const btCollisionObjectWrapper* colObj1Wrap,
			int partId1,
			int index1)
		{
			std::cout << "Collision!";
			return 1;
		}
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