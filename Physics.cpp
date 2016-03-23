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

void Physics::Init()
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

void Physics::Cleanup()
{
	delete Physics::dynamicsWorld;
	delete Physics::solver;
	delete Physics::collisionConfiguration;
	delete Physics::dispatcher;
	delete Physics::broadphase;
}

bool Physics::BroadphaseCheck(btCollisionObject* obj1,
	btCollisionObject* obj2)
{
	btBroadphasePair* pair =
		Physics::broadphase->getOverlappingPairCache()->findPair(
			obj1->getBroadphaseHandle(),
			obj2->getBroadphaseHandle());
	return (pair != NULL);
}

bool Physics::NarrowphaseCheck(btSoftBody* sb,
	btCollisionObject* rb)
{
	for (int i = 0; i < sb->m_rcontacts.size(); i++)
		if (sb->m_rcontacts[i].m_cti.m_colObj == rb)
			return true;

	return false;
	//return obj1->checkCollideWith(obj2); //Always returns true..
}

bool Physics::NarrowphaseCheck(btCollisionObject* rb, btSoftBody* sb)
{
	return NarrowphaseCheck(sb, rb);
}

//OnEnter
//OnStay
//OnLeave

//For trigger objects
/*mBody->setCollisionFlags(mBody->getCollisionFlags() |
btCollisionObject::CF_NO_CONTACT_RESPONSE));*/