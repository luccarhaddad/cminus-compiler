//
// Created by Lucca Haddad on 22/11/24.
//

#include "types.h"
#include <stdlib.h>
#include <stdio.h>

TypeInfo* createType(Type baseType) {
	TypeInfo* type = (TypeInfo*)malloc(sizeof(TypeInfo));

	if(!type) {
		fprintf(stderr, "Error allocating memory for type info\n");
		return NULL;
	}

	type->baseType = baseType;
	type->arraySize = -1;
	type->returnType = NULL;
	type->parameters.types = NULL;
	type->parameters.count = 0;

	return type;
}

TypeInfo* createArrayType(Type baseType, int arraySize) {
	TypeInfo* type = createType(baseType);
	if(type) {
		type->arraySize = arraySize;
	}
	return type;
}

TypeInfo* createFunctionType(TypeInfo* returnType) {
	TypeInfo* type = createType(TYPE_VOID); // Default to void
	if(type) {
		type->returnType = returnType;
	}
	return type;
}

void addParameter(TypeInfo* functionType, TypeInfo* parameterType) {
	if(!functionType || !parameterType) {
		fprintf(stderr, "Error adding parameters\n");
		return;
	}

	TypeInfo** newTypes = realloc(functionType->parameters.types, sizeof(TypeInfo*) * (functionType->parameters.count+1));
	if(!newTypes) {
		fprintf(stderr, "Error allocating memory for parameters\n");
		return;
	}

	functionType->parameters.types = newTypes;
	functionType->parameters.types[functionType->parameters.count] = parameterType;
	functionType->parameters.count++;
}

int areTypesCompatible(TypeInfo* t1, TypeInfo* t2) {
	if (!t1 || !t2)
		return 0;

	if (t1->baseType != t2->baseType)
		return 0;

	if (t1->baseType == TYPE_ARRAY && t1->arraySize != t2->arraySize)
		return 0;

	if (t1->returnType && t2->returnType) {
		if (!areTypesCompatible(t1->returnType, t2->returnType))
			return 0;
	}

	if (t1->parameters.count != t2->parameters.count)
		return 0;

	for (int i = 0; i < t1->parameters.count; i++) {
		if (!areTypesCompatible(t1->parameters.types[i], t2->parameters.types[i]))
			return 0;
	}

	return 1;
}

void destroyType(TypeInfo* type) {
	if (!type)
		return;

	if (type->returnType)
		destroyType(type->returnType);

	for (int i = 0; i < type->parameters.count; i++) {
		destroyType(type->parameters.types[i]);
	}

	if (type->parameters.types) {
		free(type->parameters.types);
	}

	free(type);
}