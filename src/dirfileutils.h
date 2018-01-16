#ifndef DIRFILEUTILS_H
#define DIRFILEUTILS_H

#include <string>
#include <vector>

#include <iostream>

std::string getFileLastModTime(const std::string &filename, const std::string format = "%Y%m%d_%H%M%S");

/* Example:
 * In: logs/test Out: logs/
 * In: file1.txt Out: ./
 */
std::string getPath2DirOfFile(const std::string path2file, std::string &file_name);

std::vector<std::string> getFilesOfDir(const std::string &dir);

#endif // DIRFILEUTILS_H
