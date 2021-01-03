#include "main.hpp"

#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include "XMLHeaderParser.h"
#include "keyParser.hpp"
#include "actions.h"
#include <stdio.h>
#include <usb.h>

#include <unistd.h>
#include <spawn.h>

using namespace std;

extern char **environ;
typedef enum{
	read_devices,
	verify_devices,
	clear_something,
	none

}modType;


static void sig_handler(int dummy)
{
	stop = true;
}

int main(int argc, char *argv[])
{

	modType work = none;
	struct usb_bus *bus;
    struct usb_device *dev;

    usb_init();
    usb_find_busses();
    usb_find_devices();
    for (bus = usb_busses; bus; bus = bus->next)
        for (dev = bus->devices; dev; dev = dev->next)
		{
            printf("Trying device %s/%s\n", bus->dirname, dev->filename);

            printf("\tID_VENDOR = 0x%04x\n", dev->descriptor.idVendor);
            printf("\tID_PRODUCT = 0x%04x\n", dev->descriptor.idProduct);
        }
	stop = true;
	static const char short_options[] = "rvcx:d:";
	static const struct option long_options[] = {
		{"read", 1, NULL, 'r'}, /* reads all xml from a folder and check if there are devices available -r /conboard/boards*/
		{"verify", 1, NULL, 'v'},/* verify if -v /tmp/temp.vars is a valid set of vars that*/
		{"clear", 1, NULL, 'c'}, /* don't know what I will use it for... yet*/
		{"xml",1,NULL,'x'},
		{"data",1,NULL,'d'},
		{ }
	};
    
	if(argc < 4)
	{
		std::cout<<"error, must specifi port and xml. Usage ./launcher -rcv -x \"/home/user/file.xml\" -d \"/tmp/a\""<<endl;
		return -1;
	}
    char dt_name[256];
	memset(dt_name,256,0);
	char xml_name[256];
	memset(xml_name,0,256);
	int c;


	while ((c = getopt_long(argc, 
							argv, 
							short_options,
		     				long_options, 
							NULL)) != -1) {
		switch (c) {

		case 'd':
			strcpy(dt_name,optarg);
			break;
		case 'x':
			strcpy(xml_name,optarg);
			break;
		case 'c':
			work = modType::clear_something;
			break;
		case 'r':
			work = read_devices;
			break;
		case 'v':
			work = verify_devices;
			break;
		
		default:
			std::cout<<"Try more information."<<endl;
			return 1;
		}
	

	}
 	switch (work)
	{
	case modType::read_devices:
		read_all(dt_name,xml_name);
		break;
	case modType::verify_devices:
		/* code */
		create_xml(dt_name,xml_name);
		break;	 
	case modType::clear_something:
		/* code */
		break;		 
	default:
		break;
	}

	
    return 0;
}
/**
 * This function will read all xml files from folder, identify witch of the devices are plugged in. 
 * If a device it is pluged in, this function will launch the specific handler for it.
 */ 

void read_all(char *devInfo, char *path)
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
 * The user WILL give the other information about this.
 */ 
void create_xml(char *devInfo, char *folder)
{
	bool hasHandler = false;
	char *ExecLine;
	char **argv;
	vector<dirent> xmlFiles;
    struct dirent *entry;
    DIR *dir = opendir(folder);
    if (dir == NULL) {
        return;
    }
    while ((entry = readdir(dir)) != NULL) {
		if(string(entry->d_name).find(".xml") != std::string::npos)
        	xmlFiles.push_back(*entry);
    }
    closedir(dir);
	keyParser info(devInfo,'=');
	vector<KeyValue> info_from_dev = info.GetParsed();
	for(vector<dirent>::iterator files_it = xmlFiles.begin();
		files_it!=xmlFiles.end();
		files_it++
		)
	{//for all the xml files present
		char *complete_file_name = new char[strlen(folder)+strlen(files_it->d_name)];
		sprintf(complete_file_name, "%s/%s",folder,files_it->d_name);
		XMLHeaderParser *header = new XMLHeaderParser(complete_file_name);
		if(header->GetParseOK())
		{
			for(vector<KeyValue>::iterator header_it = header->keys.begin();
				header_it != header->keys.end();
				header_it++)
			{ //check if all the keys and values from XML are present in devInfo and equal!

				for(vector<KeyValue>::iterator info_it = info_from_dev.begin();
					info_it != info_from_dev.end();
					info_it++)
				{
					if((info_it->key != header_it->key) || (info_it->value != header_it->value))
					{
						hasHandler = false;
					}
					else
					{
						hasHandler = true;
						break;
					}
				}	
			}
			if(hasHandler)
			{
				ExecLine = new char[strlen(header->ExecLine)];
				strcpy(ExecLine,header->ExecLine);
				argv = new char*[header->counter];
				for(int count = 0;count<header->counter;count++)
				{
					cout<<header->argv[count]<<endl;
					argv[count] = new char[strlen(header->argv[count])];
					strcpy(argv[count],header->argv[count]);
				}
			}
		}
		delete header;
		delete complete_file_name;
		if(hasHandler)
			break;
	}
	if(hasHandler)
	{
		cout<<"has a handler and the cmd line is:"<<ExecLine<<endl;
		pid_t pid;
		int status;
		status = posix_spawn(&pid,ExecLine,NULL,NULL,argv,environ);
	}
	else
	{
		char dummy_filename[256];
		snprintf(dummy_filename,256, "%s/XML_dummy.xml",folder);
		XMLHeaderParser *dummy = new XMLHeaderParser(devInfo,dummy_filename);
		delete dummy;
	}
	delete argv;
	delete ExecLine;
}

/**
 * don't know what this function will be used for. But it is here.
 */ 
void clear(char *folder)
{

}
