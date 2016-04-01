#include "Level.h"
#include <json.hpp>
#include <fstream>
#include "Trigger.h"

Level::Level()
{
	Meshes[(size_t)None] = nullptr;
	Meshes[(size_t)Box] = Mesh::CreateCubeWithNormals();
	Meshes[(size_t)Cylinder] = Mesh::CreateCylinderWithNormals();
}

Level::~Level()
{
	Clear();
	for (int i = 0; i < SHAPE_NUMITEMS; i++)
		delete Meshes[i];
}

Level::Level(const Level& other) : Level()
{
	*this = other;
}

Level& Level::operator=(const Level& other)
{
	Clear();
	for (Entity *r : Objects)
	{
		if (dynamic_cast<Trigger*>(r))
			Objects.push_back(new Trigger(*(Trigger*)r));
		if (dynamic_cast<Platform*>(r))
			Objects.push_back(new Platform(*(Platform*)r));
	}
	for (ParticleSystem *s : ParticleSystems)
	{
		ParticleSystems.push_back(new ParticleSystem(*s));
	}
	return *this;
}

Level::Level(Level&& other) : Level()
{
	*this = std::move(other);
}

Level& Level::operator=(Level&& other)
{
	if (this != &other)
	{
		Clear();
		Objects = std::move(other.Objects);
		ParticleSystems = std::move(other.ParticleSystems);
		Meshes = std::move(other.Meshes);
	}
	return *this;
}

std::size_t Level::AddBox(
		glm::vec3 position,
		glm::quat orientation,
		glm::vec3 dimensions,
		glm::vec4 color,
		GLuint texID,
		float mass)
{
	Platform *p =
		new Platform(Meshes[(size_t)Box], Shape::Box, position,
			orientation, dimensions, color, texID, mass);
	Objects.push_back(p);
	return Objects.size() - 1;
}

std::size_t Level::AddCylinder(
	glm::vec3 position,
	glm::quat orientation,
	glm::vec3 dimensions,
	glm::vec4 color,
	GLuint texID,
	float mass)
{
	Platform *p =
		new Platform(Meshes[(size_t)Cylinder], Shape::Cylinder, position,
			orientation, dimensions, color, texID, mass);
	Objects.push_back(p);
	return Objects.size() - 1;
}

std::size_t Level::AddTrigger(
	glm::vec3 position,
	glm::quat orientation,
	glm::vec3 dimensions)
{
	Trigger *b = new Trigger(position, orientation, dimensions);
	Objects.push_back(b);
	return Objects.size() - 1;
}

void Level::Delete(std::size_t index)
{
	Objects.erase(Objects.begin() + index);
}

void Level::Clear()
{
	for (Entity *ent : Objects)
		delete ent;
	Objects.clear();
}

int Level::Find(btRigidBody *r)
{
	for (std::size_t i = 0, n = Objects.size(); i < n; i++)
	{
		if (Objects[i]->rigidbody == r)
			return i;
	}
	return -1;
}

Entity* Level::Find(int id)
{
	for (std::size_t i = 0, n = Objects.size(); i < n; i++)
	{
		if (Objects[i]->ID == id)
			return Objects[i];
	}
	return NULL;
}

void Level::Render(GLuint uMMatrix, GLuint uColor)
{
	for (Entity *ent : Objects)
	{
		GLuint textureID = ent->textureID;
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, textureID);
		
		glUniformMatrix4fv(uMMatrix, 1, GL_FALSE, &ent->GetModelMatrix()[0][0]);
		glUniform4fv(uColor, 1, &ent->color.r);
		ent->Render();
	}
}

void Level::Serialize(std::string file)
{
	std::ofstream f(file);
	nlohmann::json objects;
	for (Entity *ent : Objects)
	{
		nlohmann::json object;

		glm::vec3 translation = ent->GetTranslation();
		glm::quat orientation = ent->GetOrientation();
		glm::vec3 scale = ent->GetScale();

		Platform* plat = dynamic_cast<Platform*>(ent);
		if (plat) 
		{
			if (ent->shapeType == Shape::Box)
				object["type"] = "box";
			else
				object["type"] = "cylinder";
		}
		else
		{
			object["type"] = "trigger";
		}
		object["position"] = {
			translation.x, translation.y,
			translation.z };
		object["orientation"] = {
			orientation.w, orientation.x,
			orientation.y, orientation.z };
		object["dimensions"] = {
			scale.x, scale.y, scale.z };
		object["color"] = {
			ent->trueColor.r, ent->trueColor.g, ent->trueColor.b, 
			ent->trueColor.a };
		object["mass"] = ent->mass;
		object["collidable"] = ent->GetCollidable();
		object["id"] = ent->ID;
		object["texID"] = ent->textureID;
		if (plat)
		{
			if (!plat->motion.Points.empty())
			{
				nlohmann::json path;
				path["speed"] = plat->motion.Speed;
				path["enabled"] = plat->motion.Enabled;
				for (auto v = plat->motion.Points.begin();
				v != plat->motion.Points.end(); ++v)
					path["points"].push_back({ v->x, v->y, v->z });
				object["path"] = path;
			}
		}
		else //trigger
		{
			Trigger* t = (Trigger*)ent;
			if (t->connectionIDs.size() > 0)
			{
				for (auto c : t->connectionIDs)
					object["conns"].push_back(c);
			}
			if (t->bDeadly)
			{
				object["deadly"] = true;
			}
		}
		objects.push_back(object);
	}
	nlohmann::json level;
	level["objects"] = objects;
	f << std::setw(2) << level << std::endl;
	f.close();
}

Level *Level::Deserialize(std::string file)
{
	std::ifstream f(file);
	if (!f.is_open())
		return nullptr;
	std::string s(
			(std::istreambuf_iterator<char>(f)),
			std::istreambuf_iterator<char>());
	auto data = nlohmann::json::parse(s);
	Level *level = new Level();
	for (auto object : data["objects"])
	{
		auto j_pos = object["position"];
		glm::vec3 position(j_pos[0], j_pos[1], j_pos[2]);
		auto j_ori = object["orientation"];
		glm::quat orientation(j_ori[0], j_ori[1], j_ori[2], j_ori[3]);
		auto j_dim = object["dimensions"];
		glm::vec3 dimensions(j_dim[0], j_dim[1], j_dim[2]);
		auto j_col = object["color"];
		glm::vec4 color(j_col[0], j_col[1], j_col[2], j_col[3]);
		auto mass = object["mass"];
		GLuint texID;
		if (!object["texID"].is_null())
			texID = object["texID"];
		else
			texID = 4;
		auto path = object["path"];
		int id;
		if (!object["id"].is_null())
			id = object["id"];
		bool collidable;
		if(!object["collidable"].is_null())
			collidable = object["collidable"];
		std::size_t i;
		if (object["type"] == "box")
			i = level->AddBox(position, orientation, dimensions, color, 
				texID, mass);
		else if (object["type"] == "cylinder")
			i = level->AddCylinder(position, orientation, dimensions, 
				color, texID, mass);
		else
			i = level->AddTrigger(position, orientation, dimensions);
		if (!object["collidable"].is_null())
			level->Objects[i]->SetCollidable(collidable);
		if (!object["id"].is_null())
		{
			level->Objects[i]->ID = id;
			if (id >= Entity::nextID)
				Entity::nextID = id + 1;
		}
		if (!path.is_null())
		{
			Platform *ent = (Platform*)level->Objects[i];
			auto speed = path["speed"];
			auto points = path["points"];
			if (!path["enabled"].is_null())
				ent->motion.Enabled = path["enabled"];
			ent->motion.Speed = speed;
			for (auto point : points)
				ent->motion.Points.insert(
					ent->motion.Points.end(),
						glm::vec3(point[0], point[1], point[2]));
		}
		auto conns = object["conns"];
		if (!conns.is_null())
		{
			Trigger *trigger = (Trigger*)level->Objects[i];
			for (auto conn : conns)
				trigger->connectionIDs.push_back(conn);
		}
		if (object["deadly"].is_boolean() && object["deadly"])
		{
			Trigger *trigger = (Trigger*)level->Objects[i];
			trigger->bDeadly = true;
		}
	}
	f.close();

	//Set links
	for (Entity* entity : level->Objects)
	{
		Trigger* trigger = dynamic_cast<Trigger*>(entity);
		if (trigger)
		{
			for (auto id : trigger->connectionIDs)
			{
				Platform* plat = (Platform*)level->Find(id);

				if (!plat->motion.Points.empty())
					trigger->LinkToPlatform(plat);
			}

			if (trigger->bDeadly)
				trigger->RegisterCallback(Physics::CreateBlob, Enter);
		}
	}

	return level;
}
