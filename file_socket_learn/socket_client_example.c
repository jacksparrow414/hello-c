/* 
使用宏，对于不同的平台使用不同的头文件

不同操作系统对应的宏见
https://stackoverflow.com/questions/142508/how-do-i-check-os-with-a-preprocessor-directive
https://sourceforge.net/p/predef/wiki/OperatingSystems/
*/
#ifdef _WIN64  
// winsock.h和winsock2.h应该使用哪个? https://stackoverflow.com/questions/14094457/winsock-h-winsock2-h-which-to-use
    #include <winsock2.h>
#elif defined(__linux__) || defined(__APPLE__)
    #include <sys/socket.h>
#endif

#include "socket_client_example.h"

void access_website_index_page_then_write_response_to_file(char *website_url) {
    
}