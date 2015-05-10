#include "stdafx.h"
#include "D3D12GSRender.h"
#include <wrl/client.h>
#include <dxgi1_4.h>

GetGSFrameCb2 GetGSFrame = nullptr;

void SetGetD3DGSFrameCallback(GetGSFrameCb2 value)
{
	GetGSFrame = value;
}

static void check(HRESULT hr)
{
	if (hr != 0)
		abort();
}

D3D12GSRender::D3D12GSRender()
	: GSRender()
{
	// Enable d3d debug layer
	Microsoft::WRL::ComPtr<ID3D12Debug> debugInterface;
	D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface));
	debugInterface->EnableDebugLayer();

	// Create adapter
	Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory;
	check(CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory)));
	IDXGIAdapter* warpAdapter;
	check(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));
	check(D3D12CreateDevice(warpAdapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device)));

	// Queues
	D3D12_COMMAND_QUEUE_DESC copyQueueDesc = {}, graphicQueueDesc = {};
	copyQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
	graphicQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	check(m_device->CreateCommandQueue(&copyQueueDesc, IID_PPV_ARGS(&m_commandQueueCopy)));
	check(m_device->CreateCommandQueue(&graphicQueueDesc, IID_PPV_ARGS(&m_commandQueueGraphic)));

	GSFrameBase2 *tmp = GetGSFrame();
	tmp->Show();
}

D3D12GSRender::~D3D12GSRender()
{
	// NOTE: Should be released only if no command are in flight !
	m_commandQueueGraphic->Release();
	m_commandQueueCopy->Release();
	m_device->Release();
}

void D3D12GSRender::Close()
{
}


void D3D12GSRender::InitDrawBuffers()
{
	if (!m_fbo.IsCreated() || RSXThread::m_width != m_lastWidth || RSXThread::m_height != m_lastHeight || m_lastDepth != m_surface_depth_format)
	{
		
		LOG_WARNING(RSX, "New FBO (%dx%d)", RSXThread::m_width, RSXThread::m_height);
		m_lastWidth = RSXThread::m_width;
		m_lastHeight = RSXThread::m_height;
		m_lastDepth = m_surface_depth_format;

/*		m_fbo.Create();
		checkForGlError("m_fbo.Create");
		m_fbo.Bind();

		m_rbo.Create(4 + 1);
		checkForGlError("m_rbo.Create");

		for (int i = 0; i < 4; ++i)
		{
			m_rbo.Bind(i);
			m_rbo.Storage(GL_RGBA, RSXThread::m_width, RSXThread::m_height);
			checkForGlError("m_rbo.Storage(GL_RGBA)");
		}

		m_rbo.Bind(4);

		switch (m_surface_depth_format)
		{
		case 0:
		{
			// case 0 found in BLJM60410-[Suzukaze no Melt - Days in the Sanctuary]
			// [E : RSXThread]: Bad depth format! (0)
			// [E : RSXThread]: glEnable: opengl error 0x0506
			// [E : RSXThread]: glDrawArrays: opengl error 0x0506
			m_rbo.Storage(GL_DEPTH_COMPONENT, RSXThread::m_width, RSXThread::m_height);
			checkForGlError("m_rbo.Storage(GL_DEPTH_COMPONENT)");
			break;
		}

		case CELL_GCM_SURFACE_Z16:
		{
			m_rbo.Storage(GL_DEPTH_COMPONENT16, RSXThread::m_width, RSXThread::m_height);
			checkForGlError("m_rbo.Storage(GL_DEPTH_COMPONENT16)");

			m_fbo.Renderbuffer(GL_DEPTH_ATTACHMENT, m_rbo.GetId(4));
			checkForGlError("m_fbo.Renderbuffer(GL_DEPTH_ATTACHMENT)");
			break;
		}


		case CELL_GCM_SURFACE_Z24S8:
		{
			m_rbo.Storage(GL_DEPTH24_STENCIL8, RSXThread::m_width, RSXThread::m_height);
			checkForGlError("m_rbo.Storage(GL_DEPTH24_STENCIL8)");

			m_fbo.Renderbuffer(GL_DEPTH_ATTACHMENT, m_rbo.GetId(4));
			checkForGlError("m_fbo.Renderbuffer(GL_DEPTH_ATTACHMENT)");

			m_fbo.Renderbuffer(GL_STENCIL_ATTACHMENT, m_rbo.GetId(4));
			checkForGlError("m_fbo.Renderbuffer(GL_STENCIL_ATTACHMENT)");

			break;

		}


		default:
		{
			LOG_ERROR(RSX, "Bad depth format! (%d)", m_surface_depth_format);
			assert(0);
			break;
		}
		}

		for (int i = 0; i < 4; ++i)
		{
			m_fbo.Renderbuffer(GL_COLOR_ATTACHMENT0 + i, m_rbo.GetId(i));
			checkForGlError(fmt::Format("m_fbo.Renderbuffer(GL_COLOR_ATTACHMENT%d)", i));
		}
		*/
		//m_fbo.Renderbuffer(GL_DEPTH_ATTACHMENT, m_rbo.GetId(4));
		//checkForGlError("m_fbo.Renderbuffer(GL_DEPTH_ATTACHMENT)");

		//if (m_surface_depth_format == 2)
		//{
		//	m_fbo.Renderbuffer(GL_STENCIL_ATTACHMENT, m_rbo.GetId(4));
		//	checkForGlError("m_fbo.Renderbuffer(GL_STENCIL_ATTACHMENT)");
		//}
	}
	/*
	if (!m_set_surface_clip_horizontal)
	{
		m_surface_clip_x = 0;
		m_surface_clip_w = RSXThread::m_width;
	}

	if (!m_set_surface_clip_vertical)
	{
		m_surface_clip_y = 0;
		m_surface_clip_h = RSXThread::m_height;
	}

	m_fbo.Bind();

	static const GLenum draw_buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };

	switch (m_surface_color_target)
	{
	case CELL_GCM_SURFACE_TARGET_NONE: break;

	case CELL_GCM_SURFACE_TARGET_0:
	{
		glDrawBuffer(draw_buffers[0]);
		checkForGlError("glDrawBuffer(0)");
		break;
	}

	case CELL_GCM_SURFACE_TARGET_1:
	{
		glDrawBuffer(draw_buffers[1]);
		checkForGlError("glDrawBuffer(1)");
		break;
	}

	case CELL_GCM_SURFACE_TARGET_MRT1:
	{
		glDrawBuffers(2, draw_buffers);
		checkForGlError("glDrawBuffers(2)");
		break;
	}

	case CELL_GCM_SURFACE_TARGET_MRT2:
	{
		glDrawBuffers(3, draw_buffers);
		checkForGlError("glDrawBuffers(3)");
		break;
	}

	case CELL_GCM_SURFACE_TARGET_MRT3:
	{
		glDrawBuffers(4, draw_buffers);
		checkForGlError("glDrawBuffers(4)");
		break;
	}

	default:
	{
		LOG_ERROR(RSX, "Bad surface color target: %d", m_surface_color_target);
		break;
	}

	}

	if (m_read_buffer)
	{
		u32 format = GL_BGRA;
		CellGcmDisplayInfo* buffers = vm::get_ptr<CellGcmDisplayInfo>(m_gcm_buffers_addr);
		u32 addr = GetAddress(buffers[m_gcm_current_buffer].offset, CELL_GCM_LOCATION_LOCAL);
		u32 width = buffers[m_gcm_current_buffer].width;
		u32 height = buffers[m_gcm_current_buffer].height;
		glDrawPixels(width, height, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8, vm::get_ptr(addr));
	}*/
}

void D3D12GSRender::OnInit()
{
}

void D3D12GSRender::OnInitThread()
{
}

void D3D12GSRender::OnExitThread()
{
}

void D3D12GSRender::OnReset()
{
}

void D3D12GSRender::ExecCMD(u32 cmd)
{
}

void D3D12GSRender::ExecCMD()
{
	ID3D12CommandAllocator *commandAllocator;
	m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
	ID3D12CommandList *commandList;
	m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator, nullptr, IID_PPV_ARGS(&commandList));


	//return;
/*  if (!LoadProgram())
	{
		LOG_ERROR(RSX, "LoadProgram failed.");
		Emu.Pause();
		return;
	}

	InitDrawBuffers();

	if (m_set_color_mask)
	{
		glColorMask(m_color_mask_r, m_color_mask_g, m_color_mask_b, m_color_mask_a);
		checkForGlError("glColorMask");
	}

	if (!m_indexed_array.m_count && !m_draw_array_count)
	{
		u32 min_vertex_size = ~0;
		for (auto &i : m_vertex_data)
		{
			if (!i.size)
				continue;

			u32 vertex_size = i.data.size() / (i.size * i.GetTypeSize());

			if (min_vertex_size > vertex_size)
				min_vertex_size = vertex_size;
		}

		m_draw_array_count = min_vertex_size;
		m_draw_array_first = 0;
	}

	Enable(m_set_depth_test, GL_DEPTH_TEST);
	Enable(m_set_alpha_test, GL_ALPHA_TEST);
	Enable(m_set_blend || m_set_blend_mrt1 || m_set_blend_mrt2 || m_set_blend_mrt3, GL_BLEND);
	Enable(m_set_scissor_horizontal && m_set_scissor_vertical, GL_SCISSOR_TEST);
	Enable(m_set_logic_op, GL_LOGIC_OP);
	Enable(m_set_cull_face, GL_CULL_FACE);
	Enable(m_set_dither, GL_DITHER);
	Enable(m_set_stencil_test, GL_STENCIL_TEST);
	Enable(m_set_line_smooth, GL_LINE_SMOOTH);
	Enable(m_set_poly_smooth, GL_POLYGON_SMOOTH);
	Enable(m_set_point_sprite_control, GL_POINT_SPRITE);
	Enable(m_set_specular, GL_LIGHTING);
	Enable(m_set_poly_offset_fill, GL_POLYGON_OFFSET_FILL);
	Enable(m_set_poly_offset_line, GL_POLYGON_OFFSET_LINE);
	Enable(m_set_poly_offset_point, GL_POLYGON_OFFSET_POINT);
	Enable(m_set_restart_index, GL_PRIMITIVE_RESTART);
	Enable(m_set_line_stipple, GL_LINE_STIPPLE);
	Enable(m_set_polygon_stipple, GL_POLYGON_STIPPLE);

	if (!is_intel_vendor)
	{
		Enable(m_set_depth_bounds_test, GL_DEPTH_BOUNDS_TEST_EXT);
	}

	if (m_set_clip_plane)
	{
		Enable(m_clip_plane_0, GL_CLIP_PLANE0);
		Enable(m_clip_plane_1, GL_CLIP_PLANE1);
		Enable(m_clip_plane_2, GL_CLIP_PLANE2);
		Enable(m_clip_plane_3, GL_CLIP_PLANE3);
		Enable(m_clip_plane_4, GL_CLIP_PLANE4);
		Enable(m_clip_plane_5, GL_CLIP_PLANE5);

		checkForGlError("m_set_clip_plane");
	}

	checkForGlError("glEnable");

	if (m_set_front_polygon_mode)
	{
		glPolygonMode(GL_FRONT, m_front_polygon_mode);
		checkForGlError("glPolygonMode(Front)");
	}

	if (m_set_back_polygon_mode)
	{
		glPolygonMode(GL_BACK, m_back_polygon_mode);
		checkForGlError("glPolygonMode(Back)");
	}

	if (m_set_point_size)
	{
		glPointSize(m_point_size);
		checkForGlError("glPointSize");
	}

	if (m_set_poly_offset_mode)
	{
		glPolygonOffset(m_poly_offset_scale_factor, m_poly_offset_bias);
		checkForGlError("glPolygonOffset");
	}

	if (m_set_logic_op)
	{
		glLogicOp(m_logic_op);
		checkForGlError("glLogicOp");
	}

	if (m_set_scissor_horizontal && m_set_scissor_vertical)
	{
		glScissor(m_scissor_x, m_scissor_y, m_scissor_w, m_scissor_h);
		checkForGlError("glScissor");
	}

	if (m_set_two_sided_stencil_test_enable)
	{
		if (m_set_stencil_fail && m_set_stencil_zfail && m_set_stencil_zpass)
		{
			glStencilOpSeparate(GL_FRONT, m_stencil_fail, m_stencil_zfail, m_stencil_zpass);
			checkForGlError("glStencilOpSeparate");
		}

		if (m_set_stencil_mask)
		{
			glStencilMaskSeparate(GL_FRONT, m_stencil_mask);
			checkForGlError("glStencilMaskSeparate");
		}

		if (m_set_stencil_func && m_set_stencil_func_ref && m_set_stencil_func_mask)
		{
			glStencilFuncSeparate(GL_FRONT, m_stencil_func, m_stencil_func_ref, m_stencil_func_mask);
			checkForGlError("glStencilFuncSeparate");
		}

		if (m_set_back_stencil_fail && m_set_back_stencil_zfail && m_set_back_stencil_zpass)
		{
			glStencilOpSeparate(GL_BACK, m_back_stencil_fail, m_back_stencil_zfail, m_back_stencil_zpass);
			checkForGlError("glStencilOpSeparate(GL_BACK)");
		}

		if (m_set_back_stencil_mask)
		{
			glStencilMaskSeparate(GL_BACK, m_back_stencil_mask);
			checkForGlError("glStencilMaskSeparate(GL_BACK)");
		}

		if (m_set_back_stencil_func && m_set_back_stencil_func_ref && m_set_back_stencil_func_mask)
		{
			glStencilFuncSeparate(GL_BACK, m_back_stencil_func, m_back_stencil_func_ref, m_back_stencil_func_mask);
			checkForGlError("glStencilFuncSeparate(GL_BACK)");
		}
	}
	else
	{
		if (m_set_stencil_fail && m_set_stencil_zfail && m_set_stencil_zpass)
		{
			glStencilOp(m_stencil_fail, m_stencil_zfail, m_stencil_zpass);
			checkForGlError("glStencilOp");
		}

		if (m_set_stencil_mask)
		{
			glStencilMask(m_stencil_mask);
			checkForGlError("glStencilMask");
		}

		if (m_set_stencil_func && m_set_stencil_func_ref && m_set_stencil_func_mask)
		{
			glStencilFunc(m_stencil_func, m_stencil_func_ref, m_stencil_func_mask);
			checkForGlError("glStencilFunc");
		}
	}

	// TODO: Use other glLightModel functions?
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, m_set_two_side_light_enable ? GL_TRUE : GL_FALSE);
	checkForGlError("glLightModeli");

	if (m_set_shade_mode)
	{
		glShadeModel(m_shade_mode);
		checkForGlError("glShadeModel");
	}

	if (m_set_depth_mask)
	{
		glDepthMask(m_depth_mask);
		checkForGlError("glDepthMask");
	}

	if (m_set_depth_func)
	{
		glDepthFunc(m_depth_func);
		checkForGlError("glDepthFunc");
	}

	if (m_set_depth_bounds && !is_intel_vendor)
	{
		glDepthBoundsEXT(m_depth_bounds_min, m_depth_bounds_max);
		checkForGlError("glDepthBounds");
	}

	if (m_set_clip)
	{
		glDepthRangef(m_clip_min, m_clip_max);
		checkForGlError("glDepthRangef");
	}

	if (m_set_line_width)
	{
		glLineWidth(m_line_width);
		checkForGlError("glLineWidth");
	}

	if (m_set_line_stipple)
	{
		glLineStipple(m_line_stipple_factor, m_line_stipple_pattern);
		checkForGlError("glLineStipple");
	}

	if (m_set_polygon_stipple)
	{
		glPolygonStipple((const GLubyte*)m_polygon_stipple_pattern);
		checkForGlError("glPolygonStipple");
	}

	if (m_set_blend_equation)
	{
		glBlendEquationSeparate(m_blend_equation_rgb, m_blend_equation_alpha);
		checkForGlError("glBlendEquationSeparate");
	}

	if (m_set_blend_sfactor && m_set_blend_dfactor)
	{
		glBlendFuncSeparate(m_blend_sfactor_rgb, m_blend_dfactor_rgb, m_blend_sfactor_alpha, m_blend_dfactor_alpha);
		checkForGlError("glBlendFuncSeparate");
	}

	if (m_set_blend_color)
	{
		glBlendColor(m_blend_color_r, m_blend_color_g, m_blend_color_b, m_blend_color_a);
		checkForGlError("glBlendColor");
	}

	if (m_set_cull_face)
	{
		glCullFace(m_cull_face);
		checkForGlError("glCullFace");
	}

	if (m_set_front_face)
	{
		glFrontFace(m_front_face);
		checkForGlError("glFrontFace");
	}

	if (m_set_alpha_func && m_set_alpha_ref)
	{
		glAlphaFunc(m_alpha_func, m_alpha_ref);
		checkForGlError("glAlphaFunc");
	}

	if (m_set_fog_mode)
	{
		glFogi(GL_FOG_MODE, m_fog_mode);
		checkForGlError("glFogi(GL_FOG_MODE)");
	}

	if (m_set_fog_params)
	{
		glFogf(GL_FOG_START, m_fog_param0);
		checkForGlError("glFogf(GL_FOG_START)");
		glFogf(GL_FOG_END, m_fog_param1);
		checkForGlError("glFogf(GL_FOG_END)");
	}

	if (m_set_restart_index)
	{
		glPrimitiveRestartIndex(m_restart_index);
		checkForGlError("glPrimitiveRestartIndex");
	}

	if (m_indexed_array.m_count && m_draw_array_count)
	{
		LOG_WARNING(RSX, "m_indexed_array.m_count && draw_array_count");
	}

	for (u32 i = 0; i < m_textures_count; ++i)
	{
		if (!m_textures[i].IsEnabled()) continue;

		glActiveTexture(GL_TEXTURE0 + i);
		checkForGlError("glActiveTexture");
		m_gl_textures[i].Create();
		m_gl_textures[i].Bind();
		checkForGlError(fmt::Format("m_gl_textures[%d].Bind", i));
		m_program.SetTex(i);
		m_gl_textures[i].Init(m_textures[i]);
		checkForGlError(fmt::Format("m_gl_textures[%d].Init", i));
	}

	for (u32 i = 0; i < m_textures_count; ++i)
	{
		if (!m_vertex_textures[i].IsEnabled()) continue;

		glActiveTexture(GL_TEXTURE0 + m_textures_count + i);
		checkForGlError("glActiveTexture");
		m_gl_vertex_textures[i].Create();
		m_gl_vertex_textures[i].Bind();
		checkForGlError(fmt::Format("m_gl_vertex_textures[%d].Bind", i));
		m_program.SetVTex(i);
		m_gl_vertex_textures[i].Init(m_vertex_textures[i]);
		checkForGlError(fmt::Format("m_gl_vertex_textures[%d].Init", i));
	}

	m_vao.Bind();

	if (m_indexed_array.m_count)
	{
		LoadVertexData(m_indexed_array.index_min, m_indexed_array.index_max - m_indexed_array.index_min + 1);
	}

	if (m_indexed_array.m_count || m_draw_array_count)
	{
		EnableVertexData(m_indexed_array.m_count ? true : false);

		InitVertexData();
		InitFragmentData();
	}

	if (m_indexed_array.m_count)
	{
		switch (m_indexed_array.m_type)
		{
		case CELL_GCM_DRAW_INDEX_ARRAY_TYPE_32:
			glDrawElements(m_draw_mode - 1, m_indexed_array.m_count, GL_UNSIGNED_INT, nullptr);
			checkForGlError("glDrawElements #4");
			break;

		case CELL_GCM_DRAW_INDEX_ARRAY_TYPE_16:
			glDrawElements(m_draw_mode - 1, m_indexed_array.m_count, GL_UNSIGNED_SHORT, nullptr);
			checkForGlError("glDrawElements #2");
			break;

		default:
			LOG_ERROR(RSX, "Bad indexed array type (%d)", m_indexed_array.m_type);
			break;
		}

		DisableVertexData();
		m_indexed_array.Reset();
	}

	if (m_draw_array_count)
	{
		//LOG_WARNING(RSX,"glDrawArrays(%d,%d,%d)", m_draw_mode - 1, m_draw_array_first, m_draw_array_count);
		glDrawArrays(m_draw_mode - 1, 0, m_draw_array_count);
		checkForGlError("glDrawArrays");
		DisableVertexData();
	}

	WriteBuffers();*/
}

void D3D12GSRender::Flip()
{
}
