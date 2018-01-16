#include "dirfileutils.h"

#include <regex>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

using namespace std;

string getFileLastModTime(const string &filename, const string format)
{
    struct stat result;
    if(stat(filename.c_str(), &result) != 0)
        throw runtime_error("Can't get info about " + filename + ": " + strerror(errno));

    char datetime[14] = {0};
    const struct tm* time = localtime(&result.st_mtime);
    if ( strftime(datetime, 100, format.c_str(), time) <= 0 )
        throw runtime_error("Can't convert last modified time of file into string");

    return datetime;
}

string getPath2DirOfFile(const string path2file, string &file_name)
{
    regex rgx("(.*/)(.*)");
    smatch match;
    if (regex_search(path2file.begin(), path2file.end(), match, rgx))
    {
        file_name = match[2];
        return match[1];
    }

    file_name = path2file;
    return "./";
}

vector<string> getFilesOfDir(const string &dir)
{
    vector<string> files = {};

    DIR *dp;
    struct dirent *dirp;
    if((dp  = opendir(dir.c_str())) == NULL)
        throw runtime_error(string("Error opening the dir: ") + dir + " " + strerror(errno));

    while ((dirp = readdir(dp)) != NULL)
    {
        string cur_file = dirp->d_name;
        if (cur_file != "." && cur_file != "..")
            files.push_back(cur_file);
    }
    closedir(dp);

    return files;
}
