#include "map_engine.h"



int MapEngine::GetOperate(char *operate){
    if(strcmp(operate,"SET") == 0){
        return 0;
    }
    else if(strcmp(operate,"GET") == 0){
        return 1;
    }
    else if(strcmp(operate,"MOD") == 0){
        return 2;
    }
    else if(strcmp(operate,"DEL") == 0){
        return 3;
    }
    else{
        return -1;
    }
}



void MapEngine::Set(char **tokens){
    if(tokens[1] == NULL || tokens[2] == NULL){
        std::cout<<"SET: KEY OR VALUE ERROR"<<std::endl;
        return;
    }
    std::string key,value;
    key = tokens[1];
    value = tokens[2];

    string_string_map_[key] = value;
    std::cout<<"map["<<key<<"]:"<<string_string_map_[key]<<" SET SUCCESS"<<std::endl;
}



std::string MapEngine::Get(char **tokens){

    std::string key,value;
    key = tokens[1];
    std::cout<<"key: "<<key<<std::endl;
    if(key == ""){
        std::cout<<"GET: NO KEY"<<std::endl;
        return "NULL";
    }
    
    if(string_string_map_.find(key) == string_string_map_.end()){
        std::cout<<"GET: NO VALUE"<<std::endl;
        return "NULL";
    }
    value = string_string_map_.find(key)->second;
    std::cout<<"map["<<key<<"]:"<<value<<std::endl;


    return value;
}



void MapEngine::Mod(char **tokens){
    if(tokens[1] == NULL || tokens[2] == NULL){
        std::cout<<"MOD: KEY OR VALUE ERROR"<<std::endl;
        return;
    }
    std::string key,value;
    key = tokens[1];
    value = tokens[2];

    if(string_string_map_.find(key) == string_string_map_.end()){
        std::cout<<"NO VALUE"<<std::endl;
        return;
    }
    string_string_map_[key] = value;
    std::cout<<"map["<<key<<"]:"<<string_string_map_[key]<<" MOD SUCCESS"<<std::endl;
}



void MapEngine::Del(char **tokens){
    if(tokens[1] == NULL){
        std::cout<<"DEL: NO KEY"<<std::endl;
        return;
    }
    std::string key;
    key = tokens[1];
    if(string_string_map_.find(key) == string_string_map_.end()){
        std::cout<<"DEL: NO VALUE"<<std::endl;
        return;
    }
    string_string_map_.erase(key);
    std::cout<<"map["<<key<<"]:"<<"DEL SUCCESS"<<std::endl;
}




