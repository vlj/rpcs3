#pragma once

#include "Emu/RSX/RSXFragmentProgram.h"
#include "Emu/RSX/RSXVertexProgram.h"
#include "Utilities/Log.h"


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
		size_t operator()(const void *program) const
		{
			// 64-bit Fowler/Noll/Vo FNV-1a hash code
			size_t hash = 0xCBF29CE484222325ULL;
			const qword *instbuffer = (const qword*)program;
			size_t instIndex = 0;
			bool end = false;
			while (true)
			{
				const qword inst = instbuffer[instIndex];
				bool end = inst.word[0] >> 31;
				if (end)
					return hash;
				hash ^= inst.dword[0];
				hash += (hash << 1) + (hash << 4) + (hash << 5) + (hash << 7) + (hash << 8) + (hash << 40);
				hash ^= inst.dword[1];
				hash += (hash << 1) + (hash << 4) + (hash << 5) + (hash << 7) + (hash << 8) + (hash << 40);
				instIndex++;
			}
			return 0;
		}
	};


	struct VertexProgramCompare
	{
		bool operator()(const void *binary1, const void *binary2) const
		{
			const qword *instBuffer1 = (const qword*)binary1;
			const qword *instBuffer2 = (const qword*)binary2;
			size_t instIndex = 0;
			while (true)
			{
				const qword& inst1 = instBuffer1[instIndex];
				const qword& inst2 = instBuffer2[instIndex];
				bool end = (inst1.word[0] >> 31) && (inst2.word[0] >> 31);
				if (end)
					return true;
				if (inst1.dword[0] != inst2.dword[0] || inst1.dword[1] != inst2.dword[1])
					return false;
				instIndex++;
			}
		}
	};

	struct FragmentProgramUtil
	{
		/**
		* returns true if the given source Operand is a constant
		*/
		static bool isConstant(u32 sourceOperand)
		{
			return ((sourceOperand >> 8) & 0x3) == 2;
		}

		/**
		* RSX fragment program constants are inlined inside shader code.
		* This function takes an instruction from a fragment program and
		* returns an equivalent instruction where inlined constants
		* are masked.
		* This allows to hash/compare fragment programs even if their
		* inlined constants are modified inbetween
		*/
		static qword fragmentMaskConstant(const qword &initialQword)
		{
			qword result = initialQword;
			if (isConstant(initialQword.word[1]))
				result.word[1] = 0;
			if (isConstant(initialQword.word[2]))
				result.word[2] = 0;
			if (isConstant(initialQword.word[3]))
				result.word[3] = 0;
			return result;
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
		size_t operator()(const void *program) const
		{
			// 64-bit Fowler/Noll/Vo FNV-1a hash code
			size_t hash = 0xCBF29CE484222325ULL;
			const qword *instbuffer = (const qword*)program;
			size_t instIndex = 0;
			while (true)
			{
				const qword& inst = instbuffer[instIndex];
				bool end = (inst.word[0] >> 8) & 0x1;
				if (end)
					return hash;
				const qword& maskedInst = FragmentProgramUtil::fragmentMaskConstant(inst);
				hash ^= maskedInst.dword[0];
				hash += (hash << 1) + (hash << 4) + (hash << 5) + (hash << 7) + (hash << 8) + (hash << 40);
				hash ^= maskedInst.dword[1];
				hash += (hash << 1) + (hash << 4) + (hash << 5) + (hash << 7) + (hash << 8) + (hash << 40);
				instIndex++;
				// Skip constants
				if (FragmentProgramUtil::isConstant(inst.word[1]) ||
					FragmentProgramUtil::isConstant(inst.word[2]) ||
					FragmentProgramUtil::isConstant(inst.word[3]))
					instIndex++;
			}
			return 0;
		}
	};

	struct FragmentProgramCompare
	{
		bool operator()(const void *binary1, const void *binary2) const
		{
			const qword *instBuffer1 = (const qword*)binary1;
			const qword *instBuffer2 = (const qword*)binary2;
			size_t instIndex = 0;
			while (true)
			{
				const qword& inst1 = instBuffer1[instIndex];
				const qword& inst2 = instBuffer2[instIndex];
				bool end = ((inst1.word[0] >> 8) & 0x1) && ((inst2.word[0] >> 8) & 0x1);
				if (end)
					return true;

				const qword& maskedInst1 = FragmentProgramUtil::fragmentMaskConstant(inst1);
				const qword& maskedInst2 = FragmentProgramUtil::fragmentMaskConstant(inst2);

				if (maskedInst1.dword[0] != maskedInst2.dword[0] || maskedInst1.dword[1] != maskedInst2.dword[1])
					return false;
				instIndex++;
				// Skip constants
				if (FragmentProgramUtil::isConstant(inst1.word[1]) ||
					FragmentProgramUtil::isConstant(inst1.word[2]) ||
					FragmentProgramUtil::isConstant(inst1.word[3]))
					instIndex++;
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
	typedef std::unordered_map<void *, typename BackendTraits::VertexProgramData, ProgramHashUtil::HashVertexProgram, ProgramHashUtil::VertexProgramCompare> binary2VS;
	typedef std::unordered_map<void *, typename BackendTraits::FragmentProgramData, ProgramHashUtil::HashFragmentProgram, ProgramHashUtil::FragmentProgramCompare> binary2FS;
	binary2VS m_cacheVS;
	binary2FS m_cacheFS;

	size_t m_currentShaderId;

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
		binary2FS::iterator It = m_cacheFS.find(vm::get_ptr<void>(rsx_fp->addr));
		if (It != m_cacheFS.end())
		{
			found = true;
			return  It->second;
		}
		found = false;
		LOG_WARNING(RSX, "FP not found in buffer!");
		size_t actualFPSize = ProgramHashUtil::FragmentProgramUtil::getFPBinarySize(vm::get_ptr<u8>(rsx_fp->addr));
		void *fpShadowCopy = malloc(actualFPSize);
		memcpy(fpShadowCopy, vm::get_ptr<u8>(rsx_fp->addr), actualFPSize);
		typename BackendTraits::FragmentProgramData &newShader = m_cacheFS[fpShadowCopy];
		BackendTraits::RecompileFragmentProgram(rsx_fp, newShader, m_currentShaderId++);

		return newShader;
	}

	typename BackendTraits::VertexProgramData& SearchVp(RSXVertexProgram* rsx_vp, bool &found)
	{
		binary2VS::iterator It = m_cacheVS.find((void*)rsx_vp->data.data());
		if (It != m_cacheVS.end())
		{
			found = true;
			return It->second;
		}
		found = false;
		LOG_WARNING(RSX, "VP not found in buffer!");
		size_t actualVPSize = rsx_vp->data.size() * 4;
		void *vpShadowCopy = malloc(actualVPSize);
		memcpy(vpShadowCopy, rsx_vp->data.data(), actualVPSize);
		typename BackendTraits::VertexProgramData& newShader = m_cacheVS[vpShadowCopy];
		BackendTraits::RecompileVertexProgram(rsx_vp, newShader, m_currentShaderId++);

		return newShader;
	}

	typename BackendTraits::PipelineData *GetProg(const PSOKey &psoKey) const
	{
		std::unordered_map<PSOKey, typename BackendTraits::PipelineData *, PSOKeyHash, PSOKeyCompare>::const_iterator It = m_cachePSO.find(psoKey);
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
		for (auto pair : m_cachePSO)
			BackendTraits::DeleteProgram(pair.second);
		for (auto pair : m_cacheVS)
			free(pair.first);
		for (auto pair : m_cacheFS)
			free(pair.first);
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
			result = GetProg({ vertexProg.Id, fragmentProg.Id, pipelineProperties });
		}

		if (result != nullptr)
			return result;
		else
		{
			LOG_WARNING(RSX, "Add program :");
			LOG_WARNING(RSX, "*** vp id = %d", vertexProg.Id);
			LOG_WARNING(RSX, "*** fp id = %d", fragmentProg.Id);

			result = BackendTraits::BuildProgram(vertexProg, fragmentProg, pipelineProperties, extraData);
			Add(result, { vertexProg.Id, fragmentProg.Id, pipelineProperties });
		}
		return result;
	}

	const std::vector<size_t> &getFragmentConstantOffsetsCache(const RSXFragmentProgram *fragmentShader) const
	{
		binary2FS::const_iterator It = m_cacheFS.find(vm::get_ptr<void>(fragmentShader->addr));
		if (It != m_cacheFS.end())
			return It->second.FragmentConstantOffsetCache;
		LOG_ERROR(RSX, "Can't retrieve constant offset cache");
		return std::vector<size_t>();
	}
};