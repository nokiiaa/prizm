#pragma once

#include <link.hh>
#include <defs.hh>
#include <vector>
#include <string>
#include <memory>

enum { ValBool, ValInt, ValHex, ValCStr, ValStr, ValTable };

struct ObjView;

struct StateValue {
    int Type;
    bool Bool;
    i64 Int;
    void *Hex;
    const char *CStr;
    std::string Str;
    std::shared_ptr<ObjView> Table;
    StateValue(bool bl) : Bool(bl), Type(ValBool) { }
    StateValue(i64 i) : Int(i), Type(ValInt) { }
    StateValue(void *ptr) : Hex(ptr), Type(ValHex) { }
    StateValue(const char *cstr) : CStr(cstr), Type(ValCStr) { }
    StateValue(std::string str) : Str(str), Type(ValStr) { }
    StateValue(std::shared_ptr<ObjView> table) : Table(table), Type(ValTable) { }
};

struct ObjView {
    const char *Name;
    std::vector<const char *> Names;
    std::vector<StateValue> Values;
    ObjView(const char *name) : Name(name) {}
    inline void ValName(const char *name)
    { Names.push_back(name); }
    template<typename T> inline void ValueHex(T value)
    { Values.push_back(StateValue((void *)(size_t)value)); }
    template<typename T> inline void ValueInt(T value)
    { Values.push_back(StateValue((i64)value)); }
    template<typename T> inline void ValueBool(T value)
    { Values.push_back(StateValue((bool)value)); }
    inline void ValueCStr(const char *value)
    { Values.push_back(StateValue(value)); }
    inline void ValueStr(std::string value)
    { Values.push_back(StateValue(value)); }
    inline void ValueTable(std::shared_ptr<ObjView> value)
    { Values.push_back(StateValue(value)); }
};