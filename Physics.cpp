#include "Physics.h"
#include "config.h"

btSoftRigidDynamicsWorld *Physics::dynamicsWorld;
btCollisionDispatcher *Physics::dispatcher;
btBroadphaseInterface *Physics::broadphase;
btSequentialImpulseConstraintSolver *Physics::solver;
btSoftBodyRigidBodyCollisionConfiguration *Physics::collisionConfiguration;
btSoftBodySolver *Physics::softBodySolver;

btSoftBodyWorldInfo Physics::softBodyWorldInfo;

bool Physics::bStepPhysics = false;
bool Physics::bShowBulletDebug = true;

void Physics::init()
{
	broadphase = new btDbvtBroadphase();
	btVector3 worldAabbMin(-1000, -1000, -1000);
	btVector3 worldAabbMax(1000, 1000, 1000);
	broadphase = new btAxisSweep3(worldAabbMin, worldAabbMax, MAX_PROXIES);

	collisionConfiguration =
		new btSoftBodyRigidBodyCollisionConfiguration();
	dispatcher = new btCollisionDispatcher(collisionConfiguration);
	solver = new btSequentialImpulseConstraintSolver();
	softBodySolver = new btDefaultSoftBodySolver();
	dynamicsWorld = new btSoftRigidDynamicsWorld(dispatcher,
		broadphase, solver, collisionConfiguration, softBodySolver);

	dynamicsWorld->setGravity(btVector3(0, -10, 0));

	softBodyWorldInfo.m_broadphase = broadphase;
	softBodyWorldInfo.m_dispatcher = dispatcher;
	softBodyWorldInfo.m_gravity.setValue(0, -10, 0);
	softBodyWorldInfo.air_density = (btScalar)1.2;
	softBodyWorldInfo.water_density = 0;
	softBodyWorldInfo.water_offset = 0;
	softBodyWorldInfo.water_normal = btVector3(0, 0, 0);
	softBodyWorldInfo.m_sparsesdf.Initialize();
}