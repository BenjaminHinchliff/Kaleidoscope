#include <iostream>
#include <sstream>
#include <string>

#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"

#include "KaleidoscopeJIT.h"
#include "parser.h"

template <class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

using jit_ptr = std::unique_ptr<llvm::orc::KaleidoscopeJIT>;

std::unique_ptr<llvm::Module> makeModule(llvm::LLVMContext &context,
                                         const jit_ptr &jit) {
  auto llvmMod = std::make_unique<llvm::Module>("KaleidoscopeJIT", context);
  llvmMod->setDataLayout(jit->getTargetMachine().createDataLayout());
  return llvmMod;
}

int main() {
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  llvm::InitializeNativeTargetAsmParser();

  llvm::LLVMContext context;
  llvm::IRBuilder<> builder{context};
  ast::named_values_t namedValues;
  ast::function_protos_t functionProtos;

  auto jit = std::make_unique<llvm::orc::KaleidoscopeJIT>();
  auto llvmModule = makeModule(context, jit);

  parser::Parser parser{};

  while (true) {
    try {
      std::cout << "ready> ";
      std::string source;
      std::getline(std::cin, source);
      std::stringstream sourceStream(source);
      lexer::Lexer lexer{sourceStream};

      while (!std::holds_alternative<tokens::Eof>(lexer.peek())) {
        auto ast = parser.parse(lexer);
        auto fnIR = std::visit(
            overloaded{[&](ast::Prototype &ast) {
                         return ast.codegen(context, builder, llvmModule,
                                            namedValues, functionProtos);
                       },
                       [&](ast::Function &ast) {
                         return ast.codegen(context, builder, llvmModule,
                                            namedValues, functionProtos);
                       }},
            *ast);
        try {
          auto end = std::get<tokens::Character>(lexer.pop());
          if (end.character != ';') {
            throw std::runtime_error("no end semicolon");
          }
        } catch (const std::exception &e) {
          std::cerr << "each expression must end with semicolon!\n";
          continue;
        }

        std::cout << "IR:\n";
        fnIR->print(llvm::outs(), nullptr);

        auto modHandle = jit->addModule(std::move(llvmModule));
        llvmModule = makeModule(context, jit);

        auto exprSymbol = jit->findSymbol("__anon_expr");
        if (exprSymbol) {
          double (*fP)() =
              (double (*)())(intptr_t)llvm::cantFail(exprSymbol.getAddress());
          std::cout << "Eval:\n" << fP() << '\n';
          jit->removeModule(modHandle);
        }
      }
    } catch (const std::exception &e) {
      std::cerr << e.what() << '\n';
    }
  }

  return 0;
}