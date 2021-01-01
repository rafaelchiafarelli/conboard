#include "main.hpp"

#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include "XMLHeaderParser.h"
#include "actions.h"
#include <stdio.h>
#include <usb.h>
using namespace std;





static void sig_handler(int dummy)
{
	stop = true;
}

int main(int argc, char *argv[])
{
	struct usb_bus *bus;
    struct usb_device *dev;
    usb_init();
    usb_find_busses();
    usb_find_devices();
    for (bus = usb_busses; bus; bus = bus->next)
        for (dev = bus->devices; dev; dev = dev->next){
            printf("Trying device %s/%s\n", bus->dirname, dev->filename);
            printf("\tID_VENDOR = 0x%04x\n", dev->descriptor.idVendor);
            printf("\tID_PRODUCT = 0x%04x\n", dev->descriptor.idProduct);
        }
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
/**
 * This function will read all xml files from folder, identify witch of the devices are plugged in. 
 * If a device it is pluged in, this function will launch the specific handler for it.
 */ 

void read_all(char *path)
{
	vector<dirent> xmlFiles;
    struct dirent *entry;
    DIR *dir = opendir(path);
    if (dir == NULL) {
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
		if(string(entry->d_name).find_last_of(".xml") != std::string::npos)
        	xmlFiles.push_back(*entry);
    }
    closedir(dir);

	for(vector<dirent>::iterator f_it = xmlFiles.begin();
		f_it != xmlFiles.end();
		f_it++)
	{
		XMLHeaderParser *xmlHeader = new XMLHeaderParser(f_it->d_name); 
		switch (xmlHeader->GetType())
		{
		case midi:
			/* code */
			break;
		case keyboard:
			/* code */
			break;
		case mouse:
			/* code */
			break;
		case joystick:
			/* code */
			break;

		default:
		//no device found, no action mus be taken
			break;
		}
	}


}

/**
 * this function will tell if a given device has a handle (xml file with an executable). If not, this function will create a dummy one.
 * The user could give the other information about this.
 * 
 * 
 */ 
void verify_xml(char *folder)
{

}

/**
 * don't know what this function will be used for. But it is here.
 * 
 * 
 * 
 */ 
void clear(char *folder)
{

}
