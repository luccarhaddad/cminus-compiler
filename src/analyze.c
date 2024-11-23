// #include "analyze.h"
// #include "globals.h"
// #include "symtab.h"
//
// /* Tracks the next available memory location for variables */
// static int location = 0;
//
// /* Keeps track of the current function's return type during analysis */
// static ExpType currentFunctionType;
//
// /*
//  * Generic tree traversal algorithm that implements the Visitor pattern:
//  * - Performs a depth-first traversal of the syntax tree
//  * - Applies preProc function before processing children (preorder)
//  * - Applies postProc function after processing children (postorder)
//  * - Handles both child nodes and sibling nodes
//  *
//  * @param t The current tree node
//  * @param preProc Function to execute before processing children
//  * @param postProc Function to execute after processing children
//  */
// static void traverse(TreeNode* t, void (*preProc)(TreeNode*), void (*postProc)(TreeNode*)) {
// 	if (t != NULL) {
// 		preProc(t);
// 		{
// 			for (int i = 0; i < MAXCHILDREN; i++) traverse(t->child[i], preProc, postProc);
// 		}
// 		postProc(t);
// 		traverse(t->sibling, preProc, postProc);
// 	}
// }
//
// /*
//  * Empty procedure used as a placeholder when only preorder or postorder
//  * traversal is needed
//  */
// static void nullProc(TreeNode* t) {
// }
//
// /*
//  * Handles symbol table insertion based on node type:
//  * - For variables: Adds them with their scope and type information
//  * - For functions: Adds function signature and creates new scope
//  * - For parameters: Adds them to current function's scope
//  *
//  * Key features:
//  * - Prevents duplicate declarations in same scope
//  * - Tracks array types vs scalar variables
//  * - Manages function parameters
//  * - Maintains scope hierarchy
//  */
// static void insertNode(TreeNode* t) {
// 	if (t == NULL || t->attr.name == NULL) return;
//
// 	switch (t->nodekind) {
// 		case StmtK:
// 			switch (t->kind.stmt) {
// 				case IfK:
// 				case WhileK:
// 				case ReturnK:
// 					break;
// 				case AssignK:
// 					// For assignments, insert variable if not already present
// 					st_insert(t->attr.name, t->lineno,
// 					          st_lookup(t->attr.name) == -1 ? location++ : 0, FALSE,
// 					          t->child[0] == NULL ? FALSE : TRUE, Integer);
// 					break;
// 				case FuncK:
// 					// Function declaration handling
// 					st_insert(t->attr.name, t->lineno, location, TRUE,
// 					          t->child[0] == NULL ? FALSE : TRUE, t->type);
//
// 					// Count and store number of parameters
// 					BucketList l = st_bucket_lookup(t->attr.name);
// 					if (l != NULL) {
// 						TreeNode* param      = t->child[0];
// 						int       paramCount = 0;
// 						while (param != NULL) {
// 							paramCount++;
// 							param = param->sibling;
// 						}
// 						l->numParams = paramCount;
// 					}
//
// 					enterScope(t->attr.name);
// 					currentFunctionType = t->type;
// 					break;
// 				case ParamK:
// 				case VarK:
// 					// Check for duplicate declarations
// 					if (st_lookup(t->attr.name) != -1) {
// 						fprintf(stderr, "Variable already declared in this scope");
// 						break;
// 					}
// 					st_insert(t->attr.name, t->lineno, location++, FALSE,
// 					          t->child[0] == NULL ? FALSE : TRUE, Integer);
// 					break;
// 			}
// 			break;
// 		case ExpK:
// 			switch (t->kind.exp) {
// 				case ConstK:
// 					t->type = Integer;
// 					break;
// 				case IdK:
// 					st_insert(t->attr.name, t->lineno,
// 					          st_lookup(t->attr.name) == -1 ? location++ : 0, FALSE,
// 					          t->child[0] == NULL ? FALSE : TRUE, Integer);
// 					break;
// 				case OpK:
// 				case CallK:
// 					break;
// 			}
// 			break;
// 		default:
// 			break;
// 	}
// }
//
// /*
//  * Handles scope cleanup when leaving a function definition
//  */
// static void leaveScopeProc(TreeNode* t) {
// 	if (t == NULL) return;
// 	if (t->nodekind == StmtK && t->kind.stmt == FuncK) {
// 		leaveScope();
// 	}
// }
//
// /*
//  * Main symbol table construction function
//  * - Creates global scope
//  * - Builds complete symbol table through tree traversal
//  * - Optionally prints the symbol table for debugging
//  */
// void buildSymtab(TreeNode* syntaxTree) {
// 	enterScope("global");
// 	traverse(syntaxTree, insertNode, leaveScopeProc);
// 	leaveScope();
// 	if (TraceAnalyze) {
// 		pc("\nSymbol table:\n\n");
// 		printSymTab();
// 	}
// }
//
// /*
//  * Reports type errors with line numbers for better debugging
//  */
// static void typeError(TreeNode* t, char* message) {
// 	pce("Semantic error at line %d: %s\n", t->lineno, message);
// 	Error = TRUE;
// }
//
// /*
//  * Verifies that binary operations have compatible operand types
//  */
// static void checkBinaryOperands(TreeNode* t, ExpType expectedType) {
// 	if (t->child[0] == NULL || t->child[1] == NULL) {
// 		typeError(t, "Binary operator requires two operands");
// 		return;
// 	}
// 	if (t->child[0]->type != expectedType) {
// 		typeError(t->child[0], "Operator applied to incompatible type");
// 	}
// 	if (t->child[1]->type != expectedType) {
// 		typeError(t->child[1], "Operator applied to incompatible type");
// 	}
// }
//
// /*
//  * Main type checking function that validates:
//  * - Operator type compatibility
//  * - Variable declarations
//  * - Function calls and parameter counts
//  * - Control structure conditions
//  * - Assignment compatibility
//  */
// static void checkNode(TreeNode* t) {
// 	switch (t->nodekind) {
// 		case ExpK:
// 			switch (t->kind.exp) {
// 				case OpK:
// 					switch (t->attr.op) {
// 						case PLUS:
// 						case MINUS:
// 						case TIMES:
// 						case OVER:
// 							checkBinaryOperands(t, Integer);
// 							t->type = Integer;
// 							break;
// 						case EQ:
// 						case NEQ:
// 						case LEQ:
// 						case LT:
// 						case GEQ:
// 						case GT:
// 							checkBinaryOperands(t, Integer);
// 							t->type = Boolean;
// 							break;
// 						default:
// 							typeError(t, "Unknown operator");
// 							break;
// 					}
// 					break;
// 				case ConstK:
// 					t->type = Integer;
// 					break;
// 				case IdK:
// 					location = st_lookup(t->attr.name);
// 					if (location == -1) typeError(t, "Undeclared variable");
// 					t->type = Integer;
// 					break;
// 				case CallK:
// 					location = st_lookup(t->attr.name);
// 					if (location == -1) {
// 						typeError(t, "Undeclared function");
// 						break;
// 					}
//
// 					BucketList funcEntry = hashTable[location];
// 					if (funcEntry == NULL) {
// 						typeError(t, "Function not found");
// 						break;
// 					}
//
// 					// Find the correct function entry
// 					while (funcEntry != NULL && strcmp(funcEntry->name, t->attr.name) != 0)
// 						funcEntry = funcEntry->next;
//
// 					if (funcEntry == NULL) {
// 						typeError(t, "Identifier is not a function");
// 						break;
// 					}
//
// 					if (!funcEntry->isFunction) {
// 						typeError(t, "Identifier is not a function");
// 						break;
// 					}
//
// 					// Validate parameter count
// 					int       argCount = 0;
// 					TreeNode* arg      = t->child[0];
// 					while (arg != NULL) {
// 						argCount++;
// 						arg = arg->sibling;
// 					}
//
// 					if (argCount != funcEntry->numParams) {
// 						char errorMsg[100];
// 						sprintf(errorMsg, "Incorrect number of arguments: expected %d, received %d",
// 						        funcEntry->numParams, argCount);
// 						typeError(t, errorMsg);
// 						break;
// 					}
//
// 					t->type = funcEntry->returnType;
// 					break;
// 				default:
// 					break;
// 			}
// 			break;
// 		case StmtK:
// 			switch (t->kind.stmt) {
// 				case IfK:
// 					if (t->child[0]->type != Boolean)
// 						typeError(t->child[0], "Conditional operator is not boolean");
// 					break;
// 				case WhileK:
// 					if (t->child[0] == NULL) {
// 						typeError(t->child[0], "While operator must have a condition");
// 						break;
// 					}
// 					if (t->child[0]->type != Boolean) {
// 						typeError(t->child[0], "Conditional operator is not boolean");
// 						break;
// 					}
// 					break;
// 				case ReturnK:
// 					// Note: Return statement validation is incomplete
// 					break;
// 				case AssignK:
// 					location = st_lookup(t->child[0]->attr.name);
// 					if (location == -1) {
// 						typeError(t->child[0], "Undeclared variable");
// 						break;
// 					}
//
// 					BucketList varEntry = hashTable[location];
// 					while (varEntry != NULL && strcmp(varEntry->name, t->child[0]->attr.name) != 0)
// 						varEntry = varEntry->next;
//
// 					if (varEntry == NULL) {
// 						typeError(t->child[0], "Undeclared variable");
// 						break;
// 					}
//
// 					if (varEntry->isFunction) {
// 						typeError(t->child[0], "Cannot assign value to a function");
// 						break;
// 					}
//
// 					if (t->child[1]->type != varEntry->type) {
// 						typeError(t, "Incompatible types in assignment");
// 						break;
// 					}
// 					break;
// 				case ParamK:
// 				case VarK:
// 				case FuncK:
// 					break;
// 				default:
// 					break;
// 			}
// 			break;
// 		default:
// 			break;
// 	}
// }
//
// /*
//  * Main type checking entry point
//  * Traverses the entire syntax tree to validate types
//  */
// void typeCheck(TreeNode* syntaxTree) {
// 	traverse(syntaxTree, nullProc, checkNode);
// }