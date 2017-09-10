#include "semantic.h"
#include <stdarg.h>

static bool semantic_error = 0;
static int scope_error = 0;
static int declaration_ratio = 8;

#define PRINT_SCOPE_INFO 1

#define TOKEN_STR(X) X->value.length, X->value.data

Infer_Node* infer_queue = 0;

static int report_semantic_error(Decl_Site* site, char* msg, ...)
{
	va_list args;
	va_start(args, msg);
	if (site) {
		fprintf(stderr, "%.*s:%d:%d: Semantic Error: ", site->filename.length, site->filename.data, site->line, site->column);
	}
	vfprintf(stderr, msg, args);
	va_end(args);
	semantic_error = true;
	return 0;
}

static int report_semantic_error(Expr_Site site, char* msg, ...)
{
	va_list args;
	va_start(args, msg);

	fprintf(stderr, "%.*s:%d:%d: Semantic Error: ", site.filename.length, site.filename.data, site.line, site.column);

	vfprintf(stderr, msg, args);
	va_end(args);
	semantic_error = true;
	return 0;
}

static int report_semantic_warning(Decl_Site* site, char* msg, ...)
{
	va_list args;
	va_start(args, msg);
	if (site) {
		fprintf(stderr, "%.*s:%d:%d: Warning: ", site->filename.length, site->filename.data, site->line, site->column);
	}
	vfprintf(stderr, msg, args);
	va_end(args);
	return 0;
}

static void report_semantic_type_mismatch(Expr_Site site, Type_Instance* i1, Type_Instance* i2) {
	report_semantic_error(site, "Type mismatch ");
	DEBUG_print_type(stderr, i1);
	fprintf(stderr, " vs ");
	DEBUG_print_type(stderr, i2);
}

// returns the number of chars before tok
static int print_token_line(FILE* out, Token* tok) {
	char* at = tok->value.data;
	int i = 0;
	for (; at[i] != 0 && at[i] != '\n'; ++i);
	return fprintf(out, "%.*s\n", i, at);
}

static Expr_Site get_site_from_token(Token* t)
{
	Expr_Site site;
	site.filename = t->filename;
	site.line = t->line;
	site.column = t->column;
	return site;
}

static void report_declaration_site(Ast* node) {
	assert(node->is_decl);
	Decl_Site* site = 0;
	Token* name = 0;
	switch (node->node) {
		case AST_NODE_VARIABLE_DECL: {
			site = &node->var_decl.site;
			name = node->var_decl.name;
		}break;
		case AST_NODE_PROC_DECLARATION: {
			site = &node->proc_decl.site;
			name = node->proc_decl.name;
		}break;
		case AST_NODE_STRUCT_DECLARATION: {
			site = &node->struct_decl.site;
			name = node->struct_decl.name;
		}break;
	}
	int num = fprintf(stderr, "%.*s:%d:%d: ", site->filename.length, site->filename.data, site->line, site->column);
	//fprintf(stderr, "%.*s\n", name->value.length, name->value.data);
	print_token_line(stderr, name);
	while (num > 0) {
		fprintf(stderr, "-");
		num -= 1;
	}
	fprintf(stderr, "^   Previously defined here.\n");
}

int check_and_submit_declarations(Ast* node, Scope* scope) {
	if (node->is_decl) {
		switch (node->node) {
		case AST_NODE_VARIABLE_DECL: {
			assert(scope->symb_table);
			//
			// check if variable exist on this scope
			//
			s64 hash = scope->symb_table->entry_exist(node->var_decl.name);
			if (hash == -1) {
				hash = scope->symb_table->insert(scope, node->var_decl.name, node);
			} else {
				report_semantic_error(&node->var_decl.site, "Variable %.*s redefinition.\n", node->var_decl.name->value.length, node->var_decl.name->value.data);
				report_declaration_site(scope->symb_table->entries[hash].node);
				return 0;
			}
		}break;
		case AST_NODE_PROC_DECLARATION: {
			assert(scope->symb_table);
			//
			// check if the proc decl is not redefined in the same scope
			//
			s64 hash = scope->symb_table->entry_exist(node->proc_decl.name);
			if (hash == -1) {
				hash = scope->symb_table->insert(scope, node->proc_decl.name, node);
			} else {
				report_semantic_error(&node->proc_decl.site, "Variable %.*s redefinition.\n", node->proc_decl.name->value.length, node->proc_decl.name->value.data);
				report_declaration_site(scope->symb_table->entries[hash].node);
				return 0;
			}
			//
			// check for duplicate arguments
			//
			int num_args = node->proc_decl.num_args;
			int error = DECL_CHECK_PASSED;
			if (num_args > 0) {
				node->proc_decl.scope->symb_table = new Symbol_Table(num_args * declaration_ratio);
#if PRINT_SCOPE_INFO
				printf("Proc (%d - level[%d]) has %d arguments.\n", node->proc_decl.scope->id, node->proc_decl.scope->level, node->proc_decl.scope->num_declarations);
#endif
				for (int i = 0; i < num_args; ++i) {
					int ret = check_and_submit_declarations(node->proc_decl.arguments[i], node->proc_decl.scope);
					if (ret != DECL_CHECK_PASSED) error = DECL_CHECK_FAILED;
				}
			}
			int ret = check_and_submit_declarations(node->proc_decl.body, node->proc_decl.scope);
			if (ret != DECL_CHECK_PASSED) error = DECL_CHECK_FAILED;
			if (error == DECL_CHECK_FAILED) return DECL_CHECK_FAILED;
		}break;
		case AST_NODE_STRUCT_DECLARATION: {
			Scope* struct_scope = node->struct_decl.scope;
#if PRINT_SCOPE_INFO
			printf("Struct (%d - level[%d]) has %d declarations.\n", struct_scope->id, struct_scope->level, struct_scope->num_declarations);
#endif
		}break;

		case AST_NODE_NAMED_ARGUMENT: {
			//
			// check for duplicate named arguments from within a function
			//
			assert(scope->symb_table);
			s64 hash = scope->symb_table->entry_exist(node->named_arg.arg_name);
			if (hash == -1) {
				hash = scope->symb_table->insert(scope, node->named_arg.arg_name, node);
			} else {
				report_semantic_error(&node->named_arg.site, "Procedure argument %.*s redefinition.\n", node->named_arg.arg_name->value.length, node->named_arg.arg_name->value.data);
				fprintf(stderr, "Previously defined as argument #%d in the procedure.\n", scope->symb_table->entries[hash].node->named_arg.index + 1);
				return 0;
			}
		}break;

		default: {
			// should not be reached
			report_semantic_error(0, "Internal compiler error: unknown declaration node type.\n");
			scope_error = 1;
			return 0;
		}break;
		}
	}
	if (node->node == AST_NODE_BLOCK) {
		//
		// check recursively for all the declarations and commands of this block
		//
		Scope* block_scope = node->block.scope;
#if PRINT_SCOPE_INFO
		printf("Block (%d - level[%d]) has %d declarations.\n", block_scope->id, block_scope->level, block_scope->num_declarations);
#endif

		if (block_scope->num_declarations) {
			// if there is at least 1 declaration, then create a symbol table (declaration ratio) times
			// bigger than the number of declarations

			block_scope->symb_table = new Symbol_Table(declaration_ratio * block_scope->num_declarations);
		}
		return check_and_submit_declarations(node->block.commands, block_scope);
	}
	if (node->node == AST_NODE_IF_STATEMENT) {
		int ret = 1;
		if (node->if_stmt.body->node == AST_NODE_BLOCK) {
			ret = check_and_submit_declarations(node->if_stmt.body, scope);
		}
		if (node->if_stmt.else_exp && node->if_stmt.else_exp->node == AST_NODE_BLOCK) {
			if (ret == 0) {
				ret = check_and_submit_declarations(node->if_stmt.else_exp, scope);
			}
		}
		return ret;
	}

	if (node->node == AST_NODE_WHILE_STATEMENT) {
		if (node->while_stmt.body->node == AST_NODE_BLOCK) {
			return check_and_submit_declarations(node->while_stmt.body, scope);
		}
	}

	return 1;
}

// return 0 if var is not yet declared
Type_Instance* get_variable_type(Ast* node, Scope* scope)
{
	assert(node->node == AST_NODE_VARIABLE_EXPRESSION);
	s64 entry = -1;
	if(scope->num_declarations > 0)
		entry = scope->symb_table->entry_exist(node->expression.variable_exp.name);
	if (entry == -1) {
		// search on previous scopes
		while(scope != 0) {
			if (!scope->symb_table) {
				scope = scope->parent;
				continue;
			}
			entry = scope->symb_table->entry_exist(node->expression.variable_exp.name);
			if (entry == -1) {
				scope = scope->parent;
				continue;
			}
			else break;
			if (scope->level == 0) break;
			scope = scope->parent;
		}
		if (entry == -1) {
			return 0;
		}
	}
	Ast* decl_node = scope->symb_table->entries[entry].node;
	if (decl_node->node == AST_NODE_VARIABLE_DECL) {
		return decl_node->var_decl.type;
	} else if (decl_node->node == AST_NODE_PROC_DECLARATION) {
		return decl_node->proc_decl.proc_type;
	} else if(decl_node->node == AST_NODE_NAMED_ARGUMENT) {
		return decl_node->named_arg.arg_type;
	} else if(decl_node->node == AST_NODE_STRUCT_DECLARATION) {
		Decl_Site site;
		site.filename = node->expression.variable_exp.name->filename;
		site.column = node->expression.variable_exp.name->column;
		site.line = node->expression.variable_exp.name->line;
		report_semantic_error(&site, "Variable '%.*s' is a struct typename and can not be addressed.\n");
		report_declaration_site(decl_node);
		return 0;
	}
}

// return the declaration node if exists
// return 0 if dost not exist
Ast* is_declared(Ast* node)
{
	assert(node->node == AST_NODE_VARIABLE_EXPRESSION);
	Scope* scope = node->expression.variable_exp.scope;
	do {
		if (scope && scope->num_declarations > 0) {
			s64 index = scope->symb_table->entry_exist(node->expression.variable_exp.name);
			if (index != -1) {
				return scope->symb_table->entries[index].node;
			}
		}
		scope = scope->parent;
	} while (scope != 0);
	return 0;
}

int check_and_submit_declarations(Ast** ast, Scope* global_scope)
{
	if (global_scope->num_declarations == 0) return DECL_CHECK_PASSED;

	size_t num_nodes = get_arr_length(ast);
	global_scope->symb_table = new Symbol_Table(global_scope->num_declarations * declaration_ratio);

	for (size_t i = 0; i < num_nodes; ++i) {
		Ast* node = ast[i];
		if (check_and_submit_declarations(node, global_scope) == 0) {
			return DECL_CHECK_FAILED;
		}
	}
	return DECL_CHECK_PASSED;
}

// do type coercion on both sides coercing up always
// return 0 if could not coerce
// return type coerced if success
// if strict, then only upcoerce, i1 being the type to be coerced
Type_Instance* do_type_coercion(Type_Instance* i1, Type_Instance* i2, bool strict)
{
	if (i1->type == TYPE_PRIMITIVE) {
		switch (i1->primitive) {
			// signed integer
			case TYPE_PRIMITIVE_S64: {
				if (i2->primitive == TYPE_PRIMITIVE_S64) {
					return i1;
				}
				if (i2->primitive == TYPE_PRIMITIVE_S32 || i2->primitive == TYPE_PRIMITIVE_S16 || i2->primitive == TYPE_PRIMITIVE_S8) {
					if (strict) return 0;
					return i1;
				}
			}break;
			case TYPE_PRIMITIVE_S32: {
				if (i2->primitive == TYPE_PRIMITIVE_S16 || i2->primitive == TYPE_PRIMITIVE_S8) {
					if (strict) return 0;
					return i1;
				}
				if (i2->primitive == TYPE_PRIMITIVE_S64 || i2->primitive == TYPE_PRIMITIVE_S32) {
					return i2;
				}
			}break;
			case TYPE_PRIMITIVE_S16: {
				if (i2->primitive == TYPE_PRIMITIVE_S8) {
					if (strict) return 0;
					return i1;
				}
				if (i2->primitive == TYPE_PRIMITIVE_S32 || i2->primitive == TYPE_PRIMITIVE_S64 || i2->primitive == TYPE_PRIMITIVE_S16) {
					return i2;
				}
			}break;
			case TYPE_PRIMITIVE_S8: {
				if (i2->primitive == TYPE_PRIMITIVE_S8) {
					return i1;
				}
				if (i2->primitive == TYPE_PRIMITIVE_S64 || i2->primitive == TYPE_PRIMITIVE_S32 || i2->primitive == TYPE_PRIMITIVE_S16) {
					return i2;
				}
			}break;

			// unsigned integer
			case TYPE_PRIMITIVE_U64: {
				if (i2->primitive == TYPE_PRIMITIVE_U64) {
					return i1;
				}
				if (i2->primitive == TYPE_PRIMITIVE_U32 || i2->primitive == TYPE_PRIMITIVE_U16 || i2->primitive == TYPE_PRIMITIVE_U8) {
					if (strict) return 0;
					return i1;
				}
			}break;
			case TYPE_PRIMITIVE_U32: {
				if (i2->primitive == TYPE_PRIMITIVE_U16 || i2->primitive == TYPE_PRIMITIVE_U8) {
					if (strict) return 0;
					return i1;
				}
				if (i2->primitive == TYPE_PRIMITIVE_U64 || i2->primitive == TYPE_PRIMITIVE_U32) {
					return i2;
				}
			}break;
			case TYPE_PRIMITIVE_U16: {
				if (i2->primitive == TYPE_PRIMITIVE_U8) {
					if (strict) return 0;
					return i1;
				}
				if (i2->primitive == TYPE_PRIMITIVE_U32 || i2->primitive == TYPE_PRIMITIVE_U64 || i2->primitive == TYPE_PRIMITIVE_U16) {
					return i2;
				}
			}break;
			case TYPE_PRIMITIVE_U8: {
				if (i2->primitive == TYPE_PRIMITIVE_U8) {
					return i1;
				}
				if (i2->primitive == TYPE_PRIMITIVE_U64 || i2->primitive == TYPE_PRIMITIVE_U32 || i2->primitive == TYPE_PRIMITIVE_U16) {
					return i2;
				}
			}break;

			case TYPE_PRIMITIVE_R32: {
				if (i2->primitive == TYPE_PRIMITIVE_R32) return i1;
				if (i2->primitive == TYPE_PRIMITIVE_R64 && !strict) {
					return i2;
				} else {
					return 0;
				}
			}break;

			case TYPE_PRIMITIVE_R64: {
				if (i2->primitive == TYPE_PRIMITIVE_R32) {
					if (strict) return i2;
					return i1;
				}
				if (i2->primitive == TYPE_PRIMITIVE_R64) return i1;
			}break;

			case TYPE_PRIMITIVE_BOOL: {
				if (i2->primitive == TYPE_PRIMITIVE_BOOL) return i1;
				return 0;
			}break;

			case TYPE_PRIMITIVE_VOID: {
				if (i2->primitive == TYPE_PRIMITIVE_VOID) return i1;
				return 0;
			}break;
		}
	} else if (i1->type == TYPE_POINTER || i1->type == TYPE_FUNCTION || i1->type == TYPE_STRUCT) {
		if (types_equal(i1, i2)) return i1;
		else return 0;
	}
	return 0;
}

Type_Instance* get_decl_type(Ast* node);

void push_infer_queue(Ast* node)
{
	Infer_Node infer_node;
	if (node->return_type) {
		node->return_type->flags |= TYPE_FLAG_QUEUED;
	}
	if (node->is_decl) {
		Type_Instance* ti = get_decl_type(node);
		if(ti)
			ti->flags |= TYPE_FLAG_NOT_DELETE;
	}
	infer_node.node = node;
	push_array(infer_queue, &node);
}

// returns 0 if failed to resolve
// returns non-zero if resolved
// this function only changed type flags
int resolve_type(Type_Instance* instance, Type_Table* type_table)
{
	//assert(!(instance->flags & TYPE_FLAG_IS_RESOLVED));

	switch (instance->type) {
	case TYPE_PRIMITIVE: {
		return 1;
	}break;
	case TYPE_POINTER: {
		assert(instance->flags & TYPE_FLAG_IS_SIZE_RESOLVED);
		assert(instance->flags & TYPE_FLAG_IS_REGISTER_SIZE);
		if (!resolve_type(instance->pointer_to, type_table)) {
			return 0;
		}
		instance->flags |= TYPE_FLAG_IS_RESOLVED;
		return 1;
	}break;

	case TYPE_FUNCTION: {
		// if could not resolve type of return, fail

		if (!(instance->type_function.return_type->flags & TYPE_FLAG_IS_RESOLVED) &&
			!resolve_type(instance->type_function.return_type, type_table)) {
			return 0;
		}
		// try resolving the type for all the arguments
		int num_args = instance->type_function.num_arguments;
		for (int i = 0; i < num_args; ++i) {
			Type_Instance* in = instance->type_function.arguments_type[i];
			if (!(in->flags & TYPE_FLAG_IS_RESOLVED) && !resolve_type(in, type_table)) {
				return 0;
			}
			in->flags |= TYPE_FLAG_IS_RESOLVED;
		}
		instance->flags |= TYPE_FLAG_IS_RESOLVED;
		return 1;
	}break;

	case TYPE_STRUCT: {
		s64 hash = -1;
		// if the entry exist, that means the structure was declared and it is on the type table
		if (type_table->entry_exist(instance, &hash)) {
			Type_Instance* in = type_table->get_entry(hash);
			assert(in->flags & TYPE_FLAG_IS_RESOLVED);
			assert(in->flags & TYPE_FLAG_IS_SIZE_RESOLVED);
			instance->flags |= TYPE_FLAG_IS_SIZE_RESOLVED;
			instance->flags |= TYPE_FLAG_IS_RESOLVED;
		} else {
			return 0;
		}
	}break;

	default: 
		assert(0); break;	// @todo
	}
}

Type_Instance* get_decl_type(Ast* node)
{
	assert(node->is_decl);
	switch (node->node) {
	case AST_NODE_VARIABLE_DECL:      return node->var_decl.type;
	case AST_NODE_NAMED_ARGUMENT:     return node->named_arg.arg_type;
	case AST_NODE_PROC_DECLARATION:   return node->proc_decl.proc_type;
	case AST_NODE_STRUCT_DECLARATION: return node->struct_decl.type_info;

	default: assert(0); // @todo
	}
}

Token* get_decl_name(Ast* node)
{
	assert(node->is_decl);
	switch (node->node) {
	case AST_NODE_VARIABLE_DECL:      return node->var_decl.name;
	case AST_NODE_NAMED_ARGUMENT:     return node->named_arg.arg_name; 
	case AST_NODE_PROC_DECLARATION:   return node->proc_decl.name;
	case AST_NODE_STRUCT_DECLARATION: return node->struct_decl.name;

	default: assert(0); // @todo
	}
	return 0;
}

int infer_node_decl_types(Ast* node, Type_Table* table);

int infer_node_expr_type(Ast* node, Type_Table* table, Type_Instance* check_against, Type_Instance** result = 0, Type_Instance** required = 0);

// return 0 if success
// return 1 if could not infer
// return -1 if error
int infer_binary_expr_type(Ast* node, Type_Table* table, Type_Instance* check_against, Type_Instance** result = 0, Type_Instance** required = 0)
{
	/*
	if (check_against) {
		switch (node->expression.binary_exp.op) {
		case BINARY_OP_PLUS:
		case BINARY_OP_MINUS: {
			if (check_against->type == TYPE_POINTER) {
				// pointer arithmetic
				break;
			}
		}
		case BINARY_OP_MULT:
		case BINARY_OP_DIV:
		case BINARY_OP_MOD: {
			assert(0);
		} break;

		case BINARY_OP_BITSHIFT_LEFT:
		case BINARY_OP_BITSHIFT_RIGHT:
		case BINARY_OP_AND:
		case BINARY_OP_OR:
		case BINARY_OP_XOR:

		case BINARY_OP_LOGIC_AND:
		case BINARY_OP_LOGIC_OR:

		case BINARY_OP_LESS_THAN:
		case BINARY_OP_GREATER_THAN:
		case BINARY_OP_LESS_EQUAL:
		case BINARY_OP_GREATER_EQUAL:
		case BINARY_OP_EQUAL_EQUAL:
		case BINARY_OP_NOT_EQUAL: {

		} break;

		case BINARY_OP_DOT:

		case ASSIGNMENT_OPERATION_EQUAL:
		case ASSIGNMENT_OPERATION_PLUS_EQUAL:
		case ASSIGNMENT_OPERATION_MINUS_EQUAL:
		case ASSIGNMENT_OPERATION_TIMES_EQUAL:
		case ASSIGNMENT_OPERATION_DIVIDE_EQUAL:
		case ASSIGNMENT_OPERATION_AND_EQUAL:
		case ASSIGNMENT_OPERATION_OR_EQUAL:
		case ASSIGNMENT_OPERATION_XOR_EQUAL:
		case ASSIGNMENT_OPERATION_SHL_EQUAL:
		case ASSIGNMENT_OPERATION_SHR_EQUAL:
			assert(0);
			break;
		}
	}
	*/

	{
		// this is supposed to be infered
		switch (node->expression.binary_exp.op) {
		case BINARY_OP_PLUS:
		case BINARY_OP_MINUS: {
			if (check_against && check_against->type == TYPE_POINTER) {
				// pointer arithmetic
				break;
			}
		}
		case BINARY_OP_MULT:
		case BINARY_OP_DIV:
		case BINARY_OP_MOD: {
			// both sides need to be the same type
			Type_Instance* left = 0;
			Type_Instance* right = 0;
			Type_Instance* required_left = 0;
			Type_Instance* required_right = 0;
			int l = infer_node_expr_type(node->expression.binary_exp.left, table, check_against, &left, &required_left);
			int r = infer_node_expr_type(node->expression.binary_exp.right, table, check_against, &right, &required_right);
			if (l == 0 && r == 0) {
				if (types_equal(left, right)) {
					if (left->flags & TYPE_FLAG_IS_RESOLVED) {
						node->return_type = left;
						if(result)
							*result = left;
						if ((required_left || required_right) && required)
							*required = left;
						return 0;
					} else {
						return 1;
					}
				} else {
					if (required_left && required_right) {
						report_semantic_type_mismatch(get_site_from_token(node->expression.binary_exp.op_token), required_left, required_right);
						fprintf(stderr, " on binary expression '%.*s'\n", TOKEN_STR(node->expression.binary_exp.op_token));
						return -1;
					}
					Type_Instance* coerced = 0;
					if (required_left) {
						coerced = do_type_coercion(right, left, true);
						if (coerced) {
							if(required)
								*required = coerced;
						}
					} else if (required_right) {
						coerced = do_type_coercion(left, right, true);
						if (coerced) {
							if(required)
								*required = coerced;
						}
					} else {
						coerced = do_type_coercion(left, right, false);
					}
					if (coerced) {
						if(result)
							*result = coerced;
						node->return_type = coerced;
						return 0;
					} else {
						report_semantic_type_mismatch(get_site_from_token(node->expression.binary_exp.op_token), left, right);
						fprintf(stderr, " on binary expression '%.*s'\n", TOKEN_STR(node->expression.binary_exp.op_token));
						return -1;
					}
				}
			} else if (l == -1 || r == -1) {
				return -1;
			} else if (l == 1 || r == 1) {
				return 1;
			}
		} break;

		case BINARY_OP_BITSHIFT_LEFT:
		case BINARY_OP_BITSHIFT_RIGHT:
		case BINARY_OP_AND:
		case BINARY_OP_OR:
		case BINARY_OP_XOR:

		case BINARY_OP_LOGIC_AND:
		case BINARY_OP_LOGIC_OR:
		case BINARY_OP_LESS_THAN:
		case BINARY_OP_GREATER_THAN:
		case BINARY_OP_LESS_EQUAL:
		case BINARY_OP_GREATER_EQUAL:
		case BINARY_OP_EQUAL_EQUAL:
		case BINARY_OP_NOT_EQUAL:

		case BINARY_OP_DOT:

		case ASSIGNMENT_OPERATION_EQUAL:
		case ASSIGNMENT_OPERATION_PLUS_EQUAL:
		case ASSIGNMENT_OPERATION_MINUS_EQUAL:
		case ASSIGNMENT_OPERATION_TIMES_EQUAL:
		case ASSIGNMENT_OPERATION_DIVIDE_EQUAL:
		case ASSIGNMENT_OPERATION_AND_EQUAL:
		case ASSIGNMENT_OPERATION_OR_EQUAL:
		case ASSIGNMENT_OPERATION_XOR_EQUAL:
		case ASSIGNMENT_OPERATION_SHL_EQUAL:
		case ASSIGNMENT_OPERATION_SHR_EQUAL:
			assert(0);
			break;
		}
	}
}

// return 0 if infered and checked correctly
// return 1 if could not resolve yet
// return -1 if check error
int infer_node_expr_type(Ast* node, Type_Table* table, Type_Instance* check_against, Type_Instance** result, Type_Instance** required)
{
	assert(!node->is_decl);

	switch (node->node)
	{
		case AST_NODE_PROCEDURE_CALL: {
			int ret_val = 0;
			Scope* aux_scope = node->expression.proc_call.scope;
			Token* proc_name = node->expression.proc_call.name;
			s64 index = -1;
			do {
				// if there is no symbol table, and therefore no declarations, skip
				if (aux_scope->symb_table > 0) {
					index = aux_scope->symb_table->entry_exist(proc_name);
					if (index != -1) {
						// found
						Type_Instance* type_instance = get_decl_type(aux_scope->symb_table->entries[index].node);
						if (type_instance && type_instance->type != TYPE_FUNCTION) {
							Decl_Site site;
							site.column = proc_name->column;
							site.line = proc_name->line;
							site.filename = proc_name->filename;
							report_semantic_error(&site, "Type of procedure '%.*s' call does not match with type of declaration.\n", TOKEN_STR(proc_name));
							report_declaration_site(aux_scope->symb_table->entries[index].node);
							return -1;
						} else {
							if (type_instance && type_instance->flags & TYPE_FLAG_IS_RESOLVED) {
								node->return_type = type_instance->type_function.return_type;
								if(required) *required = node->return_type;
								if(result) *result = node->return_type;
							} else {
								return 1;
							}
						}
						int args_ret_val = 0;
						int proc_decl_num_args = type_instance->type_function.num_arguments;

						if (node->expression.proc_call.args) {
							int proc_call_num_args = get_arr_length(node->expression.proc_call.args);
							if (proc_call_num_args != proc_decl_num_args) {
								if (proc_decl_num_args < proc_call_num_args)
									report_semantic_error(get_site_from_token(node->expression.proc_call.name), "Type mismatch: too many arguments for '%.*s' procedure call, expected #%d arguments got #%d",
										TOKEN_STR(node->expression.proc_call.name), proc_decl_num_args, proc_call_num_args);
								else
									report_semantic_error(get_site_from_token(node->expression.proc_call.name), "Type mismatch: too few arguments for '%.*s' procedure call, expected #%d arguments got #%d",
										TOKEN_STR(node->expression.proc_call.name), proc_decl_num_args, proc_call_num_args);
								return -1;
							}
							for (int i = 0; i < proc_decl_num_args; ++i) {
								int error = infer_node_expr_type(node->expression.proc_call.args[i], table, type_instance->type_function.arguments_type[i], result, required);
								if(error == -1){
									report_semantic_error(0, " in argument #%d of '%.*s' procedure call.\n", i + 1, TOKEN_STR(node->expression.proc_call.name));
								}
								if (error) args_ret_val = error;
							}
						}
						if (args_ret_val == 1 || args_ret_val == -1) ret_val = args_ret_val;
						return ret_val;
					}
				}
				aux_scope = aux_scope->parent;
			} while (aux_scope != 0);
			assert(0);	// internal compiler error
			return -1;
		}break;

		case AST_NODE_RETURN_STATEMENT: {
			node->return_type = get_primitive_type(TYPE_PRIMITIVE_VOID);
			Scope* decl_scope = node->ret_stmt.scope;
			do {
				if (decl_scope->flags & SCOPE_FLAG_PROC_SCOPE) {
					Ast* proc_decl = decl_scope->decl_node;
					if (proc_decl->proc_decl.proc_ret_type->flags & TYPE_FLAG_IS_RESOLVED) {
						int err = infer_node_expr_type(node->ret_stmt.expr, table, proc_decl->proc_decl.proc_ret_type);
						return err;
					} else {
						// decide what to do, if the return type of this function is not yet infered
						return 1;
					}
				}
				decl_scope = decl_scope->parent;
			} while (decl_scope != 0);

			// if got here return is not inside a procedure @todo report error
			return -1;
		}break;

		case AST_NODE_BINARY_EXPRESSION: {
			return infer_binary_expr_type(node, table, check_against, result, required);
		}break;
			/*
		case AST_NODE_UNARY_EXPRESSION: {
			switch (node->expression.unary_exp.op) {
			case UNARY_OP_MINUS:			return node->expression.unary_exp.operand->return_type;
			case UNARY_OP_PLUS:				return node->expression.unary_exp.operand->return_type;
			case UNARY_OP_DEREFERENCE: {
				Type_Instance* inst = node->expression.unary_exp.operand->return_type;
				s64 hash = create_type(&inst->pointer_to, false);
				return table->entries[hash].entry;
			}break;
			case UNARY_OP_ADDRESS_OF: {
				Type_Instance* inst = node->expression.unary_exp.operand->return_type;
				Type_Instance* ti = create_ptr_typeof(inst);
				return ti;
			}break;
			case UNARY_OP_VECTOR_ACCESS: assert(0); break;
			case UNARY_OP_NOT_LOGICAL:		return get_primitive_type(TYPE_PRIMITIVE_BOOL);
			case UNARY_OP_NOT_BITWISE:		return node->expression.unary_exp.operand->return_type;
			case UNARY_OP_CAST:				return node->expression.unary_exp.cast_type;
			}
		}break;
			*/
		
		case AST_NODE_LITERAL_EXPRESSION: {
			u32 tok_type = node->expression.literal_exp.lit_tok->type;
			Type_Instance* prim = 0;
			int inttype = is_integer_type(check_against);

			if (tok_type == TOKEN_INT_LITERAL) {
				if(inttype == 2)
					prim = get_primitive_type(TYPE_PRIMITIVE_U32);
				else
					prim = get_primitive_type(TYPE_PRIMITIVE_S32);
			}
			else if (tok_type == TOKEN_FLOAT_LITERAL) {
				prim = get_primitive_type(TYPE_PRIMITIVE_R32);
			}
			else if (tok_type == TOKEN_CHAR_LITERAL) {
				prim = get_primitive_type(TYPE_PRIMITIVE_S32);
			}
			else if (tok_type == TOKEN_BOOL_LITERAL) {
				prim = get_primitive_type(TYPE_PRIMITIVE_BOOL);
			}
			else if (tok_type == TOKEN_STRING_LITERAL) {
				assert(0);	// @todo
			}
			if (check_against) {
				Type_Instance* coerced = do_type_coercion(check_against, prim, false);
				if (coerced) {
					node->expression.literal_exp.type = coerced;
					node->return_type = coerced;
				} else {
					report_semantic_type_mismatch(get_site_from_token(node->expression.literal_exp.lit_tok), check_against, prim);
					return -1;
				}
			} else {
				node->return_type = prim;
			}
			if(result) *result = node->return_type;
			return 0;
		}break;
		case AST_NODE_VARIABLE_EXPRESSION: {
			Type_Instance* prim = 0;
			if (!is_declared(node)) {
				report_semantic_error(get_site_from_token(node->expression.variable_exp.name), "Undeclared variable '%.*s'\n", TOKEN_STR(node->expression.variable_exp.name));
				return -1;
			}
			prim = get_variable_type(node, node->expression.variable_exp.scope);
			if (!prim) {
				return 1;
			} else {
				if (check_against) {
					Type_Instance* coerced = do_type_coercion(check_against, prim, true);
					if (coerced) {
						node->expression.variable_exp.type = coerced;
						node->return_type = coerced;
					} else {
						report_semantic_type_mismatch(get_site_from_token(node->expression.variable_exp.name), check_against, prim);
						return -1;
					}
				} else {
					node->return_type = prim;
				}
				if (result) *result = node->return_type;
				if (required) *required = node->return_type;
			}
			return 0;
		}break;

		case AST_NODE_EXPRESSION_ASSIGNMENT: {
			// this should be deprecated
			assert(0);
		}break;
	}
	return -1;
}

int infer_node_decl_types(Ast* node, Type_Table* table)
{
	bool already_queued = (node->return_type && node->return_type->flags & TYPE_FLAG_QUEUED);
	if (node->is_decl) {
		// if this is a declaration, the return type of this node is always void
		node->return_type = get_primitive_type(TYPE_PRIMITIVE_VOID);
		switch (node->node)
		{
			case AST_NODE_NAMED_ARGUMENT: {
				if (node->named_arg.arg_type->flags & TYPE_FLAG_IS_RESOLVED) return 0;

				assert(node->named_arg.arg_type);
				if (node->named_arg.arg_type->flags & TYPE_FLAG_IS_RESOLVED) {
					// create the type, since it is resolved
					create_type(&node->named_arg.arg_type, true);
				} else {
					if (resolve_type(node->named_arg.arg_type, table)) {
						create_type(&node->named_arg.arg_type, true);
						return 0;
					} else {
						// put it on the queue to be resolved
						if(!already_queued)
							push_infer_queue(node);
						return 1;
					}
				}
			} break;

			case AST_NODE_PROC_DECLARATION: {
				// proc return type creation
				assert(node->proc_decl.proc_ret_type);

				bool infered = true;

				if (!(node->proc_decl.proc_ret_type->flags & TYPE_FLAG_IS_RESOLVED) &&
					!resolve_type(node->proc_decl.proc_ret_type, table)) {
					infered = false;
				} 
				if (infered) {
					create_type(&node->proc_decl.proc_ret_type, true);
				}
				
				// arguments
				int num_args = node->proc_decl.num_args;
				for (int i = 0; i < num_args; ++i) {
					Ast* arg = node->proc_decl.arguments[i];
					if (infer_node_decl_types(arg, table) != 0) {
						infered = false;
					}
				}

				int err = infer_node_decl_types(node->proc_decl.body, table);

				if (infered) {
					Type_Instance* instance = new Type_Instance();
					instance->type = TYPE_FUNCTION;
					instance->flags = TYPE_FLAG_IS_RESOLVED | TYPE_FLAG_IS_SIZE_RESOLVED | TYPE_FLAG_IS_REGISTER_SIZE;
					instance->type_size = get_size_of_pointer();

					instance->type_function.num_arguments = num_args;
					instance->type_function.return_type = node->proc_decl.proc_ret_type;
					instance->type_function.arguments_type = create_array(Type_Instance*, num_args);

					for (int i = 0; i < num_args; ++i) {
						Type_Instance* in = get_decl_type(node->proc_decl.arguments[i]);
						push_array(instance->type_function.arguments_type, &in);
					}
					create_type(&instance, true);
					node->proc_decl.proc_type = instance;
				} else {
					if (!already_queued)
						push_infer_queue(node);
					return 1;
				}
				
				if (err) return err;
			}break;

			case AST_NODE_VARIABLE_DECL: {
				if (node->var_decl.type && node->var_decl.type->flags & TYPE_FLAG_IS_RESOLVED) {
					// this needs to be done because more than one variable access this type, so they all need to be the same
					// as the one present in the type_table
					create_type(&node->var_decl.type, true);
					if (node->var_decl.assignment) {
						int err = infer_node_expr_type(node->var_decl.assignment, table, node->var_decl.type);
						return err;
					}
					return 0;
				}

				if (node->var_decl.type) {
					if (node->var_decl.type->flags & TYPE_FLAG_IS_RESOLVED) {
						create_type(&node->var_decl.type, true);
					} else {
						if (resolve_type(node->var_decl.type, table)) {
							create_type(&node->var_decl.type, true);
						} else {
							if (!already_queued)
								push_infer_queue(node);
							return 1;
						}
					}
				} else if(node->var_decl.assignment) {
					// infer type from rvalue
					int err = infer_node_expr_type(node->var_decl.assignment, table, 0);
					if (err == -1) {
						return -1;
					}
					Type_Instance* inst = node->var_decl.assignment->return_type;
					if (inst == 0) {
						// could not infer, put it on the queue
						if (!already_queued)
							push_infer_queue(node);
					} else {
						if (inst->flags & TYPE_FLAG_IS_RESOLVED || resolve_type(inst, table)) {
							create_type(&inst, true);
							node->var_decl.assignment->return_type = inst;
							node->var_decl.type = inst;
						} else {
							if (!already_queued)
								push_infer_queue(node);
							return 1;
						}
					}
				} else {
					report_semantic_error(&node->var_decl.site, "type of variable %.*s could not be inferred, since there is no rvalue assignment.\n", TOKEN_STR(node->var_decl.name));
					return -1;
				}

				if (node->var_decl.assignment) {
					int err = infer_node_expr_type(node->var_decl.assignment, table, node->var_decl.type);
					return err;
				}

			}break;

			case AST_NODE_STRUCT_DECLARATION: {
				bool infered = true;
				int num_fields = node->struct_decl.num_fields;
				for (int i = 0; i < num_fields; ++i) {
					Ast* field = node->struct_decl.fields[i];
					if (infer_node_decl_types(field, table) != 0) {
						infered = false;
					}
				}
				if (infered) {
					Type_Instance** ftypes = create_array(Type_Instance*, num_fields);
					string* fnames = create_array(string, num_fields);
					s64 size_bytes = 0;
					for (int i = 0; i < num_fields; ++i) {
						Type_Instance* ti = get_decl_type(node->struct_decl.fields[i]);
						Token* name = get_decl_name(node->struct_decl.fields[i]);
						string s = string();
						make_immutable_string(s, name->value.data, name->value.length);
						push_array(fnames, &s);
						size_bytes += ti->type_size;
						push_array(ftypes, &ti);
					}
					Type_Instance* struct_instance = new Type_Instance();
					struct_instance->type = TYPE_STRUCT;
					struct_instance->type_struct.name = node->struct_decl.name->value.data;
					struct_instance->type_struct.name_length = node->struct_decl.name->value.length;
					struct_instance->type_struct.struct_descriptor = node;
					struct_instance->type_struct.fields_types = ftypes;
					struct_instance->type_struct.fields_names = fnames;

					struct_instance->flags = TYPE_FLAG_IS_RESOLVED | TYPE_FLAG_IS_SIZE_RESOLVED;
					struct_instance->type_size = size_bytes;

					create_type(&struct_instance, false);

					node->struct_decl.type_info = struct_instance;
					node->struct_decl.size_bytes = size_bytes;
				} else {
					if (!already_queued)
						push_infer_queue(node);
					return 1;
				}

			}break;
		}
	} else {
		switch (node->node)
		{
		// only catch nodes that could have declarations within it
		case AST_NODE_IF_STATEMENT: {
			node->return_type = get_primitive_type(TYPE_PRIMITIVE_VOID);
			int err_body_true = infer_node_decl_types(node->if_stmt.body, table);
			int err_body_false = 0;
			if(node->if_stmt.else_exp)
				err_body_false = infer_node_decl_types(node->if_stmt.else_exp, table);

			if (err_body_true == -1 || err_body_false == -1)
				return -1;

			if (err_body_true == 1 || err_body_false == 1) {
				return 1;
			}
			return 0;
		}break;

		case AST_NODE_BLOCK: {
			node->return_type = get_primitive_type(TYPE_PRIMITIVE_VOID);
			int err = decl_type_inference(node->block.commands, table);
			if (err == -1) return -1;
			else if (err == 1) {
				if (!already_queued)
					push_infer_queue(node);
				return 1;
			}
			return 0;
		}break;

		case AST_NODE_WHILE_STATEMENT: {
			node->return_type = get_primitive_type(TYPE_PRIMITIVE_VOID);
			return infer_node_decl_types(node->while_stmt.body, table);
			return 0;
		}break;
		
		default: {
			return infer_node_expr_type(node, table, 0);
		}break;
		}
	}
	return 0;
}

// return 0 if infered correctly
// return 1 if queued
// return -1 if errored
int decl_type_inference(Ast** ast, Type_Table* table)
{
	int ret_val = 0;
	size_t num_nodes = get_arr_length(ast);
	for (size_t i = 0; i < num_nodes; ++i) {
		Ast* node = ast[i];
		int err = infer_node_decl_types(node, table);
		if (err == -1) {
			return -1;
		} else if (err == 1) {
			ret_val = 1;
		}
	}
	return ret_val;
}

// return 0 if infered correctly
// return 1 if queued
// return -1 if errored
int do_type_inference(Ast** ast, Scope* global_scope, Type_Table* type_table)
{
	infer_queue = create_array(Infer_Node, 16);
	int err = decl_type_inference(ast, type_table);
	if (get_arr_length(infer_queue) > 0) {
		int qsize = get_arr_length(infer_queue);
		while (qsize != 0) {
			for(int i = 0; i < qsize; ++i) {
				Infer_Node in = infer_queue[i];
				if (infer_node_decl_types(in.node, type_table) == 0) {
					int n = get_arr_length(infer_queue);
					array_remove(infer_queue, i);
				} else {
					continue;
				}
			}
			int newsize = get_arr_length(infer_queue);
			if (qsize == newsize) {
				report_semantic_error(0, "Detected circulary dependency on type inference, exiting.\n");
				return -1;
			} else if (newsize < qsize) {
				qsize = newsize;
				continue;
			} else {
				report_semantic_error(0, "Internal compiler error, the number of entries in the infer_queue grew, which is an unintender behaviour.\n");
				return -1;
			}
		}
	}
	return 0;
}

/*
// return 1 if passed
// return 0 if failed
int do_type_check(Ast** ast, Type_Table* table)
{
	int err = 1;
	size_t num_nodes = get_arr_length(ast);
	for (size_t i = 0; i < num_nodes; ++i) {
		Ast* node = ast[i];
		int ret = type_check_node(node, table);
		if (ret == 0) {
			err = 0;
		}
	}
	return err;
}


// return 1 if passed
// return 0 if failed
int type_check_node(Ast* node, Type_Table* table)
{
if (node->is_decl) {
switch (node->node) {
case AST_NODE_PROC_DECLARATION: {

}break;
case AST_NODE_STRUCT_DECLARATION: {

}break;
case AST_NODE_VARIABLE_DECL: {

}break;
}
} else {
switch (node->node) {
case AST_NODE_BINARY_EXPRESSION: {

}break;
case AST_NODE_BLOCK: {

}break;
case AST_NODE_BREAK_STATEMENT: {

}break;
case AST_NODE_CONTINUE_STATEMENT: {

}break;
case AST_NODE_EXPRESSION_ASSIGNMENT: {

}break;
case AST_NODE_IF_STATEMENT: {

}break;
case AST_NODE_LITERAL_EXPRESSION: {

}break;
case AST_NODE_NAMED_ARGUMENT: {

}break;
case AST_NODE_PROCEDURE_CALL: {

}break;
case AST_NODE_RETURN_STATEMENT: {

}break;
case AST_NODE_UNARY_EXPRESSION: {

}break;
case AST_NODE_VARIABLE_EXPRESSION: {

}break;
}
}
return 0;
}
*/