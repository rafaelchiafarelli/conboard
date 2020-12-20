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
		{"ID", 1, NULL, 'i'},
		{"port", 1, NULL, 'p'},
		{"xml", 1, NULL, 'x'},
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

		case 'p':
		
			strcpy(p_name,optarg);
			break;
		case 'x':
		
			xmlFileName = string(optarg);
			break;
		case 'i':
		
			sprintf(fifoFile, "%s",optarg);
			break;

		default:
			std::cout<<"Try more information."<<endl;
			return 1;
		}
		
	}
 
    return 0;
}


