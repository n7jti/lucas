%option reentrant bison-bridge bison-locations noyywrap nounput noinput
%option extra-type="lucas::frontend::LexerState*"

%{
#include "parser.hpp"
#include "parser_context.hpp"

#include <cstdlib>
#include <string>

#ifdef _WIN32
#define YY_NO_UNISTD_H 1
#include <io.h>
#define isatty _isatty
#define fileno _fileno
#endif

#define YY_DECL int yylex(YYSTYPE* yylval_param, YYLTYPE* yylloc_param, yyscan_t yyscanner)

#define YY_USER_ACTION \
    yylloc_param->first_line = static_cast<int>(yyextra->line); \
    yylloc_param->first_column = static_cast<int>(yyextra->column); \
    for (int index = 0; index < yyleng; ++index) { \
        if (yytext[index] == '\n') { ++yyextra->line; yyextra->column = 1; } \
        else { ++yyextra->column; } \
    } \
    yylloc_param->last_line = static_cast<int>(yyextra->line); \
    yylloc_param->last_column = static_cast<int>(yyextra->column);
%}

%%

[ \t\r\n]+                  ;
([0-9]+(\.[0-9]*)?|\.[0-9]+)([eE][+-]?[0-9]+)? {
                                yylval_param->number = std::strtod(yytext, nullptr);
                                return NUMBER;
                            }
[A-Za-z_][A-Za-z0-9_]*     {
                                yylval_param->name = new std::string(yytext, static_cast<std::size_t>(yyleng));
                                return IDENTIFIER;
                            }
"+"                         return '+';
"-"                         return '-';
"*"                         return '*';
"/"                         return '/';
"^"                         return '^';
"("                         return '(';
")"                         return ')';
.                           return INVALID;
<<EOF>>                     return 0;

%%
