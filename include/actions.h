#ifndef ACTIONS_H
#define ACTIONS_H
#include "string"
#include <vector>
typedef enum{
    midi,
    keyboard,
    mouse
}devType;



typedef struct{
    devType tp; /* type of the output, either feyboard or mouse ou a midi response*/

    //keyboard
    std::string data; /* data to be sent to the output*/

    //mouse
    long dx = 0;
    long gotox = 0;
    long dy = 0;
    long gotoy = 0;
    unsigned int wheel_move = 0;
    bool click = 0;
    bool right_click = 0;

    //midi
    unsigned char b0 = 0;
	unsigned char b1 = 0;
	unsigned char b2 = 0;

    //delay
    unsigned int delay; /* delay in microsseconds to wait after data was sent */
}devActions;



class Actions{

public:
    devActions in;
    std::vector<devActions> out;
    Actions(){};
    ~Actions(){};
};

#endif
