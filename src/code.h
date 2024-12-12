#ifndef _CODE_H_
#define _CODE_H_

#define AC 0
#define AC1 1
#define FP 2
#define R3 3
#define R4 4
#define GP 5
#define MP 6
#define PC 7

#define ofpFO 0
#define retFO -1
#define initFO -2

/* code emitting utilities */

/* Procedure emitComment prints a comment line
 * with comment c in the code file
 */
void emitComment(char* c);

/* Procedure emitRO emits a register-only
 * TM instruction
 * op = the opcode
 * r = target register
 * s = 1st source register
 * t = 2nd source register
 * c = a comment to be printed if TraceCode is TRUE
 */
void emitRO(char* opCode, int target, int firstSource, int secondSource, char* c);

/* Procedure emitRM emits a register-to-memory
 * TM instruction
 * op = the opcode
 * r = target register
 * d = the offset
 * s = the base register
 * c = a comment to be printed if TraceCode is TRUE
 */
void emitRM(char* opCode, int target, int offset, int base, char* c);

/* Function emitSkip skips "howMany" code
 * locations for later backpatch. It also
 * returns the current code position
 */
int emitSkip(int howMany);

/* Procedure emitBackup backs up to
 * loc = a previously skipped location
 */
void emitBackup(int loc);

/* Procedure emitRestore restores the current
 * code position to the highest previously
 * unemitted position
 */
void emitRestore(void);

/* Procedure emitRM_Abs converts an absolute reference
 * to a pc-relative reference when emitting a
 * register-to-memory TM instruction
 * op = the opcode
 * r = target register
 * a = the absolute location in memory
 * c = a comment to be printed if TraceCode is TRUE
 */
void emitRM_Abs(char* opCode, int target, int absoluteLoc, char* c);

#endif
