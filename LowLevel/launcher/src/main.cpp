#include "main.hpp"

#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include "jsonParser.h"
#include "keyParser.hpp"
#include "actions.h"
#include <stdio.h>
#include <stdlib.h> 
#include <usb.h>

#include <unistd.h>
#include <spawn.h>
#include <fstream>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

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
		{"read", 1, NULL, 'r'}, /* reads all json from a folder and check if there are devices available -r /conboard/boards*/
		{"verify", 1, NULL, 'v'},/* verify if -v /tmp/temp.vars is a valid set of vars that*/
		{"clear", 1, NULL, 'c'}, /* don't know what I will use it for... yet*/
		{"json",1,NULL,'x'},
		{"data",1,NULL,'d'},
		{ }
	};
    
	if(argc < 4)
	{
		std::cout<<"error, must specifi port and json. Usage ./launcher -rcv -x \"/home/user/file.json\" -d \"/tmp/a\""<<endl;
		return -1;
	}
    char dt_name[256];
	memset(dt_name,256,0);
	char json_name[256];
	memset(json_name,0,256);
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
			strcpy(json_name,optarg);
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
		read_all(json_name);
		break;
	case modType::verify_devices:
		/* code */
		create_json(dt_name,json_name);
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
 * This function will read all json files from folder, identify witch of them have executables, 
 * and launch the specific handler for it, if the device is not connected, the handler must handle it.
 */ 

void read_all(char *path)
{
	vector<dirent> jsonFiles;
    struct dirent *entry;
    DIR *dir = opendir(path);
	bool hasHandler = false;
    if (dir == NULL) {
		
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
		if(string(entry->d_name).find(".json") != std::string::npos)
        	jsonFiles.push_back(*entry);
    }
    closedir(dir);
	char complete_file_name[1024];
	for(vector<dirent>::iterator files_it = jsonFiles.begin();
		files_it!=jsonFiles.end();
		files_it++
		)
	{
		//for all the json files present

		sprintf(complete_file_name, "%s/%s",path,files_it->d_name);
		
		std::vector<ModeType> Mode;
		std::vector<Actions> h;

		jsonParser *header;
		char **argv;
		char *ExecLine;
		header = new jsonParser(complete_file_name,&Mode,&h);
		
		if(header->GetLoaded())
		{
			argv = new char*[header->Ex.params.size()];
			ExecLine = new char[header->Ex.exec.length()];
			strcpy(ExecLine,header->Ex.exec.c_str());
			int count = 0;
			std::cout<<"number of parameters:"<<header->Ex.params.size()<<std::endl;
			for(std::vector<KeyValue>::iterator param_it = header->Ex.params.begin();
				param_it != header->Ex.params.end();
				param_it++)
				{
					argv[count] = new char[256];
					sprintf(argv[count],"%s %s",param_it->key.c_str(), param_it->value.c_str());
					std::cout<<"argv["<<count<<"]:"<<argv[count]<<std::endl;
				}

			pid_t pid;
			int status;
			status = posix_spawn(&pid,ExecLine,NULL,NULL,argv,environ);
			std::cout<<"spanwned "<<ExecLine<<" and result is:"<<status<<std::endl;
			delete argv;
			delete ExecLine;
		}

		delete header;
	}
}

/**
 * this function will tell if a given device has a handle (json file with an executable). If not, this function will create a dummy one.
 * The user WILL give the other information about this.
 */ 
void create_json(char *devInfo, char *folder)
{
	bool hasHandler = false;
	char **argv;
	char *ExecLine;
	vector<dirent> jsonFiles;
    struct dirent *entry;
    DIR *dir = opendir(folder);
    if (dir == NULL) {
        return;
    }
    while ((entry = readdir(dir)) != NULL) {
		if(string(entry->d_name).find(".json") != std::string::npos)
        	jsonFiles.push_back(*entry);
    }
    closedir(dir);
	keyParser info(devInfo,'=');
	vector<KeyValue> info_from_dev = info.GetParsed();
	for(vector<dirent>::iterator files_it = jsonFiles.begin();
		files_it!=jsonFiles.end();
		files_it++
		)
	{//for all the json files present
		char *complete_file_name = new char[strlen(folder)+strlen(files_it->d_name)];
		sprintf(complete_file_name, "%s/%s",folder,files_it->d_name);
		std::vector<ModeType> Mode;
		std::vector<Actions> h;
		jsonParser *json;

		hasHandler = false;
		json = new jsonParser(complete_file_name,&Mode,&h);

		if(json->GetLoaded())
		{
			std::vector<KeyValue> tags = json->GetTags();
			
			if(tags.size() == 0)
			{
				hasHandler = false;
			}
			else
			{
				for(vector<KeyValue>::iterator header_it = tags.begin();
					header_it != tags.end();
					header_it++)
				{ //check if all the keys and values from json are present in devInfo and equal!

					for(vector<KeyValue>::iterator info_it = info_from_dev.begin();
						info_it != info_from_dev.end();
						info_it++)
					{
						if((!info_it->key.compare(header_it->key)) || 
							(!info_it->value.compare(header_it->value)))
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
			}
			if(hasHandler)
			{
				argv = new char*[json->Ex.params.size()];
				ExecLine = new char[json->Ex.exec.length()];
				strcpy(ExecLine,json->Ex.exec.c_str());
				int count = 0;
				for(std::vector<KeyValue>::iterator param_it = json->Ex.params.begin();
					param_it != json->Ex.params.end();
					param_it++)
				{
					argv[count] = new char[256];
					sprintf(argv[count],"%s %s",param_it->key.c_str(), param_it->value.c_str());
				}
			}
		}
		
		delete complete_file_name;
		if(hasHandler)
			break;
		delete json;
	}
	if(hasHandler)
	{
		cout<<"has a handler and the cmd line is:"<<ExecLine<<endl;
		pid_t pid;
		int status;
		status = posix_spawn(&pid,ExecLine,NULL,NULL,argv,environ);
		delete argv;
		delete ExecLine;
	}
	else
	{
	std::string dummyjson("{\"DEVICE\":{\"type\":\"\", \"name\":\"\", \"input\":\"\", \"output\":\"\"}, \"header\":{\"identifier\":{\"generics\":{");
	for(std::vector<KeyValue>::iterator info_it = info_from_dev.begin();
		info_it != info_from_dev.end();
		info_it++)
		{
			dummyjson.append("\"");
			dummyjson.append(info_it->key);
			dummyjson.append("\":\"");
			dummyjson.append(info_it->value);
			dummyjson.append("\",");
		}
		dummyjson.append("},\"executable\": {\"exec\":\"\", \"count\":\"\",\"params\":{}}}}}");
		
		char dummy_filename[256];
		int unit = rand() % 100;
		int tens = rand() % 100;
		int hundreds = rand() % 100;
		snprintf(dummy_filename,256, "%s/%d-%d-%d.json",folder,unit,tens,hundreds);

		std::ofstream dummyJsonFile(dummy_filename, std::ofstream::out);
		dummyJsonFile<<dummyjson;
		dummyJsonFile.close();
	}
}

/**
 * don't know what this function will be used for. But it is here.
 */ 
void clear(char *folder)
{

}
