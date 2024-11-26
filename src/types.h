//
// Created by Lucca Haddad on 22/11/24.
//

#ifndef TYPES_H
#define TYPES_H
#include <stdbool.h>

typedef enum { TYPE_VOID, TYPE_INT, TYPE_BOOLEAN, TYPE_ARRAY } Type;

typedef struct TypeInfo {
	Type             baseType;
	int              arraySize;  // -1 for non-arrays
	struct TypeInfo* returnType; // For functions, NULL otherwise
	struct {
		struct TypeInfo** types;
		int               count;
	} parameters;
} TypeInfo;

TypeInfo* createType(Type baseType);
TypeInfo* createArrayType(Type baseType, int size);
TypeInfo* createFunctionType(TypeInfo* returnType);
bool areTypesCompatible(TypeInfo* t1, TypeInfo* t2);
void      addParameter(TypeInfo* functionType, TypeInfo* paramType);
void      destroyType(TypeInfo* type);

#endif // TYPES_H
