#include "Object.h"

#include "../utils/util.h"
#include "../objloader/obj_parser.h"

#include "../primitives/sphere.h"
#include "../primitives/cube.h"
#include "../primitives/tesselated_sphere.h"
#include "../primitives/quad.h"

#include "Light.h"

#include <list>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

//I parametri di default vengono inizializzati nel costruttore
Object::Object()
{
	objectLoader = new objLoader();

	for(int i = 0; i < 8; i++)
	{
		textureFileNames[i] = "";
		data.textures[i] = -1; //Non inizializzata
	}
	textured = false;
	primitiveKind = "";
}

//Ritorna se il file specificato � un obj valido
int Object::loadGeometry(string filename)
{
	return objectLoader->load(filename.c_str());
}

//il nome del file da caricare con make_resources.
void Object::setTexture(int id, string name, string filename)
{
	textureNames[id] = name;
	textureFileNames[id] = filename;
}

//Imposta l'algoritmo di shading con cui effettuale il rendering dell'oggetto
void Object::setMaterial(string filename)
{
	material = filename;
}

void Object::addParameter(string key, float value)
{
	floatParameters.insert(pair<string,int>(key, value));
}

void Object::addParameter(string key, glm::vec4 value)
{
	vectorParameters.insert(pair<string,glm::vec4>(key, value));
}

//Effettivo rendering dell'oggetto
void Object::render()
{
	glUseProgram(shaderData.program);

	//Uniform float
	for(std::map<std::string, float>::iterator it= floatParameters.begin(); it != floatParameters.end(); it++)
	{
		string name = it->first;

		glUniform1f(uniformLocations[name], it->second);
	}
	
	//Uniform vec4
	for(std::map<std::string, glm::vec4>::iterator it= vectorParameters.begin(); it != vectorParameters.end(); it++)
	{
		string name = it->first;

		glUniform4f(uniformLocations[name], it->second.x, it->second.y, it->second.z, it->second.w);
	}

	glUniform1i(lightNumberLocation, Light::getNumberOfLights());

	
	//Uniform textures
	for(int i = 0; i < 8; i++)
	{
		if(data.textures[i] != -1)
		{
			glActiveTexture(GL_TEXTURE0+i);
			GLint location = glGetUniformLocation(shaderData.program, textureNames[i].c_str());
			glBindTexture(GL_TEXTURE_2D, data.textures[i]);
			glUniform1i(location, i);
		}
	}
	glBindBuffer(GL_ARRAY_BUFFER, data.vertex_buffer);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(
		3,
		GL_FLOAT,
		0,
		(void*)0
		);

	glBindBuffer(GL_ARRAY_BUFFER, data.normal_buffer);
	glEnableClientState(GL_NORMAL_ARRAY);
	glNormalPointer(
		GL_FLOAT,
		0,
		(void*)0
		);

	if(textured)
	{
		glBindBuffer(GL_ARRAY_BUFFER, data.st_buffer);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		//Texture coordinates
		glTexCoordPointer(
			2,
			GL_FLOAT,
			0,
			(void*)0
			);
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data.element_buffer);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glDrawElements(
		GL_TRIANGLES,
		elements.size(),
		GL_UNSIGNED_SHORT,
		(void*)0
		);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);

	if(textured)
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

//Creiamo i buffer OpenGL e le texture leggendo i dati dell'obj
int Object::makeResources()
{
	if (primitiveKind == "")
	{

		//Carichiamo 
		int elementCounter = 0;
		for (int fcount = 0; fcount < objectLoader->faceCount; fcount++)
		{
			obj_face *curFace = objectLoader->faceList[fcount];

			for (int vcount = 0; vcount < 3; vcount++)
			{
				obj_vector *currentVertex = objectLoader->vertexList[curFace->vertex_index[vcount]];
				vertices.push_back(glm::vec3(currentVertex->e[0], currentVertex->e[1], currentVertex->e[2]));
				if (objectLoader->textureCount > 0)
				{
					obj_vector *currentTexture = objectLoader->textureList[curFace->texture_index[vcount]];
					textured = true;
					stCoordinates.push_back(glm::vec2(currentTexture->e[0], currentTexture->e[1]));
				}

				if (objectLoader->normalCount > 0)
				{
					obj_vector *currentNormal = objectLoader->normalList[curFace->normal_index[vcount]];
					normals.push_back(glm::vec3(currentNormal->e[0], currentNormal->e[1], currentNormal->e[2]));
				}

				elements.push_back(elementCounter++); //Riempiamo l'element buffer con un ciclo per indicare che vogliamo caricare tutti i vertici
			}
		}
	}
	else
	{
		if (primitiveKind.find("sphere") == 0)
		{
			std::vector<std::string> values;
			boost::split(values, primitiveKind, boost::is_any_of(" "));

			int rings = 10;
			int sectors_or_tess_level = 10;
			string kind = "geo";

			if (values.size() > 1)
			{
				kind = values.at(1);
			}

			if (values.size() > 3)
			{
				rings = atoi(values.at(3).c_str());
			}

			if (values.size() > 2)
			{
				sectors_or_tess_level = atoi(values.at(2).c_str());
			}

			if (kind == "geo")
				make_sphere(vertices, normals, stCoordinates, elements, rings, sectors_or_tess_level);
			else if (kind == "tes")
				make_tesselated_sphere(vertices, normals, stCoordinates, elements, sectors_or_tess_level);
		}
		else if (primitiveKind.compare("cube") == 0)
		{
			make_cube(vertices, normals, stCoordinates, elements);
		}
		else if (primitiveKind.compare("quad") == 0)
		{
			make_quad(vertices, normals, stCoordinates, elements);
		}
	}
	

	data.vertex_buffer = make_buffer(
		GL_ARRAY_BUFFER,
		&vertices[0],
		vertices.size() * sizeof(glm::vec3)
		);

	data.normal_buffer = make_buffer(
		GL_ARRAY_BUFFER,
		&normals[0],
		normals.size() * sizeof(glm::vec3)
		);


	data.element_buffer = make_buffer(
		GL_ELEMENT_ARRAY_BUFFER,
		&elements[0],
		elements.size() * sizeof(GLushort)
		);

	if(textured)
	{
		data.st_buffer = make_buffer(
			GL_ARRAY_BUFFER,
			&stCoordinates[0],
			stCoordinates.size() * sizeof(glm::vec2)
			);

		for(int i = 0; i < 8; i++)
		{
			if(textureFileNames[i].compare("") != 0)
			{
				data.textures[i] = make_texture(textureFileNames[i].c_str());
				if(data.textures[i] == 0) //Zero in caso di errore
					return 0;
			}
		}
	}

	if (primitiveKind == "")
	{
		//Objectloader non serve pi� in quanto abbiamo tutti i dati nei buffer dell'oggetto data.
		delete objectLoader;
	}

	string vertexShaderFileName = boost::filesystem::canonical(material + ".vert").string();
	string fragmentShaderFileName = boost::filesystem::canonical(material + ".frag").string();

	shaderData.vertex_shader = make_shader(GL_VERTEX_SHADER,vertexShaderFileName.c_str());
	if(shaderData.vertex_shader == 0)
	{
		return 0;
	}

	shaderData.fragment_shader = make_shader(GL_FRAGMENT_SHADER, fragmentShaderFileName.c_str());
	if(shaderData.fragment_shader == 0)
		return 0;

	shaderData.program = make_program(shaderData.vertex_shader, shaderData.fragment_shader);

	if(shaderData.program == 0)
		return 0;

	lightNumberLocation = glGetUniformLocation(shaderData.program, "NUMBER_OF_LIGHTS");

	for(std::map<std::string, float>::iterator it= floatParameters.begin(); it != floatParameters.end(); it++)
	{
		const char* name = it->first.c_str();
		GLint location = glGetUniformLocation(shaderData.program, name);
		uniformLocations.insert(pair<string,GLint>(name, location));
	}
	
	for(std::map<std::string, glm::vec4>::iterator it= vectorParameters.begin(); it != vectorParameters.end(); it++)
	{
		const char* name = it->first.c_str();
		GLint location = glGetUniformLocation(shaderData.program, name);
		uniformLocations.insert(pair<string,GLint>(name, location));
	}
	
	return 1;
}