#include "token.h"

Token::operator bool() const
{
    return type != TokenType::TOK_EOF;
}

llvm::Value* IntegerAST::codegen()
{
    //
    return llvm::ConstantInt::get(*TheContext, llvm::APInt(sizeof(uint32_t), value));
}

llvm::Value* FunctionAST::codegen()
{
    std::vector<llvm::Type*> type_args;
    std::map<std::string, llvm::Type*> types = {
        { "int", llvm::Type::getInt32Ty(*TheContext) }
    };
    for (auto& [type, content, location] : args | std::views::values) {
        auto ty = types[content];
        type_args.emplace_back(ty);
    }

    const auto ft = llvm::FunctionType::get(types[result.content], type_args, false);

    const auto fun = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, name, TheModule.get());

    for (auto const& [index, arg] : fun->args() | std::views::enumerate) {
        //
        arg.setName(args[index].second.content);
    }

    return fun;
}

FunctionAST::FunctionAST(std::string name, const std::vector<std::pair<Token, Token>>& args, Token result)
    : result(std::move(result))
    , name(std::move(name))
    , args(args)
{
}
