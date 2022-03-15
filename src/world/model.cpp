#define TINYOBJLOADER_IMPLEMENTATION

#include "model.h"

#include "utils/error_handler.h"

#include <linalg.h>

#include <iostream>
#include <memory>


using namespace linalg::aliases;
using namespace cg::world;

cg::world::model::model() {}

cg::world::model::~model() {}

void cg::world::model::load_obj(const std::filesystem::path& model_path){
    std::string inputfile = model_path.string();
    tinyobj::ObjReaderConfig reader_config;
    // Path to material files
    reader_config.mtl_search_path = "models";
	reader_config.triangulate = true;

    tinyobj::ObjReader reader;

    if (!reader.ParseFromFile(inputfile, reader_config)) {
        if (!reader.Error().empty()) {
            std::cerr << "TinyObjReader: " << reader.Error();
        }
        exit(1);
    }
    if (!reader.Warning().empty()) {
        std::cout << "TinyObjReader: " << reader.Warning();
    }

    attrib = reader.GetAttrib();
    shapes = reader.GetShapes();
    materials = reader.GetMaterials();

    vertex_buffers.resize(shapes.size());
    index_buffers.resize(shapes.size());
//        textures.resize(shapes.size());

    // Loop over shapes
    for (size_t s = 0; s < shapes.size(); s++) {
        size_t vertices_num = 0;
        for(size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++)
            vertices_num += shapes[s].mesh.num_face_vertices[f];

		vertex_buffers[s] = std::make_shared<cg::resource<cg::vertex>> (vertices_num);
        index_buffers[s] = std::make_shared<cg::resource<unsigned int>> (vertices_num);
        // Loop over faces(polygon)
        // vertex_buffers[s] = resource(shapes[s].num_face_vertices.size())
        size_t index_offset = 0;
		const auto& mesh = shapes[s].mesh;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
            int fv = shapes[s].mesh.num_face_vertices[f];
			float3 normal;
			if (shapes[s].mesh.indices[index_offset].normal_index < 0){
				auto a_id = shapes[s].mesh.indices[index_offset];
				auto b_id = shapes[s].mesh.indices[index_offset + 1];
				auto c_id = shapes[s].mesh.indices[index_offset + 2];

				float3 a{
						attrib.vertices[3 * a_id.vertex_index],
						attrib.vertices[3 * a_id.vertex_index + 1],
						attrib.vertices[3 * a_id.vertex_index + 2],
				};
				float3 b{
						attrib.vertices[3 * b_id.vertex_index],
						attrib.vertices[3 * b_id.vertex_index + 1],
						attrib.vertices[3 * b_id.vertex_index + 2],
				};
				float3 c{
						attrib.vertices[3 * c_id.vertex_index],
						attrib.vertices[3 * c_id.vertex_index + 1],
						attrib.vertices[3 * c_id.vertex_index + 2],
				};
				normal = normalize(cross(b - a, c - a));
			}

            for (size_t v = 0; v < fv; v++) {

				cg::vertex* new_v = &vertex_buffers[s]->item(index_offset + v);
				index_buffers[s]->item(index_offset + v) = index_offset + v;
				// access to vertex
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
				new_v->x = attrib.vertices[3 * idx.vertex_index + 0];
				new_v->y = attrib.vertices[3 * idx.vertex_index + 1];
				new_v->z = attrib.vertices[3 * idx.vertex_index + 2];

				if (idx.normal_index > -1) {
					new_v->nx = attrib.normals[3 * idx.normal_index + 0];
					new_v->ny = attrib.normals[3 * idx.normal_index + 1];
					new_v->nz = attrib.normals[3 * idx.normal_index + 2];
				}
				else {
					new_v->nx = normal.x;
					new_v->ny = normal.y;
					new_v->nz = normal.z;
				}
				if (idx.texcoord_index > -1) {
					new_v->u = attrib.texcoords[2 * idx.texcoord_index + 0];
					new_v->v = attrib.texcoords[2 * idx.texcoord_index + 1];
				}
				else {
					new_v->u = 0;
					new_v->v = 0;
				}
				if (materials.size() > 0) {
					new_v->ambient_r = materials[shapes[s].mesh.material_ids[f]].ambient[0];
					new_v->ambient_g = materials[shapes[s].mesh.material_ids[f]].ambient[1];
					new_v->ambient_b = materials[shapes[s].mesh.material_ids[f]].ambient[2];
					new_v->emissive_r = materials[shapes[s].mesh.material_ids[f]].emission[0];
					new_v->emissive_g = materials[shapes[s].mesh.material_ids[f]].emission[1];
					new_v->emissive_b = materials[shapes[s].mesh.material_ids[f]].emission[2];
					new_v->diffuse_r = materials[shapes[s].mesh.material_ids[f]].diffuse[0];
					new_v->diffuse_g = materials[shapes[s].mesh.material_ids[f]].diffuse[1];
					new_v->diffuse_b = materials[shapes[s].mesh.material_ids[f]].diffuse[2];
				}
            }
            index_offset += fv;
            shapes[s].mesh.material_ids[f];
        }
    }
}

const std::vector<std::shared_ptr<cg::resource<cg::vertex>>>& cg::world::model::get_vertex_buffers() const{
    return vertex_buffers;
}

const std::vector<std::shared_ptr<cg::resource<unsigned int>>>& cg::world::model::get_index_buffers() const{
    return index_buffers;
}

std::vector<std::filesystem::path>
cg::world::model::get_per_shape_texture_files() const
{
	THROW_ERROR("Not implemented yet");
}


const float4x4 cg::world::model::get_world_matrix() const
{
	static float3 tr{0, 0, 0};
	static float4x4 tr_matrix{
			{1, 0, 0, tr.x},
			{0, 1, 0, tr.y},
			{0, 0, 1, tr.z},
			{0, 0, 0, 1}};

	static float3 scale{1, 1, 1};
	static float4x4 sc_matrix{
			{scale.x, 0, 0, 0},
			{0, scale.y, 0, 0},
			{0, 0, scale.z, 0},
			{0, 0, 0, 1}};

	// rotation around x, y, z
	static float alpha = 0, beta = 0, gamma = 0;
	static float4x4 r_x{
			{1, 0, 0, 0},
			{0, cos(alpha), -sin(alpha), 0},
			{0, sin(alpha), cos(alpha), 0},
			{0, 0, 0, 1}};
	static float4x4 r_y{
			{cos(beta), 0, sin(beta), 0},
			{0, 1, 0, 0},
			{-sin(beta), 0, cos(beta), 0},
			{0, 0, 0, 1}};
	static float4x4 r_z{
			{cos(gamma), -sin(gamma), 0, 0},
			{sin(gamma), cos(gamma), 0, 0},
			{0, 0, 1, 0},
			{0, 0, 0, 1}};

	return mul(tr_matrix, mul(r_x, mul(r_y, mul(r_z, sc_matrix))));
//	return mul(transpose(tr_matrix), mul(transpose(r_x), mul(transpose(r_y), mul(transpose(r_z), sc_matrix))));
}
