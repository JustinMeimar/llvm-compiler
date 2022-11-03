grammar Gazprea;

tokens {
    VAR_DECLARATION_TOKEN,
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
    EXPRESSION_TOKEN,
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
    TUPLE_TYPE_DECLARATION_ATOM_TOKEN,
    TUPLE_TYPE_DECLARATION_LIST_TOKEN,
    SUBROUTINE_EMPTY_BODY_TOKEN,
    SUBROUTINE_EXPRESSION_BODY_TOKEN,
    SUBROUTINE_BLOCK_BODY_TOKEN,
    FORMAL_PARAMETER_ATOM_TOKEN,
    FORMAL_PARAMETER_LIST_TOKEN,
    IDENTIFIER_TOKEN,
    EXPLICIT_TYPE_TOKEN,
    INFERRED_TYPE_TOKEN,
    UNQUALIFIED_TYPE_TOKEN,
    DOMAIN_EXPRESSION_TOKEN,
    REAL_CONSTANT_TOKEN
}

compilationUnit: (wS? statement)* wS? EOF;

statement:
    nonBlockStatement
    | block
    ;
exprPrecededStatement:
    wS nonBlockStatement
    | wS? block
    ;
nonBlockStatement: varDeclarationStatement
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
vectorSizeDeclarationList: vectorSizeDeclarationAtom (wS? ',' wS? vectorSizeDeclarationAtom)? ;

tupleTypeDeclarationAtom: singleTermType (wS? singleTermType (wS? singleTermType)?)?;  // for tuple(a b) it's impossible to distinguish if b is a type or an id, parse them together
tupleTypeDeclarationList: tupleTypeDeclarationAtom (wS? ',' wS? tupleTypeDeclarationAtom)* ;

singleTokenType: BOOLEAN | CHARACTER | INTEGER | REAL | STRING | INTERVAL | identifier;  // type represented by one token
singleTermType:
     singleTokenType wS? '[' wS? vectorSizeDeclarationList wS? ']'  # VectorMatrixType
     | TUPLE wS? '(' wS? tupleTypeDeclarationList wS? ')'           # TupleType
     | singleTokenType                                              # SingleTokenTypeAtom
     ;

typeQualifier: VAR | CONST ;
unqualifiedType: singleTermType (wS? singleTermType)?;
qualifiedType:
    (typeQualifier wS?)? unqualifiedType  # ExplcitType
    | typeQualifier                       # InferredType
    ;

// typedef
typedefStatement: TYPEDEF wS? unqualifiedType wS? identifier wS? ';' ;  // can not include const/var

// Variable Declaration and Assignment
varDeclarationStatement: qualifiedType wS? identifier (wS? '=' wS? expression)? wS? ';' ;
assignmentStatement: expressionList wS? '=' wS? expression wS? ';' ;

// Function and Procedure
expressionList: expression (wS? ',' wS? expression)* ;
formalParameterAtom: qualifiedType wS? identifier ;
formalParameterList: formalParameterAtom (wS? ',' wS? formalParameterAtom)* ;

subroutineBody : ';'                    # SubroutineEmptyBody
        | '=' wS? expression wS? ';'    # SubroutineExprBody
        | block                         # SubroutineBlockBody
        ;
subroutineDeclDef: (PROCEDURE | FUNCTION) wS? identifier wS?
        '(' (wS? formalParameterList)? wS? ')' (wS? RETURNS wS? unqualifiedType)? wS? subroutineBody;

returnStatement: RETURN wS? expression wS? ';';

callProcedure: CALL wS? identifier wS? '(' (wS? expressionList)? wS? ')' wS? ';';
// Conditional
conditionalStatement: IF wS? expression exprPrecededStatement (wS? elseIfStatement)* (wS? elseStatement)? ;
elseIfStatement: ELSE wS? IF wS? expression exprPrecededStatement ;
elseStatement: ELSE wS? statement ;
//
// Loop
infiniteLoopStatement: LOOP wS? statement ;
prePredicatedLoopStatement: LOOP wS? WHILE wS? expression exprPrecededStatement ;
postPredicatedLoopStatement: LOOP wS? statement wS? WHILE wS? expression wS? ';' ;
iteratorLoopStatement: LOOP wS? domainExpression (wS? ',' wS? domainExpression)* exprPrecededStatement ;
//
// Break and Continue
breakStatement: BREAK wS? ';' ;
continueStatement: CONTINUE wS? ';' ;
//
// Stream
streamStatement:
    expression wS? '->' wS? identifier wS? ';'          # OutputStream
    | expression wS? '<-' wS? identifier wS? ';'        # InputStream
    ;
//
// Block
block: '{' (wS? statement)* wS? '}' ;
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
tupleExpressionList: expression (wS? ',' wS? expression)+;
expression: expr ;
expr:
    identifier wS? '(' (wS? expressionList)? wS? ')'                                # CallProcedureFunctionInExpression
    | AS wS? '<' wS? unqualifiedType wS? '>' wS? '(' wS? expression wS? ')'         # Cast
    | '(' wS? tupleExpressionList wS? ')'                                           # TupleLiteral
    | realConstant                                                                  # RealAtom  // before tuple access
    | expr DOT (IntegerConstant | identifier)                                       # TupleAccess
    | '(' wS? expr wS? ')'                                                          # Parenthesis
    | '[' (wS? expressionList)? wS? ']'                                             # VectorLiteral
    | expr wS? '[' wS? expr wS? ']'                                                 # Indexing
    | expr wS? '..' wS? expr                                                        # Interval
    | <assoc=right> op=('+' | '-' | 'not') wS? expr                                 # UnaryOp
    | <assoc=right> expr wS? op='^' wS? expr                                        # BinaryOp
    | expr wS? op=('*' | '/' | '%' | '**') wS? expr                                 # BinaryOp
    | expr wS? op=('+' | '-') wS? expr                                              # BinaryOp
    | expr wS? op='by' wS? expr                                                     # BinaryOp
    | expr wS? op=('>' | '<' | '<=' | '>=') wS? expr                                # BinaryOp
    | expr wS? op=('==' | '!=') wS? expr                                            # BinaryOp
    | expr wS? op='and' wS? expr                                                    # BinaryOp
    | expr wS? op=('or' | 'xor') wS? expr                                           # BinaryOp
    | <assoc=right> expr wS? '||' wS? expr                                          # Concatenation
    | '[' wS? generatorDomainVariableList wS? '|' wS? expression wS? ']'            # Generator
    | '[' wS? identifier wS? IN wS? expression wS? '&' wS? expressionList wS? ']'   # Filter
    | identifier                                                                    # IdentifierAtom
    | IntegerConstant                                                               # IntegerAtom
    | CharacterConstant                                                             # CharacterAtom
    | StringLiteral                                                                 # StringLiteralAtom
    ;
//
// Generator and Filter
domainExpression: identifier wS? IN wS? expression ;
generatorDomainVariableList: domainExpression (wS? ',' wS? domainExpression)? ;
//
// White spaces
wS: SPACE+;
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
SPACE : [ \t\r\n]+;