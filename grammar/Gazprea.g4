grammar Gazprea;

tokens {
    SCALAR_VAR_DECLARATION_TOKEN,
    VECTOR_VAR_DECLARATION_TOKEN,
    TUPLE_VAR_DECLARATION_TOKEN,
    TYPEDEF_VAR_DECLARATION_TOKEN,
    ASSIGNMENT_TOKEN,
    CONDITIONAL_TOKEN,
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
    CALL_PROCEDURE_STATEMENT,
    CALL_PROCEDURE_FUNCTION_IN_EXPRESSION,
    STRING_CONCAT_TOKEN,
    EXPRESSION_LIST_TOKEN,
    TUPLE_LITERAL_TOKEN,
    VECTOR_LITERAL_TOKEN,
    SCALAR_TYPE_TOKEN,
    VECTOR_TYPE_TOKEN,
    TUPLE_TYPE_TOKEN,
    TYPE_TOKEN,
    CAST_TOKEN,
    VECTOR_SIZE_DECLARATION_LIST_TOKEN,
    TUPLE_TYPE_DECLARATION_ATOM,
    TUPLE_TYPE_DECLARATION_LIST,
    FUNCTION_DECLARATION_RETURN_TOKEN,
    PROCEDURE_DECLARATION_RETURN_TOKEN,
    FORMAL_PARAMETER_TOKEN,
    FORMAL_PARAMETER_LIST_TOKEN
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
tupleTypeDeclarationAtom: unqualifiedType Identifier? ;  // can be integer interval so can't use single term type
tupleTypeDeclarationList: tupleTypeDeclarationAtom (',' tupleTypeDeclarationAtom)* ;

singleTokenType: BOOLEAN | CHARACTER | INTEGER | REAL | STRING | INTERVAL | Identifier;  // type represented by one token
singleTermType:
     singleTokenType '[' vectorSizeDeclarationList ']'  # VectorMatrixType
     | TUPLE '(' tupleTypeDeclarationList ')'           # TupleType
     | singleTokenType                                  # SingleTokenTypeAtom
     ;

typeQualifier: VAR | CONST ;
unqualifiedType: singleTermType? singleTermType;
anyType:
    typeQualifier? unqualifiedType  # ExplcitType
    | typeQualifier                 # InferredType
    ;

// typedef
typedefStatement: TYPEDEF unqualifiedType Identifier ';' ;  // can not include const/var

// Variable Declaration and Assignment
varDeclarationStatement:
    anyType Identifier ('=' expression)? ';' ;
assignmentStatement: Identifier '=' expression ';' ;

// Function and Procedure
expressionList: expression (',' expression)* ;
formalParameter: anyType Identifier ;
formalParameterList: formalParameter (',' formalParameter)* ;

subroutineBody : ';'      # FunctionEmptyBody
        | '=' expr ';'  # FunctionExprBody
        | block         # FunctionBlockBody
        ;
subroutineDeclDef: (PROCEDURE | FUNCTION) Identifier '(' formalParameterList? ')' (RETURNS unqualifiedType)? subroutineBody;

returnStatement: RETURN expression ';';

callProcedure: CALL Identifier '(' expressionList? ')' ';';

// Conditional
conditionalStatement: IF expression statement elseIfStatement* elseStatement? ;
elseIfStatement: ELSE IF expression statement ;
elseStatement: ELSE statement ;
// 
// Loop
infiniteLoopStatement: LOOP statement ;
prePredicatedLoopStatement: LOOP WHILE expression statement ;
postPredicatedLoopStatement: LOOP statement WHILE expression ';' ;
iteratorLoopStatement: LOOP Identifier IN expression statement ;
// 
// Break and Continue
breakStatement: BREAK ';' ;
continueStatement: CONTINUE ';' ;
// 
// Stream
streamStatement:
    expression '->' Identifier ';'      # OutputStream
    | expression '<-' Identifier ';'     # InputStream
    ;
// 
// Block
block: '{' statement* '}' ;
// 
// Expression
expression: expr ;
expr: 
    Identifier '(' expressionList? ')'                                         # CallProcedureFunctionInExpression
    | AS '<' unqualifiedType '>' '(' expression ')'                            # Cast
    | '(' expressionList ')'                                                   # TupleLiteral
    | expr DOT expr                                                            # TupleAccess
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
    | '[' Identifier IN expression '&' expressionList ']'                      # Filter
    | Identifier                                                               # IdentifierAtom
    | IntegerConstant                                                          # IntegerAtom
    | RealConstant                                                             # RealAtom
    | CharacterConstant                                                        # CharacterAtom
    | StringLiteral                                                            # StringLiteralAtom
    ;
//
// Generator and Filter
generatorDomainVariable: Identifier IN expression ;
generatorDomainVariableList: generatorDomainVariable (',' generatorDomainVariable)? ;
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
DOT: '.';  // Tuple access comes before real recognition
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
Identifier: [a-zA-Z_][a-zA-Z0-9_]* ;
// 
// Integer and Real
IntegerConstant : DigitSequence;
RealConstant:
    (DigitSequence? '.' DigitSequence | DigitSequence '.' ) { this->getInputStream()->LA(1) != '.' }? ExponentPart?
    | DigitSequence ExponentPart;
fragment ExponentPart: 'e' Sign? DigitSequence ;
fragment Sign: [+-] ;
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
WS : [ \t\r\n]+ -> skip ;