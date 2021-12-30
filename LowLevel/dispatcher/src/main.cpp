
#include "main.hpp"
using namespace std;

/**
 * This is the dispatcher for the system
 * All devices launched will connect to this dispatcher and receive a unique handler. 
 * This handler will be used to talk to the BackEnd application. They will send their users interactions
 * and will receive commands:
 * - Update-json: {"update":<file name>}
 * - Restart device: {"restart":<level of restart>}
 * - change mode of operation: {"change_mode":<number of mode of operation>} wtf???
 * - 
 * - 
 * Extra features:
 * Handle the vault (with encription Key)
 *  - Add files
 *  - Remove files
 *  - Change encription key
 *  - verify integrity
 *  - 
 * Handle the files shared
 *  - add files to the img
 *  - remove files from the img
 * 
 * Handle the communication with the external screen 
 *  - put the current IP address on the screen
 *  - put the secret code to log in on the screen
 *  
 **/ 

static void sig_handler(int dummy)
{
    std::cout<<"main handler called"<<std::endl;
	stop = true;
}

void user_handler(dispatcher *dsp,atomic_bool *lst){
    
    atomic_bool *lstop = lst;
    while(!(*lstop))
    {
        {
        std::lock_guard<std::mutex> _(mtx);
        for (auto u : users)
            u->send_text(dsp->GetConfigs());
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout<<"user_handler thread close"<<std::endl;
}

int main(int argc, char *argv[])
{

	stop = false;
	static const char short_options[] = "x:";
	static const struct option long_options[] = {
		{"json", 1, NULL, 'x'},
		{ }
	};
    
	if(argc < 2)
	{
		cout<<"error, must specifi json. Usage ./dispatcher -x \"/home/user/file.json\""<<endl;
		return -1;
	}
    string jsonFileName;
	int c;
	while ((c = getopt_long(argc, 
							argv, 
							short_options,
				     		long_options, NULL)) != -1) 
	{
		switch (c) {
		case 'x':
			jsonFileName = string(optarg);
			break;
		default:
			cout<<"Try more information."<<endl;
			return 1;
		}
	}

	signal(SIGINT,  sig_handler);
	signal(SIGTERM, sig_handler);
	signal(SIGKILL, sig_handler);
	signal(SIGQUIT, sig_handler);
	std::cout<<"filename:"<<jsonFileName.c_str()<<std::endl;

	dispatcher dsp(jsonFileName, &stop);


	crow::SimpleApp app;
    std::string test_str="você acabou de entrar";


    CROW_ROUTE(app, "/config")
        .methods("GET"_method)
    ([&] {
        return crow::json::wvalue(dsp.GetConfigs());
    });

    CROW_ROUTE(app, "/iocommand")
        .methods("POST"_method)

    ([&](const crow::request& req){
        auto x = crow::json::load(req.body);
        bool ret = false;
        if (!x)
        {
            return crow::response(400);
        }
        if(x["UUID"] && x["params"])
        {
            std::vector<std::string> params(x["params"].begin(),x["params"].end());
            std::string uuid = std::string(x["UUID"]);

            ret = dsp.PostIOCommand(uuid,params);
        }
        std::ostringstream os;
        os << "{\"result\":" <<ret<<"}";
        return crow::response(os.str());
    });

    CROW_ROUTE(app, "/sharedcommand")
        .methods("POST"_method)

    ([&](const crow::request& req){
        auto x = crow::json::load(req.body);
        bool ret = false;
        if (!x)
        {
            return crow::response(400);
        }
        if(x["UUID"] && x["params"])
        {
            std::vector<std::string> params(x["params"].begin(),x["params"].end());
            std::string uuid = std::string(x["UUID"]);

            ret = dsp.PostSharedCommand(uuid,params);
        }
        std::ostringstream os;
        os << "{\"result\":" <<ret<<"}";
        return crow::response(os.str());
    });

    CROW_ROUTE(app, "/vaultcommand")
        .methods("POST"_method)

    ([&](const crow::request& req){
        auto x = crow::json::load(req.body);
        bool ret = false;
        if (!x)
        {
            return crow::response(400);
        }
        if(x["UUID"] && x["params"])
        {
            std::vector<std::string> params(x["params"].begin(),x["params"].end());
            std::string uuid = std::string(x["UUID"]);

            ret = dsp.PostVaultCommand(uuid,params);
        }
        std::ostringstream os;
        os << "{\"result\":" <<ret<<"}";
        return crow::response(os.str());
    });

    CROW_ROUTE(app, "/ws")
      .websocket()
      .onopen([&](crow::websocket::connection& conn) {
          CROW_LOG_INFO << "new websocket connection from " << conn.get_remote_ip();
          {
          std::lock_guard<std::mutex> _(mtx);

          users.insert(&conn);
          std::string var = dsp.GetLastActions();
          CROW_LOG_INFO <<"from dsp:"<<var.c_str();

          conn.send_text(var);
          }
      })
      .onclose([&](crow::websocket::connection& conn, const std::string& reason) {
          CROW_LOG_INFO << "websocket connection closed: " << reason;
          std::lock_guard<std::mutex> _(mtx);
          users.erase(&conn);
      })
      .onmessage([&](crow::websocket::connection& /*conn*/, const std::string& data, bool is_binary) {
          std::lock_guard<std::mutex> _(mtx);
          for (auto u : users)
              if (is_binary)
                  u->send_binary(data);
              else
                  u->send_text(data);
      });

    CROW_ROUTE(app, "/")
    ([] {
        char name[256];
        gethostname(name, 256);
        crow::mustache::context x;
        x["servername"] = name;
        std::cout<<"read some template"<<std::endl;
        auto page = crow::mustache::load("ws.html");
        return page.render(x);
    });

    std::thread *th_user_handler;
    th_user_handler = new std::thread(&user_handler, &dsp, &stop);
    app.port(40080)
      .multithreaded()
      .run();



	std::cout<<"stop dispatcher obj"<<std::endl;
    stop = true;
	dsp.die();
    th_user_handler->join();
    delete th_user_handler;
    return 0;
}


