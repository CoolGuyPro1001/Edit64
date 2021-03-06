#pragma once

#define ASSERT(x) if(!(x)) __builtin_trap();
#define GLCall(x) ClearErrors();\
    ASSERT(LogCall(#x, __FILE__, __LINE__))

#define GLCallAssign(x, y) ClearErrors();\
    ASSERT(LogCall(#x, __FILE__, __LINE__))\
    y = x;

bool LogCall(const char* function, const char* file, int line);
void ClearErrors();