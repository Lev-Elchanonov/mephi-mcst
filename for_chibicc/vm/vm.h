#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>


extern uint64_t r[1024];
extern uint64_t vstack[1024];
extern int vsp;
extern uint64_t rax;


// ПЕРЕСЫЛКА
// Для MOV(r[2], r[3]) или MOV(r[2], 0)
#define MOV(dst, src)        do { dst = (uint64_t)(src); } while(0)

// АРИФМЕТИКА
#define ADD(dst, src1, src2) do { dst = (uint64_t)((int64_t)(src1) + (int64_t)(src2)); } while(0)
#define SUB(dst, src1, src2) do { dst = (uint64_t)((int64_t)(src1) - (int64_t)(src2)); } while(0)
#define MUL(dst, src1, src2) do { dst = (uint64_t)((int64_t)(src1) * (int64_t)(src2)); } while(0)
#define DIV(dst, src1, src2) do { dst = (uint64_t)((int64_t)(src1) / (int64_t)(src2)); } while(0)
#define AND(dst, src1, src2) do { dst = (src1) & (src2); } while(0)
#define OR(dst, src1, src2)  do { dst = (src1) | (src2); } while(0)
#define XOR(dst, src1, src2) do { dst = (src1) ^ (src2); } while(0)
#define NEG(dst, src)        do { dst = (uint64_t)(-(int64_t)(src)); } while(0)
#define NOT(dst, src)        do { dst = ~(src); } while(0)

// СРАВНЕНИЯ
#define EQ(dst, src1, src2) do { dst = ((src1) == (src2)) ? 1 : 0; } while(0)
#define NE(dst, src1, src2) do { dst = ((src1) != (src2)) ? 1 : 0; } while(0)
#define LT(dst, src1, src2) do { dst = ((int64_t)(src1) < (int64_t)(src2)) ? 1 : 0; } while(0)
#define LE(dst, src1, src2) do { dst = ((int64_t)(src1) <= (int64_t)(src2)) ? 1 : 0; } while(0)
#define GT(dst, src1, src2) do { dst = ((int64_t)(src1) > (int64_t)(src2)) ? 1 : 0; } while(0)
#define GE(dst, src1, src2) do { dst = ((int64_t)(src1) >= (int64_t)(src2)) ? 1 : 0; } while(0)

// ПЕРЕХОДЫ
#define JMP(label) goto label
#define JZ(reg, label)  do { if ((reg) == 0) goto label; } while(0)
#define JNZ(reg, label) do { if ((reg) != 0) goto label; } while(0)
#define LABEL(label) label:

// СТЕК
#define PUSH(reg) do { vstack[vsp++] = (reg); } while(0)
#define POP() do { vsp--; } while(0)

// ПАМЯТЬ
#define LOAD(dst_reg, addr_expr, offset) do { dst_reg = *(uint64_t*)((uintptr_t)(addr_expr) + (offset)); } while(0)
#define STORE(addr_expr, offset, src_reg) do { *(uint64_t*)((uintptr_t)(addr_expr) + (offset)) = (src_reg); } while(0)
#define LEA(dst_reg, addr_expr, offset) do { dst_reg = (uint64_t)(uintptr_t)((uintptr_t)(addr_expr) + (offset)); } while(0)

// СИСТЕМНЫЕ ВЫЗОВЫ
#define SYS_OPEN()  do { r[0] = (uint64_t)open((const char*)r[0], (int)r[1], (int)r[2]); } while(0)
#define SYS_READ()  do { r[0] = (uint64_t)read((int)r[0], (void*)r[1], (size_t)r[2]); } while(0)
#define SYS_WRITE() do { r[0] = (uint64_t)write((int)r[0], (const void*)r[1], (size_t)r[2]); } while(0)
#define SYS_CLOSE() do { r[0] = (uint64_t)close((int)r[0]); } while(0)
#define SYS_MALLOC() do { r[0] = (uint64_t)malloc((size_t)r[0]); } while(0)
#define SYS_FREE()  do { free((void*)r[0]); } while(0)

// ТОЧКА ВХОДА
extern void __main__(void);

int main(int argc, char** argv) {
    r[0] = (uint64_t)argc;
    r[1] = (uint64_t)(uintptr_t)argv;
    __main__();
    return (int)rax;
}