#ifndef ROTATING_FILE_SINK_ARCHIVE_H
#define ROTATING_FILE_SINK_ARCHIVE_H

#include "spdlog/spdlog.h"
#include "dirfileutils.h"

#include <string>
#include <fstream>
#include <ctime>

#define SECONDS_PER_DAY 24*60*60

void removeOldArchiveByDate(const std::string &path2archive, const unsigned int archive_store_seconds);

void compressAndRemoveOldArchiveByDate(std::string src_name, std::string archive_name, std::string path2archive, unsigned int archive_store_seconds, bool do_archive_removing);

namespace spdlog
{
namespace sinks
{

// Расширение для класса spdlog::sinks::rotating_file_sink_mt.
// Пишет логи с заданным размером и кол-вом.
// Если кол-во логов выходит за установленное значение, то не удаляет старые логи
// , а архивирует их. Удаление архивов происходит в соответствие с установленым сроком
// хранения.
//
// Rotate files:
// log.txt -> log.1.txt
// log.1.txt -> log.2.txt
// log.2.txt -> log.3.txt
// log.3.txt -> log.txt_<last_file_modification_date_and_time>.zlib
// if (last_file_modification_date) > (CURRENT_TIME - interval) -> remove
// Note_1: Удаление старых архивов происходит каждый раз после запуска архивирования, но не чаще одного раза в сутки
// Note_2: Архивирование и удаление старых архивов происходит в отдельном потоке, но не предполагается, что задание на создание архива будет появляться быстрее, чем закончится формирование предыдущего архива. Если такое произойдет, то логирование заблокируется, пока неосвободится поток.
//
// Example:
// Create a file rotating logger with 5mb size max and 3 rotated files
//  and store archived logs during 10 days.
//  Note: "day" interval is by default, but you can change it with help of "archive_store_interval" arg.
//auto rotating_logger = spdlog::rotating_logger_mt("some_logger_name", "logs/mylogfile.txt", 100, 3, 10);
template<class Mutex>
class rotating_file_sink_archive SPDLOG_FINAL : public base_sink < Mutex >
{
public:
    rotating_file_sink_archive(const filename_t &base_filename,
                       std::size_t max_size, std::size_t max_files, unsigned int archive_store_interval_count=30, unsigned int archive_store_interval = SECONDS_PER_DAY) :
        _base_filename(base_filename),
        _max_size(max_size),
        _max_files(max_files),
        _current_size(0),
        _file_helper(),
        _archive_store_seconds(archive_store_interval_count*archive_store_interval),
        _condit_unique_generator(0),
        _do_archive_removing(false),
        _last_archive_removing(0),
        _archive_store_interval(archive_store_interval)
    {
        _file_helper.open(calc_filename(_base_filename, 0));
        _current_size = _file_helper.size(); //expensive. called only once

        _path2archive = getPath2DirOfFile(_base_filename, _filename); // return _filename
        _path2archive += _filename + "_archive/";

        struct stat sb;
        if (stat(_path2archive.c_str(), &sb) != 0 || ! S_ISDIR(sb.st_mode))
            if ( mkdir(_path2archive.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0 )
                throw spdlog_ex(std::string("Can't create 'archive' folder in log directory: ") + strerror(errno));
    }

    ~rotating_file_sink_archive()
    {
        if (working_with_archives_thread.joinable())
            working_with_archives_thread.join();
    }

    // calc filename according to index and file extension if exists.
    // e.g. calc_filename("logs/mylog.txt, 3) => "logs/mylog.3.txt".
    static filename_t calc_filename(const filename_t& filename, std::size_t index)
    {
        std::conditional<std::is_same<filename_t::value_type, char>::value, fmt::MemoryWriter, fmt::WMemoryWriter>::type w;
        if (index)
        {
            filename_t basename, ext;
            std::tie(basename, ext) = details::file_helper::split_by_extenstion(filename);
            w.write(SPDLOG_FILENAME_T("{}.{}{}"), basename, index, ext);
        }
        else
        {
            w.write(SPDLOG_FILENAME_T("{}"), filename);
        }
        return w.str();
    }

protected:
    void _sink_it(const details::log_msg& msg) override
    {
        _current_size += msg.formatted.size();
        if (_current_size > _max_size)
        {
            _rotate();
            _current_size = msg.formatted.size();
        }
        _file_helper.write(msg);
    }

    void _flush() override
    {
        _file_helper.flush();
    }

private:
    void _rotate()
    {
        using details::os::filename_to_str;
        _file_helper.close();
        for (auto i = _max_files; i > 0; --i)
        {
            filename_t src = calc_filename(_base_filename, i - 1);
            filename_t target = calc_filename(_base_filename, i);

            if (details::file_helper::file_exists(target))
            {
                // Перед архивированием лога, даем ему временное имя для того, чтобы можно было отдать его отдельному потоку на сжатие и не мешать параллельно происходящей ротации
                std::string target_tmp = target + "_for_archivating_" + std::to_string(++_condit_unique_generator);
                if (details::os::rename(target, target_tmp))
                {
                    _file_helper.reopen(true);
                    throw spdlog_ex("rotating_file_sink_archive: failed renaming " + filename_to_str(target) + " to " + filename_to_str(target_tmp), errno);
                }

                if (working_with_archives_thread.joinable())
                    working_with_archives_thread.join();

                std::string archive_filename = _path2archive + _filename + "_" + getFileLastModTime(target_tmp) + ".zlib";
                // Для очень маловероятного случая, когда архивы будут генерироваться чаще раза в секунду
                if (details::file_helper::file_exists(archive_filename))
                    archive_filename = _path2archive + _filename + "_" + getFileLastModTime(target_tmp) + "_" + std::to_string(++_condit_unique_generator) + ".zlib";

                time_t now = time(NULL);
                if (difftime(now,_last_archive_removing) > _archive_store_interval)
                {
                    _last_archive_removing = std::move(now);
                    _do_archive_removing = true;
                }
                else
                    _do_archive_removing = false;

                working_with_archives_thread = std::thread(compressAndRemoveOldArchiveByDate, std::move(target_tmp), std::move(archive_filename), _path2archive, _archive_store_seconds, _do_archive_removing);
            }
            if (details::file_helper::file_exists(src) && details::os::rename(src, target))
            {
                _file_helper.reopen(true);
                throw spdlog_ex("rotating_file_sink_archive: failed renaming " + filename_to_str(src) + " to " + filename_to_str(target), errno);
            }
        }
        _file_helper.reopen(true);
    }
    filename_t _base_filename;
    std::size_t _max_size;
    std::size_t _max_files;
    std::size_t _current_size;
    details::file_helper _file_helper;
    unsigned int _archive_store_seconds;
    std::string _path2archive;
    filename_t _filename;
    unsigned char _condit_unique_generator;
    bool _do_archive_removing;
    time_t _last_archive_removing;
    unsigned int _archive_store_interval;
    std::thread working_with_archives_thread;
};

typedef rotating_file_sink_archive<std::mutex> rotating_file_sink_archive_mt;
typedef rotating_file_sink_archive<details::null_mutex>rotating_file_sink_archive_st;
}
}

#endif // ROTATING_FILE_SINK_ARCHIVE_H
