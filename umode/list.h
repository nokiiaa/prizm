#pragma once

#include <pzapi.h>

typedef struct LLNode
{
    LLNode *Previous, *Next;
    u8 DataStart[0];
} LLNode;

#define ENUM_LIST(n, list) for (auto *n = (list).First; n; n = n->Next)
#define NODE_VALUE(node, type) (*(type*)((node)->DataStart))

typedef struct LinkedList
{
    LLNode *First, *Last;
    int Length;
} LinkedList;

inline void LLInitialize(LinkedList *list)
{
    list->First = nullptr;
    list->Last = nullptr;
    list->Length = 0;
}

inline void LLRemove(LinkedList *list, LLNode *node);

inline bool LLRemoveValue(LinkedList *list, const void *value, int size)
{
    LLNode *node = list->First;
    do {
        u8 *a = (u8 *)value;
        u8 *b = (u8 *)node->DataStart;

        for (int i = 0; i < size && a[i] == b[i]; i++) {
            if (i == size - 1) {
                LLRemove(list, node);
                return true;
            }
        }
    } while (node = node->Next);

    return false;
}

inline void LLRemove(LinkedList *list, LLNode *node)
{
    if (!node)
        return;

    if (list->Length == 1) {
        list->Length = 0;
        list->First = nullptr;
        list->Last = nullptr;
        PzFreeHeapMemory(nullptr, node);
        return;
    }

    if (node->Previous) {
        node->Previous->Next = node->Next;

        if (node == list->Last)
            list->Last = node->Previous;
    }

    if (node->Next) {
        node->Next->Previous = node->Previous;

        if (node == list->First)
            list->First = node->Next;
    }

    list->Length--;
    PzFreeHeapMemory(nullptr, node);
}

inline LLNode *LLAdd(LinkedList *list, const void *value, int size)
{
    LLNode *node = (LLNode *)PzAllocateHeapMemory(nullptr, sizeof(LLNode) + size, 0);

    if (list->Length > 0) {
        node->Previous = list->Last;
        MemCopy(node->DataStart, value, size);
        list->Last = list->Last->Next = node;
    }
    else {
        MemCopy(node->DataStart, value, size);
        list->Last = list->First = node;
    }

    list->Length++;
    return node;
}

inline LLNode *LLPrepend(LinkedList *list, const void *value, int size)
{
    if (list->Length > 0) {
        LLNode *node = (LLNode *)
            PzAllocateHeapMemory(nullptr, sizeof(LLNode) + size, 0);

        MemCopy(node->DataStart, value, size);
        node->Next = list->First;
        list->First->Previous = node;
        list->First = node;
        list->Length++;
        return node;
    }
    else
        return LLAdd(list, value, size);
}

inline LLNode *LLInsert(LinkedList *list, LLNode *after, const void *value, int size)
{
    if (after == nullptr)
        return LLPrepend(list, value, size);

    if (after == list->Last)
        return LLAdd(list, value, size);

    LLNode *node = (LLNode *)
        PzAllocateHeapMemory(nullptr, sizeof(LLNode) + size, 0);

    MemCopy(node->DataStart, value, size);

    node->Previous = after;
    node->Next = after->Next;
    after->Next->Previous = node;
    after->Next = node;
    list->Length++;
    return node;
}

inline void LLRotateLeft(LinkedList *list)
{
    if (list->Length > 1) {
        LLNode *second = list->First->Next;
        second->Previous = nullptr;
        list->First->Previous = list->Last;
        list->First->Next = nullptr;
        list->Last->Next = list->First;
        list->Last = list->First;
        list->First = second;
    }
}

inline void LLRotateRight(LinkedList *list)
{
    if (list->Length > 1) {
        LLNode *second_to_last = list->Last->Previous;
        second_to_last->Next = nullptr;
        list->First->Previous = list->Last;
        list->Last->Next = list->First;
        list->First = list->Last;
        list->Last = second_to_last;
    }
}

inline static LLNode *MergeSort(LLNode *list, int (*cmp)(LLNode *a, LLNode *b))
{
    if (!list || !list->Next)
        return list;

    LLNode
        *right = list,
        *temp = list,
        *last = list,
        *result = 0,
        *next = 0,
        *tail = 0;

    while (temp && temp->Next)
    {
        last = right;
        right = right->Next;
        temp = temp->Next->Next;
    }

    last->Next = 0;

    list = MergeSort(list, cmp);
    right = MergeSort(right, cmp);

    while (list || right)
    {
        if (!right) {
            next = list;
            list = list->Next;
        }
        else if (!list) {
            next = right;
            right = right->Next;
        }
        else if (cmp(list, right) < 0) {
            next = list;
            list = list->Next;
        }
        else {
            next = right;
            right = right->Next;
        }

        if (!result)
            result = next;
        else
            tail->Next = next;

        next->Previous = tail;
        tail = next;
    }
    return result;
}

inline void LLSort(LinkedList *list, int (*cmp)(LLNode *a, LLNode *b))
{
    MergeSort(list->First, cmp);
}

inline void LLClear(LinkedList *list)
{
    while (list->Length)
        LLRemove(list, list->First);
}