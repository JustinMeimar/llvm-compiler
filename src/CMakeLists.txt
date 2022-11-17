# Gather our source files in this directory.
set(
  gazprea_src_files
    #main
    "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
    #ast 
    "${CMAKE_CURRENT_SOURCE_DIR}/ast/AST.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ast/ASTBuilder.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ast/DefWalk.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ast/RefWalk.cpp" 
    "${CMAKE_CURRENT_SOURCE_DIR}/ast/TypeWalk.cpp" 
    #scopes 
    "${CMAKE_CURRENT_SOURCE_DIR}/scopes/BaseScope.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/scopes/GlobalScope.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/scopes/LocalScope.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/scopes/Scope.cpp" 
    "${CMAKE_CURRENT_SOURCE_DIR}/scopes/SymbolTable.cpp"
    #symbols 
    "${CMAKE_CURRENT_SOURCE_DIR}/symbols/BuiltInScalarTypeSymbol.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/symbols/SubroutineSymbol.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/symbols/Symbol.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/symbols/TypedefTypeSymbol.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/symbols/VariableSymbol.cpp"
    #types 
    "${CMAKE_CURRENT_SOURCE_DIR}/types/IdentityNullType.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/types/IntervalType.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/types/MatrixType.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/types/TupleType.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/types/Type.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/types/TypePromote.cpp"
    #llvm
    "${CMAKE_CURRENT_SOURCE_DIR}/codegen/LLVMGen.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/codegen/LLVMIRBranch.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/codegen/LLVMIRFunction.cpp"
    #exceptions
    "${CMAKE_CURRENT_SOURCE_DIR}/exceptions/exceptions.cpp"

)

# Build our executable from the source files.
add_executable(gazc ${gazprea_src_files})
target_include_directories(gazc PUBLIC ${ANTLR_GEN_DIR})

# Ensure that the antlr4-runtime is available.
add_dependencies(gazc antlr)

# Find the libraries that correspond to the LLVM components
# that we wish to use
llvm_map_components_to_libnames(llvm_libs core)

# Add the antlr runtime and parser as libraries to link.
target_link_libraries(gazc parser antlr4-runtime ${llvm_libs})

# Symbolic link our executable to the base directory so we don't have to go searching for it.
symlink_to_bin("gazc")
