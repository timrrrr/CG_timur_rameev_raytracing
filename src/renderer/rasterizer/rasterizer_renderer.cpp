#include "rasterizer_renderer.h"

#include "utils/resource_utils.h"


void cg::renderer::rasterization_renderer::init()
{
	rasterizer = std::make_shared<cg::renderer::rasterizer<cg::vertex, cg::unsigned_color>>();
	rasterizer->vertex_shader =
			[&](float4 vertex, cg::vertex vertex_data) -> std::pair<float4, cg::vertex> {
		float4x4 world_m = model->get_world_matrix();
		float4x4 view_m = camera->get_view_matrix();
		float4x4 proj_m = camera->get_projection_matrix();
		float4 result = mul(proj_m, mul(view_m, mul(world_m, vertex)));
		float4 world_v = mul(world_m, vertex);
		float4 view_v = mul(view_m, world_v);
		float4 proj_v = mul(proj_m, view_v);

//		vertex = mul(transpose(camera->get_projection_matrix()), mul(transpose(camera->get_view_matrix()), mul(transpose(world), vertex)));
		return std::make_pair(result, vertex_data);
	};

	rasterizer->pixel_shader =
			[&](const cg::vertex& vertex_data, const float z) -> cg::color {
		float3 new_color{
				vertex_data.ambient_r + vertex_data.diffuse_r + vertex_data.emissive_r,
				vertex_data.ambient_g + vertex_data.diffuse_g + vertex_data.emissive_g,
				vertex_data.ambient_b + vertex_data.diffuse_b + vertex_data.emissive_b};
		return cg::color::from_float3(new_color);
//		return cg::color::from_float3(float3 {1, 1, 1});
	};
}

void cg::renderer::rasterization_renderer::destroy() {}

void cg::renderer::rasterization_renderer::update() {}

void cg::renderer::rasterization_renderer::render()
{
	model = std::make_shared<cg::world::model>();
	model->load_obj(settings->model_path);

	camera = std::make_shared<cg::world::camera>();
	camera->set_position(float3(settings->camera_position[0], settings->camera_position[1], settings->camera_position[2]));
	camera->set_theta(settings->camera_theta);
	camera->set_phi(settings->camera_phi);
	camera->set_angle_of_view(settings->camera_angle_of_view);
	camera->set_height(settings->height);
	camera->set_width(settings->width);
	camera->set_z_near(settings->camera_z_near);
	camera->set_z_far(settings->camera_z_far);

	render_target = std::make_shared<cg::resource<cg::unsigned_color>>(get_width(), get_height());
	depth_buffer = std::make_shared<cg::resource<float>>(get_width(), get_height());


	rasterizer->set_render_target(render_target, depth_buffer);
	rasterizer->set_viewport(settings->width, settings->height);
	rasterizer->clear_render_target(cg::unsigned_color{0, 0, 0}, FLT_MAX);

	std::vector<std::shared_ptr<cg::resource<cg::vertex>>> vertex_buffers = model->get_vertex_buffers();
	std::vector<std::shared_ptr<cg::resource<unsigned int>>> index_buffers = model->get_index_buffers();
	for(size_t s = 0; s < index_buffers.size(); s++){
		rasterizer->set_vertex_buffer(vertex_buffers[s]);
		rasterizer->set_index_buffer(index_buffers[s]);
		rasterizer->draw(index_buffers[s]->get_number_of_elements(), 0);
	}

	cg::utils::save_resource(*render_target, settings->result_path);
}