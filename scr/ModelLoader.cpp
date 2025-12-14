#include "ModelLoader.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

bool loadOBJ(const char* path, std::vector<glm::vec3>& out_vertices, std::vector<glm::vec2>& out_uvs, std::vector<glm::vec3>& out_normals) {
	std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;
	std::vector<glm::vec3> temp_vertices;
	std::vector<glm::vec2> temp_uvs;
	std::vector<glm::vec3> temp_normals;

	std::ifstream file(path);
	if (!file.is_open()) {
		std::cout << "No se pudo abrir el archivo: " << path << std::endl;
		return false;
	}

	std::string line;
	while (std::getline(file, line)) {
		std::stringstream ss(line);
		std::string prefix;
		ss >> prefix;

		if (prefix == "v") {
			glm::vec3 vertex;
			ss >> vertex.x >> vertex.y >> vertex.z;
			temp_vertices.push_back(vertex);
		}
		else if (prefix == "vt") {
			glm::vec2 uv;
			ss >> uv.x >> uv.y;
			temp_uvs.push_back(uv);
		}
		else if (prefix == "vn") {
			glm::vec3 normal;
			ss >> normal.x >> normal.y >> normal.z;
			temp_normals.push_back(normal);
		}
		else if (prefix == "f") {
			unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
			char slash;
			// Asume formato v/vt/vn
			for (int i = 0; i < 3; i++) {
				ss >> vertexIndex[i] >> slash >> uvIndex[i] >> slash >> normalIndex[i];
				vertexIndices.push_back(vertexIndex[i]);
				uvIndices.push_back(uvIndex[i]);
				normalIndices.push_back(normalIndex[i]);
			}
		}
	}

	//Desenrollar los índices para OpenGL (glDrawArrays)
	for (unsigned int i = 0; i < vertexIndices.size(); i++) {
		out_vertices.push_back(temp_vertices[vertexIndices[i] - 1]);
		if (!temp_uvs.empty()) out_uvs.push_back(temp_uvs[uvIndices[i] - 1]);
		if (!temp_normals.empty()) out_normals.push_back(temp_normals[normalIndices[i] - 1]);
	}
	return true;
}