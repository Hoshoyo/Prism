#define _CRT_SECURE_NO_WARNINGS
#include "lightvm.h"
#include "ast.h"
#include <assert.h>

void
print_float_register(FILE* out, u8 reg) {
    switch(reg) {
        case FR0: fprintf(out, "FR0(32)"); break;
        case FR1: fprintf(out, "FR1(32)"); break;
        case FR2: fprintf(out, "FR2(32)"); break;
        case FR3: fprintf(out, "FR3(32)"); break;
        case FR4: fprintf(out, "FR4(64)"); break;
        case FR5: fprintf(out, "FR5(64)"); break;
        case FR6: fprintf(out, "FR6(64)"); break;
        case FR7: fprintf(out, "FR7(64)"); break;
        default: fprintf(out, "Invalid float register"); break;
    }
}

void
print_register(FILE* out, u8 reg, u8 byte_size) {
    switch(reg) {
        case R0: fprintf(out, "R0"); break;
        case R1: fprintf(out, "R1"); break;
        case R2: fprintf(out, "R2"); break;
        case R3: fprintf(out, "R3"); break;
        case R4: fprintf(out, "R4"); break;
        case R5: fprintf(out, "R5"); break;
        case R6: fprintf(out, "R6"); break;
        case R7: fprintf(out, "R7"); break;
        case RIP: fprintf(out, "RIP"); break;
        case RSP: fprintf(out, "RSP"); break;
        case RBP: fprintf(out, "RBP"); break;
        case RDP: fprintf(out, "RDP"); break;
        default: fprintf(out, "Invalid register"); break;
    }
    switch(byte_size) {
        case 1: fprintf(out, "b"); break;
        case 2: fprintf(out, "w"); break;
        case 4: fprintf(out, "d"); break;
        default: break;
    }
}

void print_immediate(FILE* out, u8 byte_size, u64 imm) {
    switch(byte_size) {
        case 1:
            fprintf(out, "0x%x", (u8)(imm & 0xff)); break;
        case 2:
            fprintf(out, "0x%x", (u16)(imm & 0xffff)); break;
        case 4:
            fprintf(out, "0x%x", (u32)(imm & 0xffffffff)); break;
        case 8:
            fprintf(out, "0x%llx", imm); break;
        default: fprintf(out, "Invalid immediate byte size");
    }
}

void
print_binary_instruction(FILE* out, Light_VM_Instruction instr, u64 immediate) {
    switch(instr.type) {
        case LVM_CMP:   fprintf(out, "CMP "); break;
        case LVM_SHL:   fprintf(out, "SHL "); break;
        case LVM_SHR:   fprintf(out, "SHR "); break;
        case LVM_OR:    fprintf(out, "OR "); break;
        case LVM_AND:   fprintf(out, "AND "); break;
        case LVM_XOR:   fprintf(out, "XOR "); break;
        case LVM_MOV:   fprintf(out, "MOV "); break;
        case LVM_ADD_S: fprintf(out, "ADDS "); break;
        case LVM_SUB_S: fprintf(out, "SUBS "); break;
        case LVM_MUL_S: fprintf(out, "MULS "); break;
        case LVM_DIV_S: fprintf(out, "DIVS "); break;
        case LVM_MOD_S: fprintf(out, "MODS "); break;
        case LVM_ADD_U: fprintf(out, "ADDU "); break;
        case LVM_SUB_U: fprintf(out, "SUBU "); break;
        case LVM_MUL_U: fprintf(out, "MULU "); break;
        case LVM_DIV_U: fprintf(out, "DIVU "); break;
        case LVM_MOD_U: fprintf(out, "MODU "); break;
        default: assert(0); break; // internal error
    }
    switch(instr.binary.addr_mode) {
        case BIN_ADDR_MODE_REG_TO_REG:{
            print_register(out, instr.binary.dst_reg, instr.binary.bytesize);
            fprintf(out, ", ");
            print_register(out, instr.binary.src_reg, instr.binary.bytesize);
        }break;
        case BIN_ADDR_MODE_REG_TO_MEM:{
            fprintf(out, "[");
            print_register(out, instr.binary.dst_reg, 8);
            fprintf(out, "], ");
            print_register(out, instr.binary.src_reg, instr.binary.bytesize);
        }break;
        case BIN_ADDR_MODE_REG_TO_IMM_MEM:{
            fprintf(out, "[");
            print_immediate(out, instr.imm_size_bytes, immediate);
            fprintf(out, "], ");
            print_register(out, instr.binary.src_reg, instr.binary.bytesize);
        }break;
        case BIN_ADDR_MODE_MEM_TO_REG:{
            print_register(out, instr.binary.dst_reg, instr.binary.bytesize);
            fprintf(out, ", [");
            print_register(out, instr.binary.src_reg, 8);
            fprintf(out, "]");
        }break;
        case BIN_ADDR_MODE_MEM_IMM_TO_REG:{
            print_register(out, instr.binary.dst_reg, instr.binary.bytesize);
            fprintf(out, ", [");
            print_immediate(out, instr.imm_size_bytes, immediate);
            fprintf(out, "]");
        }break;
        case BIN_ADDR_MODE_IMM_TO_REG:{
            print_register(out, instr.binary.dst_reg, instr.binary.bytesize);
            fprintf(out, ", ");
            print_immediate(out, instr.imm_size_bytes, immediate);
        }break;
        case BIN_ADDR_MODE_REG_TO_MEM_OFFSETED:{
            fprintf(out, "[");
            print_register(out, instr.binary.dst_reg, 8);
            fprintf(out, " + ");
            print_immediate(out, instr.imm_size_bytes, immediate);
            fprintf(out, "], ");
            print_register(out, instr.binary.src_reg, instr.binary.bytesize);
        }break;
        case BIN_ADDR_MODE_REG_OFFSETED_TO_REG: {
            print_register(out, instr.binary.dst_reg, instr.binary.bytesize);
            fprintf(out, ", [");
            print_register(out, instr.binary.src_reg, 8);
            fprintf(out, " + ");
            print_immediate(out, instr.imm_size_bytes, immediate);
            fprintf(out, "]");
        }break;
        default: {
            fprintf(out, "Invalid addressing mode");
        }break;
    }
}

void
print_push_instruction(FILE* out, Light_VM_Instruction instr, u64 immediate) {
    switch(instr.type) {
        case LVM_PUSH:   fprintf(out, "PUSH "); break;
        case LVM_EXPUSHI: fprintf(out, "EXPUSHI "); break;
        case LVM_EXPUSHF: fprintf(out, "EXPUSHF "); break;
        default: fprintf(out, "Invalid push instruction"); break;
    }
    switch(instr.push.addr_mode) {
        case PUSH_ADDR_MODE_IMMEDIATE:{
            print_immediate(out, instr.imm_size_bytes, immediate);
        } break;
        case PUSH_ADDR_MODE_IMMEDIATE_INDIRECT:{
            fprintf(out, "[");
            print_immediate(out, instr.imm_size_bytes, immediate);
            fprintf(out, "]");
        } break;
        case PUSH_ADDR_MODE_REGISTER:{
            if(instr.type == LVM_EXPUSHF) {
                print_float_register(out, instr.push.reg);
            } else {
                print_register(out, instr.push.reg, instr.push.byte_size);
            }
        } break;
        case PUSH_ADDR_MODE_REGISTER_INDIRECT:{
            fprintf(out, "[");
            print_register(out, instr.push.reg, 8);
            fprintf(out, "]");
        } break;
        default: fprintf(out, "Invalid push addressing mode"); break;
    }
}

void
print_unary_instruction(FILE* out, Light_VM_Instruction instr, u64 imm) {
    if(instr.type == LVM_FNEG) {
        fprintf(out, "FNEG ");
        print_float_register(out, instr.unary.reg);
    } else {
        switch(instr.type) {
            case LVM_NEG:  fprintf(out, "NEG "); break;
            case LVM_NOT:  fprintf(out, "NOT "); break;
            case LVM_POP:  fprintf(out, "POP "); break;
            default: fprintf(out, "Invalid unary instruction"); break;
        }
        print_register(out, instr.unary.reg, instr.unary.byte_size);
    }
}

void
print_alloc_instruction(FILE* out, Light_VM_Instruction instr, u64 imm) {
    assert(instr.type == LVM_ALLOC);
    fprintf(out, "ALLOC ");
    print_register(out, instr.alloc.dst_reg, 8);
    fprintf(out, ", ");
    print_register(out, instr.alloc.size_reg, instr.alloc.byte_size);
}

void
print_call_instruction(FILE* out, Light_VM_Instruction instr, u64 imm) {
    switch(instr.type) {
        case LVM_CALL:    fprintf(out, "CALL "); break;
        case LVM_EXTCALL: fprintf(out, "EXTCALL "); break;
        default: fprintf(out, "Invalid call instruction"); break;
    }
    switch(instr.branch.addr_mode) {
        case BRANCH_ADDR_MODE_IMMEDIATE_ABSOLUTE:{
            fprintf(out, "0x%llx", imm);
        } break;
        case BRANCH_ADDR_MODE_IMMEDIATE_RELATIVE:{
            print_immediate(out, instr.imm_size_bytes, imm);
        } break;
        case BRANCH_ADDR_MODE_REGISTER:{
            print_register(out, instr.branch.reg, 8);
        } break;
        case BRANCH_ADDR_MODE_REGISTER_INDIRECT:{
            fprintf(out, "[");
            print_register(out, instr.branch.reg, 8);
            fprintf(out, "]");
        } break;
        default: fprintf(out, "Invalid call addressing mode"); break;
    }
}

void
print_float_instruction(FILE* out, Light_VM_Instruction instr, u64 imm) {
    switch(instr.type) {
        case LVM_FADD: fprintf(out, "FADD "); break;
        case LVM_FSUB: fprintf(out, "FSUB "); break;
        case LVM_FDIV: fprintf(out, "FDIV "); break;
        case LVM_FMUL: fprintf(out, "FMUL "); break;
        case LVM_FCMP: fprintf(out, "FCMP "); break;
        case LVM_FMOV: fprintf(out, "FMOV "); break;
        default: fprintf(out, "Invalid float instruction"); break;
    }
    switch(instr.ifloat.addr_mode) {
        case FLOAT_ADDR_MODE_REG_TO_REG:{
            print_float_register(out, instr.ifloat.dst_reg);
            fprintf(out, ", ");
            print_float_register(out, instr.ifloat.src_reg);
        }break;
        case FLOAT_ADDR_MODE_REG_TO_MEM:{
            // dst reg must be a general purpose one
            fprintf(out, "[");
            print_register(out, instr.ifloat.dst_reg, 8);
            fprintf(out, "], ");
            print_float_register(out, instr.ifloat.src_reg);
        }break;
        case FLOAT_ADDR_MODE_MEM_TO_REG:{
            // src reg must be a general purpose one
            print_float_register(out, instr.ifloat.dst_reg);
            fprintf(out, ", [");
            print_register(out, instr.ifloat.src_reg, 8);
            fprintf(out, "]");
        }break;
        case FLOAT_ADDR_MODE_REG_TO_MEM_OFFSETED:{
            // dst reg must be a general purpose one
            fprintf(out, "[");
            print_register(out, instr.ifloat.dst_reg, 8);
            fprintf(out, " + ");
            print_immediate(out, instr.imm_size_bytes, imm);
            fprintf(out, "], ");
            print_float_register(out, instr.ifloat.src_reg);
        }break;
        case FLOAT_ADDR_MODE_REG_OFFSETED_TO_REG:{
            print_float_register(out, instr.ifloat.dst_reg);
            fprintf(out, ", [");
            print_register(out, instr.ifloat.src_reg, 8);
            fprintf(out, " + ");
            print_immediate(out, instr.imm_size_bytes, imm);
            fprintf(out, "]");
        }break;
        default:{
            fprintf(out, "Invalid float addressing mode");
        }break;
    }
}

void
print_float_branch_instruction(FILE* out, Light_VM_Instruction instr, u64 imm) {
    switch(instr.type) {
        case LVM_FBEQ: fprintf(out, "FBEQ "); break;
        case LVM_FBNE: fprintf(out, "FBNE "); break;
        case LVM_FBGT: fprintf(out, "FBGT "); break;
        case LVM_FBLT: fprintf(out, "FBLT "); break;
        default: fprintf(out, "Invalid float branch instruction"); break;        
    }
    switch(instr.branch.addr_mode) {
        case BRANCH_ADDR_MODE_IMMEDIATE_ABSOLUTE:
            print_immediate(out, instr.imm_size_bytes, imm);
            break;
        case BRANCH_ADDR_MODE_IMMEDIATE_RELATIVE:
            print_immediate(out, instr.imm_size_bytes, imm);
            fprintf(out, "(rel)");
            break;
        case BRANCH_ADDR_MODE_REGISTER:
            print_register(out, instr.branch.reg, 8);
            break;
        case BRANCH_ADDR_MODE_REGISTER_INDIRECT:
            fprintf(out, "[");
            print_register(out, instr.branch.reg, 8);
            fprintf(out, "]");
            break;
        default: fprintf(out, "Invalid branch addressing mode"); break;
    }
}

void
print_cmpmov_instruction(FILE* out, Light_VM_Instruction instr, u64 imm) {
    switch(instr.type) {
        case LVM_MOVEQ:   fprintf(out, "MOVEQ "); break;
        case LVM_MOVNE:   fprintf(out, "MOVNE "); break;
        case LVM_MOVLT_S: fprintf(out, "MOVLTS "); break;
        case LVM_MOVGT_S: fprintf(out, "MOVGTS "); break;
        case LVM_MOVLE_S: fprintf(out, "MOVLES "); break;
        case LVM_MOVGE_S: fprintf(out, "MOVGES "); break;
        case LVM_MOVLT_U: fprintf(out, "MOVLTU "); break;
        case LVM_MOVGT_U: fprintf(out, "MOVGTU "); break;
        case LVM_MOVLE_U: fprintf(out, "MOVLEU "); break;
        case LVM_MOVGE_U: fprintf(out, "MOVGEU "); break;
        default: fprintf(out, "Invalid comparison move instruction"); break;
    }
    print_register(out, instr.unary.reg, instr.unary.byte_size);
}

void
print_branch_instruction(FILE* out, Light_VM_Instruction instr, u64 imm) {
    switch(instr.type) {
        case LVM_BEQ:   fprintf(out, "BEQ "); break;
        case LVM_BNE:   fprintf(out, "BNE "); break;
        case LVM_BLT_S: fprintf(out, "BLTS "); break;
        case LVM_BGT_S: fprintf(out, "BGTS "); break;
        case LVM_BLE_S: fprintf(out, "BLES "); break;
        case LVM_BGE_S: fprintf(out, "BGES "); break;
        case LVM_BLT_U: fprintf(out, "BLTU "); break;
        case LVM_BGT_U: fprintf(out, "BGTU "); break;
        case LVM_BLE_U: fprintf(out, "BLEU "); break;
        case LVM_BGE_U: fprintf(out, "BGEU "); break;
        case LVM_JMP:   fprintf(out, "JMP "); break;
        default: fprintf(out, "Invalid comparison instruction"); break;
    }

    switch(instr.branch.addr_mode) {
        case BRANCH_ADDR_MODE_IMMEDIATE_ABSOLUTE:
            print_immediate(out, instr.imm_size_bytes, imm);
            break;
        case BRANCH_ADDR_MODE_IMMEDIATE_RELATIVE:
            print_immediate(out, instr.imm_size_bytes, imm);
            fprintf(out, "(rel)");
            break;
        case BRANCH_ADDR_MODE_REGISTER:
            print_register(out, instr.branch.reg, 8);
            break;
        case BRANCH_ADDR_MODE_REGISTER_INDIRECT:
            fprintf(out, "[");
            print_register(out, instr.branch.reg, 8);
            fprintf(out, "]");
            break;
        default: fprintf(out, "Invalid branch addressing mode"); break;
    }
}

void
print_copy_instruction(FILE* out, Light_VM_Instruction instr, u64 imm) {
    assert(instr.type == LVM_COPY);
    fprintf(out, "COPY ");
    print_register(out, instr.copy.dst_reg, 8);
    fprintf(out, ",");
    print_register(out, instr.copy.src_reg, 8);
    fprintf(out, ",");
    print_register(out, instr.copy.size_bytes_reg, 8);
}

void 
light_vm_print_instruction(FILE* out, Light_VM_Instruction instr, uint64_t imm) {
    
    switch(instr.type) {
        case LVM_NOP: fprintf(out, "NOP"); break;

        // Binary instructions
        case LVM_CMP:
        case LVM_SHL: case LVM_SHR:
        case LVM_OR: case LVM_AND:
        case LVM_XOR: case LVM_MOV:
        case LVM_ADD_S: case LVM_SUB_S:
        case LVM_MUL_S: case LVM_DIV_S:
        case LVM_MOD_S: case LVM_ADD_U:
        case LVM_SUB_U: case LVM_MUL_U:
        case LVM_DIV_U: case LVM_MOD_U:
            print_binary_instruction(out, instr, imm); 
            break;

        // Floating point
        case LVM_FADD:
        case LVM_FSUB:
        case LVM_FMUL:
        case LVM_FDIV:
        case LVM_FCMP:
        case LVM_FMOV:
            print_float_instruction(out, instr, imm); 
            break;

        // Unary instructions
        case LVM_NOT:
        case LVM_NEG:
        case LVM_FNEG:
        case LVM_POP:
            print_unary_instruction(out, instr, imm);
            break;

        case LVM_PUSH:
        case LVM_EXPUSHI:
        case LVM_EXPUSHF:
            print_push_instruction(out, instr, imm);
            break;

        case LVM_EXPOP: fprintf(out, "EXPOP"); break;

        // Comparison/Branch
        case LVM_BEQ:
        case LVM_BNE:
        case LVM_BLT_S:
        case LVM_BGT_S:
        case LVM_BLE_S:
        case LVM_BGE_S:
        case LVM_BLT_U:
        case LVM_BGT_U:
        case LVM_BLE_U:
        case LVM_BGE_U:
        case LVM_JMP:
            print_branch_instruction(out, instr, imm);
            break;

        case LVM_MOVEQ:
        case LVM_MOVNE:
        case LVM_MOVLT_S:
        case LVM_MOVGT_S:
        case LVM_MOVLE_S:
        case LVM_MOVGE_S:
        case LVM_MOVLT_U:
        case LVM_MOVGT_U:
        case LVM_MOVLE_U:
        case LVM_MOVGE_U:
            print_cmpmov_instruction(out, instr, imm);
            break;

        case LVM_FBEQ:
        case LVM_FBNE:
        case LVM_FBGT:
        case LVM_FBLT:
            print_float_branch_instruction(out, instr, imm);
            break;

        case LVM_EXTCALL:
        case LVM_CALL:
            print_call_instruction(out, instr, imm);
            break;
        case LVM_RET:  fprintf(out, "RET"); break;

        case LVM_COPY:
            print_copy_instruction(out, instr, imm);
            break;
        case LVM_ALLOC:
            print_alloc_instruction(out, instr, imm);
            break;

        // TODO(psv):
        case LVM_ASSERT:  fprintf(out, "ASSERT"); break;

        // Halt
        case LVM_HLT: fprintf(out, "HLT"); break;

        default: {
            fprintf(out, "UNKNOWN");
        }break;
    }
    fprintf(out, "\n");
}

#if defined(__linux__)
void 
light_vm_debug_dump_registers(FILE* out, Light_VM_State* state, u32 flags) {
    if(flags & LVM_PRINT_DECIMAL) {
        fprintf(out, "R0: %ld \t R1: %ld\n", state->registers[R0], state->registers[R1]);
        fprintf(out, "R2: %ld \t R3: %ld\n", state->registers[R2], state->registers[R3]);
        fprintf(out, "R4: %ld \t R5: %ld\n", state->registers[R4], state->registers[R5]);
        fprintf(out, "R6: %ld \t R7: %ld\n", state->registers[R6], state->registers[R7]);
        fprintf(out, "\n");
        fprintf(out, "RSP: %ld \t RBP: %ld\n", state->registers[RSP], state->registers[RBP]);
        fprintf(out, "RIP: %ld \t RDP: %ld\n", state->registers[RIP], state->registers[RDP]);
    } else {
        fprintf(out, "R0: 0x%lx \t R1: 0x%lx\n", state->registers[R0], state->registers[R1]);
        fprintf(out, "R2: 0x%lx \t R3: 0x%lx\n", state->registers[R2], state->registers[R3]);
        fprintf(out, "R4: 0x%lx \t R5: 0x%lx\n", state->registers[R4], state->registers[R5]);
        fprintf(out, "R6: 0x%lx \t R7: 0x%lx\n", state->registers[R6], state->registers[R7]);
        fprintf(out, "\n");
        fprintf(out, "RSP: 0x%lx \t RBP: 0x%lx\n", state->registers[RSP], state->registers[RBP]);
        fprintf(out, "RIP: 0x%lx \t RDP: 0x%lx\n", state->registers[RIP], state->registers[RDP]);
    }
    if(flags & LVM_PRINT_FLOATING_POINT_REGISTERS) {
        fprintf(out, "\n");
        fprintf(out, "FR0: %f \t FR1: %f\n", state->f32registers[FR0], state->f32registers[FR1]);
        fprintf(out, "FR2: %f \t FR3: %f\n", state->f32registers[FR2], state->f32registers[FR3]);
        fprintf(out, "FR4: %f \t FR5: %f\n", state->f64registers[FR4], state->f64registers[FR5]);
        fprintf(out, "FR6: %f \t FR7: %f\n", state->f64registers[FR6], state->f64registers[FR7]);
    }
    if(flags & LVM_PRINT_FLAGS_REGISTER) {
        fprintf(out, "\n");
        fprintf(out, "Carry: %d ",   state->rflags.carry);
        fprintf(out, "Zero: %d ",    state->rflags.zerof);
        fprintf(out, "Sign: %d ",    state->rflags.sign);
        fprintf(out, "Overflow: %d", state->rflags.overflow);
        fprintf(out, "\n");
    }
}
#elif defined(_WIN32) || defined(_WIN64)
void 
light_vm_debug_dump_registers(FILE* out, Light_VM_State* state, u32 flags) {
    if(flags & LVM_PRINT_DECIMAL) {
        fprintf(out, "R0: %lld \t R1: %lld\n", state->registers[R0], state->registers[R1]);
        fprintf(out, "R2: %lld \t R3: %lld\n", state->registers[R2], state->registers[R3]);
        fprintf(out, "R4: %lld \t R5: %lld\n", state->registers[R4], state->registers[R5]);
        fprintf(out, "R6: %lld \t R7: %lld\n", state->registers[R6], state->registers[R7]);
        fprintf(out, "\n");
        fprintf(out, "RSP: %lld \t RBP: %lld\n", state->registers[RSP], state->registers[RBP]);
        fprintf(out, "RIP: %lld \t RDP: %lld\n", state->registers[RIP], state->registers[RDP]);
    } else {
        fprintf(out, "R0: 0x%llx \t R1: 0x%llx\n", state->registers[R0], state->registers[R1]);
        fprintf(out, "R2: 0x%llx \t R3: 0x%llx\n", state->registers[R2], state->registers[R3]);
        fprintf(out, "R4: 0x%llx \t R5: 0x%llx\n", state->registers[R4], state->registers[R5]);
        fprintf(out, "R6: 0x%llx \t R7: 0x%llx\n", state->registers[R6], state->registers[R7]);
        fprintf(out, "\n");
        fprintf(out, "RSP: 0x%llx \t RBP: 0x%llx\n", state->registers[RSP], state->registers[RBP]);
        fprintf(out, "RIP: 0x%llx \t RDP: 0x%llx\n", state->registers[RIP], state->registers[RDP]);
    }
    if(flags & LVM_PRINT_FLOATING_POINT_REGISTERS) {
        fprintf(out, "\n");
        fprintf(out, "FR0: %f \t FR1: %f\n", state->f32registers[FR0], state->f32registers[FR1]);
        fprintf(out, "FR2: %f \t FR3: %f\n", state->f32registers[FR2], state->f32registers[FR3]);
        fprintf(out, "FR4: %f \t FR5: %f\n", state->f64registers[FR4], state->f64registers[FR5]);
        fprintf(out, "FR6: %f \t FR7: %f\n", state->f64registers[FR6], state->f64registers[FR7]);
    }
    if(flags & LVM_PRINT_FLAGS_REGISTER) {
        fprintf(out, "\n");
        fprintf(out, "Carry: %d ",   state->rflags.carry);
        fprintf(out, "Zero: %d ",    state->rflags.zerof);
        fprintf(out, "Sign: %d ",    state->rflags.sign);
        fprintf(out, "Overflow: %d", state->rflags.overflow);
        fprintf(out, "\n");
    }
}
#endif

void
light_vm_debug_dump_code(FILE* out, Light_VM_State* state) {
    for(u64 i = 0 ;;) {
        u64 imm = 0;
        Light_VM_Instruction inst = *(Light_VM_Instruction*)((u8*)state->code.block + i);
        i += sizeof(Light_VM_Instruction);

        if(inst.type == LVM_NOP) break;

        switch(inst.imm_size_bytes) {
            case 0: break;
            case 1: 
                imm = *(u8*)((u8*)state->code.block + i);
                break;
            case 2:
                imm = *(u16*)((u8*)state->code.block + i);
                break;
            case 4:
                imm = *(u32*)((u8*)state->code.block + i);
                break;
            case 8:
                imm = *(u64*)((u8*)state->code.block + i);
                break;
            default: break;
        }
        light_vm_print_instruction(out, inst, imm);
        i += inst.imm_size_bytes;
    }
}