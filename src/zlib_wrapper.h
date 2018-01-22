#ifndef ZLIB_WRAPPER_H
#define ZLIB_WRAPPER_H

#include <string>
#include <exception>

class ZlibWrapperException : public std::exception
{
public:
    ZlibWrapperException() { }
    ZlibWrapperException(const char *msg_) { msg.append(msg_); }
    ZlibWrapperException(std::string msg_) { msg.append(msg_); }

    ZlibWrapperException(int ret);

    virtual const char* what() const throw() { return msg.c_str(); }

    virtual ~ZlibWrapperException() throw() {}

protected:
    std::string msg = "Zlib: ";
};

/*
 * level:
Z_NO_COMPRESSION         0
Z_BEST_SPEED             1
Z_BEST_COMPRESSION       9
Z_DEFAULT_COMPRESSION  (-1)
*/
void compress(const std::string &src, const std::string &dest, int level = -1);

void uncompress(const std::string &src, const std::string &dest);

#endif // ZLIB_WRAPPER_H
