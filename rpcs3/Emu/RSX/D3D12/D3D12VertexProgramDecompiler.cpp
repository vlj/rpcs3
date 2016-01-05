#include "stdafx.h"
#include "stdafx_d3d12.h"
#ifdef _MSC_VER
#include "D3D12VertexProgramDecompiler.h"
#include "D3D12CommonDecompiler.h"
#include "Emu/System.h"


std::string D3D12VertexProgramDecompiler::getFloatTypeName(size_t elementCount)
{
	return getFloatTypeNameImp(elementCount);
}

std::string D3D12VertexProgramDecompiler::getIntTypeName(size_t elementCount)
{
	return "int4";
}

std::string D3D12VertexProgramDecompiler::getFunction(enum class FUNCTION f)
{
	return getFunctionImp(f);
}

std::string D3D12VertexProgramDecompiler::compareFunction(COMPARE f, const std::string &Op0, const std::string &Op1)
{
	return compareFunctionImp(f, Op0, Op1);
}

void D3D12VertexProgramDecompiler::insertHeader(std::stringstream &OS)
{
	OS << "cbuffer SCALE_OFFSET : register(b0)" << std::endl;
	OS << "{" << std::endl;
	OS << "	float4x4 scaleOffsetMat;" << std::endl;
	OS << "	float4 nv308a_color;" << std::endl;
	OS << "	int isAlphaTested;" << std::endl;
	OS << "	float alphaRef;" << std::endl;
	OS << "	int tex0_is_unorm;" << std::endl;
	OS << "	int tex1_is_unorm;" << std::endl;
	OS << "	int tex2_is_unorm;" << std::endl;
	OS << "	int tex3_is_unorm;" << std::endl;
	OS << "	int tex4_is_unorm;" << std::endl;
	OS << "	int tex5_is_unorm;" << std::endl;
	OS << "	int tex6_is_unorm;" << std::endl;
	OS << "	int tex7_is_unorm;" << std::endl;
	OS << "	int tex8_is_unorm;" << std::endl;
	OS << "	int tex9_is_unorm;" << std::endl;
	OS << "	int tex10_is_unorm;" << std::endl;
	OS << "	int tex11_is_unorm;" << std::endl;
	OS << "	int tex12_is_unorm;" << std::endl;
	OS << "	int tex13_is_unorm;" << std::endl;
	OS << "	int tex14_is_unorm;" << std::endl;
	OS << "	int tex15_is_unorm;" << std::endl;
	OS << "};" << std::endl;
}

void D3D12VertexProgramDecompiler::insertInputs(std::stringstream & OS, const std::vector<ParamType>& inputs)
{
	OS << "struct VertexInput" << std::endl;
	OS << "{" << std::endl;
	for (const ParamType PT : inputs)
	{
		for (const ParamItem &PI : PT.items)
		{
			OS << "	" << PT.type << " " << PI.name << ": TEXCOORD" << PI.location << ";" << std::endl;
			input_slots.push_back(PI.location);
		}
	}
	OS << "};" << std::endl;

}

void D3D12VertexProgramDecompiler::insertConstants(std::stringstream & OS, const std::vector<ParamType> & constants)
{
	OS << "cbuffer CONSTANT_BUFFER : register(b1)" << std::endl;
	OS << "{" << std::endl;
	for (const ParamType PT : constants)
	{
		for (const ParamItem &PI : PT.items)
			OS << "	" << PT.type << " " << PI.name << ";" << std::endl;
	}
	OS << "};" << std::endl;
}

void D3D12VertexProgramDecompiler::insertOutputs(std::stringstream & OS, const std::vector<ParamType> & outputs)
{
	OS << "struct PixelInput" << std::endl;
	OS << "{" << std::endl;
	OS << "	float4 dst_reg0 : SV_POSITION;" << std::endl;
	OS << "	float4 dst_reg1 : COLOR0;" << std::endl;
	OS << "	float4 dst_reg2 : COLOR1;" << std::endl;
	OS << "	float4 dst_reg3 : COLOR2;" << std::endl;
	OS << "	float4 dst_reg4 : COLOR3;" << std::endl;
	OS << "	float dst_reg5 : FOG;" << std::endl;
	OS << "	float4 dst_reg6 : TEXCOORD9;" << std::endl;
	OS << "	float4 dst_reg7 : TEXCOORD0;" << std::endl;
	OS << "	float4 dst_reg8 : TEXCOORD1;" << std::endl;
	OS << "	float4 dst_reg9 : TEXCOORD2;" << std::endl;
	OS << "	float4 dst_reg10 : TEXCOORD3;" << std::endl;
	OS << "	float4 dst_reg11 : TEXCOORD4;" << std::endl;
	OS << "	float4 dst_reg12 : TEXCOORD5;" << std::endl;
	OS << "	float4 dst_reg13 : TEXCOORD6;" << std::endl;
	OS << "	float4 dst_reg14 : TEXCOORD7;" << std::endl;
	OS << "	float4 dst_reg15 : TEXCOORD8;" << std::endl;
	OS << "};" << std::endl;
}

struct reg_info
{
	std::string name;
	bool need_declare;
	std::string src_reg;
	std::string src_reg_mask;
	bool need_cast;
};

static const reg_info reg_table[] =
{
	{ "diff_color", true, "dst_reg1", "", false },
	{ "spec_color", true, "dst_reg2", "", false },
	{ "front_diff_color", true, "dst_reg3", "", false },
	{ "front_spec_color", true, "dst_reg4", "", false },
	{ "fogc", true, "dst_reg5", ".x", true },
	{ "gl_PointSize", false, "dst_reg6", ".x", false },
	// TODO: Handle user clip distance properly
	{ "gl_ClipDistance[0]", false, "dst_reg5", ".y", false },
	{ "gl_ClipDistance[1]", false, "dst_reg5", ".z", false },
	{ "gl_ClipDistance[2]", false, "dst_reg5", ".w", false },
	{ "gl_ClipDistance[3]", false, "dst_reg6", ".y", false },
	{ "gl_ClipDistance[4]", false, "dst_reg6", ".z", false },
	{ "gl_ClipDistance[5]", false, "dst_reg6", ".w", false },
	{ "tc8", true, "dst_reg15", "", false },
	{ "tc9", false, "dst_reg6", "", false },
	{ "tc0", true, "dst_reg7", "", false },
	{ "tc1", true, "dst_reg8", "", false },
	{ "tc2", true, "dst_reg9", "", false },
	{ "tc3", true, "dst_reg10", "", false },
	{ "tc4", true, "dst_reg11", "", false },
	{ "tc5", true, "dst_reg12", "", false },
	{ "tc6", true, "dst_reg13", "", false },
	{ "tc7", true, "dst_reg14", "", false },
};

void D3D12VertexProgramDecompiler::insertMainStart(std::stringstream & OS)
{
	OS << "PixelInput main(VertexInput In)" << std::endl;
	OS << "{" << std::endl;

	// Declare inside main function
	for (const ParamType PT : m_parr.params[PF_PARAM_NONE])
	{
		for (const ParamItem &PI : PT.items)
		{
			OS << "	" << PT.type << " " << PI.name;
			if (!PI.value.empty())
				OS << " = " << PI.value;
			else
				OS << " = " << "float4(0., 0., 0., 0.);";
			OS << ";" << std::endl;
		}
	}

	for (const ParamType PT : m_parr.params[PF_PARAM_IN])
	{
		for (const ParamItem &PI : PT.items)
			OS << "	" << PT.type << " " << PI.name << " = In." << PI.name << ";" << std::endl;
	}
}


void D3D12VertexProgramDecompiler::insertMainEnd(std::stringstream & OS)
{
	OS << "	PixelInput Out = (PixelInput)0;" << std::endl;

	// There is always an output position
	OS << "	Out.dst_reg0 = dst_reg0;" << std::endl;

	for (unsigned i = 0; i < 22; i++)
	{
		if (!(m_output_mask & (1 << i)))
		{
			if (i == 0)
				OS << "	Out." << reg_table[i].src_reg << " =  float4(1., 1., 1., 1.);" << std::endl;
			else
				OS << "	Out." << reg_table[i].src_reg << " =  float4(0., 0., 0., 1.);" << std::endl;
			continue;
		}
		if (!m_parr.HasParam(PF_PARAM_NONE, "float4", reg_table[i].src_reg))
			continue;
			OS << "	Out." << reg_table[i].src_reg << " = " << reg_table[i].src_reg << ";" << std::endl;
	}
	OS << "	Out.dst_reg0 = mul(Out.dst_reg0, scaleOffsetMat);" << std::endl;
	OS << "	return Out;" << std::endl;
	OS << "}" << std::endl;
}

D3D12VertexProgramDecompiler::D3D12VertexProgramDecompiler(const RSXVertexProgram &prog) :
	VertexProgramDecompiler(prog)
{
}
#endif
