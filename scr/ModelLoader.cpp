#include "ModelLoader.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

bool loadOBJ(const char* path, std::vector<glm::vec3>& out_vertices, std::vector<glm::vec2>& out_uvs, std::vector<glm::vec3>& out_normals) {
	// Limpiamos los vectores por si acaso
	out_vertices.clear();
	out_uvs.clear();
	out_normals.clear();

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
			// Leemos la línea entera de la cara
			std::string vertexDef;
			std::vector<unsigned int> vIdxs, vtIdxs, vnIdxs;

			while (ss >> vertexDef) {
				unsigned int v = 0, vt = 0, vn = 0;

				// Parseamos manualmente para soportar v, v/vt, v//vn, v/vt/vn
				size_t firstSlash = vertexDef.find('/');
				size_t secondSlash = (firstSlash != std::string::npos) ? vertexDef.find('/', firstSlash + 1) : std::string::npos;

				if (firstSlash == std::string::npos) {
					v = std::stoi(vertexDef);
				}
				else {
					v = std::stoi(vertexDef.substr(0, firstSlash));
					if (secondSlash == std::string::npos) {
						// Formato v/vt
						vt = std::stoi(vertexDef.substr(firstSlash + 1));
					}
					else {
						// Formato v//vn o v/vt/vn
						if (secondSlash > firstSlash + 1) {
							vt = std::stoi(vertexDef.substr(firstSlash + 1, secondSlash - firstSlash - 1));
						}
						vn = std::stoi(vertexDef.substr(secondSlash + 1));
					}
				}
				vIdxs.push_back(v);
				vtIdxs.push_back(vt);
				vnIdxs.push_back(vn);
			}

			// Triangulación (Fan): Convierte polígonos (Quads, etc.) en triángulos
			// Triángulo 1: 0, 1, 2
			// Triángulo 2: 0, 2, 3 (si es un Quad)
			for (size_t i = 1; i < vIdxs.size() - 1; ++i) {
				vertexIndices.push_back(vIdxs[0]);
				vertexIndices.push_back(vIdxs[i]);
				vertexIndices.push_back(vIdxs[i + 1]);

				uvIndices.push_back(vtIdxs[0]);
				uvIndices.push_back(vtIdxs[i]);
				uvIndices.push_back(vtIdxs[i + 1]);

				normalIndices.push_back(vnIdxs[0]);
				normalIndices.push_back(vnIdxs[i]);
				normalIndices.push_back(vnIdxs[i + 1]);
			}
		}
	}

	// Desenrollar los índices para OpenGL (glDrawArrays)
	for (unsigned int i = 0; i < vertexIndices.size(); i++) {
		unsigned int vIndex = vertexIndices[i];
		unsigned int vtIndex = uvIndices[i];
		unsigned int vnIndex = normalIndices[i];

		// Vértices
		if (vIndex > 0 && vIndex <= temp_vertices.size())
			out_vertices.push_back(temp_vertices[vIndex - 1]);

		// UVs (Relleno con 0 si falta)
		if (vtIndex > 0 && vtIndex <= temp_uvs.size())
			out_uvs.push_back(temp_uvs[vtIndex - 1]);
		else
			out_uvs.push_back(glm::vec2(0.0f));

		// Normales (Relleno con 0,1,0 si falta)
		if (vnIndex > 0 && vnIndex <= temp_normals.size())
			out_normals.push_back(temp_normals[vnIndex - 1]);
		else
			out_normals.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
	}

	std::cout << "Modelo cargado correctamente: " << path << std::endl;
	std::cout << " - Vertices generados: " << out_vertices.size() << std::endl;

	return true;
}