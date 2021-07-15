#include "zmq_io.hpp"


zmq_io::~zmq_io(){}

zmq_io::zmq_io(std::string _devName, std::string _unique_number)
{
    unique_number = _unique_number;
    devName = _devName;
    io_socket.connect("tcp://localhost:5555");
}
zmq::send_result_t zmq_io::send(std::string snd_data)
{
    zmq::send_result_t res = io_socket.send(zmq::buffer(snd_data), zmq::send_flags::dontwait);
}