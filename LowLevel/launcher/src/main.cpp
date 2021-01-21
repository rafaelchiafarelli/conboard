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
	srand(time(NULL));
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

	std::cout<<"end of program"<<std::endl;
    return 0;
}
/**
 * This function will read all json files from folder, identify witch of them have executables and services, 
 * and launch the specific server for it, if the device is not connected, the handler must handle it.
 */ 

void read_all(char *path)
{
	/* Get all directories */
	vector<dirent> jsonFiles;
    struct dirent *json_entry;
    DIR *json_dir = opendir(path);
	bool hasHandler = false;
    if (json_dir == NULL) {
		
        return;
    }

    while ((json_entry = readdir(json_dir)) != NULL) {
		if(string(json_entry->d_name).find(".json") != std::string::npos)
        	jsonFiles.push_back(*json_entry);
    }
    closedir(json_dir);
	/* Get all services */
	vector<dirent> serviceFiles;
	struct dirent *service_entry;
	char service_folder[] = {"/etc/systemd/system/"};
	DIR *service_dir = opendir(service_folder);
	if (service_dir == NULL) {
		return;
	}
	while ((service_entry = readdir(service_dir)) != NULL) {
		if(string(service_entry->d_name).find(".service") != std::string::npos)
			serviceFiles.push_back(*service_entry);
	}
	closedir(service_dir);

	char complete_file_name[1024];
	memset(complete_file_name,0,1024);
	std::vector<ModeType> Mode;
	std::vector<Actions> h;
	jsonParser header(complete_file_name,&Mode,&h);
	std::cout<<"here"<<std::endl;
	bool has_service = false;
	for(vector<dirent>::iterator files_it = jsonFiles.begin();
		files_it!=jsonFiles.end();
		files_it++
		)
	{
		//for all the json files present
		sprintf(complete_file_name, "%s/%s",path,files_it->d_name);
		header.Reload(complete_file_name,&Mode,&h);
		std::cout<<"and here"<<std::endl;
		if(header.GetLoaded())
		{
			std::string serviceName = header.DevName;
			serviceName.append(".service");
			for(vector<dirent>::iterator service_it = serviceFiles.begin();
				service_it!=jsonFiles.end();
				service_it++)
			{
				if(!serviceName.compare(service_it->d_name))
				{
					char cmd[512];
					sprintf(cmd,"systemctl restart %s",service_it->d_name);
					system(cmd);
					has_service = true;
					break;
				}
				else
				{
					has_service = false;
				}
			}
		}
	}
}

/**
 * this function will tell if a given device has a handle (json file with an executable). If not, this function will create a dummy one.
 * The user WILL give the other information about this.
 */ 
void create_json(char *devInfo, char *folder)
{
	bool hasHandler = false;

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
	std::vector<ModeType> Mode;
	std::vector<Actions> h;
	jsonParser local_json("",&Mode,&h);
	for(vector<dirent>::iterator files_it = jsonFiles.begin();
		files_it!=jsonFiles.end();
		files_it++
		)
	{//for all the json files present
		char *complete_file_name = new char[strlen(folder)+strlen(files_it->d_name)];
		sprintf(complete_file_name, "%s/%s",folder,files_it->d_name);
		hasHandler = false;
		local_json.Reload(complete_file_name,&Mode,&h);

		if(local_json.GetLoaded())
		{
			std::vector<KeyValue> tags = local_json.GetTags();
			
			if(tags.size() == 0)
			{
				hasHandler = false;
			}
			else
			{
			for(vector<KeyValue>::iterator info_it = info_from_dev.begin();
				info_it != info_from_dev.end();
				info_it++)
				{
					for(vector<KeyValue>::iterator header_it = tags.begin();
						header_it != tags.end();
						header_it++)
					{ //check if all the keys and values from json are present in devInfo and equal!
					std::cout<<"key:"<<header_it->key.c_str()<<" value:"<<header_it->value.c_str()<<std::endl;

						if((info_it->key.compare(header_it->key)) || 
							(info_it->value.compare(header_it->value)))
						{
							hasHandler = false;
						}
						else
						{
							std::cout<<"found:"<<info_it->key.c_str()<<std::endl;
							hasHandler = true;
							break;
						}
					}	
					if(!hasHandler)
					{
						std::cout<<"a key was not found:"<<info_it->key.c_str()<<std::endl;
						break;
					}
				}
			}
			bool has_service = false;
			if(hasHandler && local_json.GetHasExec())
			{
				vector<dirent> serviceFiles;
				struct dirent *entry;
				char service_folder[] = {"/etc/systemd/system/"};
				DIR *dir = opendir(service_folder);
				if (dir == NULL) {
					return;
				}
				while ((entry = readdir(dir)) != NULL) {
					if(string(entry->d_name).find(".service") != std::string::npos)
						serviceFiles.push_back(*entry);
				}
				std::string serviceName = local_json.DevName;
				serviceName.append(".service");
				for(vector<dirent>::iterator files_it = serviceFiles.begin();
					files_it!=jsonFiles.end();
					files_it++)
				{
					if(!serviceName.compare(files_it->d_name))
					{
						char cmd[512];
						sprintf(cmd,"systemctl restart %s",files_it->d_name);
						system(cmd);
						has_service = true;
						break;
					}
					else
					{
						has_service = false;
					}
				}
			}

			if(!has_service && hasHandler && local_json.GetHasExec())
			{
			std::string ExecLine;
			ExecLine = "";
			ExecLine.append(local_json.Ex.exec);
			ExecLine.append(" ");
			
			for(std::vector<KeyValue>::iterator param_it = local_json.Ex.params.begin();
				param_it != local_json.Ex.params.end();
				param_it++)
				{
					std::cout<<"param:"<<param_it->key.c_str()<<" "<<param_it->value.c_str()<<std::endl;
					ExecLine.append(param_it->key);
					ExecLine.append(" ");
					ExecLine.append(param_it->value);
					ExecLine.append(" ");
				}
				std::string filename = "";
				filename.append("/etc/systemd/system/");
				filename.append(local_json.DevName);
				filename.append(".service");
				std::ofstream serviFileStream(filename, std::ofstream::out);
				std::cout<<"service file name:"<<filename<<std::endl;
				std::string service_data = "";
				service_data.append("[Unit]\r\n");
				service_data.append("Description=auto generated service file.\r\n");
				service_data.append("Documentation=https://github.com/rafaelchiafarelli/conboard\r\n\r\n");
				service_data.append("[Install]\r\n");
				service_data.append("WantedBy=multi-user.target\r\n");
				service_data.append("[Service]\r\n");
				service_data.append("User=root\r\n");
				service_data.append("Type=simple\r\n");
				service_data.append("ExecStart=");
				service_data.append(ExecLine);
				service_data.append("\r\n");				
				serviFileStream<<service_data;
				serviFileStream.close();					
			}

			if(!hasHandler)
			{
				std::cout<<"no handler! generate the dummy"<<std::endl;
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
					snprintf(dummy_filename,256, "%s%d-%d-%d.json",folder,unit,tens,hundreds);

					std::ofstream dummyJsonFile(dummy_filename, std::ofstream::out);
					std::cout<<"Dummy file name:"<<dummy_filename<<std::endl;
					dummyJsonFile<<dummyjson;
					dummyJsonFile.close();
			}
		}
		delete complete_file_name;

	}
}

/**
 * don't know what this function will be used for. But it is here.
 */ 
void clear(char *folder)
{

}
