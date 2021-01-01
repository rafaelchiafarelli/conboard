#include "main.hpp"


using namespace std;


static void sig_handler(int dummy)
{
	stop = true;
}

int main(int argc, char *argv[])
{
	stop = true;
	static const char short_options[] = "i:p:x:";
	static const struct option long_options[] = {
		{"read", 1, NULL, 'r'}, /* reads all xml from a folder and check if there are devices available -r /conboard/boards*/
		{"verify", 1, NULL, 'v'},/* verify if -v /tmp/temp.vars is a valid set of vars that*/
		{"clear", 1, NULL, 'c'}, /* don't know what I will use it for... yet*/
		{ }
	};
    
	if(argc < 4)
	{
		std::cout<<"error, must specifi port and xml. Usage ./midi -p: \"hw:1,0,0\" -x \"/home/user/file.xml\""<<endl;
		return -1;
	}
    char p_name[256];
	memset(p_name,256,0);
	char fifoFile[256];
	memset(fifoFile,0,256);
    string xmlFileName;
	int c;
	while ((c = getopt_long(argc, argv, short_options,
		     		long_options, NULL)) != -1) {
		switch (c) {

		case 'r':
		
			read_all(optarg);
			break;
		case 'v':
		
			verify_xml(optarg);
			break;
		case 'c':
		
			clear(optarg);
			break;

		default:
			std::cout<<"Try more information."<<endl;
			return 1;
		}
		
	}
 
    return 0;
}

void read_all(char *folder)
{

}

void verify_xml(char *folder)
{

}

void clear(char *folder)
{
	
}
