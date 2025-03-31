#include "lsm_tree.h"
#include <unistd.h>
LSMTree lsm_tree;
int main(){
    

    for(int  i = 0;i < 1000000;i++){
        lsm_tree.Put(std::to_string(i),"value" + std::to_string(i));
        if(i % 10000 == 0){
            if(i == 0)continue;
            sleep(3);
        }
    }

    int flag = 0;
    for(int i = 0;i < 1000000;i++){
        std::string value = lsm_tree.Get(std::to_string(i));
        if(value.empty()){
            flag++;
            std::cout<<"value is empty:"<<i<<std::endl;
        }
    }
    std::cout<<"flag = "<<flag<<std::endl;

    sleep(1000);
}   
