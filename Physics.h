#pragma once
#include <btBulletDynamicsCommon.h>
#include <BulletSoftBody/btSoftBody.h>
#include <BulletSoftBody/btSoftRigidDynamicsWorld.h>
#include <BulletSoftBody/btDefaultSoftBodySolver.h>
#include <BulletSoftBody/btSoftBodyHelpers.h>
#include <BulletSoftBody/btSoftBodyRigidBodyCollisionConfiguration.h>

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

	static void init();
};