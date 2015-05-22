#include "stdafx.h"
#if defined(DX12_SUPPORT)
#include "D3D12GSRender.h"
// For clarity this code deals with texture but belongs to D3D12GSRender class

static void check(HRESULT hr)
{
	if (hr != 0)
		abort();
}

size_t D3D12GSRender::UploadTextures()
{
	size_t usedTexture = 0;

	for (u32 i = 0; i < m_textures_count; ++i)
	{
		if (!m_textures[i].IsEnabled()) continue;

		// Upload at each iteration to take advantage of overlapping transfer
		ID3D12GraphicsCommandList *commandList;
		check(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_textureUploadCommandAllocator, nullptr, IID_PPV_ARGS(&commandList)));

		ID3D12Resource *Texture, *vramTexture;
		size_t textureSize = m_textures[i].GetWidth() * m_textures[i].GetHeight() * 4;
		D3D12_RESOURCE_DESC textureDesc = {};
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		textureDesc.Width = textureSize;
		textureDesc.Height = 1;
		textureDesc.DepthOrArraySize = 1;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.MipLevels = 1;
		textureDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		check(m_device->CreatePlacedResource(
			m_uploadTextureHeap,
			m_currentStorageOffset,
			&textureDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&Texture)
			));

		const u32 texaddr = GetAddress(m_textures[i].GetOffset(), m_textures[i].GetLocation());
		auto pixels = vm::get_ptr<const u8>(texaddr);
		void *textureData;
		check(Texture->Map(0, nullptr, (void**)&textureData));
		memcpy(textureData, pixels, textureSize);
		Texture->Unmap(0, nullptr);

		D3D12_RESOURCE_DESC vramTextureDesc = {};
		vramTextureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		vramTextureDesc.Width = m_textures[i].GetWidth();
		vramTextureDesc.Height = m_textures[i].GetHeight();
		vramTextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		vramTextureDesc.DepthOrArraySize = 1;
		vramTextureDesc.SampleDesc.Count = 1;
		vramTextureDesc.MipLevels = 1;
		check(m_device->CreatePlacedResource(
			m_textureStorage,
			m_currentStorageOffset,
			&vramTextureDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&vramTexture)
			));

		m_currentStorageOffset += textureSize;
		m_currentStorageOffset = (m_currentStorageOffset + 65536 - 1) & ~65535;

		D3D12_TEXTURE_COPY_LOCATION dst = {}, src = {};
		dst.pResource = vramTexture;
		dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		src.pResource = Texture;
		src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		src.PlacedFootprint.Footprint.Depth = 1;
		src.PlacedFootprint.Footprint.Width = m_textures[i].GetWidth();
		src.PlacedFootprint.Footprint.Height = m_textures[i].GetHeight();
		src.PlacedFootprint.Footprint.RowPitch = m_textures[i].GetWidth() * 4;
		src.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

		commandList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.pResource = vramTexture;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
		commandList->ResourceBarrier(1, &barrier);

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1, D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_2, D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_3, D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0);
		D3D12_CPU_DESCRIPTOR_HANDLE Handle = m_textureDescriptorsHeap->GetCPUDescriptorHandleForHeapStart();
		Handle.ptr += (m_currentTextureIndex + usedTexture) * m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_device->CreateShaderResourceView(vramTexture, &srvDesc, Handle);

		// TODO : Correctly define sampler
		D3D12_SAMPLER_DESC samplerDesc = {};
		samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		Handle = m_samplerDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		Handle.ptr += (m_currentTextureIndex + usedTexture) * m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_device->CreateSampler(&samplerDesc, Handle);

		commandList->Close();
		m_commandQueueGraphic->ExecuteCommandLists(1, (ID3D12CommandList**)&commandList);

		usedTexture++;
	}

	return usedTexture;
}

#endif