#pragma once
#include "type.h"
#include "type_table.h"
#include "ast.h"
#include "decl_check.h"

Type_Instance* infer_from_expression(Ast* expr, Decl_Error* error, bool report_undeclared, bool lval = false);
Type_Instance* type_transform_weak_to_strong(Type_Instance* weak, Type_Instance* strong, Ast* expr, Decl_Error* error);
Type_Instance* type_strength_resolve(Type_Instance* t1, Type_Instance* t2, Ast* expr1, Ast* expr2, Decl_Error* error);

// Type checking
typedef u32 Type_Error;

const u32 TYPE_OK = 0;
const u32 TYPE_ERROR_MISMATCH = FLAG(0);
const u32 TYPE_ERROR_FATAL    = FLAG(1);

Type_Error type_check(Scope* scope, Ast** ast);