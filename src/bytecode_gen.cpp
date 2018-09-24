#include "bytecode_gen.h"
#include <ho_system.h>
#include "interpreter.h"
#include "type.h"

void* load_address_of_external_function(string* name, HMODULE library);
HMODULE load_library_dynamic(string* library);

struct Register_Allocation{
	bool allocated;
	bool in_stack;
	s32  age;
};
// TODO(psv): thread safety
static Register_Allocation register_allocated[NUM_REGS];
static Registers* register_stack;

// The oldest register, the one allocated farther back gets allocated in
// case of all allocated.
Registers reg_allocate(bool* push_before_using) {
	*push_before_using = false;

	s32 oldest = -1;
	Registers result = NO_REG;
	for (int i = 0; i <= R_8; ++i) {
		if (!register_allocated[i].allocated) {
			register_allocated[i].allocated = true;
			register_allocated[i].age = 0;
			result = (Registers)i;
			break;
		}
		if (register_allocated[i].age > oldest) {
			oldest = i;
		}
		register_allocated[i].age++;
	}
	if (result != NO_REG) return result;

	// push the oldest to the stack
	if (!register_stack) {
		register_stack = array_create(Registers, 512);
	}
	register_allocated[oldest].in_stack = true;
	register_allocated[oldest].age++;
	array_push(register_stack, (Registers*)&oldest);
	*push_before_using = true;
	return (Registers)oldest;
}

// bool indicates if the caller needs to generate pop call for this register
bool reg_free(Registers r) {
	bool pop = false;
	// free the last in the stack rather
	if (register_allocated[r].in_stack) {
		Registers last = *(Registers*)array_pop(register_stack);
		assert(last == r);
		pop = true;
	}
	register_allocated[r].allocated = false;
	register_allocated[r].in_stack = false;
	register_allocated[r].age = 0;
	return pop;
}

s64 gen_entry_point() {
	// push sb				; 8  bytes
	// push next_address	; 16 bytes
	// mov ss, 16			; 16 bytes
	// jmp main				; 16 bytes
	// hlt					; 8 bytes
	// main: 

	u64 start_address = move_code_offset(0);

	u64 o1 = push_instruction(make_instruction(PUSH, INSTR_QWORD, SINGLE_REG, R_SB, NO_REG, 0, 0));
	u64* hlt_relative_address;
	u64 o2 = push_instruction(make_instruction(PUSH, INSTR_QWORD | IMMEDIATE_VALUE, SINGLE_MEM, NO_REG, NO_REG, 0, 0), &hlt_relative_address);
	u64 o3 = push_instruction(make_instruction(MOV, INSTR_QWORD | IMMEDIATE_VALUE, MEM_TO_REG, R_SS, NO_REG, 0, 0), 2 * 8);
	u64* main_relative_address;
	u64 o4 = push_instruction(make_instruction(JMP, INSTR_QWORD | IMMEDIATE_VALUE, SINGLE_MEM, NO_REG, NO_REG, 0, 0), &main_relative_address);
	u64 end_address = push_instruction(make_instruction(HLT, 0, 0, NO_REG, NO_REG, 0, 0));

	*hlt_relative_address = o4;
	*main_relative_address = end_address;

	return end_address - start_address;
}

// returns the number of bytes that advanced
s64 gen_proc_prologue(Ast* proc_body, s64 offset)
{
	// push sb
	// mov	sb, sp
	// add	sp, stack_size

	u64 start_address = move_code_offset(offset);
	s64 stack_size = 0;

	// calculate stack size from proc body
	Scope* scope = proc_body->decl_procedure.body->comm_block.block_scope;
	int num_decls = scope->decl_count;

	// @TODO OPTIMIZE THIS!!!!!
	// @TODO OPTIMIZE THIS!!!!!
	// @TODO OPTIMIZE THIS!!!!!
	// make a data struct on the scope to store all the declarations and its respective offsets within the stack
	if (num_decls > 0) {
		for (int i = 0; i < scope->symb_table.entries_count; ++i) {
			if (scope->symb_table.entries[i].occupied) {
				Ast* decl_node = scope->symb_table.entries[i].decl_node;
				if (decl_node->node_type == AST_DECL_VARIABLE) {
					assert(decl_node->decl_variable.variable_type->type_size_bits != 0);
					stack_size += decl_node->decl_variable.variable_type->type_size_bits / 8;
				}
			}
		}
	}

	push_instruction(make_instruction(PUSH, INSTR_QWORD, SINGLE_REG, R_SB, NO_REG, 0, 0));
	push_instruction(make_instruction(MOV, INSTR_QWORD, REG_TO_REG, R_SB, R_SP, 0, 0));
	u64 end_address = push_instruction(make_instruction(ADD, INSTR_QWORD | IMMEDIATE_VALUE, MEM_TO_REG, R_SP, NO_REG, 0, 0), stack_size);

#if 0
	char* add = (char*)get_data_segment_address();
	push_instruction(make_instruction(PUSH, INSTR_QWORD | IMMEDIATE_VALUE, SINGLE_MEM, NO_REG, NO_REG, 0, 0), 0);
	push_instruction(make_instruction(PUSH, INSTR_QWORD | IMMEDIATE_VALUE, SINGLE_MEM, NO_REG, NO_REG, 0, 0), *(u64*)get_data_segment_address());		// check SINGLE_MEM_PTR
	push_instruction(make_instruction(PUSH, INSTR_QWORD | IMMEDIATE_VALUE, SINGLE_MEM, NO_REG, NO_REG, 0, 0), *(u64*)get_data_segment_address());
	push_instruction(make_instruction(PUSH, INSTR_QWORD | IMMEDIATE_VALUE, SINGLE_MEM, NO_REG, NO_REG, 0, 0), 0);

	push_instruction(make_instruction(MOV, INSTR_QWORD | IMMEDIATE_VALUE, MEM_TO_REG, R_SS, NO_REG, 0, 0), 8 * 4);

	Token t;
	t.value = MAKE_STRING("MessageBoxA");

	string libname = MAKE_STRING("user32.dll");
	HMODULE libdll = load_library_dynamic(&libname);
	void * func = load_address_of_external_function(&t.value, libdll);
	end_address = push_instruction(make_instruction(EXTCALL, INSTR_QWORD | IMMEDIATE_VALUE, SINGLE_MEM, NO_REG, NO_REG, 0, 0), (u64)func);
#endif

	return end_address - start_address;
}

s64 gen_proc_epilogue(s64 offset) {
	// mov	sp, sb
	// pop	sb
	// pop	ss		; return address
	// jmp	ss		; return from call

	u64 start_address = move_code_offset(offset);

	push_instruction(make_instruction(MOV, INSTR_QWORD, REG_TO_REG, R_SP, R_SB, 0, 0));
	push_instruction(make_instruction(POP, INSTR_QWORD, SINGLE_REG, R_SB, NO_REG, 0, 0));
	u64 r = push_instruction(make_instruction(POP, INSTR_QWORD, SINGLE_REG, R_SS, NO_REG, 0, 0));
	u64 end_address = push_instruction(make_instruction(JMP, INSTR_QWORD, SINGLE_REG, R_SS, NO_REG, 0, 0));

	u64 diff = end_address - r;

	return end_address - start_address;
}

struct Expr_Generation {
	s64 offset;
	Registers reg;
};

Expr_Generation gen_code_for_expression(Ast* expr, s64 offset) {
	assert(expr->flags & AST_FLAG_IS_EXPRESSION);
	Expr_Generation result = {0};

	u64 start_address = move_code_offset(offset);
	s64 start_offset = offset;
	u64 end_address = 0;

	switch (expr->node_type) {

	case AST_EXPRESSION_BINARY: {
		// Calculate both sides
		Expr_Generation left = gen_code_for_expression(expr->expr_binary.left, offset);
		offset += left.offset;
		Expr_Generation right = gen_code_for_expression(expr->expr_binary.right, offset);
		offset += right.offset;

		u16 instruction = 0;

		switch (expr->expr_binary.op) {
			case OP_BINARY_PLUS:			instruction = ADD; break; // +
			case OP_BINARY_MINUS:			instruction = SUB; break; // -
			case OP_BINARY_MULT:			instruction = MUL; break; // *
			case OP_BINARY_DIV:				instruction = DIV; break; // /
			case OP_BINARY_MOD:				instruction = MOD; break; // %
			case OP_BINARY_AND:				instruction = AND; break; // &
			case OP_BINARY_OR:				instruction = OR; break;  // |
			case OP_BINARY_XOR:				instruction = XOR; break; // ^
			case OP_BINARY_SHL:				instruction = SHL; break; // <<
			case OP_BINARY_SHR:				instruction = SHR; break; // >>

			case OP_BINARY_LT:				// <
			case OP_BINARY_GT:				// >
			case OP_BINARY_LE:				// <=
			case OP_BINARY_GE:				// >=
			case OP_BINARY_EQUAL:			// ==
			case OP_BINARY_NOT_EQUAL:		// !=
				instruction = CMP;
				break;

			case OP_BINARY_LOGIC_AND:		instruction = AND; break; // &&
			case OP_BINARY_LOGIC_OR:		instruction = OR; break;  // ||

			// TODO(psv): dot and vector acessing operators
			case OP_BINARY_DOT:				assert_msg(0, "unimplemented bytecode generation for dot operator"); break; // .
			case OP_BINARY_VECTOR_ACCESS:	assert_msg(0, "unimplemented bytecode generation for vector acessing"); break; // []
			default: assert_msg(0, "invalid expression in bytecode generation");  break;
		}

		u16 flags = INSTR_DWORD | SIGNED;
		end_address = push_instruction(make_instruction(instruction, flags, REG_TO_REG, left.reg, right.reg, 0, 0));
		bool pop = reg_free(right.reg);
		if (pop) {
			end_address = push_instruction(make_instruction(POP, INSTR_QWORD, SINGLE_REG, result.reg, NO_REG, 0, 0));
		}
		result.reg = left.reg;
	} break;


	case AST_EXPRESSION_LITERAL: {
		bool push_before_use = false;
		Registers result_register = reg_allocate(&push_before_use);
		result.reg = result_register;
		if (push_before_use) {
			end_address = push_instruction(make_instruction(PUSH, INSTR_QWORD, SINGLE_REG, result_register, NO_REG, 0, 0));
		}

		switch (expr->expr_literal.type) {
		case LITERAL_SINT:
		case LITERAL_BIN_INT:
		case LITERAL_HEX_INT:
		case LITERAL_UINT: {
			switch (expr->type_return->primitive) {
			case TYPE_PRIMITIVE_U8:
			case TYPE_PRIMITIVE_S8:
				end_address = push_instruction(make_instruction(MOV, INSTR_BYTE|IMMEDIATE_VALUE, MEM_TO_REG, result_register, NO_REG, 0, 0), expr->expr_literal.value_u64);
				break;
			case TYPE_PRIMITIVE_U16:
			case TYPE_PRIMITIVE_S16:
				end_address = push_instruction(make_instruction(MOV, INSTR_WORD | IMMEDIATE_VALUE, MEM_TO_REG, result_register, NO_REG, 0, 0), expr->expr_literal.value_u64);
				break;
			case TYPE_PRIMITIVE_U32:
			case TYPE_PRIMITIVE_S32:
				end_address = push_instruction(make_instruction(MOV, INSTR_DWORD | IMMEDIATE_VALUE, MEM_TO_REG, result_register, NO_REG, 0, 0), expr->expr_literal.value_u64);
				break;
			case TYPE_PRIMITIVE_U64:
			case TYPE_PRIMITIVE_S64:
				end_address = push_instruction(make_instruction(MOV, INSTR_QWORD | IMMEDIATE_VALUE, MEM_TO_REG, result_register, NO_REG, 0, 0), expr->expr_literal.value_u64);
				break;
			}
		}break;
		case LITERAL_FLOAT: {

		}break;
		case LITERAL_BOOL: {
			end_address = push_instruction(make_instruction(MOV, INSTR_QWORD | IMMEDIATE_VALUE, MEM_TO_REG, result_register, NO_REG, 0, 0), (u64)expr->expr_literal.value_bool);
		}break;
		case LITERAL_POINTER: {
			end_address = push_instruction(make_instruction(MOV, INSTR_QWORD | IMMEDIATE_VALUE, MEM_TO_REG, result_register, NO_REG, 0, 0), (u64)0);
		}break;
		case LITERAL_ARRAY: {

		}break;
		case LITERAL_STRUCT: {

		} break;
		}
	} break; // case literal

	default:break;
	}

	result.offset = end_address - start_address;
	return result;
}

/*
	For each node generate code
*/
s64 gen_code_node(Ast* node, s64 offset) {
	if (!node) return 0;

	u64 start_address = move_code_offset(offset);
	u64 end_address = start_address;
	s64 start_offset = offset;

	switch (node->node_type) {
		case AST_DECL_PROCEDURE: {
			offset += gen_proc_prologue(node, offset);
			offset += gen_code_node(node->decl_procedure.body, offset);
			offset += gen_proc_epilogue(offset);
		}break;
		case AST_COMMAND_BLOCK: {
			for (size_t i = 0; i < array_get_length(node->comm_block.commands); ++i) {
				Ast* comm = node->comm_block.commands[i];
				offset += gen_code_node(comm, offset);
			}
		}break;
		case AST_COMMAND_RETURN: {
			
			if (node->comm_return.expression) {
				Expr_Generation result = gen_code_for_expression(node->comm_return.expression, offset);
				offset += result.offset;
				
				bool pop = reg_free(R_0);	// R_0 must be free in order to use it for the return value
				if (pop) {
					end_address = push_instruction(make_instruction(POP, INSTR_QWORD, SINGLE_REG, R_0, NO_REG, 0, 0));
				}

				if (result.reg != R_0) {
					end_address = push_instruction(make_instruction(MOV, INSTR_QWORD, REG_TO_REG, R_0, result.reg, 0, 0));
					bool pop = reg_free(result.reg);
					if (pop) {
						end_address = push_instruction(make_instruction(POP, INSTR_QWORD, SINGLE_REG, result.reg, NO_REG, 0, 0));
					}
				}
			}
			if(end_address != start_address)
				offset = end_address - start_address;
		}break;
		default: break;
	}

	return offset - start_offset;
}

void bytecode_generate(Ast** top_level) {
	s64 offset = 0;
	offset = gen_entry_point();

	for (size_t i = 0; i < array_get_length(top_level); ++i) {
		Ast* node = top_level[i];

		offset += gen_code_node(node, offset);
	}
	HALT;
}


/*
	Load libraries dynamically
*/

#ifdef _WIN64
// @TODO linux version
HMODULE load_library_dynamic(string* library) {
	char* c_name_library = make_c_string(*library);
	HMODULE lib = LoadLibraryA(c_name_library);
	free(c_name_library);
	return lib;
}

void free_library(HMODULE lib) {
	FreeLibrary(lib);
}

// @TODO linux version
// @TODO OPTIMIZE, load library once for all external calls
void* load_address_of_external_function(string* name, HMODULE library) {
	char* c_name = make_c_string(*name);
	void* mem = GetProcAddress(library, c_name);
	free(c_name);
	return mem;
}
#else
#error "os not supported"
#endif