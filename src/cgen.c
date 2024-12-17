#include "cgen.h"
#include "analyze.h"
#include "code.h"
#include "globals.h"
#include "hash.h"

/* tmpOffset is the memory offset for temps
   It is decremented each time a temp is
   stored, and incremeted when loaded again
*/
static int  tmpOffset    = initFO;
static int  mainLocation = 3;
static bool paramsEvaluation;
static bool isFirstFunction = TRUE;

/* prototype for internal recursive code generator */
static void cGen(ASTNode* tree);

static bool blockAfterFunction = FALSE;

static int getSymbolOffset(ASTNode* t, bool onlyCurrentScope) {
	const Symbol* symbol = onlyCurrentScope ? findSymbolInScope(currentScope, t->data.symbol.name)
	                                        : findSymbol(currentScope, t->data.symbol.name);
	return symbol ? symbol->offset : -1;
}

static void processOperand(ASTNode* t) {
	switch (t->kind) {
		case NODE_CONSTANT:
			emitRM("LDC", AC, t->data.constValue, 0, "load const");
			break;

		case NODE_IDENTIFIER:
			if (t->data.symbol.type->arraySize >= 0) {
				ASTNode* indexNode = t->children[0];
				int      loc       = getSymbolOffset(t, FALSE);
				if (currentScope == globalScope) {
					emitRM("LDC", GP, 0, 0, "load GP");
					emitRM("LD", AC, loc, GP, "get vector's address (global)");
				} else {
					emitRM("LD", AC, loc - MAX_MEMORY, FP, "get vector's address (local)");
				}

				if (indexNode->kind == NODE_CONSTANT) {
					const int index = indexNode->data.constValue;
					emitRM("LDC", AC1, index, 0, "load constant index");
				} else {
					loc = getSymbolOffset(indexNode, FALSE);
					emitRM("LD", AC1, loc - MAX_MEMORY, FP, "load index");
				}
				emitRM("LDC", R3, 1, 0, "load constant 1");
				emitRO("ADD", AC1, AC1, R3, "adjust array index");
				emitRO("SUB", AC, AC, AC1, "compute address of array element");
				emitRM("LD", AC, 0, AC, "load value from array element");
			} else {
				int loc = getSymbolOffset(t, FALSE);
				if (currentScope == globalScope) {
					emitRM("LDC", GP, 0, 0, "load GP");
					emitRM("LD", AC, loc, GP, "get variable's value (global)");
				} else {
					emitRM("LD", AC, loc - MAX_MEMORY, FP, "get variable's value (local)");
				}
			}
			break;

		case NODE_OPERATOR:
			cGen(t);
			break;

		default:
			emitComment("Unsupported operand type");
			break;
	}
}

static void generate(ASTNode* tree) {
	ASTNode *p1, *p2, *p3;
	int      savedLoc1, savedLoc2, currentLoc;
	int      loc;
	switch (tree->kind) {
		// Done
		case NODE_BLOCK:
			if (!blockAfterFunction)
				enterScope(tree->data.symbol.name);
			else
				blockAfterFunction = FALSE;
			cGen(tree->children[0]);
			break;

		// Done
		case NODE_FUNCTION: {
			enterScope(tree->data.symbol.name);
			blockAfterFunction = TRUE;
			char* name         = strdup(tree->data.symbol.name);
			p1                 = tree->children[0];
			p2                 = tree->children[1];

			if (TraceCode) {
				char msg[30];
				sprintf(msg, "-> Init Function (%s)", name);
				emitComment(msg);
			}

			int initLocation = emitSkip(0);
			if (isFirstFunction) {
				mainLocation    = emitSkip(1);
				isFirstFunction = FALSE;
				hashInsert(name, mainLocation + 1);
			} else {
				hashInsert(name, initLocation);
			}

			tmpOffset = initFO;
			if (strcmp(tree->data.symbol.name, "main") == 0) {
				savedLoc1 = emitSkip(0);
				emitBackup(mainLocation);
				if (savedLoc1 == 3)
					emitRM_Abs("LDA", PC, savedLoc1 + 1, "jump to main");
				else
					emitRM_Abs("LDA", PC, savedLoc1, "jump to main");
				emitRestore();

				if (p1) cGen(p1);
				if (p2) cGen(p2);
			} else {
				emitRM("ST", AC, retFO, FP, "store return address");
				if (p1) cGen(p1);
				if (p2) cGen(p2);
				if (tree->data.symbol.type->returnType == TYPE_VOID) {
					emitRM("LDA", AC1, ofpFO, FP, "save current FP into AC1");
					emitRM("LD", FP, ofpFO, FP, "restore old FP");
					emitRM("LD", PC, retFO, AC1, "return to caller");
				}
			}

			if (TraceCode) emitComment("<- Function");
			break;
		}

		// Done
		case NODE_VARIABLE: {
			const bool isArray = tree->data.symbol.type->arraySize >= 0;
			if (TraceCode) {
				isArray ? emitComment("-> Declare Vector") : emitComment("-> Declare Var");
			}

			// Global variable
			if (currentScope == globalScope) {
				if (isArray) {
					loc = getSymbolOffset(tree, TRUE);
					emitRM("LDC", AC, loc, 0, "load global vector");
					emitRM("LDC", GP, 0, 0, "load GP");
					emitRM("ST", AC, loc, GP, "store global vector");
				} else {
					tmpOffset--;
				}
				// Local variable
			} else {
				if (isArray) {
					emitRM("LDA", AC, tmpOffset, FP, "load local vector");
					emitRM("ST", AC, tmpOffset, FP, "store local vector");
					tmpOffset -= tree->data.symbol.type->arraySize + 1;
				} else {
					tmpOffset--;
				}
			}

			if (TraceCode)
				isArray ? emitComment("<- Declare Vector") : emitComment("<- Declare Var");
			break;
		}

		// Done
		case NODE_PARAM:
			if (TraceCode) emitComment("-> Param");
			tmpOffset--;
			if (TraceCode) emitComment("<- Param");
			break;

		// Done
		case NODE_IDENTIFIER:
			if (TraceCode) emitComment("-> Id");

			if (tree->data.symbol.type->arraySize >= 0) {
				p1  = tree->children[0];
				loc = getSymbolOffset(tree, FALSE);
				if (currentScope == globalScope) {
					emitRM("LDC", GP, 0, 0, "load GP");
					emitRM("LD", AC, loc, GP, "get vector's address (global)");
				} else {
					emitRM("LD", AC, loc - MAX_MEMORY, FP, "get vector's address (local)");
				}

				if (p1 && p1->kind == NODE_CONSTANT) {
					const int index = p1->data.constValue;
					emitRM("LDC", AC1, index, 0, "load constant index");
				} else if (p1) {
					loc = getSymbolOffset(p1, FALSE);
					emitRM("LD", AC1, loc - MAX_MEMORY, FP, "load index");
				}
				emitRM("LDC", R3, 1, 0, "load constant 1");
				emitRO("ADD", AC1, AC1, R3, "adjust array index");
				emitRO("SUB", AC, AC, AC1, "compute address of array element");
				emitRM("LD", AC, 0, AC, "load value from array element");
			} else {
				loc = getSymbolOffset(tree, FALSE);
				if (currentScope == globalScope) {
					emitRM("LDC", GP, 0, 0, "load GP");
					emitRM("LD", AC, loc, GP, "get variable's value (global)");
				} else {
					emitRM("LD", AC, loc - MAX_MEMORY, FP, "get variable's value (local)");
				}
			}

			if (TraceCode) emitComment("<- Id");
			break;

		case NODE_CALL: {
			if (TraceCode) {
				char msg[30];
				sprintf(msg, "-> Function Call (%s)", tree->data.symbol.name);
				emitComment(msg);
			}

			p1 = tree->children[0];
			if (strcmp(tree->data.symbol.name, "output") == 0) {
				cGen(p1);
				emitRO("OUT", AC, 0, 0, "print value");
				break;
			}
			if (strcmp(tree->data.symbol.name, "input") == 0) {
				emitRO("IN", AC, 0, 0, "read value");
				break;
			}
			const int tmp = tmpOffset;
			emitRM("ST", FP, tmpOffset, FP, "store FP");
			tmpOffset -= 2;

			paramsEvaluation = TRUE;
			while (p1) {
				cGen(p1);
				emitRM("ST", AC, tmpOffset--, FP, "store parameter");
				p1 = p1->next;
			}
			paramsEvaluation = FALSE;
			tmpOffset        = tmp;

			emitRM("LDA", FP, tmpOffset, FP, "load FP with parameters");
			savedLoc1 = emitSkip(0);
			emitRM("LDC", AC, savedLoc1 + 2, 0, "load AC with return address");
			const int firstLoc = hashSearch(tree->data.symbol.name);
			emitRM_Abs("LDA", PC, firstLoc, "jump to function");

			if (TraceCode) {
				char msg[30];
				sprintf(msg, "<- Function Call (%s)", tree->data.symbol.name);
				emitComment(msg);
			}
			break;
		}

		// Done
		case NODE_IF:
			if (TraceCode) emitComment("-> If");

			p1 = tree->children[0];
			p2 = tree->children[1];
			p3 = tree->children[2];

			// Condition
			cGen(p1);
			emitComment("if: jump to else belongs here");
			savedLoc1 = emitSkip(1);

			// If body
			cGen(p2);
			emitComment("if: jump to end belongs here");
			savedLoc2 = emitSkip(1);

			currentLoc = emitSkip(0);
			emitBackup(savedLoc1);
			emitRM_Abs("JEQ", AC, savedLoc2 + 1, "if: jmp to else");
			emitRestore();

			// Else body
			if (p3) cGen(p3);
			currentLoc = emitSkip(0);
			emitBackup(savedLoc2);
			emitRM_Abs("LDA", PC, currentLoc, "jmp to end");
			emitRestore();

			if (TraceCode) emitComment("<- If");
			break;

		// Done
		case NODE_WHILE:
			if (TraceCode) emitComment("-> while");

			p1        = tree->children[0];
			p2        = tree->children[1];
			savedLoc1 = emitSkip(0);
			emitComment("repeat: jump after body comes back here");

			// Condition
			cGen(p1);
			savedLoc2 = emitSkip(1);
			emitComment("while: jump to end belongs here");
			// Body
			cGen(p2);
			emitRM_Abs("LDA", PC, savedLoc1, "while: jmp back to start of body");
			currentLoc = emitSkip(0);
			emitBackup(savedLoc2);
			emitRM_Abs("JEQ", AC, currentLoc, "while: jmp to end");
			emitRestore();

			if (TraceCode) emitComment("<- while");
			break;

		// Done
		case NODE_ASSIGN:
			if (TraceCode) emitComment("-> assign");

			p1 = tree->children[0];
			p2 = tree->children[1];
			if (p1->data.symbol.type->arraySize >= 0) {
				if (p2) cGen(p2);

				if (currentScope == globalScope) {
					loc = getSymbolOffset(p1, FALSE);
					emitRM("LDC", GP, 0, 0, "load GP");
					emitRM("LD", AC1, loc, GP, "assign: get vector base address");
				} else {
					loc = getSymbolOffset(p1, FALSE);
					emitRM("LD", AC1, loc - MAX_MEMORY, FP, "assign: get vector base address");
				}

				ASTNode* indexNode = p1->children[0];
				if (indexNode->kind == NODE_CONSTANT) {
					const int index = indexNode->data.constValue;
					emitRM("LDC", R3, index, 0, "assign: load constant index");
				} else {
					const int tmp = getSymbolOffset(indexNode, FALSE);
					emitRM("LD", R3, tmp - MAX_MEMORY, FP, "assign: load index");
				}
				emitRM("LDC", R4, 1, 0, "assign: load constant 1");
				emitRO("ADD", R3, R3, R4, "assign: adjust array index");
				emitRO("SUB", AC1, AC1, R3, "assign: compute address of array element");
				emitRM("ST", AC, 0, AC1, "assign: store value in array element");
			} else {
				if (p2) cGen(p2);
				loc = getSymbolOffset(p1, FALSE);
				if (currentScope == globalScope) {
					emitRM("ST", AC, loc, FP, "assign: store value");
				} else {
					emitRM("ST", AC, loc - MAX_MEMORY, FP, "assign: store value");
				}
			}
			if (TraceCode) emitComment("<- assign");
			break;

		// Done
		case NODE_CONSTANT:
			if (TraceCode) emitComment("-> Const");
			emitRM("LDC", AC, tree->data.constValue, 0, "load const");
			if (TraceCode) emitComment("<- Const");
			break;

		// Done
		case NODE_OPERATOR:
			if (TraceCode) emitComment("-> Op");
			p1 = tree->children[0];
			p2 = tree->children[1];

			if (p1) {
				processOperand(p1);
				emitRM("ST", AC, tmpOffset--, FP, "op: push left");
			}

			if (p2) {
				processOperand(p2);
				emitRM("LD", AC1, ++tmpOffset, FP, "op: load left");
			}

			switch (tree->data.operator) {
				case OP_PLUS:
					emitRO("ADD", AC, AC1, AC, "op +");
					break;
				case OP_MINUS:
					emitRO("SUB", AC, AC1, AC, "op -");
					break;
				case OP_TIMES:
					emitRO("MUL", AC, AC1, AC, "op *");
					break;
				case OP_OVER:
					emitRO("DIV", AC, AC1, AC, "op /");
					break;
				case OP_LT:
					emitRO("SUB", AC, AC1, AC, "op <");
					emitRM("JLT", AC, 2, PC, "br if true");
					emitRM("LDC", AC, 0, AC, "false case");
					emitRM("LDA", PC, 1, PC, "unconditional jmp");
					emitRM("LDC", AC, 1, AC, "true case");
					break;
				case OP_GT:
					emitRO("SUB", AC, AC1, AC, "op >");
					emitRM("JGT", AC, 2, PC, "br if true");
					emitRM("LDC", AC, 0, AC, "false case");
					emitRM("LDA", PC, 1, PC, "unconditional jmp");
					emitRM("LDC", AC, 1, AC, "true case");
					break;
				case OP_LEQ:
					emitRO("SUB", AC, AC1, AC, "op <=");
					emitRM("JLE", AC, 2, PC, "br if true");
					emitRM("LDC", AC, 0, AC, "false case");
					emitRM("LDA", PC, 1, PC, "unconditional jmp");
					emitRM("LDC", AC, 1, AC, "true case");
					break;
				case OP_GEQ:
					emitRO("SUB", AC, AC1, AC, "op >=");
					emitRM("JGE", AC, 2, PC, "br if true");
					emitRM("LDC", AC, 0, AC, "false case");
					emitRM("LDA", PC, 1, PC, "unconditional jmp");
					emitRM("LDC", AC, 1, AC, "true case");
					break;
				case OP_NEQ:
					emitRO("SUB", AC, AC1, AC, "op !=");
					emitRM("JNE", AC, 2, PC, "br if true");
					emitRM("LDC", AC, 0, AC, "false case");
					emitRM("LDA", PC, 1, PC, "unconditional jmp");
					emitRM("LDC", AC, 1, AC, "true case");
					break;
				case OP_EQ:
					emitRO("SUB", AC, AC1, AC, "op ==");
					emitRM("JEQ", AC, 2, PC, "br if true");
					emitRM("LDC", AC, 0, AC, "false case");
					emitRM("LDA", PC, 1, PC, "unconditional jmp");
					emitRM("LDC", AC, 1, AC, "true case");
					break;
				default:
					emitComment("BUG: Unknown operator");
					break;
			}

			if (TraceCode) emitComment("<- Op");
			break;

		// Done
		case NODE_RETURN:
			if (TraceCode) emitComment("-> return");

			if (tree->children[0]) cGen(tree->children[0]);

			emitRM("LDA", AC1, ofpFO, FP, "save current FP into AC1");
			emitRM("LD", FP, ofpFO, FP, "restore old FP");
			emitRM("LD", PC, retFO, AC1, "return to caller");

			if (TraceCode) emitComment("<- return");
			break;

		default:
			break;
	}
}

/* Procedure cGen recursively generates code by
 * tree traversal
 */
static void cGen(ASTNode* tree) {
	while (tree) {
		generate(tree);
		if (!paramsEvaluation)
			tree = tree->next;
		else
			break;
	}
}

/**********************************************/
/* the primary function of the code generator */
/**********************************************/
/* Procedure codeGen generates code to a code
 * file by traversal of the syntax tree. The
 * second parameter (codefile) is the file name
 * of the code file, and is used to print the
 * file name as a comment in the code file
 */
void codeGen(ASTNode* syntaxTree) {
	emitComment("TINY Compilation to TM Code");

	/* generate standard prelude */
	emitComment("Standard prelude:");
	emitRM("LD", MP, 0, AC, "load maxaddress from location 0");
	emitRM("LD", FP, 0, AC, "load maxaddress from location 0");
	emitRM("ST", AC, 0, AC, "clear location 0");
	emitComment("End of standard prelude.");

	/* generate code for TINY program */
	currentScope = globalScope;
	cGen(syntaxTree);

	/* finish */
	emitComment("End of execution.");
	emitRO("HALT", 0, 0, 0, "");
}
