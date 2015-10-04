#include <stdio.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/Target/TargetData.h>
#include <llvm/DerivedTypes.h>
#include <llvm/Intrinsics.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Support/IRBuilder.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Analysis/Passes.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/JIT.h>
#include <llvm/PassManager.h>
#include <llvm/Support/DynamicLibrary.h>
#include <llvm/Support/IRReader.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/NoFolder.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/system_error.h>
#include <llvm/Target/TargetData.h>
#include "llvm/Transforms/IPO.h"
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Utils/BasicInliner.h>
#include <llvm/Transforms/Utils/Cloning.h>

#include <vector>

using namespace llvm;
using namespace std;

LLVMContext* context;
Module* module;
ExecutionEngine* engine;

struct StructDesc {
  vector<Type*> types;
  vector<int> offsets;
  int byte_size;
};

StructType* GenerateLlvmStruct(StructDesc* desc) {
  StructType* tuple_struct = StructType::get(*context, ArrayRef<Type*>(desc->types));

  const TargetData* target_data = engine->getTargetData();
  const StructLayout* layout = target_data->getStructLayout(tuple_struct);
  if (layout->getSizeInBytes() != desc->byte_size) {
    printf("Incorrect byte size: %d\n", layout->getSizeInBytes());
    return NULL;
  }

  for (int i = 0; i < desc->types.size(); ++i) {
    if (layout->getElementOffset(i) != desc->offsets[i]) {
      printf("Incorrect offset at %d: %d\n", i, layout->getElementOffset(i));
      return NULL;
    }
  }

  return tuple_struct;
}

int main(int argc, char** argv) {
  llvm::InitializeNativeTarget();
  context = new LLVMContext();
  module = new Module("blah", *context);
  string error;
  engine = ExecutionEngine::createJIT(module, &error, NULL, CodeGenOpt::Default);

  StructDesc desc;

  desc.types.push_back(Type::getInt8Ty(*context));
  desc.types.push_back(Type::getInt64Ty(*context));
  desc.offsets.push_back(0);
  desc.offsets.push_back(8);
  desc.byte_size = 16;

  StructType* llvm_struct = GenerateLlvmStruct(&desc);
  if (llvm_struct != NULL) {
    printf("Done.\n");
  } else {
    printf("Failed.\n");
  }
  return 0;
}
