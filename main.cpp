#include <iostream>
#include "Media.h"

void printUsage(const char* opt){
    std::cout<<"\nUsage: "<<opt<<" [options]\n"
    <<"Options: \n"
    <<" -p --path   set rtsp path \n"
    <<" -r --rate   set framerate \n\n";
}


int main(int argc,char** argv) {
    std::string rtsp_path;
    int rate = 0;
    char* endptr;
    if(argc < 2){
        printUsage(argv[0]);
        return 0;
    }

    for(int i=1;i<argc;++i){
        std::string arg = std::string(argv[i]);
        if(arg == "-h" || arg == "--help"){
            printUsage(argv[0]);
            return 0;
        }else if(arg == "-p" || arg == "--path"){
            std::cout<<arg<<" "<<argv[i+1]<<"\n";
            rtsp_path = std::string(argv[i+1]);
        }else if(arg == "-r" || arg == "--rate"){
            std::cout<<arg<<" "<<argv[i+1]<<"\n";
            rate = strtol(argv[i+1],&endptr,10);
        }else{}
    }

    auto media = new Media(rtsp_path,rate);
    media->open();
    media->push();
    return 0;
}
