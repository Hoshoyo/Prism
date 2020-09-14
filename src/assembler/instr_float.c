#include "util.h"

u8*
emit_movs_reg_to_mem(u8* stream, X64_Addressing_Mode mode, X64_XMM_Register src, X64_Register dst, bool single_precision, u8 disp8, uint32_t disp32)
{
    assert(mode == INDIRECT || mode == INDIRECT_BYTE_DISPLACED || mode == INDIRECT_DWORD_DISPLACED);
    *stream++ = (single_precision) ? 0xf3 : 0xf2;
    *stream++ = 0x0f;
    *stream++ = 0x11;
    *stream++ = make_modrm(mode, src, register_representation(dst));
    stream = emit_displacement(mode, stream, disp8, disp32);
    return stream;
}

u8*
emit_movs_mem_to_reg(u8* stream, X64_Addressing_Mode mode, X64_XMM_Register dest, X64_Register src, bool single_precision, u8 disp8, uint32_t disp32)
{
    assert(mode == INDIRECT || mode == INDIRECT_BYTE_DISPLACED || mode == INDIRECT_DWORD_DISPLACED);
    *stream++ = (single_precision) ? 0xf3 : 0xf2;
    *stream++ = 0x0f;
    *stream++ = 0x10;
    *stream++ = make_modrm(mode, dest, register_representation(src));
    stream = emit_displacement(mode, stream, disp8, disp32);
    return stream;
}

u8*
emit_movs_direct(u8* stream, X64_XMM_Register dest, X64_XMM_Register src, bool single_precision)
{
    *stream++ = (single_precision) ? 0xf3 : 0xf2;
    *stream++ = 0x0f;
    *stream++ = 0x10;
    *stream++ = make_modrm(DIRECT, dest, src);
    return stream;
}

u8*
emit_arith_sse(u8* stream, X64_XMM_Arithmetic_Instr instr, X64_XMM_Register dest, X64_XMM_Register src, bool single_precision)
{
    *stream++ = (single_precision) ? 0xf3 : 0xf2;
    *stream++ = 0x0f;
    *stream++ = instr;
    *stream++ = make_modrm(DIRECT, dest, src);
    return stream;
}

u8*
emit_cmp_sse(u8* stream, X64_SSE_Compare_Flag flag, X64_XMM_Register r1, X64_XMM_Register r2)
{
    *stream++ = 0xf3;
    *stream++ = 0x0f;
    *stream++ = 0xc2;
    *stream++ = make_modrm(DIRECT, r1, r2);
    *stream++ = flag;

    return stream;
}

u8*
emit_cvt(u8* stream, X64_SSE_Convert_Instr instr, X64_Addressing_Mode mode, u8 src, u8 dest, u8 disp8, uint32_t disp32, bool single_prec)
{
    *stream++ = (single_prec) ? 0xf3 : 0xf2;
    *stream++ = 0x0f;
    *stream++ = instr;
    *stream++ = make_modrm(mode, register_representation(dest), register_representation(src));
    stream = emit_displacement(mode, stream, disp8, disp32);
    return stream;
}

u8*
emit_float_test(u8* stream)
{
    bool single_prec = false;
#if 1
    stream = emit_cmp_sse(stream, SSE_CMP_EQ, XMM1, XMM2);
#endif
#if 1
    stream = emit_cvt(stream, SSE_CVT_F32_INT32, DIRECT, XMM3, EBX, 0, 0, single_prec);
    stream = emit_cvt(stream, SSE_CVT_F32_INT32, INDIRECT, XMM3, EBX, 0, 0, single_prec);

    stream = emit_cvt(stream, SSE_CVT_INT32_F32, DIRECT, XMM3, EBX, 0, 0, single_prec);
    stream = emit_cvt(stream, SSE_CVT_INT32_F32, INDIRECT, XMM3, EBX, 0, 0, single_prec);

    stream = emit_cvt(stream, SSE_CVT_F64_F32, DIRECT, XMM3, XMM4, 0, 0, single_prec);
    stream = emit_cvt(stream, SSE_CVT_F64_F32, INDIRECT, XMM3, EBX, 0, 0, single_prec);
#endif
#if 0
    stream = emit_movs_mem_to_reg(stream, INDIRECT, XMM7, RAX, single_prec, 0, 0);
    stream = emit_movs_reg_to_mem(stream, INDIRECT, XMM7, RAX, single_prec, 0, 0);
    stream = emit_movs_direct(stream, XMM7, XMM3, single_prec);
    stream = emit_movs_mem_to_reg(stream, INDIRECT, XMM0, RBX, single_prec, 0, 0);
    stream = emit_movs_mem_to_reg(stream, INDIRECT, XMM0, RCX, single_prec, 0, 0);
    stream = emit_movs_mem_to_reg(stream, INDIRECT, XMM0, RDX, single_prec, 0, 0);
    stream = emit_movs_mem_to_reg(stream, INDIRECT_BYTE_DISPLACED, XMM0, RBX, single_prec, 0x12, 0);
    stream = emit_movs_mem_to_reg(stream, INDIRECT_DWORD_DISPLACED, XMM0, RCX, single_prec, 0, 0x12345678);
    //stream = emit_movss(stream, RSP);
    stream = emit_movs_mem_to_reg(stream, INDIRECT, XMM1, RDI, single_prec, 0, 0);
    stream = emit_movs_mem_to_reg(stream, INDIRECT, XMM1, RSI, single_prec, 0, 0);
#endif
#if 0
    stream = emit_arith_sse(stream, XMM_ADDS, XMM1, XMM2, single_prec);
    stream = emit_arith_sse(stream, XMM_SUBS, XMM1, XMM2, single_prec);
    stream = emit_arith_sse(stream, XMM_MULS, XMM1, XMM2, single_prec);
    stream = emit_arith_sse(stream, XMM_DIVS, XMM1, XMM2, single_prec);
#endif
    return stream;
}