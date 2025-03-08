#pragma once

#include <string.h>

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>

class MapEngine{

public:
    
    //根据操作字符串返回操作码
    int GetOperate(char *operate);

    //根据tokens执行SET操作
    void Set(char **tokens);

    //根据tokens执行GET操作
    std::string Get(char **tokens);

    void Mod(char **tokens);

    void Del(char **tokens);
private:
    std::unordered_map<std::string,std::string> string_string_map_;
    const char* Operate[4] = {"SET","GET","MOD","DEL",};

};





















