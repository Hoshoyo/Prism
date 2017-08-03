#pragma once
#include "ast.h"
#include "type.h"
#include "symbol_table.h"

struct Type_Checker 
{

};

extern Symbol_Table* symbol_table;

int check_declarations(Ast** ast, Scope* global_scope);
int type_inference(Ast** ast, Scope* global_scope, Type_Table* table);

const u32 DECL_CHECK_PASSED = 1;
const u32 DECL_CHECK_FAILED = 0;