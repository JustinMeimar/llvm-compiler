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
    REAL_CONSTANT_TOKEN,
    STATEMENT_TOKEN
}

compilationUnit: (WS? (block | statement))* WS? EOF;

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
               ;

// Type and Type Qualifier
vectorSizeDeclarationAtom: '*' | expression ;
vectorSizeDeclarationList: vectorSizeDeclarationAtom (WS? ',' WS? vectorSizeDeclarationAtom)? ;

tupleTypeDeclarationAtom: singleTermType (WS? singleTermType (WS? singleTermType)?)?;  // for tuple(a b) it's impossible to distinguish if b is a type or an id, parse them together
tupleTypeDeclarationList: tupleTypeDeclarationAtom (WS? ',' WS? tupleTypeDeclarationAtom)* ;

singleTokenType: BOOLEAN | CHARACTER | INTEGER | REAL | STRING | INTERVAL | identifier;  // type represented by one token
singleTermType:
     singleTokenType WS? '[' WS? vectorSizeDeclarationList WS? ']'  # VectorMatrixType
     | TUPLE WS? '(' WS? tupleTypeDeclarationList WS? ')'           # TupleType
     | singleTokenType                                  # SingleTokenTypeAtom
     ;

typeQualifier: VAR | CONST ;
unqualifiedType: singleTermType (WS? singleTermType)?;
qualifiedType:
    (typeQualifier WS?)? unqualifiedType  # ExplcitType
    | typeQualifier                 # InferredType
    ;

// typedef
typedefStatement: TYPEDEF WS? unqualifiedType WS? identifier WS? ';' ;  // can not include const/var

// Variable Declaration and Assignment
varDeclarationStatement: qualifiedType WS? identifier (WS? '=' WS? expression)? WS? ';' ;
assignmentStatement: expressionList WS? '=' WS? expression WS? ';' ;

// Function and Procedure
expressionList: expression (WS? ',' WS? expression)* ;
formalParameter: qualifiedType WS? identifier ;
formalParameterList: formalParameter (WS? ',' WS? formalParameter)* ;

subroutineBody : ';'            # FunctionEmptyBody
        | '=' WS? expression WS? ';'    # FunctionExprBody
        | block                 # FunctionBlockBody
        ;
subroutineDeclDef: (PROCEDURE | FUNCTION) WS? identifier WS?
        '(' (WS? formalParameterList)? WS? ')' (WS? RETURNS WS? unqualifiedType)? WS? subroutineBody;

returnStatement: RETURN WS? expression WS? ';';

callProcedure: CALL WS? identifier WS? '(' (WS? expressionList)? WS? ')' WS? ';';
// Conditional
conditionalStatement: IF WS? expression (WS? block | WS statement) (WS? elseIfStatement)* (WS? elseStatement)? ;
elseIfStatement: ELSE WS? IF WS? expression (WS? block | WS statement) ;
elseStatement: ELSE WS? (block | statement) ;
//
// Loop
infiniteLoopStatement: LOOP WS? (block | statement) ;
prePredicatedLoopStatement: LOOP WS? WHILE WS? expression (WS? block | WS statement) ;
postPredicatedLoopStatement: LOOP WS? (block | statement) WS? WHILE WS? expression WS? ';' ;
iteratorLoopStatement: LOOP WS? domainExpression (WS? ',' WS? domainExpression)* (WS? block | WS statement) ;
//
// Break and Continue
breakStatement: BREAK WS? ';' ;
continueStatement: CONTINUE WS? ';' ;
//
// Stream
streamStatement:
    expression WS? '->' WS? identifier WS? ';'      # OutputStream
    | expression WS? '<-' WS? identifier WS? ';'     # InputStream
    ;
//
// Block
block: '{' (WS? (block | statement))* WS? '}' ;
//
// realConstant
identifier: 'e' | E_IdentifierToken | IdentifierToken;
signedExponentPart: 'e' ('+' | '-') IntegerConstant;
realConstantExponent: signedExponentPart | E_IdentifierToken;
// recognizes a real literal
realConstant:
    IntegerConstant? DOT IntegerConstant realConstantExponent?
    | IntegerConstant DOT realConstantExponent?
    | IntegerConstant realConstantExponent
    ;
//
// Expression
tupleExpressionList: expression (WS? ',' WS? expression)+ ;
expression: expr ;
expr:
    identifier WS? '(' (WS? expressionList)? WS? ')'                                         # CallProcedureFunctionInExpression
    | AS WS? '<' WS? unqualifiedType WS? '>' WS? '(' WS? expression WS? ')'                            # Cast
    | '(' WS? tupleExpressionList WS? ')'                                              # TupleLiteral
    | realConstant                                                             # RealAtom  // before tuple access
    | expr DOT (IntegerConstant | identifier)                                                  # TupleAccess
    | '(' WS? expr WS? ')'                                                             # Parenthesis
    | '[' (WS? expressionList)? WS? ']'                                                  # VectorLiteral
    | expr WS? '[' WS? expr WS? ']'                                                        # Indexing
    | expr WS? '..' WS? expr                                                           # Interval
    | <assoc=right> op=('+' | '-' | 'not') WS? expr                                # UnaryOp
    | <assoc=right> expr WS? op='^' WS? expr                                           # BinaryOp
    | expr WS? op=('*' | '/' | '%' | '**') WS? expr                                    # BinaryOp
    | expr WS? op=('+' | '-') WS? expr                                                 # BinaryOp
    | expr WS? op='by' WS? expr                                                        # BinaryOp
    | expr WS? op=('>' | '<' | '<=' | '>=') WS? expr                                   # BinaryOp
    | expr WS? op=('==' | '!=') WS? expr                                               # BinaryOp
    | expr WS? op='and' WS? expr                                                       # BinaryOp
    | expr WS? op=('or' | 'xor') WS? expr                                              # BinaryOp
    | <assoc=right> expr WS? '||' WS? expr                                             # Concatenation
    | '[' WS? generatorDomainVariableList WS? '|' WS? expression WS? ']'                       # Generator
    | '[' WS? identifier WS? IN WS? expression WS? '&' WS? expressionList WS? ']'                      # Filter
    | identifier                                                               # IdentifierAtom
    | IntegerConstant                                                          # IntegerAtom
    | CharacterConstant                                                        # CharacterAtom
    | StringLiteral                                                            # StringLiteralAtom
    ;
//
// Generator and Filter
domainExpression: identifier WS? IN WS? expression ;
generatorDomainVariableList: domainExpression (WS? ',' WS? domainExpression)? ;
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
WS : [ \t\r\n]+;