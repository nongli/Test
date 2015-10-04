#include <cstdio>
#include <string>
#include <sstream>
#include <map>
#include <vector>
#include <iostream>

#include <llvm/Instructions.h>
#include <llvm/DerivedTypes.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/JIT.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/PassManager.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Analysis/Passes.h>
#include <llvm/Target/TargetData.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Support/IRBuilder.h>
#include <llvm/Support/IRReader.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/system_error.h>
#include <llvm/Support/TargetSelect.h>

using namespace llvm;
using namespace std;

bool ReplaceFunctionCall(Function* caller, Function* new_fn, const string& unmangled_name) {

  Function::iterator block_iter = caller->begin();
  // loop over all blocks
  while (block_iter != caller->end()) {
    BasicBlock* block = block_iter++;
    BasicBlock::iterator instr_iter = block->begin();
    // loop over instructions
    while (instr_iter != block->end()) {
      Instruction* instr = instr_iter++;
      // look for call instructions
      if (CallInst::classof(instr)) {
        CallInst* call_instr = reinterpret_cast<CallInst*>(instr);
        Function* old_fn = call_instr->getCalledFunction();
        string old_fn_name = old_fn->getName();
        // look for call instruction that matches the name
        if (old_fn_name.find(unmangled_name) != string::npos) {
          // Insert a new call instruction to the new function
          IRBuilder<> builder(block, instr_iter);
          vector<Value*> calling_args;
          builder.CreateCall(new_fn, calling_args);
          // remove the old call instruction
          call_instr->removeFromParent();
          block->dump();
          return true;
        }
      }
    }
  }

  return true;
}

void JitImplementation() {
  printf("JIT.\n");
}

int main(int argc, char** argv) {
  llvm::InitializeNativeTarget();
  string error;
  Module* compiled_module;
  OwningPtr<MemoryBuffer> buffer;

  // Load in the bitcode file containing the functions for each
  // bytecode operation.
  error_code err = MemoryBuffer::getFile("loop.ll", buffer);
  if (err) {
    cout << "Could not load module: " << err.message() << endl;
    return 1;
  }

  compiled_module = ParseBitcodeFile(buffer.get(), getGlobalContext(), &error);
  if (compiled_module == NULL) {
    cout << "Could not load module: " << error << endl;
    return 1;
  }

  ExecutionEngine* engine = ExecutionEngine::createJIT(compiled_module, &error);
  if (engine == NULL) {
    cout << "Could not create JIT engine." << endl;
    return 1;
  }

  const string main_fn_name = "TestLoop";
  const string body_fn_name = "DefaultImplementation";
  Function* loop_fn = NULL;

  llvm::Module::iterator function_iter = compiled_module->begin();
  while (function_iter != compiled_module->end()) {
    Function* function = function_iter++;
    string fn_name = function->getName();
    if (fn_name.find(main_fn_name) != string::npos) {
      loop_fn = function;
      break;
    }
  }

  if (loop_fn == NULL) {
    cout << "Could not find loop function." << endl;
    return 1;
  }
  
  vector<Type*> args;
  FunctionType* fn_type = FunctionType::get(Type::getVoidTy(getGlobalContext()), args, false);
  Function* jitted_body = Function::Create(
      fn_type, GlobalValue::ExternalLinkage, "JitImplementation", compiled_module);
  engine->addGlobalMapping(jitted_body, reinterpret_cast<void*>(&JitImplementation));
  
  typedef void (*TestLoopFn)(int);

  /*
  void* body_original = engine->getPointerToFunction(loop_fn);
  TestLoopFn fn = reinterpret_cast<TestLoopFn>(body_original);
  fn(5);
  */

  cout << "Replacing with JIT" << endl;
  bool ret = ReplaceFunctionCall(loop_fn, jitted_body, body_fn_name);
  if (!ret) {
    cout << "Could not replace with JIT function." << endl;
    return 1;
  }
  void* body_jit = engine->getPointerToFunction(loop_fn);
  TestLoopFn fn = reinterpret_cast<TestLoopFn>(body_jit);
  fn(5);

  compiled_module->dump();
  return 0;
}
