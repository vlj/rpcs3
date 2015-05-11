#include "stdafx.h"
#if defined(DX12_SUPPORT)
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
	: GSRender(), m_fbo(nullptr), m_PSO(nullptr)
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

	// Create a global command allocator
	m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator));

	m_frame = GetGSFrame();

	// Create swap chain and put them in a descriptor heap as rendertarget
	DXGI_SWAP_CHAIN_DESC swapChain = {};
	swapChain.BufferCount = 2;
	swapChain.Windowed = true;
	swapChain.OutputWindow = m_frame->getHandle();
	swapChain.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChain.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChain.SampleDesc.Count = 1;
	swapChain.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	swapChain.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;

	check(dxgiFactory->CreateSwapChain(m_commandQueueGraphic, &swapChain, (IDXGISwapChain**)&m_swapChain));
	m_swapChain->GetBuffer(0, IID_PPV_ARGS(&m_backBuffer[0]));
	m_swapChain->GetBuffer(1, IID_PPV_ARGS(&m_backBuffer[1]));

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = 1;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	D3D12_RENDER_TARGET_VIEW_DESC rttDesc = {};
	rttDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rttDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_backbufferAsRendertarget[0]));
	m_device->CreateRenderTargetView(m_backBuffer[0], &rttDesc, m_backbufferAsRendertarget[0]->GetCPUDescriptorHandleForHeapStart());
	m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_backbufferAsRendertarget[1]));
	m_device->CreateRenderTargetView(m_backBuffer[1], &rttDesc, m_backbufferAsRendertarget[1]->GetCPUDescriptorHandleForHeapStart());
}

D3D12GSRender::~D3D12GSRender()
{
	// NOTE: Should be released only if no command are in flight !
	m_commandAllocator->Release();
	m_commandQueueGraphic->Release();
	m_commandQueueCopy->Release();
	m_backbufferAsRendertarget[0]->Release();
	m_backbufferAsRendertarget[1]->Release();
	m_backBuffer[0]->Release();
	m_backBuffer[1]->Release();
	m_swapChain->Release();
	m_device->Release();
}

void D3D12GSRender::Close()
{
	Stop();
	m_frame->Hide();
}

void D3D12GSRender::InitDrawBuffers()
{
	if (m_fbo == nullptr || RSXThread::m_width != m_lastWidth || RSXThread::m_height != m_lastHeight || m_lastDepth != m_surface_depth_format)
	{
		
		LOG_WARNING(RSX, "New FBO (%dx%d)", RSXThread::m_width, RSXThread::m_height);
		m_lastWidth = RSXThread::m_width;
		m_lastHeight = RSXThread::m_height;
		m_lastDepth = m_surface_depth_format;

		m_fbo = new D3D12RenderTargetSets(m_device, (u8)m_lastDepth, m_lastWidth, m_lastHeight);
	}
}

void D3D12GSRender::OnInit()
{
	m_frame->Show();
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
	assert(cmd == NV4097_CLEAR_SURFACE);

	InitDrawBuffers();

	ID3D12GraphicsCommandList *commandList;
	check(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator, nullptr, IID_PPV_ARGS(&commandList)));
	m_inflightCommandList.push_back(commandList);

/*	if (m_set_color_mask)
	{
		glColorMask(m_color_mask_r, m_color_mask_g, m_color_mask_b, m_color_mask_a);
		checkForGlError("glColorMask");
	}

	if (m_set_scissor_horizontal && m_set_scissor_vertical)
	{
		glScissor(m_scissor_x, m_scissor_y, m_scissor_w, m_scissor_h);
		checkForGlError("glScissor");
	}*/

	// TODO: Merge depth and stencil clear when possible
	if (m_clear_surface_mask & 0x1)
		commandList->ClearDepthStencilView(m_fbo->getDSVCPUHandle(), D3D12_CLEAR_FLAG_DEPTH, m_clear_surface_z / (float)0xffffff, 0, 0, nullptr);

	if (m_clear_surface_mask & 0x2)
		commandList->ClearDepthStencilView(m_fbo->getDSVCPUHandle(), D3D12_CLEAR_FLAG_STENCIL, 0.f, m_clear_surface_s, 0, nullptr);

	if (m_clear_surface_mask & 0xF0)
	{
		float clearColor[] =
		{
			m_clear_surface_color_r / 255.0f,
			m_clear_surface_color_g / 255.0f,
			m_clear_surface_color_b / 255.0f,
			m_clear_surface_color_a / 255.0f
		};
		switch (m_surface_color_target)
		{
			case CELL_GCM_SURFACE_TARGET_NONE: break;

			case CELL_GCM_SURFACE_TARGET_0:
				commandList->ClearRenderTargetView(m_fbo->getRTTCPUHandle(0), clearColor, 0, nullptr);
				break;
			case CELL_GCM_SURFACE_TARGET_1:
				commandList->ClearRenderTargetView(m_fbo->getRTTCPUHandle(1), clearColor, 0, nullptr);
				break;
			case CELL_GCM_SURFACE_TARGET_MRT1:
				commandList->ClearRenderTargetView(m_fbo->getRTTCPUHandle(0), clearColor, 0, nullptr);
				commandList->ClearRenderTargetView(m_fbo->getRTTCPUHandle(1), clearColor, 0, nullptr);
				break;
			case CELL_GCM_SURFACE_TARGET_MRT2:
				commandList->ClearRenderTargetView(m_fbo->getRTTCPUHandle(0), clearColor, 0, nullptr);
				commandList->ClearRenderTargetView(m_fbo->getRTTCPUHandle(1), clearColor, 0, nullptr);
				commandList->ClearRenderTargetView(m_fbo->getRTTCPUHandle(2), clearColor, 0, nullptr);
				break;
			case CELL_GCM_SURFACE_TARGET_MRT3:
				commandList->ClearRenderTargetView(m_fbo->getRTTCPUHandle(0), clearColor, 0, nullptr);
				commandList->ClearRenderTargetView(m_fbo->getRTTCPUHandle(1), clearColor, 0, nullptr);
				commandList->ClearRenderTargetView(m_fbo->getRTTCPUHandle(2), clearColor, 0, nullptr);
				commandList->ClearRenderTargetView(m_fbo->getRTTCPUHandle(3), clearColor, 0, nullptr);
				break;
			default:
				LOG_ERROR(RSX, "Bad surface color target: %d", m_surface_color_target);
		}
	}

	check(commandList->Close());
	m_commandQueueGraphic->ExecuteCommandLists(1, (ID3D12CommandList**) &commandList);
}

// Where are these type defined ???
static
DXGI_FORMAT getFormat(u8 type, u8 size)
{
	/*static const u32 gl_types[] =
	{
		GL_SHORT,
		GL_FLOAT,
		GL_HALF_FLOAT,
		GL_UNSIGNED_BYTE,
		GL_SHORT,
		GL_FLOAT, // Needs conversion
		GL_UNSIGNED_BYTE,
	};

	static const bool gl_normalized[] =
	{
		GL_TRUE,
		GL_FALSE,
		GL_FALSE,
		GL_TRUE,
		GL_FALSE,
		GL_TRUE,
		GL_FALSE,
	};*/
	static const DXGI_FORMAT typeX1[] =
	{
		DXGI_FORMAT_R16_SNORM,
		DXGI_FORMAT_R32_FLOAT,
		DXGI_FORMAT_R16_FLOAT,
		DXGI_FORMAT_R8_UNORM,
		DXGI_FORMAT_R16_SINT,
		DXGI_FORMAT_R32_FLOAT,
		DXGI_FORMAT_R8_UINT
	};
	static const DXGI_FORMAT typeX2[] =
	{
		DXGI_FORMAT_R16G16_SNORM,
		DXGI_FORMAT_R32G32_FLOAT,
		DXGI_FORMAT_R16G16_FLOAT,
		DXGI_FORMAT_R8G8_UNORM,
		DXGI_FORMAT_R16G16_SINT,
		DXGI_FORMAT_R32G32_FLOAT,
		DXGI_FORMAT_R8G8_UINT
	};
	static const DXGI_FORMAT typeX3[] =
	{
		DXGI_FORMAT_R16G16B16A16_SNORM,
		DXGI_FORMAT_R32G32B32_FLOAT,
		DXGI_FORMAT_R16G16B16A16_FLOAT,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		DXGI_FORMAT_R16G16B16A16_SINT,
		DXGI_FORMAT_R32G32B32_FLOAT,
		DXGI_FORMAT_R8G8B8A8_UINT
	};
	static const DXGI_FORMAT typeX4[] =
	{
		DXGI_FORMAT_R16G16B16A16_SNORM,
		DXGI_FORMAT_R32G32B32A32_FLOAT,
		DXGI_FORMAT_R16G16B16A16_FLOAT,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		DXGI_FORMAT_R16G16B16A16_SINT,
		DXGI_FORMAT_R32G32B32A32_FLOAT,
		DXGI_FORMAT_R8G8B8A8_UINT
	};

	switch (size)
	{
	case 1:
		return typeX1[type];
	case 2:
		return typeX2[type];
	case 3:
		return typeX3[type];
	case 4:
		return typeX4[type];
	}
}

void D3D12GSRender::EnableVertexData(bool indexed_draw)
{
	static u32 offset_list[m_vertex_count];
	u32 cur_offset = 0;

	const u32 data_offset = indexed_draw ? 0 : m_draw_array_first;

	for (u32 i = 0; i < m_vertex_count; ++i)
	{
		offset_list[i] = cur_offset;

		if (!m_vertex_data[i].IsEnabled()) continue;
		const size_t item_size = m_vertex_data[i].GetTypeSize() * m_vertex_data[i].size;
		const size_t data_size = m_vertex_data[i].data.size() - data_offset * item_size;
		const u32 pos = m_vdata.size();

		cur_offset += data_size;
		m_vdata.resize(m_vdata.size() + data_size);
		memcpy(&m_vdata[pos], &m_vertex_data[i].data[data_offset * item_size], data_size);
	}
	// TODO: Use default heap and upload data
	D3D12_HEAP_PROPERTIES heapProp = {};
	heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resDesc.Width = (UINT)m_vdata.size();
	resDesc.Height = 1;
	resDesc.DepthOrArraySize = 1;
	resDesc.SampleDesc.Count = 1;
	resDesc.MipLevels = 1;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	check(m_device->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_vertexBuffer)
		));
	void *bufferMap;

	check(m_vertexBuffer->Map(0, nullptr, (void**)&bufferMap));
	memcpy(bufferMap, m_vdata.data(), m_vdata.size());
	m_vertexBuffer->Unmap(0, nullptr);

	if (indexed_draw)
	{
		D3D12_RESOURCE_DESC resDesc = {};
		resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resDesc.Width = (UINT)m_indexed_array.m_data.size();
		resDesc.Height = 1;
		resDesc.DepthOrArraySize = 1;
		resDesc.SampleDesc.Count = 1;
		resDesc.MipLevels = 1;
		resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		check(m_device->CreateCommittedResource(
			&heapProp,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_indexBuffer)
			));

		check(m_indexBuffer->Map(0, nullptr, (void**)&bufferMap));
		memcpy(bufferMap, m_indexed_array.m_data.data(), m_indexed_array.m_data.size());
		m_indexBuffer->Unmap(0, nullptr);

		D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
		indexBufferView.SizeInBytes = (UINT)m_indexed_array.m_data.size();
		indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
	}

#if	DUMP_VERTEX_DATA
	rFile dump("VertexDataArray.dump", rFile::write);
#endif

	m_IASet.clear();

	for (u32 i = 0; i < m_vertex_count; ++i)
	{
		if (!m_vertex_data[i].IsEnabled()) continue;

#if	DUMP_VERTEX_DATA
		dump.Write(wxString::Format("VertexData[%d]:\n", i));
		switch (m_vertex_data[i].type)
		{
		case CELL_GCM_VERTEX_S1:
			for (u32 j = 0; j < m_vertex_data[i].data.size(); j += 2)
			{
				dump.Write(wxString::Format("%d\n", *(u16*)&m_vertex_data[i].data[j]));
				if (!(((j + 2) / 2) % m_vertex_data[i].size)) dump.Write("\n");
			}
			break;

		case CELL_GCM_VERTEX_F:
			for (u32 j = 0; j < m_vertex_data[i].data.size(); j += 4)
			{
				dump.Write(wxString::Format("%.01f\n", *(float*)&m_vertex_data[i].data[j]));
				if (!(((j + 4) / 4) % m_vertex_data[i].size)) dump.Write("\n");
			}
			break;

		case CELL_GCM_VERTEX_SF:
			for (u32 j = 0; j < m_vertex_data[i].data.size(); j += 2)
			{
				dump.Write(wxString::Format("%.01f\n", *(float*)&m_vertex_data[i].data[j]));
				if (!(((j + 2) / 2) % m_vertex_data[i].size)) dump.Write("\n");
			}
			break;

		case CELL_GCM_VERTEX_UB:
			for (u32 j = 0; j < m_vertex_data[i].data.size(); ++j)
			{
				dump.Write(wxString::Format("%d\n", m_vertex_data[i].data[j]));
				if (!((j + 1) % m_vertex_data[i].size)) dump.Write("\n");
			}
			break;

		case CELL_GCM_VERTEX_S32K:
			for (u32 j = 0; j < m_vertex_data[i].data.size(); j += 2)
			{
				dump.Write(wxString::Format("%d\n", *(u16*)&m_vertex_data[i].data[j]));
				if (!(((j + 2) / 2) % m_vertex_data[i].size)) dump.Write("\n");
			}
			break;

			// case CELL_GCM_VERTEX_CMP:

		case CELL_GCM_VERTEX_UB256:
			for (u32 j = 0; j < m_vertex_data[i].data.size(); ++j)
			{
				dump.Write(wxString::Format("%d\n", m_vertex_data[i].data[j]));
				if (!((j + 1) % m_vertex_data[i].size)) dump.Write("\n");
			}
			break;

		default:
			LOG_ERROR(HLE, "Bad cv type! %d", m_vertex_data[i].type);
			return;
		}

		dump.Write("\n");
#endif

		if (m_vertex_data[i].type < 1 || m_vertex_data[i].type > 7)
		{
			LOG_ERROR(RSX, "GLGSRender::EnableVertexData: Bad vertex data type (%d)!", m_vertex_data[i].type);
		}

		D3D12_INPUT_ELEMENT_DESC IAElement = {};
/*		if (!m_vertex_data[i].addr)
		{
			switch (m_vertex_data[i].type)
			{
			case CELL_GCM_VERTEX_S32K:
			case CELL_GCM_VERTEX_S1:
				switch (m_vertex_data[i].size)
				{
				case 1: glVertexAttrib1s(i, (GLshort&)m_vertex_data[i].data[0]); break;
				case 2: glVertexAttrib2sv(i, (GLshort*)&m_vertex_data[i].data[0]); break;
				case 3: glVertexAttrib3sv(i, (GLshort*)&m_vertex_data[i].data[0]); break;
				case 4: glVertexAttrib4sv(i, (GLshort*)&m_vertex_data[i].data[0]); break;
				}
				break;

			case CELL_GCM_VERTEX_F:
				switch (m_vertex_data[i].size)
				{
				case 1: glVertexAttrib1f(i, (GLfloat&)m_vertex_data[i].data[0]); break;
				case 2: glVertexAttrib2fv(i, (GLfloat*)&m_vertex_data[i].data[0]); break;
				case 3: glVertexAttrib3fv(i, (GLfloat*)&m_vertex_data[i].data[0]); break;
				case 4: glVertexAttrib4fv(i, (GLfloat*)&m_vertex_data[i].data[0]); break;
				}
				break;

			case CELL_GCM_VERTEX_CMP:
			case CELL_GCM_VERTEX_UB:
				glVertexAttrib4ubv(i, (GLubyte*)&m_vertex_data[i].data[0]);
				break;
			}

			checkForGlError("glVertexAttrib");
		}
		else*/
		{
			IAElement.SemanticName = "TEXCOORD";
			IAElement.SemanticIndex = i;
			IAElement.Format = getFormat(m_vertex_data[i].type - 1, m_vertex_data[i].size);

			IAElement.AlignedByteOffset = offset_list[i];
		}
		m_IASet.push_back(IAElement);
	}
}


bool D3D12GSRender::LoadProgram()
{
	if (!m_cur_fragment_prog)
	{
		LOG_WARNING(RSX, "LoadProgram: m_cur_shader_prog == NULL");
		return false;
	}

	m_cur_fragment_prog->ctrl = m_shader_ctrl;

	if (!m_cur_vertex_prog)
	{
		LOG_WARNING(RSX, "LoadProgram: m_cur_vertex_prog == NULL");
		return false;
	}

	m_PSO = m_cachePSO.getGraphicPipelineState(m_device, m_cur_vertex_prog, m_cur_fragment_prog, m_IASet);
	return true;
}

void D3D12GSRender::ExecCMD()
{
	if (m_indexed_array.m_count)
	{
		//		LoadVertexData(m_indexed_array.index_min, m_indexed_array.index_max - m_indexed_array.index_min + 1);
	}

	if (m_indexed_array.m_count || m_draw_array_count)
	{
		EnableVertexData(m_indexed_array.m_count ? true : false);

		//		InitVertexData();
		//		InitFragmentData();
	}

	if (!LoadProgram())
	{
		LOG_ERROR(RSX, "LoadProgram failed.");
		Emu.Pause();
		return;
	}

	InitDrawBuffers();

//	ID3D12CommandList *commandList;
//	m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator, nullptr, IID_PPV_ARGS(&commandList));

/*	if (m_set_color_mask)
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
	}*/

/*	if (m_indexed_array.m_count)
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
	ID3D12GraphicsCommandList *commandList;
	m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator, nullptr, IID_PPV_ARGS(&commandList));
	m_inflightCommandList.push_back(commandList);

	switch (m_surface_color_target)
	{
	case CELL_GCM_SURFACE_TARGET_0:
	case CELL_GCM_SURFACE_TARGET_1:
	case CELL_GCM_SURFACE_TARGET_MRT1:
	case CELL_GCM_SURFACE_TARGET_MRT2:
	case CELL_GCM_SURFACE_TARGET_MRT3:
	{
		D3D12_RESOURCE_BARRIER barriers[2] = {};
		barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barriers[0].Transition.pResource = m_backBuffer[m_swapChain->GetCurrentBackBufferIndex()];
		barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;

		barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barriers[1].Transition.pResource = m_fbo->getRenderTargetTexture(0);
		barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;

		commandList->ResourceBarrier(2, barriers);
		D3D12_TEXTURE_COPY_LOCATION src = {}, dst = {};
		src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX, dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		src.SubresourceIndex = 0, dst.SubresourceIndex = 0;
		src.pResource = m_fbo->getRenderTargetTexture(0), dst.pResource = m_backBuffer[m_swapChain->GetCurrentBackBufferIndex()];
		D3D12_BOX box = { 0, 0, 0, RSXThread::m_width, RSXThread::m_height, 1 };
		commandList->CopyTextureRegion(&dst, 0, 0, 0, &src, &box);

		barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
		barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		commandList->ResourceBarrier(2, barriers);
		commandList->Close();
		m_commandQueueGraphic->ExecuteCommandLists(1, (ID3D12CommandList**)&commandList);
	}
	}

	check(m_swapChain->Present(1, 0));
	// Wait execution is over
	// TODO: It's suboptimal, we should use 2 command allocator
	Microsoft::WRL::ComPtr<ID3D12Fence> fence;
	m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	HANDLE gfxqueuecompletion = CreateEvent(0, 0, 0, 0);
	fence->SetEventOnCompletion(1, gfxqueuecompletion);
	m_commandQueueGraphic->Signal(fence.Get(), 1);
	WaitForSingleObject(gfxqueuecompletion, INFINITE);
	CloseHandle(gfxqueuecompletion);
	m_commandAllocator->Reset();
	for (ID3D12GraphicsCommandList *gfxCommandList : m_inflightCommandList)
		gfxCommandList->Release();
	m_inflightCommandList.clear();
}
#endif