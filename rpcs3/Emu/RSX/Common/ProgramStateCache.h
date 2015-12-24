#pragma once

#include "Emu/RSX/RSXFragmentProgram.h"
#include "Emu/RSX/RSXVertexProgram.h"


enum class SHADER_TYPE
{
	SHADER_TYPE_VERTEX,
	SHADER_TYPE_FRAGMENT
};

namespace ProgramHashUtil
{
	// Based on
	// https://github.com/AlexAltea/nucleus/blob/master/nucleus/gpu/rsx_pgraph.cpp
	union qword
	{
		u64 dword[2];
		u32 word[4];
	};

	struct HashVertexProgram
	{
		size_t operator()(const RSXVertexProgram &program) const
		{
			// 64-bit Fowler/Noll/Vo FNV-1a hash code
			size_t hash = 0xCBF29CE484222325ULL;
			const qword *instbuffer = (const qword*)program.data.data();
			size_t instIndex = 0;
			bool end = false;
			for (unsigned i = 0; i < program.data.size() / 4; i++)
			{
				const qword inst = instbuffer[instIndex];
				hash ^= inst.dword[0];
				hash += (hash << 1) + (hash << 4) + (hash << 5) + (hash << 7) + (hash << 8) + (hash << 40);
				hash ^= inst.dword[1];
				hash += (hash << 1) + (hash << 4) + (hash << 5) + (hash << 7) + (hash << 8) + (hash << 40);
				instIndex++;
			}
			return hash;
		}
	};


	struct VertexProgramCompare
	{
		bool operator()(const RSXVertexProgram &binary1, const RSXVertexProgram &binary2) const
		{
			if (binary1.data.size() != binary2.data.size() || binary1.output_mask != binary2.output_mask || binary1.input_mask != binary2.input_mask) return false;
			const qword *instBuffer1 = (const qword*)binary1.data.data();
			const qword *instBuffer2 = (const qword*)binary2.data.data();
			size_t instIndex = 0;
			for (unsigned i = 0; i < binary1.data.size() / 4; i++)
			{
				const qword& inst1 = instBuffer1[instIndex];
				const qword& inst2 = instBuffer2[instIndex];
				if (inst1.dword[0] != inst2.dword[0] || inst1.dword[1] != inst2.dword[1])
					return false;
				instIndex++;
			}
			return true;
		}
	};

	struct FragmentProgramUtil
	{
		struct key_type
		{
			void *ptr;
			u32 transform_program_outputs;
		};

		/**
		* returns true if the given source Operand is a constant
		*/
		static bool isConstant(u32 sourceOperand)
		{
			return ((sourceOperand >> 8) & 0x3) == 2;
		}

		static
		size_t getFPBinarySize(void *ptr)
		{
			const qword *instBuffer = (const qword*)ptr;
			size_t instIndex = 0;
			while (true)
			{
				const qword& inst = instBuffer[instIndex];
				bool isSRC0Constant = isConstant(inst.word[1]);
				bool isSRC1Constant = isConstant(inst.word[2]);
				bool isSRC2Constant = isConstant(inst.word[3]);
				bool end = (inst.word[0] >> 8) & 0x1;

				if (isSRC0Constant || isSRC1Constant || isSRC2Constant)
				{
					instIndex += 2;
					if (end)
						return instIndex * 4 * 4;
					continue;
				}
				instIndex++;
				if (end)
					return (instIndex)* 4 * 4;
			}
		}
	};

	struct HashFragmentProgram
	{
		size_t operator()(const FragmentProgramUtil::key_type &program) const
		{
			// 64-bit Fowler/Noll/Vo FNV-1a hash code
			size_t hash = 0xCBF29CE484222325ULL;
			const qword *instbuffer = (const qword*)program.ptr;
			size_t instIndex = 0;
			while (true)
			{
				const qword& inst = instbuffer[instIndex];
				hash ^= inst.dword[0];
				hash += (hash << 1) + (hash << 4) + (hash << 5) + (hash << 7) + (hash << 8) + (hash << 40);
				hash ^= inst.dword[1];
				hash += (hash << 1) + (hash << 4) + (hash << 5) + (hash << 7) + (hash << 8) + (hash << 40);
				instIndex++;
				// Skip constants
				if (FragmentProgramUtil::isConstant(inst.word[1]) ||
					FragmentProgramUtil::isConstant(inst.word[2]) ||
					FragmentProgramUtil::isConstant(inst.word[3]))
					instIndex++;

				bool end = (inst.word[0] >> 8) & 0x1;
				if (end)
					return hash;
			}
			return 0;
		}
	};

	struct FragmentProgramCompare
	{
		bool operator()(const FragmentProgramUtil::key_type &binary1, const FragmentProgramUtil::key_type &binary2) const
		{
			if (binary1.transform_program_outputs != binary2.transform_program_outputs) return false;
			const qword *instBuffer1 = (const qword*)binary1.ptr;
			const qword *instBuffer2 = (const qword*)binary2.ptr;
			size_t instIndex = 0;
			while (true)
			{
				const qword& inst1 = instBuffer1[instIndex];
				const qword& inst2 = instBuffer2[instIndex];

				if (inst1.dword[0] != inst2.dword[0] || inst1.dword[1] != inst2.dword[1])
					return false;
				instIndex++;
				// Skip constants
				if (FragmentProgramUtil::isConstant(inst1.word[1]) ||
					FragmentProgramUtil::isConstant(inst1.word[2]) ||
					FragmentProgramUtil::isConstant(inst1.word[3]))
					instIndex++;

				bool end = ((inst1.word[0] >> 8) & 0x1) && ((inst2.word[0] >> 8) & 0x1);
				if (end)
					return true;
			}
		}
	};
}


/**
* Cache for program help structure (blob, string...)
* The class is responsible for creating the object so the state only has to call getGraphicPipelineState
* Template argument is a struct which has the following type declaration :
* - a typedef VertexProgramData to a type that encapsulate vertex program info. It should provide an Id member.
* - a typedef FragmentProgramData to a types that encapsulate fragment program info. It should provide an Id member and a fragment constant offset vector.
* - a typedef PipelineData encapsulating monolithic program.
* - a typedef PipelineProperties to a type that encapsulate various state info relevant to program compilation (alpha test, primitive type,...)
* - a	typedef ExtraData type that will be passed to the buildProgram function.
* It should also contains the following function member :
* - static void RecompileFragmentProgram(RSXFragmentProgram *RSXFP, FragmentProgramData& fragmentProgramData, size_t ID);
* - static void RecompileVertexProgram(RSXVertexProgram *RSXVP, VertexProgramData& vertexProgramData, size_t ID);
* - static PipelineData *BuildProgram(VertexProgramData &vertexProgramData, FragmentProgramData &fragmentProgramData, const PipelineProperties &pipelineProperties, const ExtraData& extraData);
* - void DeleteProgram(PipelineData *ptr);
*/
template<typename BackendTraits>
class ProgramStateCache
{
private:

	typedef std::unordered_map<RSXVertexProgram, typename BackendTraits::VertexProgramData, ProgramHashUtil::HashVertexProgram, ProgramHashUtil::VertexProgramCompare> binary2VS;
	typedef std::unordered_map<ProgramHashUtil::FragmentProgramUtil::key_type, typename BackendTraits::FragmentProgramData, ProgramHashUtil::HashFragmentProgram, ProgramHashUtil::FragmentProgramCompare> binary2FS;
	binary2VS m_cacheVS;
	binary2FS m_cacheFS;

	size_t m_currentShaderId;
	std::vector<size_t> dummyFragmentConstantCache;

	struct PSOKey
	{
		u32 vpIdx;
		u32 fpIdx;
		typename BackendTraits::PipelineProperties properties;
	};

	struct PSOKeyHash
	{
		size_t operator()(const PSOKey &key) const
		{
			size_t hashValue = 0;
			hashValue ^= std::hash<unsigned>()(key.vpIdx);
			hashValue ^= std::hash<unsigned>()(key.fpIdx);
			hashValue ^= std::hash<typename BackendTraits::PipelineProperties>()(key.properties);
			return hashValue;
		}
	};

	struct PSOKeyCompare
	{
		size_t operator()(const PSOKey &key1, const PSOKey &key2) const
		{
			return (key1.vpIdx == key2.vpIdx) && (key1.fpIdx == key2.fpIdx) && (key1.properties == key2.properties);
		}
	};

	std::unordered_map<PSOKey, typename BackendTraits::PipelineData*, PSOKeyHash, PSOKeyCompare> m_cachePSO;

	typename BackendTraits::FragmentProgramData& SearchFp(RSXFragmentProgram* rsx_fp, bool& found)
	{
		ProgramHashUtil::FragmentProgramUtil::key_type key = { vm::base(rsx_fp->addr), rsx_fp->transform_program_outputs };
		typename binary2FS::iterator It = m_cacheFS.find(key);
		if (It != m_cacheFS.end())
		{
			found = true;
			return  It->second;
		}
		found = false;
		LOG_WARNING(RSX, "FP not found in buffer!");
		size_t actualFPSize = ProgramHashUtil::FragmentProgramUtil::getFPBinarySize(vm::base(rsx_fp->addr));
		void *fpShadowCopy = malloc(actualFPSize);
		std::memcpy(fpShadowCopy, vm::base(rsx_fp->addr), actualFPSize);
		ProgramHashUtil::FragmentProgramUtil::key_type new_fp_key;
		new_fp_key.ptr = fpShadowCopy;
		new_fp_key.transform_program_outputs = rsx_fp->transform_program_outputs;
		typename BackendTraits::FragmentProgramData &newShader = m_cacheFS[new_fp_key];
		BackendTraits::RecompileFragmentProgram(rsx_fp, newShader, m_currentShaderId++);

		return newShader;
	}

	typename BackendTraits::VertexProgramData& SearchVp(RSXVertexProgram* rsx_vp, bool &found)
	{
		typename binary2VS::iterator It = m_cacheVS.find(*rsx_vp);
		if (It != m_cacheVS.end())
		{
			found = true;
			return It->second;
		}
		found = false;
		LOG_WARNING(RSX, "VP not found in buffer!");
		typename BackendTraits::VertexProgramData& newShader = m_cacheVS[*rsx_vp];
		BackendTraits::RecompileVertexProgram(rsx_vp, newShader, m_currentShaderId++);

		return newShader;
	}

	typename BackendTraits::PipelineData *GetProg(const PSOKey &psoKey) const
	{
		typename std::unordered_map<PSOKey, typename BackendTraits::PipelineData *, PSOKeyHash, PSOKeyCompare>::const_iterator It = m_cachePSO.find(psoKey);
		if (It == m_cachePSO.end())
			return nullptr;
		return It->second;
	}

	void Add(typename BackendTraits::PipelineData *prog, const PSOKey& PSOKey)
	{
		m_cachePSO.insert(std::make_pair(PSOKey, prog));
	}

public:
	ProgramStateCache() : m_currentShaderId(0) {}
	~ProgramStateCache()
	{
		clear();
	}

	const typename BackendTraits::VertexProgramData* get_transform_program(const RSXVertexProgram& rsx_vp) const
	{
		typename binary2VS::const_iterator It = m_cacheVS.find(rsx_vp);
		if (It == m_cacheVS.end())
			return nullptr;
		return &It->second;
	}

	const typename BackendTraits::FragmentProgramData* get_shader_program(const RSXFragmentProgram& rsx_fp) const
	{
		ProgramHashUtil::FragmentProgramUtil::key_type key = { vm::base(rsx_fp.addr), rsx_fp.transform_program_outputs };
		typename binary2FS::const_iterator It = m_cacheFS.find(key);
		if (It == m_cacheFS.end())
			return nullptr;
		return &It->second;
	}

	void clear()
	{
		for (auto pair : m_cachePSO)
			BackendTraits::DeleteProgram(pair.second);
		m_cachePSO.clear();

		for (auto pair : m_cacheFS)
			free(pair.first.ptr);

		m_cacheFS.clear();
	}

	typename BackendTraits::PipelineData *getGraphicPipelineState(
		RSXVertexProgram *vertexShader,
		RSXFragmentProgram *fragmentShader,
		const typename BackendTraits::PipelineProperties &pipelineProperties,
		const typename BackendTraits::ExtraData& extraData
		)
	{
		typename BackendTraits::PipelineData *result = nullptr;
		bool fpFound, vpFound;
		typename BackendTraits::VertexProgramData &vertexProg = SearchVp(vertexShader, vpFound);
		typename BackendTraits::FragmentProgramData &fragmentProg = SearchFp(fragmentShader, fpFound);

		if (fpFound && vpFound)
		{
			result = GetProg({ vertexProg.id, fragmentProg.id, pipelineProperties });
		}

		if (result != nullptr)
			return result;
		else
		{
			LOG_WARNING(RSX, "Add program :");
			LOG_WARNING(RSX, "*** vp id = %d", vertexProg.id);
			LOG_WARNING(RSX, "*** fp id = %d", fragmentProg.id);

			result = BackendTraits::BuildProgram(vertexProg, fragmentProg, pipelineProperties, extraData);
			Add(result, { vertexProg.id, fragmentProg.id, pipelineProperties });
		}
		return result;
	}

	size_t get_fragment_constants_buffer_size(const RSXFragmentProgram *fragmentShader) const
	{
		ProgramHashUtil::FragmentProgramUtil::key_type key = { vm::base(fragmentShader->addr), fragmentShader->transform_program_outputs };
		typename binary2FS::const_iterator It = m_cacheFS.find(key);
		if (It != m_cacheFS.end())
			return It->second.FragmentConstantOffsetCache.size() * 4 * sizeof(float);
		LOG_ERROR(RSX, "Can't retrieve constant offset cache");
		return 0;
	}

	void fill_fragment_constans_buffer(void *buffer, const RSXFragmentProgram *fragment_program) const
	{
		ProgramHashUtil::FragmentProgramUtil::key_type key = { vm::base(fragment_program->addr), fragment_program->transform_program_outputs };
		typename binary2FS::const_iterator It = m_cacheFS.find(key);
		if (It == m_cacheFS.end())
			return;
		__m128i mask = _mm_set_epi8(0xE, 0xF, 0xC, 0xD,
			0xA, 0xB, 0x8, 0x9,
			0x6, 0x7, 0x4, 0x5,
			0x2, 0x3, 0x0, 0x1);

		size_t offset = 0;
		for (size_t offset_in_fragment_program : It->second.FragmentConstantOffsetCache)
		{
			void *data = vm::base(fragment_program->addr + (u32)offset_in_fragment_program);
			const __m128i &vector = _mm_loadu_si128((__m128i*)data);
			const __m128i &shuffled_vector = _mm_shuffle_epi8(vector, mask);
			_mm_stream_si128((__m128i*)((char*)buffer + offset), shuffled_vector);
			offset += 4 * sizeof(u32);
		}
	}
};
