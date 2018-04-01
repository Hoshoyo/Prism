#include "type_infer.h"
#include "decl_check.h"

Decl_Error report_type_mismatch(Type_Instance* t1, Type_Instance* t2) {
	report_semantic_error(DECL_ERROR_FATAL, "type mismatch ");
	DEBUG_print_type(stderr, t1, true);
	fprintf(stderr, " vs ");
	DEBUG_print_type(stderr, t2, true);
	return DECL_ERROR_FATAL;
}

Type_Instance* type_strength_resolve(Type_Instance* t1, Type_Instance* t2, Ast* expr1, Ast* expr2, Decl_Error* error);
Type_Instance* infer_from_expression(Ast* expr, Decl_Error* error, bool rep_undeclared);
Type_Instance* infer_from_binary_expr(Ast* expr, Decl_Error* error, bool rep_undeclared);
Type_Instance* infer_from_unary_expr(Ast* expr, Decl_Error* error, bool rep_undeclared);
Type_Instance* infer_from_literal_expr(Ast* expr, Decl_Error* error, bool rep_undeclared);
Type_Instance* infer_from_variable_expr(Ast* expr, Decl_Error* error, bool rep_undeclared);
Type_Instance* infer_from_proc_call_expr(Ast* expr, Decl_Error* error, bool rep_undeclared);

Type_Instance* infer_from_binary_expr(Ast* expr, Decl_Error* error, bool rep_undeclared) {
	Type_Instance* left  = infer_from_expression(expr->expr_binary.left, error, rep_undeclared);
	Type_Instance* right = infer_from_expression(expr->expr_binary.right, error, rep_undeclared);

	if (!left || !right)
		return 0;

	switch (expr->expr_binary.op) {
		case OP_BINARY_MOD:{
			if (type_primitive_int(left) && type_primitive_int(right)) {
				Type_Instance* inferred = type_strength_resolve(left, right, expr->expr_binary.left, expr->expr_binary.right, error);
				if (*error & DECL_ERROR_FATAL) return 0;
				return inferred;
			}
			report_error_location(expr);
			*error |= report_type_error(DECL_ERROR_FATAL, "binary modulo operator is not defined for the type '");
			DEBUG_print_type(stderr, left, true);
			fprintf(stderr, "'\n");
		}break;
		case OP_BINARY_PLUS:
		case OP_BINARY_MINUS: {
			if ((left->kind == KIND_POINTER && type_primitive_int(right)) ||
				((left->kind == KIND_POINTER) && (right->kind == KIND_POINTER) && (left == right))) 
			{
				assert(left->flags & TYPE_FLAG_INTERNALIZED);
				return left;
			}
		}
		case OP_BINARY_MULT:
		case OP_BINARY_DIV: {
			if ((type_primitive_int(left) && type_primitive_int(right)) ||
				(type_primitive_float(left) && type_primitive_float(right))) 
			{
				Type_Instance* inferred = type_strength_resolve(left, right, expr->expr_binary.left, expr->expr_binary.right, error);
				if (*error & DECL_ERROR_FATAL) return 0;
				return inferred;
			}
			report_error_location(expr);
			if (type_hash(left) == type_hash(right)) {
				*error |= report_type_error(DECL_ERROR_FATAL, "binary arithmetic expressions are not defined for the type '");
				DEBUG_print_type(stderr, left, true);
				fprintf(stderr, "'\n");
			} else {
				*error |= report_type_mismatch(left, right);
				fprintf(stderr, "\n");
			}
		}break;
		case OP_BINARY_XOR: {
			if (type_primitive_bool(left) && type_primitive_bool(right)) {
				return left;
			}
		}
		case OP_BINARY_AND:
		case OP_BINARY_OR:
		case OP_BINARY_SHL:
		case OP_BINARY_SHR: {
			if (type_primitive_int(left) && type_primitive_int(right)) 
			{
				Type_Instance* inferred = type_strength_resolve(left, right, expr->expr_binary.left, expr->expr_binary.right, error);
				if (*error & DECL_ERROR_FATAL) return 0;
				return inferred;
			}
			report_error_location(expr);
			if (type_hash(left) == type_hash(right)) {
				*error |= report_type_error(DECL_ERROR_FATAL, "binary bitwise expressions are not defined for the type '");
				DEBUG_print_type(stderr, left, true);
				fprintf(stderr, "'\n");
			} else {
				*error |= report_type_mismatch(left, right);
				fprintf(stderr, "\n");
			}
		}break;

		case OP_BINARY_LOGIC_AND:
		case OP_BINARY_LOGIC_OR: {
			return type_primitive_get(TYPE_PRIMITIVE_BOOL);
		}break;

		case OP_BINARY_EQUAL:
		case OP_BINARY_NOT_EQUAL: {
			if (type_primitive_bool(left) && type_primitive_bool(right)) {
				return left;
			}
		}
		case OP_BINARY_GE:
		case OP_BINARY_GT:
		case OP_BINARY_LE:
		case OP_BINARY_LT: {
			if ((type_primitive_int(left) && type_primitive_int(right)) ||
				(type_primitive_float(left) && type_primitive_float(right))) 
			{
				Type_Instance* inferred = type_strength_resolve(left, right, expr->expr_binary.left, expr->expr_binary.right, error);
				if (*error & DECL_ERROR_FATAL) return 0;
				return inferred;
			} else {
				if (type_hash(left) == type_hash(right)) {
					return left;
				} else {
					report_error_location(expr);
					*error = report_type_mismatch(left, right);
					fprintf(stderr, "\n");
				}
			}
		}break;
		case OP_BINARY_DOT: {
			// TODO(psv): implement
			assert(0);
		}break;
	}
	return 0;
}

Type_Instance* infer_from_unary_expr(Ast* expr, Decl_Error* error, bool rep_undeclared) {
	assert(expr->node_type == AST_EXPRESSION_UNARY);

	switch (expr->expr_unary.op) {
		case OP_UNARY_CAST:{
			Type_Instance* res = resolve_type(expr->scope, expr->expr_unary.type_to_cast, rep_undeclared);
			if (!res) {
				*error |= report_type_error(DECL_ERROR_FATAL, "could not infer type of cast, type '");
				DEBUG_print_type(stderr, expr->expr_unary.type_to_cast, true);
				fprintf(stderr, "' is invalid\n");
			}
			return res;
		}break;
		case OP_UNARY_ADDRESSOF: {
			// TODO(psv): check if it can only ask for address of lvalue (strong type)
			Type_Instance* res = infer_from_expression(expr->expr_unary.operand, error, rep_undeclared);
			if (*error & DECL_ERROR_FATAL) return 0;
			Type_Instance* newtype = type_new_temporary();
			newtype->kind = KIND_POINTER;
			newtype->pointer_to = res;
			newtype->type_size_bits = type_pointer_size() * 8;
			newtype->flags = TYPE_FLAG_SIZE_RESOLVED;
			if (res->flags & TYPE_FLAG_INTERNALIZED) {
				newtype->flags |= TYPE_FLAG_RESOLVED;
				internalize_type(&newtype, true);
			} else {
				newtype->flags |= TYPE_FLAG_WEAK;
			}
			return newtype;
		}break;
		case OP_UNARY_VECTOR_ACCESSER:
		case OP_UNARY_DEREFERENCE: {
			Type_Instance* operand_type = infer_from_expression(expr->expr_unary.operand, error, rep_undeclared);
			if (*error & DECL_ERROR_FATAL) return 0;
			if (operand_type->kind != KIND_POINTER) {
				report_error_location(expr);
				*error |= report_type_error(DECL_ERROR_FATAL, "cannot dereference a non pointer type '");
				DEBUG_print_type(stderr, operand_type, true);
				fprintf(stderr, "'\n");
			}
			Type_Instance* res = operand_type->pointer_to;
			assert(res->flags & TYPE_FLAG_INTERNALIZED);
			return res;
		}break;
		case OP_UNARY_LOGIC_NOT: {
			Type_Instance* operand_type = infer_from_expression(expr->expr_unary.operand, error, rep_undeclared);
			if (*error & DECL_ERROR_FATAL) return 0;
			if (type_primitive_bool(operand_type)) {
				assert(operand_type->flags & TYPE_FLAG_INTERNALIZED);
				return operand_type;
			} else {
				*error |= report_type_error(DECL_ERROR_FATAL, "unary operator '!' can only be used in a boolean type\n");
				return 0;
			}
		}break;
		case OP_UNARY_BITWISE_NOT: {
			Type_Instance* operand_type = infer_from_expression(expr->expr_unary.operand, error, rep_undeclared);
			if (*error & DECL_ERROR_FATAL) return 0;
			if (type_primitive_int(operand_type)) {
				return operand_type;
			} else {
				*error |= report_type_error(DECL_ERROR_FATAL, "unary operator not '~' can only be used in integer types\n");
				return 0;
			}
		}break;
		case OP_UNARY_PLUS:
		case OP_UNARY_MINUS: {
			Type_Instance* operand_type = infer_from_expression(expr->expr_unary.operand, error, rep_undeclared);
			if (*error & DECL_ERROR_FATAL) return 0;
			if (type_primitive_int(operand_type) || type_primitive_float(operand_type)) {
				return operand_type;
			} else {
				*error |= report_type_error(DECL_ERROR_FATAL, "unary operators '+' or '-' can only be used in numeric types\n");
				return 0;
			}
		}break;
	}
	return 0;
}

Type_Instance* infer_from_literal_expr(Ast* expr, Decl_Error* error, bool rep_undeclared) {
	Type_Instance* result = type_new_temporary();
	result->flags = 0 | TYPE_FLAG_WEAK;
	switch (expr->expr_literal.type) {
		case LITERAL_BIN_INT:
		case LITERAL_HEX_INT: {
			result->kind = KIND_PRIMITIVE;
			result->primitive = TYPE_PRIMITIVE_U64;
		}break;
		case LITERAL_SINT: {
			result->kind = KIND_PRIMITIVE;
			result->primitive = TYPE_PRIMITIVE_S64;
		}break;
		case LITERAL_BOOL: {
			return type_primitive_get(TYPE_PRIMITIVE_BOOL);
		}break;
		case LITERAL_FLOAT: {
			result->kind = KIND_PRIMITIVE;
			result->primitive = TYPE_PRIMITIVE_R64;
		}break;
		case LITERAL_STRUCT:
		case LITERAL_ARRAY: assert(0); break;	// TODO(psv): not implemented yet
	}
	return result;
}

Type_Instance* infer_from_variable_expr(Ast* expr, Decl_Error* error, bool rep_undeclared) {
	assert(expr->node_type == AST_EXPRESSION_VARIABLE);

	Ast* decl = decl_from_name(expr->scope, expr->expr_variable.name);
	if (!decl) {
		if(rep_undeclared)
			*error |= report_undeclared(expr->expr_variable.name);
		return 0;
	}
	if (decl->node_type != AST_DECL_VARIABLE && decl->node_type != AST_DECL_CONSTANT) {
		*error |= report_semantic_error(DECL_ERROR_FATAL, 
			"could not infer type from name '%.*s', it is not a variable nor constant\n", TOKEN_STR(expr->expr_variable.name));
		return 0;
	}
	Type_Instance* type = decl->decl_variable.variable_type;
	return type;
}

Type_Instance* infer_from_proc_call_expr(Ast* expr, Decl_Error* error, bool rep_undeclared) {
	Ast* decl = decl_from_name(expr->scope, expr->expr_proc_call.name);
	if (!decl) {
		*error |= report_undeclared(expr->expr_proc_call.name);
		return 0;
	}
	if (decl->node_type != AST_DECL_PROCEDURE) {
		*error |= report_semantic_error(DECL_ERROR_FATAL, 
			"'%.*s' used in a procedure call, but is not a procedure\n", TOKEN_STR(expr->expr_proc_call.name));
		return 0;
	}
	Type_Instance* type = decl->decl_procedure.type_return;
	assert(type->flags & TYPE_FLAG_INTERNALIZED);
	return type;
}

Type_Instance* infer_from_expression(Ast* expr, Decl_Error* error, bool rep_undeclared) {
	assert(expr->flags & AST_FLAG_IS_EXPRESSION);
	
	Type_Instance* type = 0;
	
	switch (expr->node_type) {
		case AST_EXPRESSION_BINARY:				type = infer_from_binary_expr(expr, error, rep_undeclared); break;
		case AST_EXPRESSION_UNARY:				type = infer_from_unary_expr(expr, error, rep_undeclared); break;
		case AST_EXPRESSION_LITERAL:			type = infer_from_literal_expr(expr, error, rep_undeclared); break;
		case AST_EXPRESSION_VARIABLE:			type = infer_from_variable_expr(expr, error, rep_undeclared); break;
		case AST_EXPRESSION_PROCEDURE_CALL:		type = infer_from_proc_call_expr(expr, error, rep_undeclared); break;
	}
	return type;
}

// Resolve the strength of a type TODO(psv): "and updated its children when necessary."
// the returned type is always the correct type, unless an error occurred, then this function returns 0
Type_Instance* type_strength_resolve(Type_Instance* t1, Type_Instance* t2, Ast* expr1, Ast* expr2, Decl_Error* error) {
	if (t1->flags & TYPE_FLAG_WEAK && t2->flags & TYPE_FLAG_WEAK) {
		if (type_hash(t1) == type_hash(t2)) {
			return t1;
		} else {
			report_error_location(expr1);
			*error |= report_semantic_error(DECL_ERROR_FATAL, "could not infer type from two different weak types: ");
			DEBUG_print_type(stderr, t1, true);
			fprintf(stderr, " and ");
			DEBUG_print_type(stderr, t2, true);
			fprintf(stderr, "\n");
			return 0;
		}
	}

	if (t1->flags & TYPE_FLAG_WEAK && t2->flags & TYPE_FLAG_STRONG) {
		if (type_hash(t1) == type_hash(t2)) return t2;
		// transform t1 into t2 and update expr1
		assert(0);
		return t2;
	}
	if (t1->flags & TYPE_FLAG_STRONG && t2->flags & TYPE_FLAG_WEAK) {
		if (type_hash(t1) == type_hash(t2)) return t1;

		// transform t2 into t1 and update expr2
		assert(0);
		return t1;
	}

	if (t1->flags & TYPE_FLAG_STRONG && t2->flags & TYPE_FLAG_STRONG) {
		if (t1 == t2) return t1;

		report_error_location(expr1);
		*error |= report_semantic_error(DECL_ERROR_FATAL, "type mismatch ");
		DEBUG_print_type(stderr, t1, true);
		fprintf(stderr, " vs ");
		DEBUG_print_type(stderr, t2, true);
		fprintf(stderr, "\n");
		return 0;
	}
}