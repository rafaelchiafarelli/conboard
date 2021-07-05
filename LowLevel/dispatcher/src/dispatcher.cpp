#include "dispatcher.hpp"
/**
 * Dispatcher works as a buffer or holder for all the communication between the low level
 * and the high level.
 * That means that the lower level sends it's data via zmq_io and the dispatcher sends it 
 * to the user backend if it is connected, if it is not connected, it will not send to any
 * one.
 * On the other hand, if the dispatcher receives a command from the backend from the user,
 * than it will send it to the low level if it is present. 
 * In other terms, the dispatcher ensures that the LowLevel is connected as well as the user
 * Since most communications happen assynchronously, this have to ensure that communications 
 * don misfire or get forgothen.
 * For the vault the ideia is simple. Ensure access to the user, if it is "allowed", and 
 * protect the data in case of access.
 * A data is transfered to the vault only from sharing space and users can tell the system 
 * to protect any given data from the sharing space.
 * But data can only be transfered out of the vault by authentication and decription from 
 * https connection. That means that the size of the vault is unknown, and download can 
 * happen from the internet, only if authorized.
 * It is a known fact that the size of the SDCard is the limiting factor.
 * 
 * The sharing of data is a litle diferent. Data can be only moved around from the USB 
 * connection and have a limited space:
 * - 8mb if the SDCard is smaller than 8G
 * - 25% of the sdcard if the SDCard is 8G
 * - 50% of the sdcard if the SDCard is 16G
 * - 16G for all the other sizes
 * no criptography or protection is allowed/needed
 * 
 */ 

dispatcher::dispatcher():disp("/conboard/dispatcher/assets/config.json"){
    fileName = "/conboard/dispatcher/assets/config.json";
}

dispatcher::dispatcher(std::string fileName, std::atomic_bool *stop):disp(fileName){
    fileName = fileName;
}

/**
 * zmq_coms
 * has the same protocol as the lower levels, however 
 */ 