#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/System.h"

#include "GLVertexProgram.h"

std::string GLVertexDecompilerThread::getFloatTypeName(size_t elementCount)
{
	switch (elementCount)
	{
	default:
		abort();
	case 1:
		return "float";
	case 2:
		return "vec2";
	case 3:
		return "vec3";
	case 4:
		return "vec4";
	}
}

std::string GLVertexDecompilerThread::getFunction(FUNCTION f)
{
	switch (f)
	{
	default:
		abort();
	case FUNCTION::FUNCTION_DP2:
		return "vec4(dot($0.xy, $1.xy))";
	case FUNCTION::FUNCTION_DP2A:
		return "";
	case FUNCTION::FUNCTION_DP3:
		return "vec4(dot($0.xyz, $1.xyz))";
	case FUNCTION::FUNCTION_DP4:
		return "vec4(dot($0, $1))";
	case FUNCTION::FUNCTION_SFL:
		return "vec4(0., 0., 0., 0.)";
	case FUNCTION::FUNCTION_STR:
		return "vec4(1., 1., 1., 1.)";
	case FUNCTION::FUNCTION_FRACT:
		return "fract($0)";
	case FUNCTION::FUNCTION_TEXTURE_SAMPLE:
		return "texture($t, $0.xy)";
	case FUNCTION::FUNCTION_DFDX:
		return "dFdx($0)";
	case FUNCTION::FUNCTION_DFDY:
		return "dFdy($0)";
	}
}

/*std::string GLVertexDecompilerThread::BuildCode()
{
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
		{ "gl_Position", false, "dst_reg0", "", false },
		{ "diff_color", true, "dst_reg1", "", false },
		{ "spec_color", true, "dst_reg2", "", false },
		{ "front_diff_color", true, "dst_reg3", "", false },
		{ "front_spec_color", true, "dst_reg4", "", false },
		{ "fogc", true, "dst_reg5", ".x", true },
		{ "gl_ClipDistance[0]", false, "dst_reg5", ".y", false },
		{ "gl_ClipDistance[1]", false, "dst_reg5", ".z", false },
		{ "gl_ClipDistance[2]", false, "dst_reg5", ".w", false },
		{ "gl_PointSize", false, "dst_reg6", ".x", false },
		{ "gl_ClipDistance[3]", false, "dst_reg6", ".y", false },
		{ "gl_ClipDistance[4]", false, "dst_reg6", ".z", false },
		{ "gl_ClipDistance[5]", false, "dst_reg6", ".w", false },
		{ "tc0", true, "dst_reg7", "", false },
		{ "tc1", true, "dst_reg8", "", false },
		{ "tc2", true, "dst_reg9", "", false },
		{ "tc3", true, "dst_reg10", "", false },
		{ "tc4", true, "dst_reg11", "", false },
		{ "tc5", true, "dst_reg12", "", false },
		{ "tc6", true, "dst_reg13", "", false },
		{ "tc7", true, "dst_reg14", "", false },
		{ "tc8", true, "dst_reg15", "", false },
		{ "tc9", true, "dst_reg6", "", false }  // In this line, dst_reg6 is correct since dst_reg goes from 0 to 15.
	};

	std::string f;

	for (auto &i : reg_table)
	{
		if (m_parr.HasParam(PARAM_NONE, "vec4", i.src_reg))
		{
			if (i.need_declare)
			{
				m_parr.AddParam(PARAM_OUT, "vec4", i.name);
			}

			if (i.need_cast)
			{
				f += "\t" + i.name + " = vec4(" + i.src_reg + i.src_reg_mask + ");\n";
			}
			else
			{
				f += "\t" + i.name + " = " + i.src_reg + i.src_reg_mask + ";\n";
			}
		}
	}

	std::string p;

	for (auto& param : m_parr.params) {
		p += param.Format();
	}

	std::string fp;

	for (int i = m_funcs.size() - 1; i > 0; --i)
	{
		fp += fmt::Format("void %s();\n", m_funcs[i].name.c_str());
	}

	f = fmt::Format("void %s()\n{\n\t%s();\n%s\tgl_Position = gl_Position * scaleOffsetMat;\n}\n",
		m_funcs[0].name.c_str(), m_funcs[1].name.c_str(), f.c_str());

	std::string main_body;
	for (uint i = 0, lvl = 1; i < m_instr_count; i++)
	{
		lvl -= m_instructions[i].close_scopes;
		if (lvl < 1) lvl = 1;
		//assert(lvl >= 1);
		for (uint j = 0; j < m_instructions[i].put_close_scopes; ++j)
		{
			--lvl;
			if (lvl < 1) lvl = 1;
			main_body.append(lvl, '\t') += "}\n";
		}

		for (uint j = 0; j < m_instructions[i].do_count; ++j)
		{
			main_body.append(lvl, '\t') += "do\n";
			main_body.append(lvl, '\t') += "{\n";
			lvl++;
		}

		for (uint j = 0; j < m_instructions[i].body.size(); ++j)
		{
			main_body.append(lvl, '\t') += m_instructions[i].body[j] + "\n";
		}

		lvl += m_instructions[i].open_scopes;
	}

	f += fmt::Format("\nvoid %s()\n{\n%s}\n", m_funcs[1].name.c_str(), main_body.c_str());

	for (uint i = 2; i<m_funcs.size(); ++i)
	{
		f += fmt::Format("\nvoid %s()\n{\n%s}\n", m_funcs[i].name.c_str(), BuildFuncBody(m_funcs[i]).c_str());
	}

	static const std::string& prot =
		"#version 420\n"
		"\n"
		"uniform mat4 scaleOffsetMat = mat4(1.0);\n"
		"%s\n"
		"%s\n"
		"%s";

	return fmt::Format(prot.c_str(), p.c_str(), fp.c_str(), f.c_str());
}*/


void GLVertexDecompilerThread::insertHeader(std::stringstream &OS)
{
	OS << "#version 420" << std::endl << std::endl;
	OS << "uniform mat4 scaleOffsetMat = mat4(1.0);" << std::endl;
}

void GLVertexDecompilerThread::insertInputs(std::stringstream & OS, const std::vector<ParamType>& inputs)
{
	for (const ParamType PT : inputs)
	{
		for (const ParamItem &PI : PT.items)
			OS << "layout(location = " << PI.location << ") in " << PT.type << " " << PI.name << ";" << std::endl;
	}
}

void GLVertexDecompilerThread::insertConstants(std::stringstream & OS, const std::vector<ParamType> & constants)
{
	for (const ParamType PT : constants)
	{
		for (const ParamItem &PI : PT.items)
			OS << "uniform " << PT.type << " " << PI.name << ";" << std::endl;
	}
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
	{ "gl_Position", false, "dst_reg0", "", false },
	{ "diff_color", true, "dst_reg1", "", false },
	{ "spec_color", true, "dst_reg2", "", false },
	{ "front_diff_color", true, "dst_reg3", "", false },
	{ "front_spec_color", true, "dst_reg4", "", false },
	{ "fogc", true, "dst_reg5", ".x", true },
	{ "gl_ClipDistance[0]", false, "dst_reg5", ".y", false },
	{ "gl_ClipDistance[1]", false, "dst_reg5", ".z", false },
	{ "gl_ClipDistance[2]", false, "dst_reg5", ".w", false },
	{ "gl_PointSize", false, "dst_reg6", ".x", false },
	{ "gl_ClipDistance[3]", false, "dst_reg6", ".y", false },
	{ "gl_ClipDistance[4]", false, "dst_reg6", ".z", false },
	{ "gl_ClipDistance[5]", false, "dst_reg6", ".w", false },
	{ "tc0", true, "dst_reg7", "", false },
	{ "tc1", true, "dst_reg8", "", false },
	{ "tc2", true, "dst_reg9", "", false },
	{ "tc3", true, "dst_reg10", "", false },
	{ "tc4", true, "dst_reg11", "", false },
	{ "tc5", true, "dst_reg12", "", false },
	{ "tc6", true, "dst_reg13", "", false },
	{ "tc7", true, "dst_reg14", "", false },
	{ "tc8", true, "dst_reg15", "", false },
	{ "tc9", true, "dst_reg6", "", false }  // In this line, dst_reg6 is correct since dst_reg goes from 0 to 15.
};

void GLVertexDecompilerThread::insertOutputs(std::stringstream & OS, const std::vector<ParamType> & outputs)
{
	for (auto &i : reg_table)
	{
		if (m_parr.HasParam(PF_PARAM_NONE, "vec4", i.src_reg) && i.need_declare)
			OS << "out vec4 " << i.name << ";" << std::endl;
	}
}


void GLVertexDecompilerThread::insertMainStart(std::stringstream & OS)
{
	OS << "void main()" << std::endl;
	OS << "{" << std::endl;

	// Declare inside main function
	for (const ParamType PT : m_parr.params[PF_PARAM_NONE])
	{
		for (const ParamItem &PI : PT.items)
		{
			OS << "	" << PT.type << " " << PI.name;
			if (!PI.value.empty())
				OS << " = " << PI.value;
			OS << ";" << std::endl;
		}
	}
}

void GLVertexDecompilerThread::insertMainEnd(std::stringstream & OS)
{
	for (auto &i : reg_table)
	{
		if (m_parr.HasParam(PF_PARAM_NONE, "vec4", i.src_reg))
			OS << "	" << i.name << " = " << i.src_reg << ";" << std::endl;
	}
	OS << "}" << std::endl;
}


void GLVertexDecompilerThread::Task()
{
	m_shader = Decompile();
}

GLVertexProgram::GLVertexProgram()
	: m_decompiler_thread(nullptr)
	, Id(0)
{
}

GLVertexProgram::~GLVertexProgram()
{
	if (m_decompiler_thread)
	{
		Wait();
		if (m_decompiler_thread->IsAlive())
		{
			m_decompiler_thread->Stop();
		}

		delete m_decompiler_thread;
		m_decompiler_thread = nullptr;
	}

	Delete();
}

void GLVertexProgram::Wait()
{
	if (m_decompiler_thread && m_decompiler_thread->IsAlive())
	{
		m_decompiler_thread->Join();
	}
}

void GLVertexProgram::Decompile(RSXVertexProgram& prog)
{
	GLVertexDecompilerThread decompiler(prog.data, shader, parr);
	decompiler.Task();
}

void GLVertexProgram::DecompileAsync(RSXVertexProgram& prog)
{
	if (m_decompiler_thread)
	{
		Wait();
		if (m_decompiler_thread->IsAlive())
		{
			m_decompiler_thread->Stop();
		}

		delete m_decompiler_thread;
		m_decompiler_thread = nullptr;
	}

	m_decompiler_thread = new GLVertexDecompilerThread(prog.data, shader, parr);
	m_decompiler_thread->Start();
}

void GLVertexProgram::Compile()
{
	if (Id)
	{
		glDeleteShader(Id);
	}

	Id = glCreateShader(GL_VERTEX_SHADER);

	const char* str = shader.c_str();
	const int strlen = shader.length();

	glShaderSource(Id, 1, &str, &strlen);
	glCompileShader(Id);

	GLint r = GL_FALSE;
	glGetShaderiv(Id, GL_COMPILE_STATUS, &r);
	if (r != GL_TRUE)
	{
		glGetShaderiv(Id, GL_INFO_LOG_LENGTH, &r);

		if (r)
		{
			char* buf = new char[r + 1]();
			GLsizei len;
			glGetShaderInfoLog(Id, r, &len, buf);
			LOG_ERROR(RSX, "Failed to compile vertex shader: %s", buf);
			delete[] buf;
		}

		LOG_NOTICE(RSX, "%s", shader.c_str());
		Emu.Pause();
	}
	//else LOG_WARNING(RSX, "Vertex shader compiled successfully!");

}

void GLVertexProgram::Delete()
{
	shader.clear();

	if (Id)
	{
		if (Emu.IsStopped())
		{
			LOG_WARNING(RSX, "GLVertexProgram::Delete(): glDeleteShader(%d) avoided", Id);
		}
		else
		{
			glDeleteShader(Id);
		}
		Id = 0;
	}
}
