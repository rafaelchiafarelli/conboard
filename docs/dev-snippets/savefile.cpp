
#include <fstream>

int save(std::string filename, char *text)
{
    std::ofstream FileStream(filename, std::ofstream::app);
    FileStream<<text;
    FileStream.close();	
    return 0;
}