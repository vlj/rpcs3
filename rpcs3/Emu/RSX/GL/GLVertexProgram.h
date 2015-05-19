#pragma once
#include "../Common/VertexProgramDecompiler.h"
#include "Emu/RSX/RSXVertexProgram.h"
#include "Utilities/Thread.h"
#include "OpenGL.h"

struct GLVertexDecompilerThread : public ThreadBase, public VertexProgramDecompiler
{
	std::string &m_shader;
protected:
	virtual std::string getFloatTypeName(size_t elementCount) override;
	virtual std::string getFunction(enum class FUNCTION) override;
	virtual std::string compareFunction(enum class COMPARE, const std::string&, const std::string&) override;

	virtual void insertHeader(std::stringstream &OS);
	virtual void insertInputs(std::stringstream &OS, const std::vector<ParamType> &inputs);
	virtual void insertConstants(std::stringstream &OS, const std::vector<ParamType> &constants);
	virtual void insertOutputs(std::stringstream &OS, const std::vector<ParamType> &outputs);
	virtual void insertMainStart(std::stringstream &OS);
	virtual void insertMainEnd(std::stringstream &OS);
public:
	GLVertexDecompilerThread(std::vector<u32>& data, std::string& shader, ParamArray& parr)
		: ThreadBase("Vertex Shader Decompiler Thread"), VertexProgramDecompiler(data), m_shader(shader)
	{
	}

	virtual void Task();
};

class GLVertexProgram
{ 
public:
	GLVertexProgram();
	~GLVertexProgram();

	ParamArray parr;
	u32 Id;
	std::string shader;

	void Decompile(RSXVertexProgram& prog);
	void DecompileAsync(RSXVertexProgram& prog);
	void Wait();
	void Compile();

private:
	GLVertexDecompilerThread* m_decompiler_thread;
	void Delete();
};
