#ifndef ZLIB_WRAPPER_H
#define ZLIB_WRAPPER_H

#include <string>
#include <exception>

class ZlibWrapperException : public std::exception {
    std::string msg = "Zlib: ";
public:
    ZlibWrapperException() { }
    ZlibWrapperException(const char *msg_) { msg.append(msg_); }
    ZlibWrapperException(std::string msg_) { msg.append(msg_); }

    ZlibWrapperException(int ret);

    virtual const char* what() const throw() { return msg.c_str(); }
};

/*
 * level:
Z_NO_COMPRESSION         0
Z_BEST_SPEED             1
Z_BEST_COMPRESSION       9
Z_DEFAULT_COMPRESSION  (-1)
*/
void compress(std::string src, std::string dest, int level = -1);

void uncompress(std::string src, std::string dest);

#endif // ZLIB_WRAPPER_H
