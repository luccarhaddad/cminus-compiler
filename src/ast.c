//
// Created by Lucca Haddad on 22/11/24.
//

#include "ast.h"

#include <globals.h>
#include <stdio.h>
#include <stdlib.h>

ASTNode* createNode(const int kind) {
	ASTNode* node = (ASTNode*) malloc(sizeof(ASTNode));

	if (node == NULL) {
		fprintf(stderr, "Failed to allocate memory for ASTNode\n");
		return NULL;
	}

	node->kind        = kind;
	node->children[0] = NULL;
	node->children[1] = NULL;
	node->children[2] = NULL;
	node->next        = NULL;
	node->resultType  = NULL;
	node->symbol      = NULL;
	node->lineNo      = lineno;

	switch (kind) {
		case NODE_VARIABLE:
		case NODE_FUNCTION:
		case NODE_CALL:
		case NODE_IDENTIFIER:
			node->data.symbol.name = NULL;
			node->data.symbol.type = NULL;
			break;
		case NODE_OPERATOR:
			node->data.operator= 0;
			break;
		case NODE_CONSTANT:
			node->data.constValue = 0;
			break;
		case NODE_BLOCK:
			node->data.symbol.name = "block";
			node->data.symbol.type = NULL;
			break;
		default:
			break;
	}

	return node;
}

void destroyNode(ASTNode* node) {
	if (!node) return;

	for (int i = 0; i < 3; i++) {
		if (node->children[i]) {
			destroyNode(node->children[i]);
		}
	}

	if (node->next) destroyNode(node->next);

	if ((node->kind == NODE_VARIABLE || node->kind == NODE_FUNCTION || node->kind == NODE_CALL ||
	     node->kind == NODE_IDENTIFIER) &&
	    node->data.symbol.name) {
		free((char*) node->data.symbol.name);
	}

	if (node->data.symbol.type) destroyType(node->data.symbol.type);

	free(node);
}

void addChild(ASTNode* parent, ASTNode* child) {
	if (!parent || !child) return;

	for (int i = 0; i < 3; i++) {
		if (parent->children[i] == NULL) {
			parent->children[i] = child;
			return;
		}
	}
	fprintf(stderr, "Failed to add child to ASTNode\n");
}

void addSibling(ASTNode* node, ASTNode* sibling) {
	if (!node || !sibling) return;

	ASTNode* current = node;
	while (current->next != NULL) current = current->next;

	current->next = sibling;
}