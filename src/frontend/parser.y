%define api.pure full
%define parse.error detailed
%locations

%code requires {
    #include "lucas/frontend/ast.hpp"
    #include "parser_context.hpp"

    typedef void* yyscan_t;
}

%code {
    #include <utility>

    static lucas::frontend::SourceSpan source_span(const YYLTYPE& location) {
        return {
            static_cast<std::size_t>(location.first_line),
            static_cast<std::size_t>(location.first_column),
            static_cast<std::size_t>(location.last_line),
            static_cast<std::size_t>(location.last_column)
        };
    }

    int yylex(YYSTYPE* value, YYLTYPE* location, yyscan_t scanner);

    void yyerror(
        YYLTYPE* location,
        lucas::frontend::ParserContext* context,
        yyscan_t,
        const char* message) {
        context->diagnostics.push_back(
            {lucas::frontend::DiagnosticStage::Parse, message, source_span(*location)});
    }
}

%parse-param { lucas::frontend::ParserContext* context }
%parse-param { yyscan_t scanner }
%lex-param { yyscan_t scanner }

%union {
    double number;
    std::string* name;
    lucas::frontend::Expression* expression;
}

%token <number> NUMBER
%token <name> IDENTIFIER
%token INVALID
%type <expression> expression
%destructor { delete $$; } <name>
%destructor { delete $$; } <expression>

%left '+' '-'
%left '*' '/'
%precedence UPLUS UMINUS
%right '^'

%%

input:
    expression {
        context->root.reset($1);
    }
;

expression:
    NUMBER {
        $$ = lucas::frontend::Expression::number_literal($1, source_span(@1)).release();
    }
| '(' expression ')' {
        $$ = $2;
        $$->span = source_span(@$);
    }
| IDENTIFIER {
        $$ = lucas::frontend::Expression::identifier(std::move(*$1), source_span(@1)).release();
        delete $1;
    }
| IDENTIFIER '(' expression ')' {
        $$ = lucas::frontend::Expression::function(
            std::move(*$1),
            std::unique_ptr<lucas::frontend::Expression>($3),
            source_span(@$)).release();
        delete $1;
    }
| '+' expression %prec UPLUS {
        $$ = lucas::frontend::Expression::unary('+', std::unique_ptr<lucas::frontend::Expression>($2), source_span(@$)).release();
    }
| '-' expression %prec UMINUS {
        $$ = lucas::frontend::Expression::unary('-', std::unique_ptr<lucas::frontend::Expression>($2), source_span(@$)).release();
    }
| expression '+' expression {
        $$ = lucas::frontend::Expression::binary('+', std::unique_ptr<lucas::frontend::Expression>($1), std::unique_ptr<lucas::frontend::Expression>($3), source_span(@$)).release();
    }
| expression '-' expression {
        $$ = lucas::frontend::Expression::binary('-', std::unique_ptr<lucas::frontend::Expression>($1), std::unique_ptr<lucas::frontend::Expression>($3), source_span(@$)).release();
    }
| expression '*' expression {
        $$ = lucas::frontend::Expression::binary('*', std::unique_ptr<lucas::frontend::Expression>($1), std::unique_ptr<lucas::frontend::Expression>($3), source_span(@$)).release();
    }
| expression '/' expression {
        $$ = lucas::frontend::Expression::binary('/', std::unique_ptr<lucas::frontend::Expression>($1), std::unique_ptr<lucas::frontend::Expression>($3), source_span(@$)).release();
    }
| expression '^' expression {
        $$ = lucas::frontend::Expression::binary('^', std::unique_ptr<lucas::frontend::Expression>($1), std::unique_ptr<lucas::frontend::Expression>($3), source_span(@$)).release();
    }
;
