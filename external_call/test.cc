#include <dlfcn.h>
#include <stdio.h>
#include <assert.h>
#include <string>
#include <vector>

#include <llvm/Analysis/Verifier.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/JIT.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/TargetSelect.h>

using namespace llvm;
using namespace std;

typedef void (*Fn)();

int main(int argc, char** argv) {
  void* handle = dlopen("./fn.so", RTLD_NOW);
  assert(handle != NULL);
  void* symbol = dlsym(handle, "_Z6TestFnv");
  assert(symbol != NULL);

  Fn fn = (Fn)symbol;
  fn();

  InitializeNativeTarget();
  LLVMContext &context = getGlobalContext();

  // Make the module, which holds all the code.
  Module* module = new Module("test", context);
  assert(module != NULL);

  string err;
  ExecutionEngine* execution_engine = EngineBuilder(module).setErrorStr(&err).create();
  assert(execution_engine != NULL);

  FunctionType* ft = FunctionType::get(Type::getVoidTy(context), false);
  assert(ft != NULL);
  Function* loaded_fn = Function::Create(
      ft, Function::ExternalLinkage, "TestFn", module);
  assert(loaded_fn != NULL);
  execution_engine->addGlobalMapping(loaded_fn, symbol);

  Function* wrapper_fn =
      Function::Create(ft, Function::ExternalLinkage, "Wrapper", module);

  BasicBlock* entry_block = BasicBlock::Create(context, "entry", wrapper_fn);
  IRBuilder<> builder(entry_block);
  builder.CreateCall(loaded_fn);
  builder.CreateRetVoid();
  verifyFunction(*wrapper_fn);

  Fn jit_fn = (Fn)execution_engine->getPointerToFunction(wrapper_fn);
  assert(jit_fn != NULL);
  jit_fn();

  return 0;
}
