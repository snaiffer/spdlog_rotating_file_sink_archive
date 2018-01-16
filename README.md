# info
Расширение для класса spdlog::sinks::rotating_file_sink_mt из библиотеки https://github.com/gabime/spdlog .

Пишет логи с заданным размером и кол-вом. Если кол-во логов выходит за установленное значение, то не удаляет старые логи, а архивирует их. Удаление архивов происходит в соответствие с установленым сроком хранения.

# Rotate files
 * log.txt -> log.1.txt
 * log.1.txt -> log.2.txt
 * log.2.txt -> log.3.txt
 * log.3.txt -> log.txt_<last_file_modification_date_and_time>.zlib
 *  if (last_file_modification_date) > (CURRENT_TIME - interval) -> remove

Note_1: Удаление старых архивов происходит каждый раз после запуска архивирования, но не чаще одного раза в сутки.

Note_2: Архивирование и удаление старых архивов происходит в отдельном потоке, но не предполагается, что задание на создание архива будет появляться быстрее, чем закончится формирование предыдущего архива. Если такое произойдет, то логирование заблокируется, пока неосвободится поток.

# Example
Create a file rotating logger with 5mb size max and 3 rotated files and store archived logs during 10 days.

 Note: "day" interval is by default, but you can change it with help of "archive_store_interval" arg.

```C++
auto rotating_logger = spdlog::rotating_logger_mt("some_logger_name", "logs/mylogfile.txt", 100, 3, 10);
```
