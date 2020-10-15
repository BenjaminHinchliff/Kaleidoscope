#ifndef AST_H_
#define AST_H_

#include <iostream>
#include <memory>
#include <variant>
#include <vector>

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"

namespace ast {
using named_values_t = std::unordered_map<std::string, llvm::Value *>;

class AstInterface {
public:
  virtual ~AstInterface() {}
  virtual llvm::Value *codegen(llvm::LLVMContext &context,
                               llvm::IRBuilder<> &builder,
                               std::shared_ptr<llvm::Module> llvmModule,
                               named_values_t &namedValues) const = 0;
};

namespace expr {
class Number;
class Variable;
class Binary;
class Call;

using ExprNode = std::variant<Number, Variable, Binary, Call>;

class ExprInterface {
public:
  virtual ~ExprInterface() {}
  virtual llvm::Value *codegen(llvm::LLVMContext &context,
                               llvm::IRBuilder<> &builder,
                               std::shared_ptr<llvm::Module> llvmModule,
                               named_values_t &namedValues) const = 0;
};

class Number : public ExprInterface {
public:
  Number(double val);

  virtual llvm::Value *codegen(llvm::LLVMContext &context,
                               llvm::IRBuilder<> &builder,
                               std::shared_ptr<llvm::Module> llvmModule,
                               named_values_t &namedValues) const override;

  friend std::ostream &operator<<(std::ostream &out, const Number &expr);

  double val;
};

class Variable : public ExprInterface {
public:
  Variable(const std::string &name);

  virtual llvm::Value *codegen(llvm::LLVMContext &context,
                               llvm::IRBuilder<> &builder,
                               std::shared_ptr<llvm::Module> llvmModule,
                               named_values_t &namedValues) const override;

  friend std::ostream &operator<<(std::ostream &out, const Variable &var);

  std::string name;
};

class Binary : public ExprInterface {
public:
  Binary(char op, std::unique_ptr<ExprNode> lhs, std::unique_ptr<ExprNode> rhs);

  virtual llvm::Value *codegen(llvm::LLVMContext &context,
                               llvm::IRBuilder<> &builder,
                               std::shared_ptr<llvm::Module> llvmModule,
                               named_values_t &namedValues) const override;

  char op;
  std::unique_ptr<ExprNode> lhs;
  std::unique_ptr<ExprNode> rhs;

  friend std::ostream &operator<<(std::ostream &out, const Binary &op);
};

class Call : public ExprInterface {
public:
  Call(const std::string &callee, std::vector<std::unique_ptr<ExprNode>> args);

  virtual llvm::Value *codegen(llvm::LLVMContext &context,
                               llvm::IRBuilder<> &builder,
                               std::shared_ptr<llvm::Module> llvmModule,
                               named_values_t &namedValues) const override;

  std::string callee;
  std::vector<std::unique_ptr<ExprNode>> args;

  friend std::ostream &operator<<(std::ostream &out, const Call &call);
};
} // namespace expr

class Prototype : public AstInterface {
public:
  Prototype(const std::string &name, std::vector<std::string> args);

  virtual llvm::Function *codegen(llvm::LLVMContext &context,
                               llvm::IRBuilder<> &builder,
                               std::shared_ptr<llvm::Module> llvmModule,
                               named_values_t &namedValues) const override;

  std::string name;
  std::vector<std::string> args;
};

class Function : public AstInterface {
public:
  Function(std::unique_ptr<Prototype> proto,
           std::unique_ptr<expr::ExprNode> body);

  virtual llvm::Function *codegen(llvm::LLVMContext &context,
                               llvm::IRBuilder<> &builder,
                               std::shared_ptr<llvm::Module> llvmModule,
                               named_values_t &namedValues) const override;

  std::unique_ptr<Prototype> proto;
  std::unique_ptr<expr::ExprNode> body;
};

using AstNode = std::variant<Prototype, Function>;
} // namespace ast

#endif // !AST_H_
