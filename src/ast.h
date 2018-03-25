#pragma once
#include "type.h"
#include "lexer.h"
#include "symbol_table.h"

#define SITE_FROM_TOKEN(T) {(T)->filename, (T)->line, (T)->column }

enum Ast_NodeType {
	AST_UNKNOWN = 0,
	
	// Declarations
	AST_DECL_PROCEDURE,
	AST_DECL_VARIABLE,
	AST_DECL_STRUCT,
	AST_DECL_UNION,
	AST_DECL_ENUM,
	AST_DECL_CONSTANT,

	// Commands
	AST_COMMAND_BLOCK,
	AST_COMMAND_VARIABLE_ASSIGNMENT,
	AST_COMMAND_IF,
	AST_COMMAND_FOR,
	AST_COMMAND_BREAK,
	AST_COMMAND_CONTINUE,
	AST_COMMAND_RETURN,

	// Expressions
	AST_EXPRESSION_BINARY,
	AST_EXPRESSION_UNARY,
	AST_EXPRESSION_CAST,
	AST_EXPRESSION_LITERAL,
	AST_EXPRESSION_VARIABLE,
	AST_EXPRESSION_PROCEDURE_CALL,

};

enum Literal_Type {
	LITERAL_UNKNOWN = 0,

	LITERAL_SINT,
	LITERAL_UINT,
	LITERAL_HEX_INT,
	LITERAL_BIN_INT,
	LITERAL_FLOAT,
	LITERAL_BOOL,

	LITERAL_STRUCT,
	LITERAL_ARRAY,
};

enum Operator_Unary {
	OP_UNARY_UNKNOWN = 0,
	
	OP_UNARY_PLUS,
	OP_UNARY_MINUS,
	OP_UNARY_DEREFERENCE,
	OP_UNARY_ADDRESSOF,
	OP_UNARY_BITWISE_NOT,
	OP_UNARY_VECTOR_ACCESSER,
	OP_UNARY_CAST,

	OP_UNARY_LOGIC_NOT,
};

enum Operator_Binary {
	OP_BINARY_UNKNOWN = 0,

	OP_BINARY_PLUS,			// +
	OP_BINARY_MINUS,		// -
	OP_BINARY_MULT,			// *
	OP_BINARY_DIV,			// /
	OP_BINARY_MOD,			// %
	OP_BINARY_AND,			// &
	OP_BINARY_OR,			// |
	OP_BINARY_XOR,			// ^
	OP_BINARY_SHL,			// <<
	OP_BINARY_SHR,			// >>

	OP_BINARY_LT,			// <
	OP_BINARY_GT,			// >
	OP_BINARY_LE,			// <=
	OP_BINARY_GE,			// >=
	OP_BINARY_EQUAL,		// ==
	OP_BINARY_NOT_EQUAL,	// !=

	OP_BINARY_LOGIC_AND,	// &&
	OP_BINARY_LOGIC_OR,		// ||

	OP_BINARY_DOT,			// .
};

enum Precedence {
	PRECEDENCE_0 = 0,	//	 assignments
	PRECEDENCE_1 = 1,	//	 || &&
	PRECEDENCE_2 = 2,	//	 == >= <= != > <
	PRECEDENCE_3 = 3,	//	 ^ | & >> <<
	PRECEDENCE_4 = 4,	//	 + -
	PRECEDENCE_5 = 5,	//	 * / %
	PRECEDENCE_6 = 6,	//	 &(addressof) ~ 
	PRECEDENCE_7 = 7,	//	 *(dereference)	cast !
	PRECEDENCE_8 = 8,	//	 .
	PRECEDENCE_MAX,
};

struct Symbol_Table;
struct Ast;

const u32 SCOPE_PROCEDURE_ARGUMENTS = FLAG(0);
const u32 SCOPE_PROCEDURE_BODY      = FLAG(1);
const u32 SCOPE_STRUCTURE           = FLAG(2);
const u32 SCOPE_ENUM                = FLAG(3);
const u32 SCOPE_FILESCOPE           = FLAG(4);
const u32 SCOPE_BLOCK               = FLAG(5);
const u32 SCOPE_LOOP                = FLAG(6);
struct Scope {
	s32           id;
	s32           level;
	s32           decl_count;
	u32           flags;
	Symbol_Table  symb_table;
	Scope*        parent;
	union {
		Ast*   creator_node;
		string filename;
	};
};

struct Site {
	string filename;
	s32    line;
	s32    column;
};

// ----------------------------------------
// ------------ Declarations --------------
// ----------------------------------------

struct Ast_Decl_Procedure {
	Token*         name;
	Ast**          arguments;		// DECL_VARIABLE
	Ast*           body;			// COMMAND_BLOCK
	Type_Instance* type_return;
	Type_Instance* type_procedure;
	Scope*         arguments_scope;

	Site   site;

	u32    flags;
	s32    arguments_count;
	
	Token* extern_library_name;
};

struct Ast_Decl_Variable {
	Token*         name;
	Ast*           assignment;		// EXPRESSION
	Type_Instance* variable_type;

	Site site;

	u32 flags;
	s32 size_bytes;
	s32 alignment;
	u32 temporary_register;
};
struct Ast_Decl_Struct {
	Token*         name;
	Ast**          fields;			// DECL_VARIABLE
	Type_Instance* type_info;
	Scope*         struct_scope;

	Site site;

	u32 flags;
	s32 fields_count;
	s32 alignment;
	s64 size_bytes;
};
struct Ast_Decl_Union {
	Token*         name;
	Ast**          fields;			// DECL_VARIABLE
	Type_Instance* type_info;

	Site site;

	u32 flags;
	s32 fields_count;
	s32 alignment;
	s64 size_bytes;
};
struct Ast_Decl_Enum {
	Token*         name;
	Ast**          fields;			// DECL_CONSTANT
	Type_Instance* type_hint;
	Scope*         enum_scope;

	Site site;

	u32 flags;
	s32 fields_count;
};
struct Ast_Decl_Constant {
	Token*         name;
	Ast*           value;		// LITERAL | CONSTANT
	Type_Instance* type_info;

	Site site;

	u32 flags;
};

// ----------------------------------------
// -------------- Commands ----------------
// ----------------------------------------

struct Ast_Comm_Block {
	Ast**  commands;	// COMMANDS
	Scope* block_scope;
	s32    command_count;
};
struct Ast_Comm_VariableAssign {
	Ast*   lvalue;	// EXPRESSION
	Ast*   rvalue;	// EXPRESSION
};
struct Ast_Comm_If {
	Ast* condition;		// EXPRESSION (boolean)
	Ast* body_true;		// COMMAND
	Ast* body_false;	// COMMAND
};
struct Ast_Comm_For {
	Ast* condition;		// EXPRESSION (boolean)
	Ast* body;			// COMMAND
};
struct Ast_Comm_Break {
	Ast* level;			// INT LITERAL [0, MAX_INT]
};
struct Ast_Comm_Return {
	Ast* expression;	// EXPRESSION
};

// ----------------------------------------
// ------------- Expressions --------------
// ----------------------------------------

struct Ast_Expr_Binary {
	Ast* left;
	Ast* right;
	Operator_Binary op;
};
const u32 UNARY_EXPR_FLAG_PREFIXED  = FLAG(0);
const u32 UNARY_EXPR_FLAG_POSTFIXED = FLAG(1);
struct Ast_Expr_Unary {
	Ast*           operand;
	Operator_Unary op;
	u32            flags;
};
struct Ast_Expr_Cast {
	Ast*           operand;
	Type_Instance* type_to_cast;
};

const u32 LITERAL_FLAG_STRING = FLAG(0);
struct Ast_Expr_Literal {
	Literal_Type type;
	u32 flags;
	union {
		u64 value_u64;
		s64 value_s64;

		r32 value_r32;
		r64 value_r64;

		bool value_bool;

		Ast*  value_struct;		// LITERALS
		void* value_array;		// DATA IN MEMORY
	};
};
struct Ast_Expr_Variable {
	Token* name;
};
struct Ast_Expr_ProcCall {
	Token* name;
	Ast**  args;		// EXPRESSIONS
	s32    args_count;
};

const u32 AST_FLAG_IS_DECLARATION = FLAG(0);
const u32 AST_FLAG_IS_COMMAND     = FLAG(1);
const u32 AST_FLAG_IS_EXPRESSION  = FLAG(2);
struct Ast {
	Ast_NodeType   node_type;
	Type_Instance* type_return;
	Scope*         scope;
	
	u32 flags;

	union {
		Ast_Decl_Procedure      decl_procedure;
		Ast_Decl_Variable       decl_variable;
		Ast_Decl_Struct         decl_struct;
		Ast_Decl_Union          decl_union;
		Ast_Decl_Enum           decl_enum;
		Ast_Decl_Constant       decl_constant;

		Ast_Comm_Block          comm_block;
		Ast_Comm_VariableAssign comm_var_assign;
		Ast_Comm_If             comm_if;
		Ast_Comm_For            comm_for;
		Ast_Comm_Break          comm_break;
		Ast_Comm_Return         comm_return;

		Ast_Expr_Binary         expr_binary;
		Ast_Expr_Unary          expr_unary;
		Ast_Expr_Cast           expr_cast;
		Ast_Expr_Literal        expr_literal;
		Ast_Expr_Variable       expr_variable;
		Ast_Expr_ProcCall       expr_proc_call;
	};
};

Scope* scope_create(Ast* creator, Scope* parent, u32 flags);

Ast* ast_create_decl_proc(Token* name, Scope* scope, Scope* arguments_scope, Ast** arguments, Ast* body, Type_Instance* type_return, u32 flags, s32 arguments_count);
Ast* ast_create_decl_variable(Token* name, Scope* scope, Ast* assignment, Type_Instance* var_type, u32 flags);
Ast* ast_create_decl_struct(Token* name, Scope* scope, Scope* struct_scope, Ast** fields, u32 flags, s32 field_count);
Ast* ast_create_decl_enum(Token* name, Scope* scope, Scope* enum_scope, Ast** fields, Type_Instance* type_hint, u32 flags, s32 field_count);
Ast* ast_create_decl_constant(Token* name, Scope* scope, Ast* value, Type_Instance* type, u32 flags);

Ast* ast_create_expr_variable(Token* name, Scope* scope, Type_Instance* type);
Ast* ast_create_expr_literal(Scope* scope, Literal_Type literal_type, u32 flags, Type_Instance* type);
Ast* ast_create_expr_binary(Scope* scope, Ast* left, Ast* right, Operator_Binary op);
Ast* ast_create_expr_proc_call(Scope* scope, Token* name, Ast** arguments, s32 args_count);
Ast* ast_create_expr_unary(Scope* scope, Ast* operand, Operator_Unary op, u32 flags);

Ast* ast_create_comm_block(Scope* parent_scope, Scope* block_scope, Ast** commands, s32 command_count);
Ast* ast_create_comm_if(Scope* scope, Ast* condition, Ast* command_true, Ast* command_false);
Ast* ast_create_comm_for(Scope* scope, Ast* condition, Ast* body);
Ast* ast_create_comm_break(Scope* scope, Ast* lit);
Ast* ast_create_comm_continue(Scope* scope);
Ast* ast_create_comm_return(Scope* scope, Ast* expr);
Ast* ast_create_comm_variable_assignment(Scope* scope, Ast* lvalue, Ast* rvalue);

#if 0
bool ast_is_expression(Ast* ast);
Ast* create_proc(Memory_Arena* arena, Token* name, Token* extern_name, Type_Instance* return_type, Ast** arguments, int nargs, Ast* body, Scope* scope, Decl_Site* site);
Ast* create_named_argument(Memory_Arena* arena, Token* name, Type_Instance* type, Ast* default_val, int index, Scope* scope, Decl_Site* site);
Ast* create_variable_decl(Memory_Arena* arena, Token* name, Type_Instance* type, Ast* assign_val, Scope* scope, Decl_Site* site);
Ast* create_literal(Memory_Arena* arena, u32 flags, Token* lit_tok, u64 value = 0);
Ast* create_variable(Memory_Arena* arena, Token* var_token, Scope* scope);
Ast* create_block(Memory_Arena* arena, Scope* scope);
Ast* create_proc_call(Memory_Arena* arena, Token* name, Ast** args, Scope* scope);
Ast* create_if(Memory_Arena* arena, Ast* bool_exp, Ast* body, Ast* else_stmt, Scope* scope);
Ast* create_while(Memory_Arena* arena, Ast* bool_exp, Ast* body, Scope* scope);
Ast* create_return(Memory_Arena* arena, Ast* exp, Scope* scope, Token* token);
Ast* create_unary_expression(Memory_Arena* arena, Ast* operand, UnaryOperation op, Token* op_tok, u32 flags, Type_Instance* cast_type, Precedence precedence, Scope* scope);
Ast* create_binary_operation(Memory_Arena* arena, Ast* left_op, Ast *right_op, Token* op, Precedence precedence, Scope* scope);
Ast* create_break(Memory_Arena* arena, Scope* scope, Token* token);
Ast* create_continue(Memory_Arena* arena, Scope* scope, Token* token);
Ast* create_struct_decl(Memory_Arena* arena, Token* name, Ast** fields, int num_fields, Scope* struct_scope, Decl_Site* site);
Ast* create_enum_decl(Memory_Arena* arena, Token* name, Ast_EnumField* fields, int num_fields, Type_Instance* base_type, Scope* struct_scope, Decl_Site* site);
Ast* create_directive(Memory_Arena* arena, Token* directive_token, Ast* literal_argument, Ast* declaration);
Ast* create_constant(Memory_Arena* arena, Token* name, Ast* expression, Type_Instance* type, Scope* scope);

UnaryOperation get_unary_op(Token* token);
BinaryOperation get_binary_op(Token* token);

void block_push_command(Ast* block, Ast* command);
void push_ast_list(Ast*** list, Ast* arg);


void DEBUG_print_node(FILE* out, Ast* node);
void DEBUG_print_ast(FILE* out, Ast** ast);
void DEBUG_print_type(FILE* out, Type_Instance* type, bool short_ = true);

void DEBUG_print_indent_level();

#endif

void DEBUG_print_node(FILE* out, Ast* node);
void DEBUG_print_ast(FILE* out, Ast** ast);
void DEBUG_print_type(FILE* out, Type_Instance* type, bool short_ = true);