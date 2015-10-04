Function* FunctionCall::Codegen(LlvmCodeGen* codegen) {
  return NULL;
// TODO: This is currently written to bail out halfway through if the function
// is not supported (only sqrt is).  This leaves zombie functions in the module
// and llvm does not like that.
  LLVMContext& context = codegen->context();
  LlvmCodeGen::LlvmBuilder builder(context);

  // Generate child functions
  for (int i = 0; i < GetNumChildren(); ++i) {
    Function* child = children()[i]->Codegen(codegen);
    if (child == NULL) return NULL;
  }

  Type* return_type = codegen->GetType(type());
  Function* function = CreateComputeFnPrototype(codegen, "FunctionCall");
  BasicBlock* entry_block = BasicBlock::Create(context, "entry", function);

  BasicBlock* not_null_block = BasicBlock::Create(context, "not_null_block", function);
  BasicBlock* ret_block = BasicBlock::Create(context, "ret_block", function);

  builder.SetInsertPoint(entry_block);
  scoped_ptr<LlvmCodeGen::FnPrototype> prototype;
  Type* ret_type = NULL;
  Value* result = NULL;
  switch (op()) {
    case TExprOpcode::MATH_SQRT: {
      ret_type = codegen->double_type();
      prototype.reset(new LlvmCodeGen::FnPrototype(codegen, "sqrt", ret_type));
      prototype->AddArgument(LlvmCodeGen::NamedVariable("x", codegen->double_type()));
      break;
    }
    default:
      return NULL;
  }

  Function* external_function = codegen->GetLibCFunction(prototype.get());

  // Call child functions.  TODO: this needs to be an IR loop over all children
  DCHECK_EQ(GetNumChildren(), 1);
  vector<Value*> args;
  args.resize(GetNumChildren());
  for (int i = 0; i < GetNumChildren(); ++i) {
    args[i] = children()[i]->CodegenGetValue(
        codegen, entry_block, ret_block, not_null_block);
  }

  builder.SetInsertPoint(not_null_block);
  result = builder.CreateCall(external_function, args, "tmp_" + prototype->name());
  builder.CreateBr(ret_block);

  builder.SetInsertPoint(ret_block);
  PHINode* phi_node = builder.CreatePHI(return_type, 2, "tmp_phi");
  phi_node->addIncoming(GetNullReturnValue(codegen), entry_block);
  phi_node->addIncoming(result, not_null_block);
  builder.CreateRet(phi_node);

  return codegen->FinalizeFunction(function);
