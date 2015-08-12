#include<vector>
#include<string>
#include<iostream>
#include "utf8.h"

int main()
{
    std::vector<std::string> ss{
        u8"▲",
        u8"Language Learning and Teaching Изучение и обучение иностранных языков",
        u8"是一个专为语文教学而设计的电脑软件"
    };
    for(auto s : ss) {
        std::cout<<s<<'\n';
        int size = utf8::size(s);
        std::cout<<"Size: "<<size<<'\n';
        int i = 0;
        while(std::cin and (i<1 or i>size)) {
            std::cout<<"\tWhich character to index? ";
            std::cin>>i;
        }
        if(!std::cin) {
            std::cerr<<"Input failed.\n";
            return 1;
        }
        std::cout<<utf8::at(s,i-1)<<'\n';
    }
}
