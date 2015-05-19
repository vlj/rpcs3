#pragma once
#include "ShaderParam.h"
#include "Emu/RSX/RSXFragmentProgram.h"
#include <sstream>

enum class FUNCTION {
	FUNCTION_SATURATE,
	FUNCTION_DP2,
	FUNCTION_DP2A,
	FUNCTION_DP3,
	FUNCTION_DP4,
	FUNCTION_SEQ,
	FUNCTION_SFL, // Set zero
	FUNCTION_SGE,
	FUNCTION_SGT,
	FUNCTION_SLE,
	FUNCTION_SLT,
	FUNCTION_SNE,
	FUNCTION_STR, // Set One
	FUNCTION_FRACT,
};

class FragmentProgramDecompiler
{
	std::string main;
	u32 m_addr;
	u32& m_size;
	u32 m_const_index;
	u32 m_offset;
	u32 m_location;
	u32 m_ctrl;
	u32 m_loop_count;
	int m_code_level;
	std::vector<u32> m_end_offsets;
	std::vector<u32> m_else_offsets;

	std::string GetMask();

	void SetDst(std::string code, bool append_mask = true);
	void AddCode(const std::string& code);
	std::string AddReg(u32 index, int fp16);
	bool HasReg(u32 index, int fp16);
	std::string AddCond();
	std::string AddConst();
	std::string AddTex();
	std::string Format(const std::string& code);

	void AddCodeCond(const std::string& dst, const std::string& src);
	std::string GetCond();
	template<typename T> std::string GetSRC(T src);
	std::string BuildCode();

	u32 GetData(const u32 d) const { return d << 16 | d >> 16; }
protected:
	virtual std::string getFloatTypeName(size_t elementCount) = 0;
	virtual std::string getFunction(enum class FUNCTION) = 0;

	virtual void insertHeader(std::stringstream &OS) = 0;
	virtual void insertIntputs(std::stringstream &OS) = 0;
	virtual void insertOutputs(std::stringstream &OS) = 0;
	virtual void insertConstants(std::stringstream &OS) = 0;
	virtual void insertMainStart(std::stringstream &OS) = 0;
	virtual void insertMainEnd(std::stringstream &OS) = 0;
public:
	ParamArray m_parr;
	FragmentProgramDecompiler() = delete;
	FragmentProgramDecompiler(u32 addr, u32& size, u32 ctrl);
	std::string Decompile();
};
