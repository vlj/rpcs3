#include "stdafx.h"
#include "Utilities/rPlatform.h" // only for rImage
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/state.h"
#include "VKGSRender.h"
#include "../rsx_methods.h"
#include "../Common/BufferUtils.h"

namespace vk
{
	bool requires_component_expansion(rsx::vertex_base_type type, u32 size)
	{
		if (size == 3)
		{
			switch (type)
			{
			case rsx::vertex_base_type::f:
				return true;
			}
		}

		return false;
	}

	u32 get_suitable_vk_size(rsx::vertex_base_type type, u32 size)
	{
		if (size == 3)
		{
			switch (type)
			{
			case rsx::vertex_base_type::f:
				return 16;
			}
		}

		return rsx::get_vertex_type_size_on_host(type, size);
	}

	VkFormat get_suitable_vk_format(rsx::vertex_base_type type, u8 size)
	{
		/**
		* Set up buffer fetches to only work on 4-component access. This is hardware dependant so we use 4-component access to avoid branching based on IHV implementation
		* AMD GCN 1.0 for example does not support RGB32 formats for texel buffers
		*/
		const VkFormat vec1_types[] = { VK_FORMAT_R16_UNORM, VK_FORMAT_R32_SFLOAT, VK_FORMAT_R16_SFLOAT, VK_FORMAT_R8_UNORM, VK_FORMAT_R16_SINT, VK_FORMAT_R16_SFLOAT, VK_FORMAT_R8_UNORM };
		const VkFormat vec2_types[] = { VK_FORMAT_R16G16_UNORM, VK_FORMAT_R32G32_SFLOAT, VK_FORMAT_R16G16_SFLOAT, VK_FORMAT_R8G8_UNORM, VK_FORMAT_R16G16_SINT, VK_FORMAT_R16G16_SFLOAT, VK_FORMAT_R8G8_UNORM };
		const VkFormat vec3_types[] = { VK_FORMAT_R16G16B16A16_UNORM, VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R16G16B16A16_SINT, VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R8G8B8A8_UNORM };	//VEC3 COMPONENTS NOT SUPPORTED!
		const VkFormat vec4_types[] = { VK_FORMAT_R16G16B16A16_UNORM, VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R16G16B16A16_SINT, VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R8G8B8A8_UNORM };

		const VkFormat* vec_selectors[] = { 0, vec1_types, vec2_types, vec3_types, vec4_types };

		if (type > rsx::vertex_base_type::ub256)
			throw EXCEPTION("VKGS error: unknown vertex base type 0x%X.", (u32)type);

		return vec_selectors[size][(int)type];
	}

	VkPrimitiveTopology get_appropriate_topology(rsx::primitive_type& mode, bool &requires_modification)
	{
		requires_modification = false;

		switch (mode)
		{
		case rsx::primitive_type::lines:
			return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
		case rsx::primitive_type::line_loop:
			requires_modification = true;
			return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
		case rsx::primitive_type::line_strip:
			return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
		case rsx::primitive_type::points:
			return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
		case rsx::primitive_type::triangles:
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		case rsx::primitive_type::triangle_strip:
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		case rsx::primitive_type::triangle_fan:
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
		case rsx::primitive_type::quads:
		case rsx::primitive_type::quad_strip:
		case rsx::primitive_type::polygon:
			requires_modification = true;
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		default:
			throw ("Unsupported primitive topology 0x%X", (u8)mode);
		}
	}

	/**
	* Expand line loop array to line strip array; simply loop back the last vertex to the first..
	*/
	u32 expand_line_loop_array_to_strip(u32 vertex_draw_count, std::vector<u16>& indices)
	{
		u32 i = 0;
		indices.resize(vertex_draw_count + 1);

		for (; i < vertex_draw_count; ++i)
			indices[i] = i;

		indices[i] = 0;
		return static_cast<u32>(indices.size());
	}

	template<typename T>
	u32 expand_indexed_line_loop_to_strip(u32 original_count, const T* original_indices, std::vector<T>& indices)
	{
		indices.resize(original_count + 1);

		u32 i = 0;
		for (; i < original_count; ++i)
			indices[i] = original_indices[i];

		indices[i] = original_indices[0];
		return static_cast<u32>(indices.size());
	}

	/**
	 * Template: Expand any N-compoent vector to a larger X-component vector and pad unused slots with 1
	 */
	template<typename T, u8 src_components, u8 dst_components, u32 padding>
	void expand_array_components(const T* src_data, std::vector<u8>& dst_data, u32 vertex_count)
	{
		u32 dst_size = (vertex_count * dst_components * sizeof(T));
		dst_data.resize(dst_size);

		T* src = const_cast<T*>(src_data);
		T* dst = reinterpret_cast<T*>(dst_data.data());

		for (u32 index = 0; index < vertex_count; ++index)
		{
			for (u8 channel = 0; channel < dst_components; channel++)
			{
				if (channel < src_components)
				{
					*dst = *src;
					
					dst++;
					src++;
				}
				else
				{
					*dst = (T)(padding);
					dst++;
				}
			}
		}
	}

	void prepare_buffer_for_writing(void *data, rsx::vertex_base_type type, u8 vertex_size, u32 vertex_count)
	{
		switch (type)
		{
		case rsx::vertex_base_type::sf:
		{
			if (vertex_size == 3)
			{
				/**
				* Pad the 4th component for half-float arrays to 1, since texelfetch does not mask components
				*/
				u16 *dst = reinterpret_cast<u16*>(data);
				for (u32 i = 0, idx = 3; i < vertex_count; ++i, idx += 4)
					dst[idx] = 0x3c00;
			}

			break;
		}
		}
	}
}

std::tuple<bool, u32, VkDeviceSize, VkIndexType> VKGSRender::upload_vertex_data()
{
	std::vector<std::pair<u32, u32> > vertex_ranges;
	bool is_indexed_draw;
	rsx::index_array_type index_format;

	size_t offset_in_index_buffer;

	std::tie(vertex_ranges, is_indexed_draw, vertex_draw_count, index_format) = vertex_loader_helper::set_index(draw_command, draw_mode, vertex_arrays_info, inline_vertex_array.size() * sizeof(u32), first_count_commands,
		[&offset_in_index_buffer, this](size_t size) {
		offset_in_index_buffer = m_index_buffer_ring_info.alloc<256>(size);
		void* buf = m_index_buffer_ring_info.heap->map(offset_in_index_buffer, size);
		return gsl::span<gsl::byte> { reinterpret_cast<gsl::byte*>(buf), gsl::narrow<int>(size) };
	},
		[this]() {m_index_buffer_ring_info.heap->unmap(); }
		);

	const std::string reg_table[] =
	{
		"in_pos_buffer", "in_weight_buffer", "in_normal_buffer",
		"in_diff_color_buffer", "in_spec_color_buffer",
		"in_fog_buffer",
		"in_point_size_buffer", "in_7_buffer",
		"in_tc0_buffer", "in_tc1_buffer", "in_tc2_buffer", "in_tc3_buffer",
		"in_tc4_buffer", "in_tc5_buffer", "in_tc6_buffer", "in_tc7_buffer"
	};

	if (draw_command == rsx::draw_command::inlined_array)
	{
		std::vector<std::tuple<u8, size_t, u32> > vertex_buffer_index_offset_size;
		auto map_function = [this, &vertex_buffer_index_offset_size](u8 index, u32 requested_size) {
			u32 size = requested_size;
			size_t offset_in_attrib_buffer = m_attrib_ring_info.alloc<256>(requested_size);
			void* buf = m_attrib_ring_info.heap->map(offset_in_attrib_buffer, requested_size);
			vertex_buffer_index_offset_size.push_back(std::make_tuple(index, offset_in_attrib_buffer, requested_size));
			return gsl::span<gsl::byte> { reinterpret_cast<gsl::byte*>(buf), gsl::narrow<int>(requested_size) };
		};
		auto unmap_function = [this]() { m_attrib_ring_info.heap->unmap(); };

		vertex_loader_helper::set_vertex_array_inlined(vertex_arrays_info, inline_vertex_array, vk::get_suitable_vk_size, map_function, unmap_function);

		for (const auto &vb_info : vertex_buffer_index_offset_size)
		{
			u8 index;
			size_t offset;
			u32 size;
			std::tie(index, offset, size) = vb_info;

			if (!m_program->has_uniform(reg_table[index])) continue;

			const VkFormat format = vk::get_suitable_vk_format(vertex_arrays_info[index].type, vertex_arrays_info[index].size);
			m_buffer_view_to_clean.push_back(std::make_unique<vk::buffer_view>(*m_device, m_attrib_ring_info.heap->value, format, offset, size));
			m_program->bind_uniform(m_buffer_view_to_clean.back()->value, reg_table[index], descriptor_sets);
		}
		return std::make_tuple(is_indexed_draw, vertex_draw_count, offset_in_index_buffer, vk::get_index_type(index_format));
	}

	u32 input_mask = rsx::method_registers[NV4097_SET_VERTEX_ATTRIB_INPUT_MASK];

	for (int index = 0; index < rsx::limits::vertex_count; ++index)
	{
		bool enabled = !!(input_mask & (1 << index));

		if (!m_program->has_uniform(reg_table[index])) continue;
		if (!enabled) continue;

		if (vertex_arrays_info[index].size > 0)
		{
			size_t offset_in_attrib_buffer;
			u32 size;

			auto map_function = [this, &offset_in_attrib_buffer, &size](u32 requested_size) {
				size = requested_size;
				offset_in_attrib_buffer = m_attrib_ring_info.alloc<256>(requested_size);
				void* buf = m_attrib_ring_info.heap->map(offset_in_attrib_buffer, requested_size);
				return gsl::span<gsl::byte> { reinterpret_cast<gsl::byte*>(buf), gsl::narrow<int>(requested_size) };
			};
			auto unmap_function = [this]() { m_attrib_ring_info.heap->unmap(); };

			vertex_loader_helper::set_vertex_array(vertex_arrays_info[index], index, vertex_ranges, vk::get_suitable_vk_size, map_function, unmap_function);
			const VkFormat format = vk::get_suitable_vk_format(vertex_arrays_info[index].type, vertex_arrays_info[index].size);
			m_buffer_view_to_clean.push_back(std::make_unique<vk::buffer_view>(*m_device, m_attrib_ring_info.heap->value, format, offset_in_attrib_buffer, size));
			m_program->bind_uniform(m_buffer_view_to_clean.back()->value, reg_table[index], descriptor_sets);
		}
		else if (register_vertex_info[index].size > 0)
		{
			size_t offset_in_attrib_buffer;
			u32 size;

			auto map_function = [this, &offset_in_attrib_buffer, &size](u32 requested_size) {
				size = requested_size;
				offset_in_attrib_buffer = m_attrib_ring_info.alloc<256>(requested_size);
				void* buf = m_attrib_ring_info.heap->map(offset_in_attrib_buffer, requested_size);
				return gsl::span<gsl::byte> { reinterpret_cast<gsl::byte*>(buf), gsl::narrow<int>(requested_size) };
			};
			auto unmap_function = [this]() { m_attrib_ring_info.heap->unmap(); };

			vertex_loader_helper::set_vertex_register(register_vertex_info[index], register_vertex_data[index], vk::get_suitable_vk_size, map_function, unmap_function);
			VkFormat format = vk::get_suitable_vk_format(register_vertex_info[index].type, register_vertex_info[index].size);
			m_buffer_view_to_clean.push_back(std::make_unique<vk::buffer_view>(*m_device, m_attrib_ring_info.heap->value, format, offset_in_attrib_buffer, size));
			m_program->bind_uniform(m_buffer_view_to_clean.back()->value, reg_table[index], descriptor_sets);
		}
	}

	return std::make_tuple(is_indexed_draw, vertex_draw_count, offset_in_index_buffer, vk::get_index_type(index_format));
}