#include "Level.h"
#include <json.hpp>
#include <fstream>
#include "Trigger.h"

Level::Level()
{
	//Meshes[(size_t)None] = nullptr;
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
	for (GameObject *r : Objects)
		Objects.push_back(new GameObject(*r));
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

std::size_t Level::AddGameObject(
		glm::vec3 position,
		glm::quat orientation,
		glm::vec3 dimensions,
		glm::vec4 color,
		GLuint texID,
		std::string type,
		float mass
		)
{
	GameObject *p;
	
	if(type == "box")
		p = new GameObject(Meshes[(size_t)Box], Shape::Box, position,
			orientation, dimensions, color, texID, mass);
	else
		p = new GameObject(Meshes[(size_t)Cylinder], Shape::Cylinder, 
			position, orientation, dimensions, color, texID, mass);
	Objects.push_back(p);
	return Objects.size() - 1;
}

void Level::Delete(std::size_t index)
{
	Objects.erase(Objects.begin() + index);
}

void Level::Clear()
{
	for (GameObject *ent : Objects)
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

GameObject* Level::Find(int id)
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
	for (GameObject *ent : Objects)
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
	for (GameObject *ent : Objects)
	{
		nlohmann::json object;

		glm::vec3 translation = ent->GetTranslation();
		glm::quat orientation = ent->GetOrientation();
		glm::vec3 scale = ent->GetScale();
		
		if (ent->shapeType == Shape::Box)
			object["type"] = "box";
		else
			object["type"] = "cylinder";

		if(ent->motion.Points.size() == 0)
			object["position"] = {
				translation.x, translation.y,
				translation.z };
		else
			object["position"] = {
				ent->motion.Points[0].x,
				ent->motion.Points[0].y,
				ent->motion.Points[0].z };
		object["orientation"] = {
			orientation.w, orientation.x,
			orientation.y, orientation.z };
		object["dimensions"] = {
			scale.x, scale.y, scale.z };
		object["color"] = {
			ent->trueColor.r, ent->trueColor.g, ent->trueColor.b, 
			ent->trueColor.a };
		object["mass"] = ent->GetMass();
		object["collidable"] = ent->GetCollidable();
		object["id"] = ent->ID;
		object["texID"] = ent->textureID;
		object["drawable"] = ent->drawable;

		if (!ent->motion.Points.empty())
		{
			nlohmann::json path;
			path["speed"] = ent->motion.Speed;
			path["enabled"] = ent->motion.Enabled;
			for (auto v = ent->motion.Points.begin();
			v != ent->motion.Points.end(); ++v)
				path["points"].push_back({ v->x, v->y, v->z });
			object["path"] = path;
		}
			
		if (ent->trigger.connectionIDs.size() > 0)
		{
			for (auto c : ent->trigger.connectionIDs)
				object["conns"].push_back(c);
		}
		if (ent->trigger.bDeadly)
		{
			object["deadly"] = true;
		}
		if (ent->trigger.bLoopy)
		{
			object["loopy"] = true;
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
			texID = 3;
		auto path = object["path"];
		int id;
		if (!object["id"].is_null())
			id = object["id"];
		bool collidable;
		if(!object["collidable"].is_null())
			collidable = object["collidable"];
		std::size_t i = level->AddGameObject(position, orientation, 
			dimensions, color, texID, object["type"], mass);
		if (!object["drawable"].is_null())
			level->Objects[i]->drawable = object["drawable"];
		if (!object["collidable"].is_null())
			level->Objects[i]->SetCollidable(collidable);
		if (!object["id"].is_null())
		{
			level->Objects[i]->ID = id;
			if (id >= GameObject::nextID)
				GameObject::nextID = id + 1;
		}
		if (!path.is_null())
		{
			GameObject *ent = level->Objects[i];
			auto speed = path["speed"];
			auto points = path["points"];
			if (!path["enabled"].is_null())
				ent->motion.Enabled = path["enabled"];
			ent->motion.Curved = false;
			ent->motion.Speed = speed;
			for (auto point : points)
				ent->motion.Points.insert(
					ent->motion.Points.end(),
						glm::vec3(point[0], point[1], point[2]));
		}
		auto conns = object["conns"];
		if (!conns.is_null())
		{
			for (auto conn : conns)
				level->Objects[i]->trigger.connectionIDs.push_back(conn);
		}
		if (object["deadly"].is_boolean() && object["deadly"])
			level->Objects[i]->trigger.bDeadly = true;
		if (object["loopy"].is_boolean() && object["loopy"])
			level->Objects[i]->trigger.bLoopy = true;
	}
	f.close();

	//Set links
	for (GameObject* entity : level->Objects)
	{
		for (auto id : entity->trigger.connectionIDs)
		{
			GameObject* plat = level->Find(id);

			if (!plat->motion.Points.empty())
				entity->trigger.LinkToPlatform(plat, entity);
		}

		if (entity->trigger.bDeadly)
			entity->trigger.RegisterCallback([]() {
			Physics::CreateBlob();
		}, Enter);

		if (entity->trigger.bLoopy)
			entity->trigger.RegisterCallback([]() {
			Physics::CreateBlob(glm::vec4(0,0,1,1));
		}, Enter);
	}

	return level;
}
