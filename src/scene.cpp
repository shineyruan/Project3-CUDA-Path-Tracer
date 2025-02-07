#include "scene.h"

#include <algorithm>
#include <cstring>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtx/string_cast.hpp>
#include <iostream>
#include <limits>

#include "model_loader.hpp"

using namespace std;

Scene::Scene(string filename) {
  cout << "Reading scene from " << filename << " ..." << endl;
  cout << " " << endl;
  char *fname = (char *)filename.c_str();
  fp_in.open(fname);
  if (!fp_in.is_open()) {
    cout << "Error reading from file - aborting!" << endl;
    throw;
  }
  while (fp_in.good()) {
    string line;
    utilityCore::safeGetline(fp_in, line);
    if (!line.empty()) {
      vector<string> tokens = utilityCore::tokenizeString(line);
      if (strcmp(tokens[0].c_str(), "MATERIAL") == 0) {
        loadMaterial(tokens[1]);
        cout << " " << endl;
      } else if (strcmp(tokens[0].c_str(), "OBJECT") == 0) {
        loadGeom(tokens[1]);
        cout << " " << endl;
      } else if (strcmp(tokens[0].c_str(), "CAMERA") == 0) {
        loadCamera();
        cout << " " << endl;
      }
    }
  }

  if (!geoms.empty()) {
    // determine scene's boundary
    float scene_x_min = std::numeric_limits<float>::infinity(),
          scene_y_min = std::numeric_limits<float>::infinity(),
          scene_z_min = std::numeric_limits<float>::infinity(),
          scene_x_max = -std::numeric_limits<float>::infinity(),
          scene_y_max = -std::numeric_limits<float>::infinity(),
          scene_z_max = -std::numeric_limits<float>::infinity();
    for (const auto &geom : geoms) {
      float x_min = std::numeric_limits<float>::infinity(),
            x_max = -std::numeric_limits<float>::infinity(),
            y_min = std::numeric_limits<float>::infinity(),
            y_max = -std::numeric_limits<float>::infinity(),
            z_min = std::numeric_limits<float>::infinity(),
            z_max = -std::numeric_limits<float>::infinity();
      if (geom.type == GeomType::TRIANGLE) {
        // triangles have already been transformed in world coordinates
        std::vector<float> list_x{geom.triangle.vertices[0].x,
                                  geom.triangle.vertices[1].x,
                                  geom.triangle.vertices[2].x};
        std::vector<float> list_y{geom.triangle.vertices[0].y,
                                  geom.triangle.vertices[1].y,
                                  geom.triangle.vertices[2].y};
        std::vector<float> list_z{geom.triangle.vertices[0].z,
                                  geom.triangle.vertices[1].z,
                                  geom.triangle.vertices[2].z};
        x_min = *std::min_element(list_x.begin(), list_x.end());
        y_min = *std::min_element(list_y.begin(), list_y.end());
        z_min = *std::min_element(list_z.begin(), list_z.end());
        x_max = *std::max_element(list_x.begin(), list_x.end());
        y_max = *std::max_element(list_y.begin(), list_y.end());
        z_max = *std::max_element(list_z.begin(), list_z.end());
      } else if (geom.type == GeomType::CUBE) {
        // cubes are in default -0.5 to 0.5 in each axis and is centered at the
        // origin.
        std::vector<glm::vec3> box_vertices{
            glm::vec3(-0.5, -0.5, -0.5), glm::vec3(-0.5, -0.5, 0.5),
            glm::vec3(-0.5, 0.5, -0.5),  glm::vec3(-0.5, 0.5, 0.5),
            glm::vec3(0.5, -0.5, -0.5),  glm::vec3(0.5, -0.5, 0.5),
            glm::vec3(0.5, 0.5, -0.5),   glm::vec3(0.5, 0.5, 0.5)};
        for (const auto &vertex : box_vertices) {
          glm::vec3 transformed_vertex =
              glm::vec3(geom.transform * glm::vec4(vertex, 1.0f));
          x_min = std::min(x_min, transformed_vertex.x);
          x_max = std::max(x_max, transformed_vertex.x);
          y_min = std::min(y_min, transformed_vertex.y);
          y_max = std::max(y_max, transformed_vertex.y);
          z_min = std::min(z_min, transformed_vertex.z);
          z_max = std::max(z_max, transformed_vertex.z);
        }
      } else if (geom.type == GeomType::SPHERE) {
        // spheres are assumed to have radius 0.5 and is centered at the origin.
        // assume the endpoints on the principal axes are the farthest (no
        // rotation)
        std::vector<glm::vec3> sphere_endpoints{
            glm::vec3(0.5f, 0.f, 0.f), glm::vec3(-0.5f, 0.f, 0.f),
            glm::vec3(0.f, 0.5f, 0.f), glm::vec3(0.f, -0.5f, 0.f),
            glm::vec3(0.f, 0.f, 0.5f), glm::vec3(0.f, 0.f, -0.5f)};
        for (const auto &endpoint : sphere_endpoints) {
          glm::vec3 transformed_endpoint =
              glm::vec3(geom.transform * glm::vec4(endpoint, 1.0f));
          x_min = std::min(x_min, transformed_endpoint.x);
          x_max = std::max(x_max, transformed_endpoint.x);
          y_min = std::min(y_min, transformed_endpoint.y);
          y_max = std::max(y_max, transformed_endpoint.y);
          z_min = std::min(z_min, transformed_endpoint.z);
          z_max = std::max(z_max, transformed_endpoint.z);
        }
      } else {
        // unknown geom type
        x_min = 0.0f;
        x_max = 0.0f;
        y_min = 0.0f;
        y_max = 0.0f;
        z_min = 0.0f;
        z_max = 0.0f;
      }
      scene_x_min = std::min(scene_x_min, x_min);
      scene_x_max = std::max(scene_x_max, x_max);
      scene_y_min = std::min(scene_y_min, y_min);
      scene_y_max = std::max(scene_y_max, y_max);
      scene_z_min = std::min(scene_z_min, z_min);
      scene_z_max = std::max(scene_z_max, z_max);
    }
    boundary.min_xyz = glm::vec3(scene_x_min, scene_y_min, scene_z_min);
    boundary.max_xyz = glm::vec3(scene_x_max, scene_y_max, scene_z_max);
  }
}

std::vector<Geom> Scene::loadObjMesh(const std::string &file_path,
                                     const std::string &material_path) {
  std::vector<Geom> mesh;
  ObjLoader loader(file_path, material_path);

  if (!loader.isReady()) return mesh;

  for (int shape_id = 0; shape_id < loader.numShapes(); ++shape_id) {
    int idx_offset = 0;
    for (int face_id = 0; face_id < loader.numFaces(shape_id); ++face_id) {
      int num_vertices = loader.numVertices(shape_id, face_id);
      if (num_vertices != 3) {
        cout << "ERROR: Non-triangular meshes not supported!\n";
        continue;
      }
      for (int vert_id = 0; vert_id < 3; ++vert_id) {
        Geom geom_triangle;
        geom_triangle.type = TRIANGLE;
        geom_triangle.triangle.vertices[vert_id] =
            loader.getVertexPos(shape_id, idx_offset + vert_id);
        geom_triangle.triangle.normals[vert_id] =
            loader.getNormalVec(shape_id, idx_offset + vert_id);
        mesh.push_back(std::move(geom_triangle));
      }
      idx_offset += num_vertices;
    }
  }
  return mesh;
}

int Scene::loadGeom(string objectid) {
  int id = atoi(objectid.c_str());
  cout << "Loading Geom " << id << "..." << endl;

  std::vector<Geom> mesh;
  bool is_objMesh = false;
  Geom newGeom;
  string line;

  // load object type
  utilityCore::safeGetline(fp_in, line);
  if (!line.empty() && fp_in.good()) {
    vector<string> tokens = utilityCore::tokenizeString(line);
    if (strcmp(tokens[0].c_str(), "sphere") == 0) {
      cout << "Creating new sphere..." << endl;
      newGeom.type = SPHERE;
    } else if (strcmp(tokens[0].c_str(), "cube") == 0) {
      cout << "Creating new cube..." << endl;
      newGeom.type = CUBE;
    } else if (strcmp(tokens[0].c_str(), "mesh_obj") == 0) {
      cout << "Creating new OBJ mesh..." << endl;
      mesh       = loadObjMesh(tokens[1], tokens[2]);
      is_objMesh = true;
      cout << "OBJ Mesh Loaded" << endl;
    }
  }

  // link material
  utilityCore::safeGetline(fp_in, line);
  if (!line.empty() && fp_in.good()) {
    vector<string> tokens = utilityCore::tokenizeString(line);
    newGeom.materialid    = atoi(tokens[1].c_str());
    cout << "Connecting Geom " << objectid << " to Material "
         << newGeom.materialid << "..." << endl;
  }

  // load transformations
  utilityCore::safeGetline(fp_in, line);
  while (!line.empty() && fp_in.good()) {
    vector<string> tokens = utilityCore::tokenizeString(line);

    // load tranformations
    if (strcmp(tokens[0].c_str(), "TRANS") == 0) {
      newGeom.translation =
          glm::vec3(atof(tokens[1].c_str()), atof(tokens[2].c_str()),
                    atof(tokens[3].c_str()));
    } else if (strcmp(tokens[0].c_str(), "ROTAT") == 0) {
      newGeom.rotation =
          glm::vec3(atof(tokens[1].c_str()), atof(tokens[2].c_str()),
                    atof(tokens[3].c_str()));
    } else if (strcmp(tokens[0].c_str(), "SCALE") == 0) {
      newGeom.scale =
          glm::vec3(atof(tokens[1].c_str()), atof(tokens[2].c_str()),
                    atof(tokens[3].c_str()));
    }

    utilityCore::safeGetline(fp_in, line);
  }

  if (is_objMesh) {
    for (auto &geom_triangle : mesh) {
      geom_triangle.materialid  = newGeom.materialid;
      geom_triangle.translation = newGeom.translation;
      geom_triangle.rotation    = newGeom.rotation;
      geom_triangle.scale       = newGeom.scale;
      geom_triangle.transform   = utilityCore::buildTransformationMatrix(
          newGeom.translation, newGeom.rotation, newGeom.scale);
      geom_triangle.inverseTransform = glm::inverse(geom_triangle.transform);
      geom_triangle.invTranspose =
          glm::inverseTranspose(geom_triangle.transform);
      for (int i = 0; i < 3; ++i) {
        auto &vertex = geom_triangle.triangle.vertices[i];
        auto &normal = geom_triangle.triangle.normals[i];
        vertex = glm::vec3(geom_triangle.transform * glm::vec4(vertex, 1.0f));
        normal =
            glm::vec3(geom_triangle.invTranspose * glm::vec4(normal, 0.0f));
      }
      geoms.push_back(geom_triangle);
    }
  } else {
    newGeom.transform = utilityCore::buildTransformationMatrix(
        newGeom.translation, newGeom.rotation, newGeom.scale);
    newGeom.inverseTransform = glm::inverse(newGeom.transform);
    newGeom.invTranspose     = glm::inverseTranspose(newGeom.transform);
    geoms.push_back(newGeom);
  }

  return 1;
}

int Scene::loadCamera() {
  cout << "Loading Camera ..." << endl;
  RenderState &state = this->state;
  Camera &camera     = state.camera;
  float fovy;

#ifdef DEPTH_OF_FIELD
  int num_static_prop = 7;
#else
  int num_static_prop = 5;
#endif

  // load static properties
  for (int i = 0; i < num_static_prop; i++) {
    string line;
    utilityCore::safeGetline(fp_in, line);
    vector<string> tokens = utilityCore::tokenizeString(line);
    if (strcmp(tokens[0].c_str(), "RES") == 0) {
      camera.resolution.x = atoi(tokens[1].c_str());
      camera.resolution.y = atoi(tokens[2].c_str());
    } else if (strcmp(tokens[0].c_str(), "FOVY") == 0) {
      fovy = atof(tokens[1].c_str());
    } else if (strcmp(tokens[0].c_str(), "ITERATIONS") == 0) {
      state.iterations = atoi(tokens[1].c_str());
    } else if (strcmp(tokens[0].c_str(), "DEPTH") == 0) {
      state.traceDepth = atoi(tokens[1].c_str());
    } else if (strcmp(tokens[0].c_str(), "FILE") == 0) {
      state.imageName = tokens[1];
    } else if (strcmp(tokens[0].c_str(), "FOCALDIST") == 0) {
      camera.focalDistance = atof(tokens[1].c_str());
    } else if (strcmp(tokens[0].c_str(), "LENRADIUS") == 0) {
      camera.lensRadius = atof(tokens[1].c_str());
    }
  }

  string line;
  utilityCore::safeGetline(fp_in, line);
  while (!line.empty() && fp_in.good()) {
    vector<string> tokens = utilityCore::tokenizeString(line);
    if (strcmp(tokens[0].c_str(), "EYE") == 0) {
      camera.position =
          glm::vec3(atof(tokens[1].c_str()), atof(tokens[2].c_str()),
                    atof(tokens[3].c_str()));
    } else if (strcmp(tokens[0].c_str(), "LOOKAT") == 0) {
      camera.lookAt =
          glm::vec3(atof(tokens[1].c_str()), atof(tokens[2].c_str()),
                    atof(tokens[3].c_str()));
    } else if (strcmp(tokens[0].c_str(), "UP") == 0) {
      camera.up = glm::vec3(atof(tokens[1].c_str()), atof(tokens[2].c_str()),
                            atof(tokens[3].c_str()));
    }

    utilityCore::safeGetline(fp_in, line);
  }

  // calculate fov based on resolution
  float yscaled = tan(fovy * (PI / 180));
  float xscaled = (yscaled * camera.resolution.x) / camera.resolution.y;
  float fovx    = (atan(xscaled) * 180) / PI;
  camera.fov    = glm::vec2(fovx, fovy);

  camera.right       = glm::normalize(glm::cross(camera.view, camera.up));
  camera.pixelLength = glm::vec2(2 * xscaled / (float)camera.resolution.x,
                                 2 * yscaled / (float)camera.resolution.y);

  camera.view = glm::normalize(camera.lookAt - camera.position);

  // set up render camera stuff
  int arraylen = camera.resolution.x * camera.resolution.y;
  state.image.resize(arraylen);
  std::fill(state.image.begin(), state.image.end(), glm::vec3());

  cout << "Loaded camera!" << endl;
  return 1;
}

int Scene::loadMaterial(string materialid) {
  int id = atoi(materialid.c_str());
  if (id != materials.size()) {
    cout << "ERROR: MATERIAL ID does not match expected number of materials"
         << endl;
    return -1;
  } else {
    cout << "Loading Material " << id << "..." << endl;
    Material newMaterial;

    // load static properties
    for (int i = 0; i < 7; i++) {
      string line;
      utilityCore::safeGetline(fp_in, line);
      vector<string> tokens = utilityCore::tokenizeString(line);
      if (strcmp(tokens[0].c_str(), "RGB") == 0) {
        glm::vec3 color(atof(tokens[1].c_str()), atof(tokens[2].c_str()),
                        atof(tokens[3].c_str()));
        newMaterial.color = color;
      } else if (strcmp(tokens[0].c_str(), "SPECEX") == 0) {
        newMaterial.specular.exponent = atof(tokens[1].c_str());
      } else if (strcmp(tokens[0].c_str(), "SPECRGB") == 0) {
        glm::vec3 specColor(atof(tokens[1].c_str()), atof(tokens[2].c_str()),
                            atof(tokens[3].c_str()));
        newMaterial.specular.color = specColor;
      } else if (strcmp(tokens[0].c_str(), "REFL") == 0) {
        newMaterial.hasReflective = atof(tokens[1].c_str());
      } else if (strcmp(tokens[0].c_str(), "REFR") == 0) {
        newMaterial.hasRefractive = atof(tokens[1].c_str());
      } else if (strcmp(tokens[0].c_str(), "REFRIOR") == 0) {
        newMaterial.indexOfRefraction = atof(tokens[1].c_str());
      } else if (strcmp(tokens[0].c_str(), "EMITTANCE") == 0) {
        newMaterial.emittance = atof(tokens[1].c_str());
      }
    }
    materials.push_back(newMaterial);
    return 1;
  }
}
