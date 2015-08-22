#include "stdafx.h"
#ifdef LLVM_AVAILABLE
#include "Utilities/Log.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUDisAsm.h"
#include "Emu/Cell/PPULLVMRecompiler.h"
#include "Emu/Memory/Memory.h"
#ifdef _MSC_VER
#pragma warning(push, 0)
#endif
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Analysis/MemoryDependenceAnalysis.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Vectorize.h"
#include "llvm/MC/MCDisassembler.h"
#include "llvm/IR/Verifier.h"
#ifdef _MSC_VER
#pragma warning(pop)
#endif

using namespace llvm;
using namespace ppu_recompiler_llvm;

#ifdef ID_MANAGER_INCLUDED
#error "ID Manager cannot be used in this module"
#endif

u64  Compiler::s_rotate_mask[64][64];
bool Compiler::s_rotate_mask_inited = false;

Compiler::Compiler(RecompilationEngine & recompilation_engine, const Executable execute_unknown_function,
	const Executable execute_unknown_block, bool(*poll_status_function)(PPUThread * ppu_state))
	: m_recompilation_engine(recompilation_engine)
	, m_poll_status_function(poll_status_function) {
	InitializeNativeTarget();
	InitializeNativeTargetAsmPrinter();
	InitializeNativeTargetDisassembler();

	m_llvm_context = new LLVMContext();
	m_ir_builder = new IRBuilder<>(*m_llvm_context);

	std::vector<Type *> arg_types;
	arg_types.push_back(m_ir_builder->getInt8PtrTy());
	arg_types.push_back(m_ir_builder->getInt64Ty());
	m_compiled_function_type = FunctionType::get(m_ir_builder->getInt32Ty(), arg_types, false);

	m_executableMap["execute_unknown_function"] = execute_unknown_function;
	m_executableMap["execute_unknown_block"] = execute_unknown_block;

	if (!s_rotate_mask_inited) {
		InitRotateMask();
		s_rotate_mask_inited = true;
	}
}

Compiler::~Compiler() {
	delete m_ir_builder;
	delete m_llvm_context;
}

llvm::ExecutionEngine *Compiler::InitializeModuleAndExecutionEngine()
{
	m_module = new llvm::Module("Module", *m_llvm_context);
	m_execute_unknown_function = (Function *)m_module->getOrInsertFunction("execute_unknown_function", m_compiled_function_type);
	m_execute_unknown_function->setCallingConv(CallingConv::X86_64_Win64);

	m_execute_unknown_block = (Function *)m_module->getOrInsertFunction("execute_unknown_block", m_compiled_function_type);
	m_execute_unknown_block->setCallingConv(CallingConv::X86_64_Win64);

	std::string targetTriple = "x86_64-pc-windows-elf";
	m_module->setTargetTriple(targetTriple);

	llvm::ExecutionEngine *execution_engine =
		EngineBuilder(std::unique_ptr<llvm::Module>(m_module))
		.setEngineKind(EngineKind::JIT)
		.setMCJITMemoryManager(std::unique_ptr<llvm::SectionMemoryManager>(new CustomSectionMemoryManager(m_executableMap)))
		.setOptLevel(llvm::CodeGenOpt::Aggressive)
		.setMCPU("nehalem")
		.create();
	m_module->setDataLayout(execution_engine->getDataLayout());

	return execution_engine;
}

llvm::FunctionPassManager *Compiler::createFunctionPassManager(llvm::Module *module)
{
	llvm::FunctionPassManager *fpm = new llvm::FunctionPassManager(module);
	fpm->add(createNoAAPass());
	fpm->add(createBasicAliasAnalysisPass());
	fpm->add(createNoTargetTransformInfoPass());
	fpm->add(createEarlyCSEPass());
	fpm->add(createTailCallEliminationPass());
	fpm->add(createReassociatePass());
	fpm->add(createInstructionCombiningPass());
	fpm->add(new DominatorTreeWrapperPass());
	fpm->add(new MemoryDependenceAnalysis());
	fpm->add(createGVNPass());
	fpm->add(createInstructionCombiningPass());
	fpm->add(new MemoryDependenceAnalysis());
	fpm->add(createDeadStoreEliminationPass());
	fpm->add(new LoopInfo());
	fpm->add(new ScalarEvolution());
	fpm->add(createSLPVectorizerPass());
	fpm->add(createInstructionCombiningPass());
	fpm->add(createCFGSimplificationPass());
	fpm->doInitialization();

	return fpm;
}

std::pair<Executable, llvm::ExecutionEngine *> Compiler::CompileBlock(const std::string & name, const ControlFlowGraph & cfg, bool generate_linkable_exits) {
	auto compilation_start = std::chrono::high_resolution_clock::now();

	llvm::ExecutionEngine *execution_engine = InitializeModuleAndExecutionEngine();
	llvm::FunctionPassManager *fpm = createFunctionPassManager(m_module);

	m_state.cfg = &cfg;
	m_state.generate_linkable_exits = generate_linkable_exits;

	m_state.m_instruction_count = 0;
	m_state.m_is_function_completly_compilable = false;

	// Create the function
	m_state.function = (Function *)m_module->getOrInsertFunction(name, m_compiled_function_type);
	m_state.function->setCallingConv(CallingConv::X86_64_Win64);
	auto arg_i = m_state.function->arg_begin();
	arg_i->setName("ppu_state");
	m_state.args[CompileTaskState::Args::State] = arg_i;
	(++arg_i)->setName("context");
	m_state.args[CompileTaskState::Args::Context] = arg_i;

	// Create the entry block and add code to branch to the first instruction
	m_ir_builder->SetInsertPoint(GetBasicBlockFromAddress(0));
	m_ir_builder->CreateBr(GetBasicBlockFromAddress(cfg.start_address));

	// Used to decode instructions
	PPUDisAsm dis_asm(CPUDisAsm_DumpMode);
	dis_asm.offset = vm::get_ptr<u8>(cfg.start_address);

	m_recompilation_engine.Log() << "Recompiling block :\n\n";

	// Convert each instruction in the CFG to LLVM IR
	std::vector<PHINode *> exit_instr_list;
	for (u32 instr_i : cfg.instruction_addresses) {
		m_state.hit_branch_instruction = false;
		m_state.current_instruction_address = instr_i;
		BasicBlock *instr_bb = GetBasicBlockFromAddress(m_state.current_instruction_address);
		m_ir_builder->SetInsertPoint(instr_bb);

		if (instr_bb->empty()) {
			u32 instr = vm::ps3::read32(m_state.current_instruction_address);

			// Dump PPU opcode
			dis_asm.dump_pc = m_state.current_instruction_address * 4;
			(*PPU_instr::main_list)(&dis_asm, instr);
			m_recompilation_engine.Log() << dis_asm.last_opcode;

			Decode(instr);
			if (!m_state.hit_branch_instruction)
				m_ir_builder->CreateBr(GetBasicBlockFromAddress(m_state.current_instruction_address + 4));
		}
	}

	// Generate exit logic for all empty blocks
	const std::string &default_exit_block_name = GetBasicBlockNameFromAddress(0xFFFFFFFF);
	for (BasicBlock &block_i : *m_state.function) {
		if (!block_i.getInstList().empty() || block_i.getName() == default_exit_block_name)
			continue;

		// Found an empty block
		m_state.current_instruction_address = GetAddressFromBasicBlockName(block_i.getName());

		m_ir_builder->SetInsertPoint(&block_i);
		PHINode *exit_instr_i32 = m_ir_builder->CreatePHI(m_ir_builder->getInt32Ty(), 0);
		exit_instr_list.push_back(exit_instr_i32);

		SetPc(m_ir_builder->getInt32(m_state.current_instruction_address));

		if (generate_linkable_exits) {
			Value *context_i64 = m_ir_builder->CreateZExt(exit_instr_i32, m_ir_builder->getInt64Ty());
			context_i64 = m_ir_builder->CreateOr(context_i64, (u64)cfg.function_address << 32);
			Value *ret_i32 = IndirectCall(m_state.current_instruction_address, context_i64, false);
			Value *cmp_i1 = m_ir_builder->CreateICmpNE(ret_i32, m_ir_builder->getInt32(0));
			BasicBlock *then_bb = GetBasicBlockFromAddress(m_state.current_instruction_address, "then_0");
			BasicBlock *merge_bb = GetBasicBlockFromAddress(m_state.current_instruction_address, "merge_0");
			m_ir_builder->CreateCondBr(cmp_i1, then_bb, merge_bb);

			m_ir_builder->SetInsertPoint(then_bb);
			context_i64 = m_ir_builder->CreateZExt(ret_i32, m_ir_builder->getInt64Ty());
			context_i64 = m_ir_builder->CreateOr(context_i64, (u64)cfg.function_address << 32);
			m_ir_builder->CreateCall2(m_execute_unknown_block, m_state.args[CompileTaskState::Args::State], context_i64);
			m_ir_builder->CreateBr(merge_bb);

			m_ir_builder->SetInsertPoint(merge_bb);
			m_ir_builder->CreateRet(m_ir_builder->getInt32(0));
		}
		else {
			m_ir_builder->CreateRet(exit_instr_i32);
		}
	}

	// If the function has a default exit block then generate code for it
	BasicBlock *default_exit_bb = GetBasicBlockFromAddress(0xFFFFFFFF, "", false);
	if (default_exit_bb) {
		m_ir_builder->SetInsertPoint(default_exit_bb);
		PHINode *exit_instr_i32 = m_ir_builder->CreatePHI(m_ir_builder->getInt32Ty(), 0);
		exit_instr_list.push_back(exit_instr_i32);

		if (generate_linkable_exits) {
			Value *cmp_i1 = m_ir_builder->CreateICmpNE(exit_instr_i32, m_ir_builder->getInt32(0));
			BasicBlock *then_bb = GetBasicBlockFromAddress(0xFFFFFFFF, "then_0");
			BasicBlock *merge_bb = GetBasicBlockFromAddress(0xFFFFFFFF, "merge_0");
			m_ir_builder->CreateCondBr(cmp_i1, then_bb, merge_bb);

			m_ir_builder->SetInsertPoint(then_bb);
			Value *context_i64 = m_ir_builder->CreateZExt(exit_instr_i32, m_ir_builder->getInt64Ty());
			context_i64 = m_ir_builder->CreateOr(context_i64, (u64)cfg.function_address << 32);
			m_ir_builder->CreateCall2(m_execute_unknown_block, m_state.args[CompileTaskState::Args::State], context_i64);
			m_ir_builder->CreateBr(merge_bb);

			m_ir_builder->SetInsertPoint(merge_bb);
			m_ir_builder->CreateRet(m_ir_builder->getInt32(0));
		}
		else {
			m_ir_builder->CreateRet(exit_instr_i32);
		}
	}

	// Add incoming values for all exit instr PHI nodes
	for (PHINode *exit_instr_i : exit_instr_list) {
		BasicBlock *block = exit_instr_i->getParent();
		for (pred_iterator pred_i = pred_begin(block); pred_i != pred_end(block); pred_i++) {
			u32 pred_address = GetAddressFromBasicBlockName((*pred_i)->getName());
			exit_instr_i->addIncoming(m_ir_builder->getInt32(pred_address), *pred_i);
		}
	}

	m_recompilation_engine.Log() << "LLVM bytecode:\n";
	m_recompilation_engine.Log() << *m_module;

	std::string        verify;
	raw_string_ostream verify_ostream(verify);
	if (verifyFunction(*m_state.function, &verify_ostream)) {
		m_recompilation_engine.Log() << "Verification failed: " << verify << "\n";
	}

	auto ir_build_end = std::chrono::high_resolution_clock::now();
	m_stats.ir_build_time += std::chrono::duration_cast<std::chrono::nanoseconds>(ir_build_end - compilation_start);

	// Optimize this function
	fpm->run(*m_state.function);
	auto optimize_end = std::chrono::high_resolution_clock::now();
	m_stats.optimization_time += std::chrono::duration_cast<std::chrono::nanoseconds>(optimize_end - ir_build_end);

	// Translate to machine code
	execution_engine->finalizeObject();
	void *function = execution_engine->getPointerToFunction(m_state.function);
	auto translate_end = std::chrono::high_resolution_clock::now();
	m_stats.translation_time += std::chrono::duration_cast<std::chrono::nanoseconds>(translate_end - optimize_end);

	/*    m_recompilation_engine.Log() << "\nDisassembly:\n";
		auto disassembler = LLVMCreateDisasm(sys::getProcessTriple().c_str(), nullptr, 0, nullptr, nullptr);
		for (size_t pc = 0; pc < mci.size();) {
			char str[1024];

			auto size = LLVMDisasmInstruction(disassembler, ((u8 *)mci.address()) + pc, mci.size() - pc, (uint64_t)(((u8 *)mci.address()) + pc), str, sizeof(str));
			m_recompilation_engine.Log() << fmt::Format("0x%08X: ", (u64)(((u8 *)mci.address()) + pc)) << str << '\n';
			pc += size;
		}

		LLVMDisasmDispose(disassembler);*/

	auto compilation_end = std::chrono::high_resolution_clock::now();
	m_stats.total_time += std::chrono::duration_cast<std::chrono::nanoseconds>(compilation_end - compilation_start);
	delete fpm;

	assert(function != nullptr);
	return std::make_pair((Executable)function, execution_engine);
}


std::pair<Executable, llvm::ExecutionEngine *> Compiler::CompileFunction(u32 address, u32 instruction_count) {
	auto compilation_start = std::chrono::high_resolution_clock::now();

	llvm::ExecutionEngine *execution_engine = InitializeModuleAndExecutionEngine();
	llvm::FunctionPassManager *fpm = createFunctionPassManager(m_module);

	m_state.cfg = nullptr;
	m_state.generate_linkable_exits = false;

	m_state.m_instruction_count = instruction_count;
	m_state.m_is_function_completly_compilable = true;

	// Create the function
	std::string name = fmt::Format("fonction_0x%08X", address);
	m_state.function = (Function *)m_module->getOrInsertFunction(name, m_compiled_function_type);
	m_state.function->setCallingConv(CallingConv::X86_64_Win64);
	auto arg_i = m_state.function->arg_begin();
	arg_i->setName("ppu_state");
	m_state.args[CompileTaskState::Args::State] = arg_i;
	(++arg_i)->setName("context");
	m_state.args[CompileTaskState::Args::Context] = arg_i;

	// Create the entry block and add code to branch to the first instruction
	m_ir_builder->SetInsertPoint(GetBasicBlockFromAddress(0));
	m_ir_builder->CreateBr(GetBasicBlockFromAddress(address));

	// Used to decode instructions
	PPUDisAsm dis_asm(CPUDisAsm_DumpMode);
	dis_asm.offset = vm::get_ptr<u8>(address);

	m_recompilation_engine.Log() << "Recompiling block :\n\n";

	// Convert each instruction in the CFG to LLVM IR
	for (u32 instructionAddress = address; instructionAddress < address + instruction_count * 4; instructionAddress += 4) {
		m_state.hit_branch_instruction = false;
		m_state.current_instruction_address = instructionAddress;
		BasicBlock *instr_bb = GetBasicBlockFromAddress(instructionAddress);
		m_ir_builder->SetInsertPoint(instr_bb);

		u32 instr = vm::ps3::read32(instructionAddress);

		// Dump PPU opcode
		dis_asm.dump_pc = instructionAddress;
		(*PPU_instr::main_list)(&dis_asm, instr);
		m_recompilation_engine.Log() << dis_asm.last_opcode;

		Decode(instr);
		if (!m_state.hit_branch_instruction)
			m_ir_builder->CreateBr(GetBasicBlockFromAddress(instructionAddress + 4));
	}

	m_recompilation_engine.Log() << "LLVM bytecode:\n";
	m_recompilation_engine.Log() << *m_module;

	std::string        verify;
	raw_string_ostream verify_ostream(verify);
	if (verifyFunction(*m_state.function, &verify_ostream)) {
		m_recompilation_engine.Log() << "Verification failed: " << verify << "\n";
	}

	auto ir_build_end = std::chrono::high_resolution_clock::now();
	m_stats.ir_build_time += std::chrono::duration_cast<std::chrono::nanoseconds>(ir_build_end - compilation_start);

	// Optimize this function
	fpm->run(*m_state.function);
	auto optimize_end = std::chrono::high_resolution_clock::now();
	m_stats.optimization_time += std::chrono::duration_cast<std::chrono::nanoseconds>(optimize_end - ir_build_end);

	// Translate to machine code
	execution_engine->finalizeObject();
	void *function = execution_engine->getPointerToFunction(m_state.function);
	auto translate_end = std::chrono::high_resolution_clock::now();
	m_stats.translation_time += std::chrono::duration_cast<std::chrono::nanoseconds>(translate_end - optimize_end);

	auto compilation_end = std::chrono::high_resolution_clock::now();
	m_stats.total_time += std::chrono::duration_cast<std::chrono::nanoseconds>(compilation_end - compilation_start);
	delete fpm;

	// We assume the function will not be deleted except if app is exited.
	m_executableMap[name] = (Executable)function;

	assert(function != nullptr);
	return std::make_pair((Executable)function, execution_engine);
}

Compiler::Stats Compiler::GetStats() {
	return m_stats;
}

void Compiler::Decode(const u32 code) {
	(*PPU_instr::main_list)(this, code);
}

std::mutex                           RecompilationEngine::s_mutex;
std::shared_ptr<RecompilationEngine> RecompilationEngine::s_the_instance = nullptr;

RecompilationEngine::RecompilationEngine()
	: m_log(nullptr)
	, m_currentId(0)
	, m_compiler(*this, CPUHybridDecoderRecompiler::ExecuteFunction, CPUHybridDecoderRecompiler::ExecuteTillReturn, CPUHybridDecoderRecompiler::PollStatus) {
	m_compiler.RunAllTests();
}

RecompilationEngine::~RecompilationEngine() {
	m_function_to_compiled_executable.clear();
	m_block_to_compiled_executable.clear();
}

Executable executeFunc;
Executable executeUntilReturn;

const Executable *RecompilationEngine::GetExecutable(u32 address, bool isFunction) {
	return isFunction ? &executeFunc : &executeUntilReturn;
}

const Executable RecompilationEngine::GetCompiledFunctionIfAvailable(u32 address)
{
	{
		std::lock_guard<std::mutex> lock(m_address_to_function_lock);
		std::unordered_map<u32, Executable>::iterator It = m_function_to_compiled_executable.find(address);
		if (It != m_function_to_compiled_executable.end())
			return It->second;
		m_function_to_compiled_executable.insert(std::make_pair(address, nullptr));
	}

	BlockEntry key(address, true);
	TryCompileFunction(key);

	return GetCompiledFunctionIfAvailable(address);
}

const Executable *RecompilationEngine::GetCompiledBlockIfAvailable(u32 address)
{
	return nullptr;
/*	std::lock_guard<std::mutex> lock(m_address_to_function_lock);
	std::unordered_map<u32, ExecutableStorage>::iterator It = m_block_to_compiled_executable.find(address);
	if (It == m_block_to_compiled_executable.end())
		return nullptr;
	u32 id = std::get<3>(It->second);
	if (Ini.LLVMExclusionRange.GetValue() && (id >= Ini.LLVMMinId.GetValue() && id <= Ini.LLVMMaxId.GetValue()))
		return nullptr;
	return &(std::get<0>(It->second));*/
}

raw_fd_ostream & RecompilationEngine::Log() {
	if (!m_log) {
		std::error_code error;
		m_log = new raw_fd_ostream("PPULLVMRecompiler.log", error, sys::fs::F_Text);
		m_log->SetUnbuffered();
	}

	return *m_log;
}

/**
 * This code is inspired from Dolphin PPC Analyst
 */
inline s32 SignExt16(s16 x) { return (s32)(s16)x; }
inline s32 SignExt26(u32 x) { return x & 0x2000000 ? (s32)(x | 0xFC000000) : (s32)(x); }


bool RecompilationEngine::AnalyseFunction(BlockEntry &functionData, u32 maxSize)
{
	u32 startAddress = functionData.address;
	u32 farthestBranchTarget = startAddress;
	functionData.instructionCount = 0;
	functionData.calledFunctions.clear();
	functionData.is_analysed = true;
	functionData.is_compilable_function = true;
	Log() << "Analysing " << (u32*)startAddress << "\n";
	for (size_t instructionAddress = startAddress; instructionAddress < startAddress + maxSize; instructionAddress += 4)
	{
		u32 instr = vm::ps3::read32(instructionAddress);
		functionData.instructionCount++;
		if (instr == PPU_instr::implicts::BLR() && instructionAddress >= farthestBranchTarget && functionData.is_compilable_function)
		{
			Log() << "Analysis: Block is compilable into a function \n";
			return functionData.instructionCount > 1;
		}
		else if (PPU_instr::fields::GD_13(instr) == PPU_opcodes::G_13Opcodes::BCCTR)
		{
			if (!PPU_instr::fields::LK(instr))
			{
				Log() << "Analysis: indirect branching found \n";
				functionData.is_compilable_function = false;
				return functionData.instructionCount > 1;
			}
		}
		else if (PPU_instr::fields::OPCD(instr) == PPU_opcodes::PPU_MainOpcodes::BC)
		{
			u32 target = SignExt16(PPU_instr::fields::BD(instr));
			if (!PPU_instr::fields::AA(instr)) // Absolute address
				target += instructionAddress;
			if (target > farthestBranchTarget && !PPU_instr::fields::LK(instr))
				farthestBranchTarget = target;
		}
		else if (PPU_instr::fields::OPCD(instr) == PPU_opcodes::PPU_MainOpcodes::B)
		{
			u32 target = SignExt26(PPU_instr::fields::LL(instr));
			if (!PPU_instr::fields::AA(instr)) // Absolute address
				target += instructionAddress;

			if (!PPU_instr::fields::LK(instr))
			{
				if (target < startAddress)
				{
					Log() << "Analysis: branch to previous block\n";
					functionData.is_compilable_function = false;
					return functionData.instructionCount > 1;
				}
				else if (target > farthestBranchTarget)
					farthestBranchTarget = target;
			}
			else
				functionData.calledFunctions.insert(target);
		}
	}
	Log() << "Analysis: maxSize reached \n";
	functionData.is_compilable_function = false;
	return false;
}

void RecompilationEngine::CompileBlock(BlockEntry & block_entry) {
	return;
	/*
	Log() << "Compile: " << block_entry.ToString() << "\n";

	assert(!block_entry.is_compiled);

	const std::pair<Executable, llvm::ExecutionEngine *> &compileResult =
		m_compiler.CompileBlock(fmt::Format("block_0x%08X", block_entry.cfg.start_address), block_entry.cfg,
			block_entry.is_function );

	std::lock_guard<std::mutex> lock(m_address_to_function_lock);

	std::get<1>(m_block_to_compiled_executable[block_entry.address]) = std::unique_ptr<llvm::ExecutionEngine>(compileResult.second);
	std::get<0>(m_block_to_compiled_executable[block_entry.address]) = compileResult.first;
	std::get<3>(m_block_to_compiled_executable[block_entry.address]) = m_currentId;

	Log() << "ID IS " << m_currentId << "\n";
	m_currentId++;
	block_entry.is_compiled = true;*/
}

void RecompilationEngine::TryCompileFunction(BlockEntry & block_entry) {
	assert(!block_entry.is_compiled);
	if (!block_entry.is_analysed)
		AnalyseFunction(block_entry);

	if (!block_entry.is_compilable_function)
		return;

	Log() << "Compile: " << block_entry.ToString() << "\n";
	Log() << "Size :" << block_entry.instructionCount << "\n";
	Log() << "called function count : " << block_entry.calledFunctions.size() << "\n";

	std::pair<Executable, llvm::ExecutionEngine *> compileResult;
	{
		std::lock_guard<std::mutex> lock(m_compiler_lock);
		compileResult = m_compiler.CompileFunction(block_entry.address, block_entry.instructionCount);
	}

	{
		std::lock_guard<std::mutex> lock(m_address_to_function_lock);
		m_function_storage.push_back(std::unique_ptr<llvm::ExecutionEngine>(compileResult.second));
		m_function_to_compiled_executable[block_entry.address] = compileResult.first;
	}

	block_entry.is_compiled = true;
}

std::shared_ptr<RecompilationEngine> RecompilationEngine::GetInstance() {
	std::lock_guard<std::mutex> lock(s_mutex);

	if (s_the_instance == nullptr) {
		s_the_instance = std::shared_ptr<RecompilationEngine>(new RecompilationEngine());
	}

	return s_the_instance;
}

ppu_recompiler_llvm::CPUHybridDecoderRecompiler::CPUHybridDecoderRecompiler(PPUThread & ppu)
	: m_ppu(ppu)
	, m_interpreter(new PPUInterpreter(ppu))
	, m_decoder(m_interpreter)
	, m_recompilation_engine(RecompilationEngine::GetInstance()) {
	executeFunc = CPUHybridDecoderRecompiler::ExecuteFunction;
	executeUntilReturn = CPUHybridDecoderRecompiler::ExecuteTillReturn;
}

ppu_recompiler_llvm::CPUHybridDecoderRecompiler::~CPUHybridDecoderRecompiler() {
}

u32 ppu_recompiler_llvm::CPUHybridDecoderRecompiler::DecodeMemory(const u32 address) {
	ExecuteFunction(&m_ppu, 0);
	return 0;
}

u32 ppu_recompiler_llvm::CPUHybridDecoderRecompiler::ExecuteFunction(PPUThread * ppu_state, u64 context) {
	auto execution_engine = (CPUHybridDecoderRecompiler *)ppu_state->GetDecoder();
	const Executable executable = execution_engine->m_recompilation_engine->GetCompiledFunctionIfAvailable(ppu_state->PC);
	if (executable)
		return (u32)executable(ppu_state, 0);
	else
		return ExecuteTillReturn(ppu_state, 0);
}

/// Get the branch type from a branch instruction
static BranchType GetBranchTypeFromInstruction(u32 instruction) {
	u32 instructionOpcode = PPU_instr::fields::OPCD(instruction);
	u32 lk = instruction & 1;

	if (instructionOpcode == PPU_opcodes::PPU_MainOpcodes::B ||
		instructionOpcode == PPU_opcodes::PPU_MainOpcodes::BC)
		return lk ? BranchType::FunctionCall : BranchType::LocalBranch;
	if (instructionOpcode == PPU_opcodes::PPU_MainOpcodes::G_13) {
		u32 G13Opcode = PPU_instr::fields::GD_13(instruction);
		if (G13Opcode == PPU_opcodes::G_13Opcodes::BCLR)
			return lk ? BranchType::FunctionCall : BranchType::Return;
		if (G13Opcode == PPU_opcodes::G_13Opcodes::BCCTR)
			return lk ? BranchType::FunctionCall : BranchType::LocalBranch;
		return BranchType::NonBranch;
	}
	if (instructionOpcode == 1 && (instruction & EIF_PERFORM_BLR)) // classify HACK instruction
		return instruction & EIF_USE_BRANCH ? BranchType::FunctionCall : BranchType::Return;
	if (instructionOpcode == 1 && (instruction & EIF_USE_BRANCH))
		return BranchType::LocalBranch;
	return BranchType::NonBranch;
}

u32 ppu_recompiler_llvm::CPUHybridDecoderRecompiler::ExecuteTillReturn(PPUThread * ppu_state, u64 context) {
	CPUHybridDecoderRecompiler *execution_engine = (CPUHybridDecoderRecompiler *)ppu_state->GetDecoder();

	while (PollStatus(ppu_state) == false) {
		const Executable *executable = execution_engine->m_recompilation_engine->GetCompiledBlockIfAvailable(ppu_state->PC);
		if (executable)
		{
			auto entry = ppu_state->PC;
			u32 exit = (u32)(*executable)(ppu_state, 0);
			if (exit == 0)
				return 0;
			continue;
		}
		u32 instruction = vm::ps3::read32(ppu_state->PC);
		u32 oldPC = ppu_state->PC;
		execution_engine->m_decoder.Decode(instruction);
		auto branch_type = ppu_state->PC != oldPC ? GetBranchTypeFromInstruction(instruction) : BranchType::NonBranch;
		ppu_state->PC += 4;

		switch (branch_type) {
		case BranchType::Return:
			if (Emu.GetCPUThreadStop() == ppu_state->PC) ppu_state->fast_stop();
			return 0;
		case BranchType::FunctionCall: {
			ExecuteFunction(ppu_state, 0);
			break;
		}
		case BranchType::LocalBranch:
			break;
		case BranchType::NonBranch:
			break;
		default:
			assert(0);
			break;
		}
	}

	return 0;
}

bool ppu_recompiler_llvm::CPUHybridDecoderRecompiler::PollStatus(PPUThread * ppu_state) {
	return ppu_state->check_status();
}
#endif // LLVM_AVAILABLE