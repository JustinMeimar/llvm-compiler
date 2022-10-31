grammar Gazprea;

tokens {
    SCALAR_VAR_DECLARATION_TOKEN,
    VECTOR_VAR_DECLARATION_TOKEN,
    TUPLE_VAR_DECLARATION_TOKEN,
    TYPEDEF_VAR_DECLARATION_TOKEN,
    ASSIGNMENT_TOKEN,
    CONDITIONAL_STATEMENT_TOKEN,
    ELSEIF_TOKEN,
    ELSE_TOKEN,
    INFINITE_LOOP_TOKEN,
    PRE_PREDICATE_LOOP_TOKEN,
    POST_PREDICATE_LOOP_TOKEN,
    ITERATOR_LOOP_TOKEN,
    INPUT_STREAM_TOKEN,
    OUTPUT_STREAM_TOKEN,
    GENERATOR_TOKEN,
    GENERATOR_DOMAIN_VARIABLE_TOKEN,
    GENERATOR_DOMAIN_VARIABLE_LIST_TOKEN,
    FILTER_TOKEN,
    FILTER_PREDICATE_TOKEN,
    EXPR_TOKEN,
    BLOCK_TOKEN,
    INDEXING_TOKEN,
    TUPLE_ACCESS_TOKEN,
    TYPE_QUALIFIER_TOKEN,
    UNARY_TOKEN,
    CALL_PROCEDURE_STATEMENT_TOKEN,
    CALL_PROCEDURE_FUNCTION_IN_EXPRESSION,
    STRING_CONCAT_TOKEN,
    EXPRESSION_LIST_TOKEN,
    TUPLE_LITERAL_TOKEN,
    VECTOR_LITERAL_TOKEN,
    SINGLE_TOKEN_TYPE_TOKEN,
    VECTOR_TYPE_TOKEN,
    TUPLE_TYPE_TOKEN,
    CAST_TOKEN,
    VECTOR_SIZE_DECLARATION_LIST_TOKEN,
    TUPLE_TYPE_DECLARATION_ATOM,
    TUPLE_TYPE_DECLARATION_LIST,
    FUNCTION_DECLARATION_RETURN_TOKEN,
    PROCEDURE_DECLARATION_RETURN_TOKEN,
    SUBROUTINE_EMPTY_BODY_TOKEN,
    SUBROUTINE_EXPRESSION_BODY_TOKEN,
    SUBROUTINE_BLOCK_BODY_TOKEN,
    FORMAL_PARAMETER_TOKEN,
    FORMAL_PARAMETER_LIST_TOKEN,
    IDENTIFIER_TOKEN,
    EXPLICIT_TYPE_TOKEN,
    INFERRED_TYPE_TOKEN,
    UNQUALIFIED_TYPE_TOKEN,
    DOMAIN_EXPRESSION_TOKEN,
    REAL_CONSTANT_TOKEN
}

@parser::members {
    bool noSpaceAhead() {
        return true;

        antlr4::CommonTokenStream * ins = (antlr4::CommonTokenStream *)getInputStream();
        int index = ins->index();

        if (index == 0) return true;

        antlr4::Token *prevToken = ins->get(index - 1);
        return prevToken->getChannel() != 1;
    }
}

compilationUnit: statement* EOF;

// statement: 'test';
statement: varDeclarationStatement
        | assignmentStatement
        | conditionalStatement
        | infiniteLoopStatement
        | prePredicatedLoopStatement
        | postPredicatedLoopStatement
        | breakStatement
        | continueStatement
        | iteratorLoopStatement
        | streamStatement
        | subroutineDeclDef
        | returnStatement
        | callProcedure
        | typedefStatement
        | block
        ;

// Type and Type Qualifier
vectorSizeDeclarationAtom: '*' | expression ;
vectorSizeDeclarationList: vectorSizeDeclarationAtom (',' vectorSizeDeclarationAtom)? ;

tupleTypeDeclarationAtom: singleTermType (singleTermType (singleTermType)?)?;  // for tuple(a b) it's impossible to distinguish if b is a type or an id, parse them together
tupleTypeDeclarationList: tupleTypeDeclarationAtom (',' tupleTypeDeclarationAtom)* ;

singleTokenType: BOOLEAN | CHARACTER | INTEGER | REAL | STRING | INTERVAL | identifier;  // type represented by one token
singleTermType:
     singleTokenType '[' vectorSizeDeclarationList ']'  # VectorMatrixType
     | TUPLE '(' tupleTypeDeclarationList ')'           # TupleType
     | singleTokenType                                  # SingleTokenTypeAtom
     ;

typeQualifier: VAR | CONST ;
unqualifiedType: singleTermType singleTermType?;
anyType:
    typeQualifier? unqualifiedType  # ExplcitType
    | typeQualifier                 # InferredType
    ;

// typedef
typedefStatement: TYPEDEF unqualifiedType identifier ';' ;  // can not include const/var

// Variable Declaration and Assignment
varDeclarationStatement: anyType identifier ('=' expression)? ';' ;
assignmentStatement: expressionList '=' expression ';' ;

// Function and Procedure
expressionList: expression (',' expression)* ;
formalParameter: anyType identifier ;
formalParameterList: formalParameter (',' formalParameter)* ;

subroutineBody : ';'            # FunctionEmptyBody
        | '=' expression ';'    # FunctionExprBody
        | block                 # FunctionBlockBody
        ;
subroutineDeclDef: (PROCEDURE | FUNCTION) identifier '(' formalParameterList? ')' (RETURNS unqualifiedType)? subroutineBody;

returnStatement: RETURN expression ';';

callProcedure: CALL identifier '(' expressionList? ')' ';';

// Conditional
conditionalStatement: IF expression statement elseIfStatement* elseStatement? ;
elseIfStatement: ELSE IF expression statement ;
elseStatement: ELSE statement ;
//
// Loop
infiniteLoopStatement: LOOP statement ;
prePredicatedLoopStatement: LOOP WHILE expression statement ;
postPredicatedLoopStatement: LOOP statement WHILE expression ';' ;
iteratorLoopStatement: LOOP domainExpression (',' domainExpression)* statement ;
//
// Break and Continue
breakStatement: BREAK ';' ;
continueStatement: CONTINUE ';' ;
//
// Stream
streamStatement:
    expression '->' identifier ';'      # OutputStream
    | expression '<-' identifier ';'     # InputStream
    ;
//
// Block
block: '{' statement* '}' ;
//
// realConstant
identifier: 'e' | E_IdentifierToken | IdentifierToken;
signedExponentPart: 'e' {noSpaceAhead()}? ('+' | '-') {noSpaceAhead()}? IntegerConstant;
realConstantExponent: signedExponentPart | E_IdentifierToken;
// recognizes a real literal
realConstant:
    (IntegerConstant {noSpaceAhead()}?)?  DOT {noSpaceAhead()}? IntegerConstant ({noSpaceAhead()}? realConstantExponent)?
    | IntegerConstant {noSpaceAhead()}? DOT ({noSpaceAhead()}? realConstantExponent)?
    | IntegerConstant {noSpaceAhead()}? realConstantExponent
    ;
//
// Expression
expression: expr ;
tupleExpressionList: expression (',' expression)+ ;
tupleAccessIndex: IntegerConstant | identifier;
expr:
    identifier '(' expressionList? ')'                                         # CallProcedureFunctionInExpression
    | AS '<' unqualifiedType '>' '(' expression ')'                            # Cast
    | '(' tupleExpressionList ')'                                              # TupleLiteral
    | expr op=DOT tupleAccessIndex                                             # TupleAccess
    | '(' expr ')'                                                             # Parenthesis
    | '[' expressionList? ']'                                                  # VectorLiteral
    | expr '[' expr ']'                                                        # Indexing
    | expr '..' expr                                                           # Interval
    | <assoc=right> op=('+' | '-' | 'not') expr                                # UnaryOp
    | <assoc=right> expr op='^' expr                                           # BinaryOp
    | expr op=('*' | '/' | '%' | '**') expr                                    # BinaryOp
    | expr op=('+' | '-') expr                                                 # BinaryOp
    | expr op='by' expr                                                        # BinaryOp
    | expr op=('>' | '<' | '<=' | '>=') expr                                   # BinaryOp
    | expr op=('==' | '!=') expr                                               # BinaryOp
    | expr op='and' expr                                                       # BinaryOp
    | expr op=('or' | 'xor') expr                                              # BinaryOp
    | <assoc=right> expr '||' expr                                             # Concatenation
    | '[' generatorDomainVariableList '|' expression ']'                       # Generator
    | '[' identifier IN expression '&' expressionList ']'                      # Filter
    | identifier                                                               # IdentifierAtom
    | IntegerConstant                                                          # IntegerAtom
    | realConstant                                                             # RealAtom
    | CharacterConstant                                                        # CharacterAtom
    | StringLiteral                                                            # StringLiteralAtom
    ;
//
// Generator and Filter
domainExpression: identifier IN expression ;
generatorDomainVariableList: domainExpression (',' domainExpression)? ;
//
// Reserve Keywords
AND : 'and' ;
AS : 'as' ;
BOOLEAN: 'boolean' ;
BREAK : 'break' ;
BY: 'by' ;
CALL : 'call' ;
CHARACTER : 'character' ;
CONST : 'const' ;
CONTINUE : 'continue' ;
E_TOKEN : 'e';
ELSE : 'else' ;
FUNCTION: 'function' ;
IF: 'if' ;
IN: 'in' ;
INTEGER: 'integer' ;
INTERVAL: 'interval' ;
LOOP : 'loop' ;
NOT : 'not' ;
OR : 'or' ;
PROCEDURE : 'procedure' ;
REAL : 'real' ;
RETURN : 'return' ;
RETURNS : 'returns' ;
STRING : 'string' ;
TUPLE : 'tuple' ;
TYPEDEF: 'typedef' ;
VAR : 'var' ;
WHILE : 'while' ;
XOR : 'xor' ;
//
DOT: '.';
PLUS: '+' ;
MINUS: '-' ;
ASTERISK: '*' ;
DIV: '/' ;
MODULO: '%' ;
DOTPRODUCT: '**' ;
LESSTHAN: '<' ;
GREATERTHAN: '>' ;
LESSTHANOREQUAL: '<=' ;
GREATERTHANOREQUAL: '>=' ;
ISEQUAL: '==' ;
ISNOTEQUAL: '!=' ;
//
// Identifier, Integer and Exponent
E_IdentifierToken: 'e' DigitSequence;
IdentifierToken: [a-zA-Z_][a-zA-Z0-9_]* ;
IntegerConstant : DigitSequence;
fragment DigitSequence: Digit+ ;
fragment Digit: [0-9] ;
//
// String and Char
CharacterConstant: '\'' CChar '\'' ;
fragment CChar
    :   ~['\\]
    |   EscapeSequence
    ;
//
StringLiteral: '"' SCharSequence? '"' ;
fragment SCharSequence: SChar+ ;
fragment SChar
    :   ~["]
    |   EscapeSequence
    ;
fragment EscapeSequence: '\\' ['"0abnrt\\] ;
//
// Comment
LineComment : '//' ~[\r\n]* -> skip ;
BlockComment: '/*' .*? '*/' -> skip ;
// Skip whitespace
WS : [ \t\r\n]+ -> channel(1);