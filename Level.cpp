#include "Level.h"
#include <json.hpp>
#include <fstream>
#include "Button.h"

Level::~Level()
{
	Clear();
}

Level::Level(const Level& other)
{
	*this = other;
}

Level& Level::operator=(const Level& other)
{
	Clear();
	for (Entity *r : Objects)
	{
		if (dynamic_cast<Button*>(r))
			Objects.push_back(new Button(*(Button*)r));
		if (dynamic_cast<Platform*>(r))
			Objects.push_back(new Platform(*(Platform*)r));
	}
	return *this;
}

Level::Level(Level&& other)
{
	*this = std::move(other);
}

Level& Level::operator=(Level&& other)
{
	if (this != &other)
	{
		Clear();
		Objects = std::move(other.Objects);
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
	Mesh *box(Mesh::CreateCubeWithNormals());
	Platform *p =
		new Platform(box, Shape::Box, position,
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
	Mesh *cylinder(Mesh::CreateCylinderWithNormals());
	Platform *p =
		new Platform(cylinder, Shape::Cylinder, position,
			orientation, dimensions, color, texID, mass);
	Objects.push_back(p);
	return Objects.size() - 1;
}

std::size_t Level::AddButton(
	glm::vec3 position,
	glm::quat orientation,
	glm::vec3 dimensions,
	glm::vec4 color,
	float mass)
{
	Button *b = new Button(position, orientation, dimensions,
		color, 1.0f);
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

		/*if (typeid(r) == typeid(Button))
			object["type"] = "button";
		else*/ if(ent->shapeType == Shape::Box)
			object["type"] = "box";
		else
			object["type"] = "cylinder";
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
		object["texID"] = ent->textureID;
		Platform* plat = dynamic_cast<Platform*>(ent);
		if (plat)
		{
			if (!plat->motion.Points.empty())
			{
				nlohmann::json path;
				path["speed"] = plat->motion.Speed;
				for (auto v = plat->motion.Points.begin();
				v != plat->motion.Points.end(); ++v)
					path["points"].push_back({ v->x, v->y, v->z });
				object["path"] = path;
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
		auto j_tex = object["texID"];
		auto path = object["path"];
		std::size_t i;
		if (object["type"] == "box")
			i = level->AddBox(position, orientation, dimensions, color, mass);
		else if (object["type"] == "cylinder")
			i = level->AddCylinder(position, orientation, dimensions, 
				color, mass);
		if (!path.is_null())
		{
			Platform *ent = (Platform*)level->Objects[i];
			auto speed = path["speed"];
			auto points = path["points"];
			ent->motion.Speed = speed;
			for (auto point : points)
				ent->motion.Points.insert(
					ent->motion.Points.end(),
						glm::vec3(point[0], point[1], point[2]));
		}
	}
	f.close();
	return level;
}
