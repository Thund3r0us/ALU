#include "spimcore.h"

/* ALU */
/* 10 Points */
void ALU(unsigned A, unsigned B, char ALUControl, unsigned *ALUresult, char *Zero)
{
  // Implement ALU operations based on ALUControl
  switch (ALUControl) {
    case '0':
      *ALUresult = A + B;
      break;
    case '1':
      *ALUresult = A - B;
      break;
    case '2':
      *ALUresult = (A < B) ? 1u : 0u;
      break;
    case '3':
      *ALUresult = (A < B) ? 1u : 0u;
      break;
    case '4':
      *ALUresult = A & B;
      break;
    case '5':
      *ALUresult = A | B;
      break;
    case '6':
      *ALUresult = B << 16;
      break;
    case '7':
      *ALUresult = ~A;
      break;
    default:
      // Handle invalid ALUControl value
      // For simplicity, we can set the result to zero and zero flag to 1
      *ALUresult = 0;
      *Zero = '1';
      return;
  }

  // Set Zero flag
  *Zero = (*ALUresult == 0) ? '1' : '0';
}


/* instruction fetch */
/* 10 Points */
int instruction_fetch(unsigned PC, unsigned *Mem, unsigned *instruction)
{
  // Check if the PC is aligned
  if ((PC % 4) != 0) {
    // Halt if condition is not met
    return 1;
  }

  // Fetch the instruction from memory
  *instruction = Mem[PC >> 2];

  // Check if the fetched instruction is a halt condition (0x00000000)
  if (*instruction == 0x00000000) {
    // Set halt flag and return
    return 1;
  }

  // No halt condition occurred
  return 0;
}


/* instruction partition */
/* 10 Points */
void instruction_partition(unsigned instruction, unsigned *op, unsigned *r1, unsigned *r2, unsigned *r3, unsigned *funct, unsigned *offset, unsigned *jsec)
{
  // Extract different parts of the instruction
  *op = (instruction >> 26) & 0x3F;
  *r1 = (instruction >> 21) & 0x1F;
  *r2 = (instruction >> 16) & 0x1F;
  *r3 = (instruction >> 11) & 0x1F;
  *funct = instruction & 0x3F;
  *offset = instruction & 0xFFFF;
  *jsec = instruction & 0x3FFFFFF;
}


/* instruction decode */
/* 15 Points */
int instruction_decode(unsigned op, struct_controls *controls)
{
  // Initialize control signals structure
  controls->RegDst = 2;    // Don't care
  controls->Jump = 0;
  controls->Branch = 0;
  controls->MemRead = 0;
  controls->MemtoReg = 0;
  controls->ALUOp = 0;
  controls->MemWrite = 0;
  controls->ALUSrc = 0;
  controls->RegWrite = 0;

  // Set control signals based on opcode
  switch (op) {
    case 0x00: // R-type instruction
      controls->RegDst = 1;
      controls->RegWrite = 1;
      controls->ALUSrc = 0;
      controls->MemtoReg = 0;
      controls->MemRead = 0;
      controls->MemWrite = 0;
      controls->ALUOp = 0x00;
      break;
    case 0x02: // Jump instruction
      controls->Jump = 1;
      break;
    case 0x23: // lw instruction
      controls->RegDst = 0;
      controls->RegWrite = 1;
      controls->ALUSrc = 1;
      controls->MemtoReg = 1;
      controls->MemRead = 1;
      controls->MemWrite = 0;
      controls->ALUOp = 0x00;
      break;
    case 0x2B: // sw instruction
      controls->RegDst = 2;
      controls->RegWrite = 0;
      controls->ALUSrc = 1;
      controls->MemtoReg = 2;
      controls->MemRead = 0;
      controls->MemWrite = 1;
      controls->ALUOp = 0x00;
      break;
    // Add more cases for other opcodes as needed
    default:
      // Invalid opcode encountered
      return 1; // Halt condition
  }

  // No halt condition occurred
  return 0;
}

/* Read Register */
/* 5 Points */
void read_register(unsigned r1, unsigned r2, unsigned *Reg, unsigned *data1, unsigned *data2)
{
  // Read the values from the registers addressed by r1 and r2
  *data1 = Reg[r1];
  *data2 = Reg[r2];
}


/* Sign Extend */
/* 10 Points */
void sign_extend(unsigned offset,unsigned *extended_value)
{
  // Check if the offset's sign bit is set
  if (offset & 0x8000) {
    // If sign bit is set, extend with 1s
    *extended_value = offset | 0xFFFF0000;
  } 
  else {
    // If sign bit is not set, extend with 0s
    *extended_value = offset & 0x0000FFFF;
  }
}

/* ALU operations */
/* 10 Points 
ALUSrc is the source of the second input of the ALU 
*/
int ALU_operations(unsigned data1,unsigned data2,unsigned extended_value,unsigned funct,char ALUOp,char ALUSrc,unsigned *ALUresult,char *Zero)
{
  // Perform ALU operation based on ALUOp
  switch (ALUOp) {
      case 0: // Add
        *ALUresult = data1 + (ALUSrc ? extended_value : data2);
        break;
      case 1: // Subtract
        *ALUresult = data1 - (ALUSrc ? extended_value : data2);
        break;
      case 2: // Set if less than (signed)
        *ALUresult = ((int)data1 < (int)(ALUSrc ? extended_value : data2)) ? 1 : 0;
        break;
      case 3: // Set if less than (unsigned)
        *ALUresult = (data1 < (ALUSrc ? extended_value : data2)) ? 1 : 0;
        break;
      case 4: // Bitwise AND
        *ALUresult = data1 & (ALUSrc ? extended_value : data2);
        break;
      case 5: // Bitwise OR
        *ALUresult = data1 | (ALUSrc ? extended_value : data2);
        break;
      case 6: // Shift left by 16 bits
        *ALUresult = (ALUSrc ? extended_value : data2) << 16;
        break;
      case 7: // Bitwise NOT
        *ALUresult = ~data1;
        break;
      default:
        // Invalid ALUOp encountered
        return 1; // Halt condition
  }

  // Update Zero flag
  *Zero = (*ALUresult == 0) ? 1 : 0;

  // No halt condition occurred
  return 0;
}

/* Read / Write Memory */
/* 10 Points */
int rw_memory(unsigned ALUresult,unsigned data2,char MemWrite,char MemRead,unsigned *memdata,unsigned *Mem)
{
  // Check if ALUresult represents an address and is not word-aligned
  if ((ALUresult % 4) != 0 && (MemRead || MemWrite)) {
    // Halt if the condition is met
    return 1;
  }
  
  // Check if MemRead signal is asserted
  if (MemRead) {
    // Read data from memory location addressed by ALUresult
    *memdata = Mem[ALUresult >> 2];
  }

  // Check if MemWrite signal is asserted
  if (MemWrite) {
    // Write data2 to memory location addressed by ALUresult
    Mem[ALUresult >> 2] = data2;
  }

  // No halt condition occurred
  return 0;
}


/* Write Register */
/* 10 Points 
RedDst defines which register is the destination register of an instruction
*/

void write_register(unsigned r2,unsigned r3,unsigned memdata,unsigned ALUresult,char RegWrite,char RegDst,char MemtoReg,unsigned *Reg)
{
  // Determine the destination register based on RegDst
  unsigned reg_dest = (RegDst == 0) ? r2 : r3;

  // Determine the value to be written to the destination register based on MemtoReg
  unsigned write_value = (MemtoReg == 1) ? memdata : ALUresult;

  // Write the value to the destination register if RegWrite is asserted
  if (RegWrite) {
    // Ensure destination register is not $zero
    if (reg_dest != 0) {
      Reg[reg_dest] = write_value;
    }
  }
}

/* PC update */
/* 10 Points */
void PC_update(unsigned jsec,unsigned extended_value,char Branch,char Jump,char Zero,unsigned *PC)
{
  // Increment PC by 4 to prepare for next instruction
  *PC += 4;

  // Check if Branch signal is asserted and Zero flag is set
  if (Branch && Zero) {
    // Calculate new PC value for branch instruction
    *PC = *PC + (extended_value << 2);
  }

  // Check if Jump signal is asserted
  if (Jump) {
    // Calculate new PC value for jump instruction
    *PC = (jsec << 2) | (*PC & 0xF0000000);
  }
}

