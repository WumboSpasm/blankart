// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  m_aatree.h
/// \brief AA trees code

#include <stdlib.h>
#include <string.h>

#include "m_aatree.h"
#include "m_queue.h"
#include "z_zone.h"
#include "doomdef.h"

// A partial implementation of AA trees,
// according to the algorithms given on Wikipedia.
// http://en.wikipedia.org/wiki/AA_tree

typedef struct aatree_node_s
{
	INT32	level;
	INT32	key;
	void*	value;

	struct aatree_node_s *left, *right;
} aatree_node_t;

struct aatree_s
{
	aatree_node_t	*root;
	UINT32		flags;
    size_t size;
};

typedef struct aatree_iterator_node_s {
    mqueueitem_t item;
    aatree_node_t *node;
} aatree_iterator_node_t;

struct aatree_iterator_s
{
    mqueue_t nodes;
};

aatree_t *M_AATreeAlloc(UINT32 flags)
{
	aatree_t *aatree = Z_Malloc(sizeof (aatree_t), PU_STATIC, NULL);
	aatree->root = NULL;
	aatree->flags = flags;
    aatree->size = 0;
	return aatree;
}

static void M_AATreeFree_Node(aatree_node_t *node)
{
	if (node->left) M_AATreeFree_Node(node->left);
	if (node->right) M_AATreeFree_Node(node->right);
	Z_Free(node);
}

void M_AATreeFree(aatree_t *aatree)
{
	if (aatree->root)
		M_AATreeFree_Node(aatree->root);

	Z_Free(aatree);
}

static aatree_node_t *M_AATreeSkew(aatree_node_t *node)
{
	if (node && node->left && node->left->level == node->level)
	{
		// Not allowed: horizontal left-link. Reverse the
		// horizontal link and hook the orphan back in.
		aatree_node_t *oldleft = node->left;
		node->left = oldleft->right;
		oldleft->right = node;

		return oldleft;
	}

	// No change needed.
	return node;
}

static aatree_node_t *M_AATreeSplit(aatree_node_t *node)
{
	if (node && node->right && node->right->right && node->level == node->right->right->level)
	{
		// Not allowed: two consecutive horizontal right-links.
		// The middle one becomes the new root at this point,
		// with suitable adjustments below.

		aatree_node_t *oldright = node->right;
		node->right = oldright->left;
		oldright->left = node;
		oldright->level++;

		return oldright;
	}

	// No change needed.
	return node;
}

static aatree_node_t *M_AATreeSet_Node(aatree_t *aatree, aatree_node_t *node, UINT32 flags, INT32 key, void* value)
{
	if (!node)
	{
		// Nothing here, so just add where we are
        ++aatree->size;
		node = Z_Malloc(sizeof (aatree_node_t), PU_STATIC, NULL);
		node->level = 1;
		node->key = key;
		if (value && (flags & AATREE_ZUSER)) Z_SetUser(value, &node->value);
		else node->value = value;
		node->left = node->right = NULL;
	}
	else
	{
		if (key < node->key)
			node->left = M_AATreeSet_Node(aatree, node->left, flags, key, value);
		else if (key > node->key)
			node->right = M_AATreeSet_Node(aatree, node->right, flags, key, value);
		else
		{
			if (value && (flags & AATREE_ZUSER)) Z_SetUser(value, &node->value);
			else node->value = value;
		}

		node = M_AATreeSkew(node);
		node = M_AATreeSplit(node);
	}

	return node;
}

void M_AATreeSet(aatree_t *aatree, INT32 key, void* value)
{
	aatree->root = M_AATreeSet_Node(aatree, aatree->root, aatree->flags, key, value);
}

// Caveat: we don't distinguish between nodes that don't exists
// and nodes with value == NULL.
static void *M_AATreeGet_Node(aatree_node_t *node, INT32 key)
{
	if (node)
	{
		if (node->key == key)
			return node->value;
		else if(node->key < key)
			return M_AATreeGet_Node(node->right, key);
		else
			return M_AATreeGet_Node(node->left, key);
	}

	return NULL;
}

void *M_AATreeGet(aatree_t *aatree, INT32 key)
{
	return M_AATreeGet_Node(aatree->root, key);
}

boolean M_AATreeEmpty(aatree_t *aatree)
{
    return aatree->root == NULL;
}

size_t M_AATreeSize(aatree_t *aatree)
{
    return aatree->size;
}

static void M_AATreeIterate_Node(aatree_node_t *node, aatree_iter_t callback)
{
	if (node->left) M_AATreeIterate_Node(node->left, callback);
	callback(node->key, node->value);
	if (node->right) M_AATreeIterate_Node(node->right, callback);
}

void M_AATreeIterate(aatree_t *aatree, aatree_iter_t callback)
{
	if (aatree->root)
		M_AATreeIterate_Node(aatree->root, callback);
}

static void M_AATreeBegin_Node(aatree_node_t *node, mqueue_t *node_queue)
{
    if (node->left) M_AATreeBegin_Node(node->left, node_queue);

    aatree_iterator_node_t *itnode = calloc(1, sizeof(aatree_iterator_node_t));
    itnode->node = node;
    M_QueueInsert((mqueueitem_t*)itnode, node_queue);

    if (node->right) M_AATreeBegin_Node(node->right, node_queue);
}

aatree_iterator_t *M_AATreeBegin(aatree_t *aatree)
{
    aatree_iterator_t *iterator = malloc(sizeof(aatree_iterator_t));

    M_QueueInit(&iterator->nodes);

    if (aatree->root)
        M_AATreeBegin_Node(aatree->root, &iterator->nodes);

    // Advance it so iterator->nodes->rover points to current element
    M_QueueIterator(&iterator->nodes);

    return iterator;
}

static void M_AATreeRBegin_Node(aatree_node_t *node, mqueue_t *node_queue)
{
    if (node->left) M_AATreeRBegin_Node(node->left, node_queue);

    aatree_iterator_node_t *itnode = calloc(1, sizeof(aatree_iterator_node_t));
    itnode->node = node;
    M_QueueInsert((mqueueitem_t*)itnode, node_queue);

    if (node->right) M_AATreeRBegin_Node(node->right, node_queue);
}

aatree_iterator_t *M_AATreeRBegin(aatree_t *aatree)
{
    aatree_iterator_t *iterator = malloc(sizeof(aatree_iterator_t));

    M_QueueInit(&iterator->nodes);

    if (aatree->root)
        M_AATreeRBegin_Node(aatree->root, &iterator->nodes);

    // Advance it so iterator->nodes->rover points to current element
    M_QueueIterator(&iterator->nodes);

    return iterator;
}

void *M_AATreeIteratorNext(aatree_iterator_t *iterator)
{
    return ((aatree_iterator_node_t*)M_QueueIterator(&iterator->nodes))->node->value;
}

INT32 M_AATreeIteratorKey(aatree_iterator_t *iterator)
{
    // Uuh
    if (iterator->nodes.rover == NULL)
        I_Error("Attempt to get key from invalid iterator");

    return ((aatree_iterator_node_t*)iterator->nodes.rover)->node->key;
}

void *M_AATreeIteratorValue(aatree_iterator_t *iterator)
{
    if (iterator->nodes.rover == NULL)
        return NULL;

    return ((aatree_iterator_node_t*)iterator->nodes.rover)->node->value;
}

void M_AATreeIteratorClose(aatree_iterator_t *iterator)
{
    M_QueueFree(&iterator->nodes);
}
