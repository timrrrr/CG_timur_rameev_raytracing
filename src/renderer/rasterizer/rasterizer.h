#pragma once

#include "resource.h"

#include <functional>
#include <iostream>
#include <linalg.h>
#include <memory>


using namespace linalg::aliases;

namespace cg::renderer
{
	template<typename VB, typename RT>
	class rasterizer
	{
	public:
		rasterizer(){};
		~rasterizer(){};
		void set_render_target(
				std::shared_ptr<resource<RT>> in_render_target,
				std::shared_ptr<resource<float>> in_depth_buffer = nullptr);
		void clear_render_target(
				const RT& in_clear_value, const float in_depth = FLT_MAX);

		void set_vertex_buffer(std::shared_ptr<resource<VB>> in_vertex_buffer);
		void set_index_buffer(std::shared_ptr<resource<unsigned int>> in_index_buffer);

		void set_viewport(size_t in_width, size_t in_height);

		void draw(size_t num_vertexes, size_t vertex_offset);

		std::function<std::pair<float4, VB>(float4 vertex, VB vertex_data)> vertex_shader;
		std::function<cg::color(const VB& vertex_data, const float z)> pixel_shader;

	protected:
		std::shared_ptr<cg::resource<VB>> vertex_buffer;
		std::shared_ptr<cg::resource<unsigned int>> index_buffer;
		std::shared_ptr<cg::resource<RT>> render_target;
		std::shared_ptr<cg::resource<float>> depth_buffer;

		size_t width = 1920;
		size_t height = 1080;

		float edge_function(float2 a, float2 b, float2 c);
		bool depth_test(float z, size_t x, size_t y);
	};

	template<typename VB, typename RT>
	inline void rasterizer<VB, RT>::set_render_target(
			std::shared_ptr<resource<RT>> in_render_target,
			std::shared_ptr<resource<float>> in_depth_buffer)
	{
		if(in_render_target)
		    render_target = in_render_target;

		if(in_depth_buffer)
		    depth_buffer = in_depth_buffer;
	}

	template<typename VB, typename RT>
	inline void rasterizer<VB, RT>::clear_render_target(
			const RT& in_clear_value, const float in_depth)
	{
		for(size_t i = 0; i < render_target->get_number_of_elements(); i++)
            render_target->item(i) = in_clear_value;

		for(size_t i = 0; i < depth_buffer->get_number_of_elements(); i++)
		    depth_buffer->item(i) = in_depth;
	}

	template<typename VB, typename RT>
	inline void rasterizer<VB, RT>::set_vertex_buffer(
			std::shared_ptr<resource<VB>> in_vertex_buffer)
	{
		vertex_buffer = in_vertex_buffer;
	}

	template<typename VB, typename RT>
	inline void rasterizer<VB, RT>::set_index_buffer(
			std::shared_ptr<resource<unsigned int>> in_index_buffer)
	{
		index_buffer = in_index_buffer;
	}

	template<typename VB, typename RT>
	inline void rasterizer<VB, RT>::set_viewport(size_t in_width, size_t in_height)
	{
		width = in_width;
		height = in_height;
	}

	template<typename VB, typename RT>
	inline void rasterizer<VB, RT>::draw(size_t num_vertexes, size_t vertex_offset)
	{
		cg::resource<float4> vertices = cg::resource<float4> (num_vertexes);
		for(size_t i = 0; i < num_vertexes; i++) {
			vertices.item(i).x = vertex_buffer->item(index_buffer->item(vertex_offset + i)).x;
			vertices.item(i).y = vertex_buffer->item(index_buffer->item(vertex_offset + i)).y;
			vertices.item(i).z = vertex_buffer->item(index_buffer->item(vertex_offset + i)).z;
			vertices.item(i).w = 1;
			vertices.item(i) = vertex_shader(vertices.item(i), vertex_buffer->item(index_buffer->item(vertex_offset + i))).first;
			vertices.item(i) /= vertices.item(i).w;
		}

		for(size_t i = 0; i < num_vertexes; i++) {
			float4& vertex = vertices.item(i);
			vertex.x = (vertex.x + 1) * width / 2;
			vertex.y = (-vertex.y + 1) * height / 2;
		}

		for(int i = 0; i < vertices.get_number_of_elements(); i+=3){
			float2 v1 {vertices.item(i).x, vertices.item(i).y};
			float2 v2 {vertices.item(i + 1).x, vertices.item(i + 1).y};
			float2 v3 {vertices.item(i + 2).x, vertices.item(i + 2).y};

			if(cross(v3 - v1, v2 - v1) < 0)
				std::swap(v3, v2);

			int min_w = std::floor(std::min(v1.x, std::min(v2.x, v3.x)));
			int max_w = std::ceil(std::max(v1.x, std::max(v2.x, v3.x)));
			int min_h = std::floor(std::min(v1.y, std::min(v2.y, v3.y)));
			int max_h = std::ceil(std::max(v1.y, std::max(v2.y, v3.y)));
			for(int h = min_h; h <= max_h; h++){
				for(int w = min_w; w <= max_w; w++){
					if (edge_function(v1, v2, float2 {static_cast<float> (w), static_cast<float> (h)}) > 0 &&
						edge_function(v2, v3, float2 {static_cast<float> (w), static_cast<float> (h)}) > 0 &&
						edge_function(v3, v1, float2 {static_cast<float> (w), static_cast<float> (h)}) > 0) {
						float area = edge_function(v1, v2, v3);
						float w1 = edge_function(v2, v3, float2{static_cast<float> (w), static_cast<float> (h)}) / area;
						float w2 = edge_function(v3, v1, float2{static_cast<float> (w), static_cast<float> (h)}) / area;
						float w3 = edge_function(v1, v2, float2{static_cast<float> (w), static_cast<float> (h)}) / area;
						float z1 = vertices.item(i).z;
						float z2 = vertices.item(i + 1).z;
						float z3 = vertices.item(i + 2).z;

						if (depth_test(w1 * z1 + w2 * z2 + w3 * z3, w, h)) {
							render_target->item(w, h) = cg::unsigned_color::from_color(
									pixel_shader(vertex_buffer->item(index_buffer->item(vertex_offset + i)), vertices.item(i).z));
						}
					}
				}
			}
		}
	}

	template<typename VB, typename RT>
	inline float
	rasterizer<VB, RT>::edge_function(float2 a, float2 b, float2 c)
	{
		return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
	}

	template<typename VB, typename RT>
	inline bool rasterizer<VB, RT>::depth_test(float z, size_t x, size_t y)
	{
		return depth_buffer->item(x, y) > z;
	}

}// namespace cg::renderer