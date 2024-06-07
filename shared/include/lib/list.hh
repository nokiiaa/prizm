#pragma once

#include <lib/malloc.hh>
#include <spinlock.hh>

template<typename T>
struct LLNode
{
#ifdef DEBUGGER_INCLUDE
    DEBUGGER_TARGET_PTR Previous, Next;
#else
    LLNode<T> *Previous, *Next;
#endif
    union {
        T Value;
        u8 ValueStart[0];
    };
    inline LLNode()
    {
        Previous = nullptr;
        Next = nullptr;
    }
};

#define ENUM_LIST(n, list) for (auto *n = (list).First; n; n = n->Next)

template<typename T>
struct LinkedList
{
#ifdef DEBUGGER_INCLUDE
    DEBUGGER_TARGET_PTR First, Last;
#else
    LLNode<T> *First, *Last;
#endif
    int Length;
    PzSpinlock Spinlock;

#ifndef DEBUGGER_INCLUDE
    inline LinkedList()
    {
        First = nullptr;
        Last = nullptr;
        Length = 0;
    }

    inline void Enumerate(void (*enumerator)(T &value))
    {
        LLNode<T> *node = First;
        do enumerator(node->Value);
        while (node = node->Next);
    }

    inline bool RemoveValue(T value)
    {
        LLNode<T> *node = First;
        do {
            if (node->Value == value) {
                Remove(node);
                return true;
            }
        } while (node = node->Next);
        return false;
    }

    inline void Remove(LLNode<T> *node)
    {
        if (!node)
            return;
        if (Length == 1) {
            Length = 0;
            First = nullptr;
            Last = nullptr;
            delete node;
            return;
        }
        if (node->Previous) {
            node->Previous->Next = node->Next;
            if (node == Last)
                Last = node->Previous;
        }
        if (node->Next) {
            node->Next->Previous = node->Previous;
            if (node == First)
                First = node->Next;
        }
        Length--;
        delete node;
    }

    inline LLNode<T> *Add(const T &value)
    {
        LLNode<T> *node;
        if (Length > 0) {
            node = new LLNode<T>();
            node->Previous = Last;
            node->Value = value;
            Last = Last->Next = node;
        }
        else {
            node = new LLNode<T>();
            node->Value = value;
            Last = First = node;
        }
        Length++;
        return node;
    }

    inline LLNode<T> *Prepend(const T &value)
    {
        if (Length > 0) {
            LLNode<T> *node = new LLNode<T>();
            node->Value = value;
            node->Next = First;
            First->Previous = node;
            First = node;
            Length++;
            return node;
        }
        else
            return Add(value);
    }

    inline LLNode<T> *Insert(LLNode<T> *after, const T &value)
    {
        if (after == nullptr)
            return Prepend(value);
        if (after == Last)
            return Add(value);
        LLNode<T> *node = new LLNode<T>();
        node->Value = value;
        node->Previous = after;
        node->Next = after->Next;
        after->Next->Previous = node;
        after->Next = node;
        Length++;
        return node;
    }

    inline void RotateLeft()
    {
        if (Length > 1) {
            LLNode<T> *second = First->Next;
            second->Previous = nullptr;
            First->Previous = Last;
            First->Next = nullptr;
            Last->Next = First;
            Last = First;
            First = second;
        }
    }

    inline void RotateRight()
    {
        if (Length > 1) {
            LLNode<T> *second_to_last = Last->Previous;
            second_to_last->Next = nullptr;
            First->Previous = Last;
            Last->Next = First;
            First = Last;
            Last = second_to_last;
        }
    }

    inline static LLNode<T> *MergeSort(LLNode<T> *list, int (*cmp)(LLNode<T> *a, LLNode<T> *b))
    {
        if (!list || !list->Next)
            return list;

        LLNode<T>
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

    inline void Sort(int (*cmp)(LLNode<T> *a, LLNode<T> *b)) { MergeSort(First, cmp); }

    inline void Clear()
    {
        while (Length)
            Remove(First);
    }

    inline ~LinkedList()
    {
        Clear();
    }
#endif
};