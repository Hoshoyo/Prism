#include "ast.h"
#include "memory.h"

static Memory_Arena arena_scope(65536);
static Memory_Arena arena_ast(65536);

#define ALLOC_AST() (Ast*)arena_ast.allocate(sizeof(Ast))
#define ALLOC_SCOPE() (Scope*)arena_scope.allocate(sizeof(Scope))

Scope* scope_create(Ast* creator, Scope* parent, u32 flags) {
	static s64 id = 1;

	Scope* scope = ALLOC_SCOPE();
	scope->id = id++;
	scope->level = parent->level + 1;
	scope->creator_node = creator;
	scope->decl_count = 0;
	scope->flags = 0;
	scope->parent = parent;
	scope->symb_table = 0;

	return scope;
}

Ast* ast_create_decl_proc(Token* name, Scope* scope, Ast** arguments, Ast* body, Type_Instance* type_return, u32 flags, s32 arguments_count) {
	Ast* dp = ALLOC_AST();

	dp->node_type = AST_DECL_PROCEDURE;
	dp->type_return = type_primitive_get(TYPE_PRIMITIVE_VOID);
	dp->scope = scope;
	dp->flags = AST_FLAG_IS_DECLARATION;

	dp->decl_procedure.name = name;
	dp->decl_procedure.arguments = arguments;
	dp->decl_procedure.body = body;
	dp->decl_procedure.type_return = type_return;
	dp->decl_procedure.flags = 0;
	dp->decl_procedure.arguments_count = arguments_count;
	dp->decl_procedure.extern_library_name = 0;
	dp->decl_procedure.type_procedure = 0;

	dp->decl_procedure.site.filename = name->filename;
	dp->decl_procedure.site.line = name->line;
	dp->decl_procedure.site.column = name->column;

	return dp;
}

Ast* ast_create_decl_variable(Token* name, Scope* scope, Ast* assignment, Type_Instance* var_type, u32 flags) {
	Ast* dv = ALLOC_AST();

	dv->node_type = AST_DECL_VARIABLE;
	dv->type_return = type_primitive_get(TYPE_PRIMITIVE_VOID);
	dv->scope = scope;
	dv->flags = AST_FLAG_IS_DECLARATION;

	dv->decl_variable.name = name;
	dv->decl_variable.flags = flags;
	dv->decl_variable.size_bytes = 0;
	dv->decl_variable.temporary_register = 0;
	dv->decl_variable.variable_type = var_type;

	dv->decl_variable.site.filename = name->filename;
	dv->decl_variable.site.line = name->line;
	dv->decl_variable.site.column = name->column;

	return dv;
}

Ast* ast_create_decl_struct(Token* name, Scope* scope, Ast** fields, u32 flags, s32 field_count) {
	Ast* ds = ALLOC_AST();

	ds->node_type = AST_DECL_STRUCT;
	ds->type_return = type_primitive_get(TYPE_PRIMITIVE_VOID);
	ds->scope = scope;
	ds->flags = AST_FLAG_IS_DECLARATION;

	ds->decl_struct.name = name;
	ds->decl_struct.fields = fields;
	ds->decl_struct.fields_count = field_count;
	ds->decl_struct.flags = flags;
	ds->decl_struct.size_bytes = 0;
	ds->decl_struct.alignment = 0;

	ds->decl_struct.site.filename = name->filename;
	ds->decl_struct.site.line = name->line;
	ds->decl_struct.site.column = name->column;

	return ds;
}

Ast* ast_create_decl_enum(Token* name, Scope* scope, Ast** fields, Type_Instance* type_hint, u32 flags, s32 field_count) {
	Ast* de = ALLOC_AST();

	de->node_type = AST_DECL_ENUM;
	de->type_return = type_primitive_get(TYPE_PRIMITIVE_VOID);
	de->scope = scope;
	de->flags = AST_FLAG_IS_DECLARATION;

	de->decl_enum.name = name;
	de->decl_enum.fields = fields;
	de->decl_enum.fields_count = field_count;
	de->decl_enum.flags = flags;
	de->decl_enum.type_hint = type_hint;

	de->decl_enum.site.filename = name->filename;
	de->decl_enum.site.line = name->line;
	de->decl_enum.site.column = name->column;

	return de;
}

Ast* ast_create_decl_constant(Token* name, Scope* scope, Ast* value, Type_Instance* type, u32 flags) {
	Ast* dc = ALLOC_AST();

	dc->node_type = AST_DECL_CONSTANT;
	dc->type_return = type_primitive_get(TYPE_PRIMITIVE_VOID);
	dc->scope = scope;
	dc->flags = AST_FLAG_IS_DECLARATION;

	dc->decl_constant.name = name;
	dc->decl_constant.flags = flags;
	dc->decl_constant.type_info = type;
	dc->decl_constant.value = value;

	dc->decl_constant.site.filename = name->filename;
	dc->decl_constant.site.line = name->line;
	dc->decl_constant.site.column = name->column;

	return dc;
}

// Expressions
Ast* ast_create_expr_variable(Token* name, Scope* scope, Type_Instance* type) {
	Ast* ev = ALLOC_AST();

	ev->node_type = AST_EXPRESSION_VARIABLE;
	ev->type_return = type;
	ev->scope = scope;
	ev->flags = AST_FLAG_IS_EXPRESSION;

	ev->expr_variable.name = name;

	return ev;
}

#if 0
bool ast_is_expression(Ast* ast) {
	return (ast->node == AST_NODE_BINARY_EXPRESSION ||
		AST_NODE_UNARY_EXPRESSION ||
		AST_NODE_LITERAL_EXPRESSION ||
		AST_NODE_VARIABLE_EXPRESSION ||
		AST_NODE_PROCEDURE_CALL ||
		AST_NODE_EXPRESSION_ASSIGNMENT);
}

UnaryOperation get_unary_op(Token* token)
{
	Token_Type t = token->type;
	switch (t) {
	case '-':			return UNARY_OP_MINUS;
	case '+':			return UNARY_OP_PLUS;
	case '*':			return UNARY_OP_DEREFERENCE;
	case '&':			return UNARY_OP_ADDRESS_OF;
	case '[':			return UNARY_OP_VECTOR_ACCESS;
	case '~':			return UNARY_OP_NOT_BITWISE;
	case '!':			return UNARY_OP_NOT_LOGICAL;
	case TOKEN_CAST:	return UNARY_OP_CAST;
	}
}

BinaryOperation get_binary_op(Token* token)
{
	Token_Type t = token->type;
	switch (t) {
	case TOKEN_LOGIC_AND:			return BINARY_OP_LOGIC_AND;
	case TOKEN_LOGIC_OR:			return BINARY_OP_LOGIC_OR;
	case TOKEN_EQUAL_COMPARISON:	return BINARY_OP_EQUAL_EQUAL;
	case TOKEN_LESS_EQUAL:			return BINARY_OP_LESS_EQUAL;
	case TOKEN_GREATER_EQUAL:		return BINARY_OP_GREATER_EQUAL;
	case TOKEN_NOT_EQUAL:			return BINARY_OP_NOT_EQUAL;
	case '>':						return BINARY_OP_GREATER_THAN;
	case '<':						return BINARY_OP_LESS_THAN;
	case '^':						return BINARY_OP_XOR;
	case '|':						return BINARY_OP_OR;
	case '&':						return BINARY_OP_AND;
	case TOKEN_BITSHIFT_LEFT:		return BINARY_OP_BITSHIFT_LEFT;
	case TOKEN_BITSHIFT_RIGHT:		return BINARY_OP_BITSHIFT_RIGHT;
	case '+':						return BINARY_OP_PLUS;
	case '-':						return BINARY_OP_MINUS;
	case '*':						return BINARY_OP_MULT;
	case '/':						return BINARY_OP_DIV;
	case '%':						return BINARY_OP_MOD;
	case '.':						return BINARY_OP_DOT;
	case '=':						return ASSIGNMENT_OPERATION_EQUAL;
	case TOKEN_PLUS_EQUAL:			return ASSIGNMENT_OPERATION_PLUS_EQUAL;
	case TOKEN_MINUS_EQUAL:			return ASSIGNMENT_OPERATION_MINUS_EQUAL;
	case TOKEN_TIMES_EQUAL:			return ASSIGNMENT_OPERATION_TIMES_EQUAL;
	case TOKEN_DIV_EQUAL:			return ASSIGNMENT_OPERATION_DIVIDE_EQUAL;
	case TOKEN_AND_EQUAL:			return ASSIGNMENT_OPERATION_AND_EQUAL;
	case TOKEN_OR_EQUAL:			return ASSIGNMENT_OPERATION_OR_EQUAL;
	case TOKEN_XOR_EQUAL:			return ASSIGNMENT_OPERATION_XOR_EQUAL;
	case TOKEN_SHL_EQUAL:			return ASSIGNMENT_OPERATION_SHL_EQUAL;
	case TOKEN_SHR_EQUAL:			return ASSIGNMENT_OPERATION_SHR_EQUAL;
	case TOKEN_MOD_EQUAL:			return ASSIGNMENT_OPERATION_MOD_EQUAL;
	}
}

Ast* create_proc(Memory_Arena* arena, Token* name, Token* extern_name, Type_Instance* return_type, Ast** arguments, int nargs, Ast* body, Scope* scope, Decl_Site* site) {
	Ast* proc = ALLOC_AST(arena);

	if (body)
		proc->node = AST_NODE_PROC_DECLARATION;
	else if (extern_name) {
		proc->node = AST_NODE_PROC_DECLARATION;
	} else
		proc->node = AST_NODE_PROC_FORWARD_DECL;

	proc->is_decl = true;
	proc->is_expr = false;
	proc->return_type = 0;
	proc->type_checked = false;

	proc->proc_decl.scope = scope;
	proc->proc_decl.name = name;
	proc->proc_decl.proc_ret_type = return_type;
	proc->proc_decl.proc_type = 0;
	proc->proc_decl.arguments = arguments;
	proc->proc_decl.body = body;
	proc->proc_decl.num_args = nargs;
	proc->proc_decl.flags = 0;
	if (extern_name)
		proc->proc_decl.flags |= PROC_DECL_FLAG_IS_EXTERNAL;

	proc->proc_decl.site.filename = site->filename;
	proc->proc_decl.site.line = site->line;
	proc->proc_decl.site.column = site->column;

	proc->proc_decl.external_runtime_address = 0;

	scope->decl_node = proc;

	return proc;
}

Ast* create_named_argument(Memory_Arena* arena, Token* name, Type_Instance* type, Ast* default_val, int index, Scope* scope, Decl_Site* site)
{
	Ast* narg = ALLOC_AST(arena);

	narg->node = AST_NODE_NAMED_ARGUMENT;
	narg->is_decl = true;
	narg->is_expr = false;
	narg->return_type = 0;
	narg->type_checked = false;

	narg->named_arg.arg_name = name;
	narg->named_arg.arg_type = type;
	narg->named_arg.index = index;
	narg->named_arg.default_value = default_val;
	narg->named_arg.scope = scope;

	narg->named_arg.site.filename = site->filename;
	narg->named_arg.site.line = site->line;
	narg->named_arg.site.column = site->column;

	return narg;
}

Ast* create_variable_decl(Memory_Arena* arena, Token* name, Type_Instance* type, Ast* assign_val, Scope* scope, Decl_Site* site)
{
	Ast* vdecl = 0;
	if (arena) {
		vdecl = ALLOC_AST(arena);
	} else {
		vdecl = (Ast*)malloc(sizeof(Ast));
	}

	vdecl->node = AST_NODE_VARIABLE_DECL;
	vdecl->is_decl = true;
	vdecl->is_expr = false;
	vdecl->return_type = 0;
	vdecl->type_checked = false;

	vdecl->var_decl.scope = scope;
	vdecl->var_decl.name = name;
	vdecl->var_decl.alignment = 4;
	vdecl->var_decl.assignment = assign_val;
	vdecl->var_decl.type = type;
	vdecl->var_decl.size_bytes = 0;
	
	vdecl->var_decl.site.filename = site->filename;
	vdecl->var_decl.site.line = site->line;
	vdecl->var_decl.site.column = site->column;

	return vdecl;
}

Ast* create_literal(Memory_Arena* arena, u32 flags, Token* lit_tok, u64 value)
{
	Ast* lit = ALLOC_AST(arena);

	lit->node = AST_NODE_LITERAL_EXPRESSION;
	lit->is_decl = false;
	lit->is_expr = true;
	lit->return_type = 0;
	lit->type_checked = false;

	lit->expression.expression_type = EXPRESSION_TYPE_LITERAL;
	lit->expression.is_lvalue = false;
	lit->expression.literal_exp.flags = flags;
	lit->expression.literal_exp.type = 0;
	if (lit_tok) {
		lit->expression.literal_exp.lit_tok = lit_tok;
	} else {
		lit->expression.literal_exp.lit_value = value;
		lit->expression.literal_exp.flags |= LITERAL_FLAG_EVALUATED;
	}
	return lit;
}

Ast* create_binary_operation(Memory_Arena* arena, Ast* left_op, Ast *right_op, Token* op, Precedence precedence, Scope* scope)
{
	Ast* binop = ALLOC_AST(arena);

	binop->node = AST_NODE_BINARY_EXPRESSION;
	binop->is_decl = false;
	binop->is_expr = true;
	binop->return_type = 0;
	binop->type_checked = false;

	binop->expression.expression_type = EXPRESSION_TYPE_BINARY;
	binop->expression.is_lvalue = false;
	binop->expression.binary_exp.left = left_op;
	binop->expression.binary_exp.right = right_op;
	binop->expression.binary_exp.op = get_binary_op(op);
	binop->expression.binary_exp.op_token = op;
	binop->expression.binary_exp.scope = scope;
	binop->expression.binary_exp.precedence = precedence;

	return binop;
}

Ast* create_variable(Memory_Arena* arena, Token* var_token, Scope* scope)
{
	Ast* var = ALLOC_AST(arena);

	var->node = AST_NODE_VARIABLE_EXPRESSION;
	var->is_decl = false;
	var->is_expr = true;
	var->return_type = 0;
	var->type_checked = false;

	var->expression.expression_type = EXPRESSION_TYPE_VARIABLE;
	var->expression.is_lvalue = true;

	var->expression.variable_exp.name = var_token;
	var->expression.variable_exp.type = 0;
	var->expression.variable_exp.scope = scope;
	
	return var;
}

Ast* create_block(Memory_Arena* arena, Scope* scope)
{
	Ast* block = ALLOC_AST(arena);

	block->node = AST_NODE_BLOCK;
	block->is_decl = false;
	block->is_expr = false;
	block->return_type = 0;
	block->type_checked = false;

	block->block.scope = create_scope(scope->level + 1, scope, SCOPE_FLAG_BLOCK_SCOPE);
	block->block.commands = array_create(Ast*, 16);

	scope->decl_node = block;

	return block;
}

Ast* create_proc_call(Memory_Arena* arena, Token* name, Ast** args, Scope* scope)
{
	Ast* proc_call = ALLOC_AST(arena);

	proc_call->node = AST_NODE_PROCEDURE_CALL;
	proc_call->is_decl = false;
	proc_call->is_expr = true;
	proc_call->return_type = 0;
	proc_call->type_checked = false;

	proc_call->expression.expression_type = EXPRESSION_TYPE_PROC_CALL;
	proc_call->expression.is_lvalue = false;

	proc_call->expression.proc_call.name = name;
	proc_call->expression.proc_call.args = args;
	proc_call->expression.proc_call.scope = scope;

	//size_t num_args = array_get_length(proc_call->expression.proc_call.args);

	return proc_call;
}

Ast* create_if(Memory_Arena* arena, Ast* bool_exp, Ast* body, Ast* else_stmt, Scope* scope)
{
	Ast* if_stmt = ALLOC_AST(arena);
	if_stmt->node = AST_NODE_IF_STATEMENT;
	if_stmt->is_decl = false;
	if_stmt->is_expr = false;
	if_stmt->return_type = 0;
	if_stmt->type_checked = false;

	if_stmt->if_stmt.bool_exp = bool_exp;
	if_stmt->if_stmt.body = body;
	if_stmt->if_stmt.scope = scope;
	if_stmt->if_stmt.else_exp = else_stmt;

	return if_stmt;
}

Ast* create_while(Memory_Arena* arena, Ast* bool_exp, Ast* body, Scope* scope)
{
	Ast* while_stmt = ALLOC_AST(arena);
	while_stmt->node = AST_NODE_WHILE_STATEMENT;
	while_stmt->is_decl = false;
	while_stmt->is_expr = false;
	while_stmt->return_type = 0;
	while_stmt->type_checked = false;

	while_stmt->while_stmt.bool_exp = bool_exp;
	while_stmt->while_stmt.body = body;
	while_stmt->while_stmt.scope = scope;

	return while_stmt;
}

Ast* create_return(Memory_Arena* arena, Ast* exp, Scope* scope, Token* token)
{
	Ast* ret_stmt = ALLOC_AST(arena);
	ret_stmt->node = AST_NODE_RETURN_STATEMENT;
	ret_stmt->is_decl = false;
	ret_stmt->is_expr = false;
	ret_stmt->return_type = 0;
	ret_stmt->type_checked = false;

	ret_stmt->ret_stmt.expr = exp;
	ret_stmt->ret_stmt.scope = scope;

	ret_stmt->ret_stmt.site.filename = token->filename;
	ret_stmt->ret_stmt.site.line = token->line;
	ret_stmt->ret_stmt.site.column = token->column;
	
	return ret_stmt;
}

Ast* create_unary_expression(Memory_Arena* arena, Ast* operand, UnaryOperation op, Token* token, u32 flags, Type_Instance* cast_type, Precedence precedence, Scope* scope)
{
	Ast* unop = ALLOC_AST(arena);

	unop->node = AST_NODE_UNARY_EXPRESSION;
	unop->is_decl = false;
	unop->is_expr = true;
	unop->return_type = 0;
	unop->type_checked = false;

	unop->expression.expression_type = EXPRESSION_TYPE_UNARY;
	unop->expression.is_lvalue = 0;
	unop->expression.unary_exp.op = op;
	unop->expression.unary_exp.op_token = token;
	unop->expression.unary_exp.operand = operand;
	unop->expression.unary_exp.cast_type = cast_type;
	unop->expression.unary_exp.precedence = precedence;
	unop->expression.unary_exp.flags = flags;
	unop->expression.unary_exp.scope = scope;

	return unop;
}

Ast* create_break(Memory_Arena* arena, Scope* scope, Token* token)
{
	Ast* break_stmt = ALLOC_AST(arena);

	break_stmt->is_decl = false;
	break_stmt->is_expr = false;
	break_stmt->node = AST_NODE_BREAK_STATEMENT;
	break_stmt->return_type = 0;
	break_stmt->type_checked = false;

	break_stmt->break_stmt.scope = scope;

	break_stmt->break_stmt.site.filename = token->filename;
	break_stmt->break_stmt.site.line = token->line;
	break_stmt->break_stmt.site.column = token->column;

	return break_stmt;
}

Ast* create_continue(Memory_Arena* arena, Scope* scope, Token* token)
{
	Ast* continue_stmt = ALLOC_AST(arena);

	continue_stmt->is_decl = false;
	continue_stmt->is_expr = false;
	continue_stmt->node = AST_NODE_CONTINUE_STATEMENT;
	continue_stmt->return_type = 0;
	continue_stmt->type_checked = false;

	continue_stmt->continue_stmt.scope = scope;

	continue_stmt->continue_stmt.site.filename = token->filename;
	continue_stmt->continue_stmt.site.line = token->line;
	continue_stmt->continue_stmt.site.column = token->column;

	return continue_stmt;
}

Ast* create_struct_decl(Memory_Arena* arena, Token* name, Ast** fields, int num_fields, Scope* struct_scope, Decl_Site* site)
{
	Ast* struct_decl = 0;
	if (arena) {
		struct_decl = ALLOC_AST(arena);
	} else {
		struct_decl = (Ast*)malloc(sizeof(Ast));
	}

	struct_decl->is_decl = true;
	struct_decl->is_expr = false;
	struct_decl->node = AST_NODE_STRUCT_DECLARATION;
	struct_decl->return_type = 0;
	struct_decl->type_checked = false;

	struct_decl->struct_decl.alignment = 4;
	struct_decl->struct_decl.fields = fields;
	struct_decl->struct_decl.name = name;
	struct_decl->struct_decl.num_fields = num_fields;
	struct_decl->struct_decl.size_bytes = 0;
	struct_decl->struct_decl.type_info = 0;
	struct_decl->struct_decl.scope = struct_scope;

	struct_decl->struct_decl.site.filename = site->filename;
	struct_decl->struct_decl.site.line = site->line;
	struct_decl->struct_decl.site.column = site->column;

	struct_scope->decl_node = struct_decl;

	return struct_decl;
}

Ast* create_enum_decl(Memory_Arena* arena, Token* name, Ast_EnumField* fields, int num_fields, Type_Instance* base_type, Scope* enum_scope, Decl_Site* site) {
	Ast* enum_decl = ALLOC_AST(arena);

	enum_decl->is_decl = true;
	enum_decl->is_expr = false;
	enum_decl->node = AST_NODE_ENUM_DECLARATION;
	enum_decl->return_type = 0;
	enum_decl->type_checked = false;

	enum_decl->enum_decl.base_type = base_type;
	enum_decl->enum_decl.fields = fields;
	enum_decl->enum_decl.num_fields = num_fields;
	enum_decl->enum_decl.name = name;
	enum_decl->enum_decl.scope = enum_scope;
	enum_decl->enum_decl.site = *site;

	return enum_decl;
}

Ast* create_directive(Memory_Arena* arena, Token* directive_token, Ast* literal_argument, Ast* declaration)
{
	Ast* directive = ALLOC_AST(arena);

	directive->node = AST_NODE_DIRECTIVE;

	directive->return_type = 0;
	directive->is_decl = true;
	directive->is_expr = false;
	directive->type_checked = false;

	directive->directive.declaration = declaration;
	directive->directive.token = directive_token;
	directive->directive.literal_argument = literal_argument;

	return directive;
}

Ast* create_constant(Memory_Arena* arena, Token* name, Ast* expression, Type_Instance* type, Scope* scope) 
{
	Ast* constant = ALLOC_AST(arena);

	constant->node = AST_NODE_CONSTANT;

	constant->is_decl = true;
	constant->is_expr = false;
	constant->return_type = 0;
	constant->type_checked = false;

	constant->constant.name = name;
	constant->constant.expression = expression;
	constant->constant.scope = scope;
	constant->constant.type = type;

	return constant;
}

void block_push_command(Ast* block, Ast* command)
{
	array_push(block->block.commands, &command);
}

void push_ast_list(Ast*** list, Ast* arg)
{
	array_push(*list, &arg);
}

s64 generate_scope_id()
{
	scope_manager.current_id++;
	return scope_manager.current_id - 1;
}

Scope* create_scope(s32 level, Scope* parent, u32 flags)
{
	Scope* res = (Scope*)malloc(sizeof(Scope));
	res->num_declarations = 0;
	res->symb_table = 0;
	res->id = generate_scope_id();
	res->level = level;
	res->parent = parent;
	res->flags = flags;
	res->decl_node = 0;
	return res;
}







/***************************************

				DEBUG

****************************************/


static int DEBUG_indent_level = 0;
void DEBUG_print_indent_level() {
	for (int i = 0; i < DEBUG_indent_level; ++i) {
		fprintf(stdout, "   ");
	}
}

void DEBUG_print_type(FILE* out, Type_Instance* type, bool short_) {
	if (!type) {
		fprintf(out, "(TYPE_IS_NULL)");
		return;
	}
	if (type->type == TYPE_PRIMITIVE) {
		switch (type->primitive) {
		case TYPE_PRIMITIVE_S64:	fprintf(out, "s64"); break;
		case TYPE_PRIMITIVE_S32:	fprintf(out, "s32"); break;
		case TYPE_PRIMITIVE_S16:	fprintf(out, "s16"); break;
		case TYPE_PRIMITIVE_S8:		fprintf(out, "s8"); break;
		case TYPE_PRIMITIVE_U64:	fprintf(out, "u64"); break;
		case TYPE_PRIMITIVE_U32:	fprintf(out, "u32"); break;
		case TYPE_PRIMITIVE_U16:	fprintf(out, "u16"); break;
		case TYPE_PRIMITIVE_U8:		fprintf(out, "u8"); break;
		case TYPE_PRIMITIVE_BOOL:	fprintf(out, "bool"); break;
		case TYPE_PRIMITIVE_R64:	fprintf(out, "r64"); break;
		case TYPE_PRIMITIVE_R32:	fprintf(out, "r32"); break;
		case TYPE_PRIMITIVE_VOID:	fprintf(out, "void"); break;
		}
	} else if (type->type == TYPE_POINTER) {
		fprintf(out, "^");
		DEBUG_print_type(out, type->pointer_to);
	} else if (type->type == TYPE_STRUCT) {
		if (short_) {
			fprintf(out, "%.*s", type->type_struct.name_length, type->type_struct.name);
		} else {
			fprintf(out, "%.*s struct {\n", type->type_struct.name_length, type->type_struct.name);
			int num_fields = array_get_length(type->type_struct.fields_types);
			for (int i = 0; i < num_fields; ++i) {
				Type_Instance* field_type = type->type_struct.fields_types[i];
				string name = type->type_struct.fields_names[i];
				fprintf(out, "%.*s : ", name.length, name.data);
				DEBUG_print_type(out, field_type);
				fprintf(out, ";\n");
			}
			fprintf(out, "}\n");
		}
	} else if (type->type == TYPE_FUNCTION) {
		fprintf(out, "(");
		int num_args = type->type_function.num_arguments;
		for (int i = 0; i < num_args; ++i) {
			DEBUG_print_type(out, type->type_function.arguments_type[i]);
			if(i + 1 < num_args) fprintf(out, ",");
		}
		fprintf(out, ") -> ");
		DEBUG_print_type(out, type->type_function.return_type);
	}

}

void DEBUG_print_block(FILE* out, Ast* block)
{
	size_t num_commands = array_get_length(block->block.commands);
	fprintf(out, "{\n");
	DEBUG_indent_level += 1;
	for (size_t i = 0; i < num_commands; ++i) {
		DEBUG_print_indent_level();
		DEBUG_print_node(out, block->block.commands[i]);
		fprintf(out, ";\n");
	}
	DEBUG_indent_level -= 1;
	DEBUG_print_indent_level();
	fprintf(out, "}");
}

void DEBUG_print_proc(FILE* out, Ast* proc_node) {
	fprintf(out, "%.*s :: (", TOKEN_STR(proc_node->proc_decl.name));
	for (int i = 0; i < proc_node->proc_decl.num_args; ++i) {
		Ast* narg = proc_node->proc_decl.arguments[i];
		fprintf(out, "%.*s : ", TOKEN_STR(narg->named_arg.arg_name));
		DEBUG_print_type(out, narg->named_arg.arg_type);
		if (i + 1 != proc_node->proc_decl.num_args) fprintf(out, ", ");
	}
	fprintf(out, ") -> ");
	DEBUG_print_type(out, proc_node->proc_decl.proc_ret_type);
	if (proc_node->proc_decl.body == 0) {
		fprintf(out, ";");
	} else {
		DEBUG_print_block(out, proc_node->proc_decl.body);
	}
}

void DEBUG_print_expression(FILE* out, Ast* node) {
	switch (node->expression.expression_type) {
	case EXPRESSION_TYPE_LITERAL: {
		if (node->expression.literal_exp.flags & LITERAL_FLAG_EVALUATED) {
			fprintf(out, "0x%llu", node->expression.literal_exp.lit_value);
		} else {
			fprintf(out, "%.*s", TOKEN_STR(node->expression.literal_exp.lit_tok));
		}
	} break;
	case EXPRESSION_TYPE_BINARY: {
		fprintf(out, "(");
		DEBUG_print_expression(out, node->expression.binary_exp.left);
		switch (node->expression.binary_exp.op) {
		case BINARY_OP_PLUS:			 fprintf(out, " + "); break;
		case BINARY_OP_MINUS:			 fprintf(out, " - "); break;
		case BINARY_OP_MULT:			 fprintf(out, " * "); break;
		case BINARY_OP_DIV:				 fprintf(out, " / "); break;
		case BINARY_OP_AND:				 fprintf(out, " & "); break;
		case BINARY_OP_OR:				 fprintf(out, " | "); break;
		case BINARY_OP_XOR:				 fprintf(out, " ^ "); break;
		case BINARY_OP_MOD:				 fprintf(out, " %% "); break;
		case BINARY_OP_LOGIC_AND:		 fprintf(out, " && "); break;
		case BINARY_OP_LOGIC_OR:		 fprintf(out, " || "); break;
		case BINARY_OP_BITSHIFT_LEFT:	 fprintf(out, " << "); break;
		case BINARY_OP_BITSHIFT_RIGHT:	 fprintf(out, " >> "); break;
		case BINARY_OP_LESS_THAN:		 fprintf(out, " < "); break;
		case BINARY_OP_GREATER_THAN:	 fprintf(out, " > "); break;
		case BINARY_OP_LESS_EQUAL:		 fprintf(out, " <= "); break;
		case BINARY_OP_GREATER_EQUAL:	 fprintf(out, " >= "); break;
		case BINARY_OP_EQUAL_EQUAL:		 fprintf(out, " == "); break;
		case BINARY_OP_NOT_EQUAL:		 fprintf(out, " != "); break;
		case BINARY_OP_DOT:				 fprintf(out, "."); break;
		case ASSIGNMENT_OPERATION_EQUAL:		fprintf(out, " = "); break;
		case ASSIGNMENT_OPERATION_PLUS_EQUAL:	fprintf(out, " += "); break;
		case ASSIGNMENT_OPERATION_MINUS_EQUAL:	fprintf(out, " -= "); break;
		case ASSIGNMENT_OPERATION_TIMES_EQUAL:	fprintf(out, " *= "); break;
		case ASSIGNMENT_OPERATION_DIVIDE_EQUAL:	fprintf(out, " /= "); break;
		case ASSIGNMENT_OPERATION_MOD_EQUAL:	fprintf(out, " %%= "); break;
		case ASSIGNMENT_OPERATION_AND_EQUAL:	fprintf(out, " &= "); break;
		case ASSIGNMENT_OPERATION_OR_EQUAL:		fprintf(out, " |= "); break;
		case ASSIGNMENT_OPERATION_XOR_EQUAL:	fprintf(out, " ^= "); break;
		case ASSIGNMENT_OPERATION_SHL_EQUAL:	fprintf(out, " <<= "); break;
		case ASSIGNMENT_OPERATION_SHR_EQUAL:	fprintf(out, " >>= "); break;
		}
		DEBUG_print_expression(out, node->expression.binary_exp.right);
		fprintf(out, ")");
	} break;
	case EXPRESSION_TYPE_UNARY: {
		if (node->expression.unary_exp.flags & UNARY_EXP_FLAG_PREFIXED) {
			switch (node->expression.unary_exp.op) {
			case UNARY_OP_ADDRESS_OF:	fprintf(out, "&"); break;
			case UNARY_OP_MINUS:		fprintf(out, "-"); break;
			case UNARY_OP_DEREFERENCE:	fprintf(out, "*"); break;
			case UNARY_OP_NOT_BITWISE:	fprintf(out, "~"); break;
			case UNARY_OP_NOT_LOGICAL:	fprintf(out, "!"); break;
			case UNARY_OP_PLUS:			fprintf(out, "+"); break;
			case UNARY_OP_CAST: {
				fprintf(out, "cast (");
				DEBUG_print_type(out, node->expression.unary_exp.cast_type);
				fprintf(out, ")");
			}break;
			}
			DEBUG_print_expression(out, node->expression.unary_exp.operand);
		}
		else {
			fprintf(out, "ERROR POSTFIXED");
		}
	}break;
	case EXPRESSION_TYPE_VARIABLE: {
		fprintf(out, "%.*s", TOKEN_STR(node->expression.variable_exp.name));
	} break;
	case EXPRESSION_TYPE_PROC_CALL: {
		int num_args = 0;
		if (node->expression.proc_call.args) {
			num_args = array_get_length(node->expression.proc_call.args);
		}
		fprintf(out, "%.*s(", TOKEN_STR(node->expression.proc_call.name));
		for (int i = 0; i < num_args; ++i) {
			DEBUG_print_expression(out, node->expression.proc_call.args[i]);
			if(i + 1 != num_args) fprintf(out, ", ");
		}
		fprintf(out, ")");
	} break;
	}
}

void DEBUG_print_constant_decl(FILE* out, Ast* node) {
	fprintf(out, "%.*s : ", TOKEN_STR(node->constant.name));
	DEBUG_print_type(out, node->constant.type);
	if (node->constant.expression) {
		fprintf(out, " = ");
		DEBUG_print_expression(out, node->constant.expression);
	}
}

void DEBUG_print_var_decl(FILE* out, Ast* node) {
	fprintf(out, "%.*s : ", TOKEN_STR(node->var_decl.name));
	DEBUG_print_type(out, node->var_decl.type);
	if (node->var_decl.assignment) {
		fprintf(out, " = ");
		DEBUG_print_expression(out, node->var_decl.assignment);
	}
}

void DEBUG_print_if_statement(FILE* out, Ast* node)
{
	fprintf(out, "if (");
	DEBUG_print_expression(out, node->if_stmt.bool_exp);
	fprintf(out, ")");
	if (node->if_stmt.body->node == AST_NODE_BLOCK) {
		DEBUG_print_node(out, node->if_stmt.body);
	} else {
		fprintf(out, " ");
		DEBUG_print_node(out, node->if_stmt.body);
		fprintf(out, ";");
	}
	if (node->if_stmt.else_exp) {
		fprintf(out, "\n");
		DEBUG_print_indent_level();
		fprintf(out, "else");
		if (node->if_stmt.body->node == AST_NODE_BLOCK) {
			DEBUG_print_node(out, node->if_stmt.else_exp);
		} else {
			fprintf(out, " ");
			DEBUG_print_node(out, node->if_stmt.else_exp);
			fprintf(out, ";");
		}
	}
}

void DEBUG_print_while_statement(FILE* out, Ast* node)
{
	fprintf(out, "while (");
	DEBUG_print_expression(out, node->while_stmt.bool_exp);
	fprintf(out, ")");
	DEBUG_print_node(out, node->while_stmt.body);
}

void DEBUG_print_return_statement(FILE* out, Ast* node)
{
	fprintf(out, "return ");
	if (node->ret_stmt.expr) {
		DEBUG_print_expression(out, node->ret_stmt.expr);
	}
}

void DEBUG_print_break_statement(FILE* out, Ast* node) 
{
	fprintf(out, "break");
}

void DEBUG_print_continue_statement(FILE* out, Ast* node)
{
	fprintf(out, "continue");
}

void DEBUG_print_struct_declaration(FILE* out, Ast* node)
{
	Ast_StructDecl* sd = &node->struct_decl;
	fprintf(out, "%.*s :: struct{\n", TOKEN_STR(sd->name));
	DEBUG_indent_level += 1;
	for (int i = 0; i < sd->num_fields; ++i) {
		DEBUG_print_var_decl(out, sd->fields[i]);
		fprintf(out, ";\n");
	}
	DEBUG_indent_level -= 1;
	fprintf(out, "}\n");
}

void DEBUG_print_enum_declaration(FILE* out, Ast* node) {
	Ast_EnumDecl* ed = &node->enum_decl;
	fprintf(out, "%.*s :", TOKEN_STR(ed->name));
	DEBUG_print_type(out, ed->base_type);
	fprintf(out, ": enum {\n");
	DEBUG_indent_level += 1;
	for (int i = 0; i < ed->num_fields; ++i) {
		fprintf(out, "%.*s = ", TOKEN_STR(ed->fields[i].name));
		DEBUG_print_expression(out, ed->fields[i].expression);
		fprintf(out, ",\n");
	}
	DEBUG_indent_level -= 1;
	fprintf(out, "}\n");
}

void DEBUG_print_node(FILE* out, Ast* node) {
	switch (node->node) {
	case AST_NODE_PROC_FORWARD_DECL:
	case AST_NODE_PROC_DECLARATION:		DEBUG_print_proc(out, node); break;
	case AST_NODE_VARIABLE_DECL:		DEBUG_print_var_decl(out, node); break;
	case AST_NODE_CONSTANT:				DEBUG_print_constant_decl(out, node); break;
	case AST_NODE_UNARY_EXPRESSION:		
	case AST_NODE_BINARY_EXPRESSION:	DEBUG_print_expression(out, node); break;
	case AST_NODE_BLOCK:				DEBUG_print_block(out, node); break;
	case AST_NODE_IF_STATEMENT:			DEBUG_print_if_statement(out, node); break;
	case AST_NODE_WHILE_STATEMENT:		DEBUG_print_while_statement(out, node); break;
	case AST_NODE_PROCEDURE_CALL:		DEBUG_print_expression(out, node); break;
	case AST_NODE_RETURN_STATEMENT:		DEBUG_print_return_statement(out, node); break;
	case AST_NODE_BREAK_STATEMENT:		DEBUG_print_break_statement(out, node); break;
	case AST_NODE_CONTINUE_STATEMENT:	DEBUG_print_continue_statement(out, node); break;
	case AST_NODE_STRUCT_DECLARATION:	DEBUG_print_struct_declaration(out, node); break;
	case AST_NODE_ENUM_DECLARATION:		DEBUG_print_enum_declaration(out, node); break;
	default: {
		fprintf(out, "<Unsupported AST Node>\n");
	}break;
	}
}

void DEBUG_print_ast(FILE* out, Ast** ast) {
	int l = array_get_length(ast);
	for (int i = 0; i < array_get_length(ast); ++i) {
		DEBUG_print_node(out, ast[i]);
		fprintf(out, "\n");
	}
}

#endif