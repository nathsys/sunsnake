
#include <cstdio>
#include <stdexcept>

#pragma once

template <typename T, size_t s>
struct Stack;

// Sublist of a stack. Automatically sets the parent's stack pointer back when
// this list goes out of scope or deleted.
template <typename T, size_t s>
struct ListFromStack
{
	ListFromStack(T *start, T *last, Stack<T, s> &parent) : last(last), start(start), parent(parent) {};
	~ListFromStack() { parent.set_last(start); }
	size_t size() { return last - start; }
	T *begin() { return start; }
	T *end() { return last; }

private:
	T *start;
	T *last;
	Stack<T, s> &parent;
};

// Custom stack.
template <typename T, size_t s>
struct Stack
{
	T stack[s];
	Stack() : last(stack) {};
	void push(T val) { *last++ = val; }
	T pop()
	{
		if (last == stack)
			return *last;
		return *(--last);
	}
	T &top() { return (*last); }
	T &go_next() { return (*last++); }
	ListFromStack<T, s> from(T *t) { return ListFromStack<T, s>(t, last, *this); }
	void destroy(ListFromStack<T, s> *sub) { last = sub->begin(); }
	void set_last(T *l) { last = l; }
	void point_prev() { last--; }
	void point_next() { last++; }

private:
	T *last;
};