#ifndef LEXER_H_
#define LEXER_H_

#include <list>
#include <cctype>
#include <string>

#include "tokens.h"

namespace lexer {
class Lexer {
public:
  Lexer(std::wistream &input);

  tokens::Token peek(size_t forward = 0ull) const;
  tokens::Token pop();

private:
  std::list<tokens::Token> tokens;

  tokens::Token extractToken(std::wistream &input);
};
} // namespace lexer

#endif // !LEXER_H_
