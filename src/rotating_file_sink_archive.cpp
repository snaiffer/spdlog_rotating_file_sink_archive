#include "rotating_file_sink_archive.h"
#include "zlib_wrapper.h"

#include <cstdio>
#include <cerrno>
#include <sys/types.h>
#include <sys/stat.h>
#include <regex>
#include <thread>

using namespace std;

void removeOldArchiveByDate(const std::string &path2archive, const unsigned int archive_store_seconds)
{
    vector<string> files = getFilesOfDir(path2archive);
    time_t now = time(NULL);
    for (unsigned int i = 0; i < files.size(); ++i)
    {
        const std::string& cur_file = files[i];

        std::regex rgx(R"(.*\.zlib$)");
        std::smatch match;
        if (std::regex_search(cur_file.begin(), cur_file.end(), match, rgx))
        {
            std::string path2file = path2archive + cur_file;
            struct stat cur_file_stat;
            if(stat(path2file.c_str(), &cur_file_stat) != 0)
                throw runtime_error(std::string(path2file) + " " + strerror(errno));

            time_t mod_time = cur_file_stat.st_mtime;
            if (difftime(now,mod_time) > archive_store_seconds)
               std::remove(path2file.c_str());
        }
    }
}

void compressAndRemoveOldArchiveByDate(std::string src_name, std::string archive_name, std::string path2archive, unsigned int archive_store_seconds, bool do_archive_removing)
{
    compress(src_name, archive_name);
    std::remove(src_name.c_str());

    if (do_archive_removing)
        removeOldArchiveByDate(path2archive, archive_store_seconds);
}


