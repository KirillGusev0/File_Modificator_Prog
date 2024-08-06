#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTimer>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QDebug>
#include <iostream>
#include <fstream>

// Глобальные переменные для хранения настроек
QString inputMask;
QString outputPath;
bool deleteInputFiles;
bool overwriteFiles;
int pollInterval;
quint64 xorValue;
bool singleRun;

// Функция для обработки файлов
void processFiles() {
    QDir directory;
    // Поиск файлов, соответствующих маске
    QStringList files = directory.entryList(QStringList() << inputMask, QDir::Files);

    if (files.isEmpty()) {
        qDebug() << "Нет файлов, соответствующих маске:" << inputMask;
        return;
    }

    for (const QString &fileName : files) {
        QFile inputFile(fileName);
        
        // Проверка, можно ли открыть файл для чтения
        if (!inputFile.open(QIODevice::ReadOnly)) {
            qDebug() << "Не удалось открыть файл для чтения:" << fileName << ", ошибка:" << inputFile.errorString();
            continue;
        }

        // Чтение содержимого файла
        QByteArray fileData = inputFile.readAll();
        inputFile.close();

        // Выполнение XOR операции
        for (int i = 0; i < fileData.size(); ++i) {
            fileData[i] = fileData[i] ^ ((char *)&xorValue)[i % 8];
        }

        // Формирование имени выходного файла
        QString outputFileName = outputPath + "/" + fileName;

        // Проверка, нужно ли перезаписывать выходной файл
        if (QFile::exists(outputFileName) && !overwriteFiles) {
            int counter = 1;
            while (QFile::exists(outputFileName)) {
                outputFileName = outputPath + "/" + fileName + "." + QString::number(counter);
                counter++;
            }
        }

        // Открытие выходного файла для записи
        QFile outputFile(outputFileName);
        if (!outputFile.open(QIODevice::WriteOnly)) {
            qDebug() << "Не удалось открыть файл для записи:" << outputFileName << ", ошибка:" << outputFile.errorString();
            continue;
        }

        // Запись измененного содержимого в выходной файл
        outputFile.write(fileData);
        outputFile.close();

        // Удаление входного файла, если это указано в настройках
        if (deleteInputFiles) {
            inputFile.remove();
        }
    }

    // Если разовый запуск, завершаем приложение после обработки файлов
    if (singleRun) {
        QCoreApplication::quit();
    }
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    // Настройка парсера командной строки
    QCommandLineParser parser;
    parser.setApplicationDescription("File Modifier");
    parser.addHelpOption();

    // Определение опций командной строки
    QCommandLineOption inputMaskOption(QStringList() << "m" << "mask", "Маска входных файлов", "mask");
    parser.addOption(inputMaskOption);

    QCommandLineOption outputPathOption(QStringList() << "o" << "output", "Путь для сохранения результирующих файлов", "output");
    parser.addOption(outputPathOption);

    QCommandLineOption deleteOption(QStringList() << "d" << "delete", "Удалять входные файлы");
    parser.addOption(deleteOption);

    QCommandLineOption overwriteOption(QStringList() << "w" << "overwrite", "Перезаписывать выходные файлы");
    parser.addOption(overwriteOption);

    QCommandLineOption intervalOption(QStringList() << "i" << "interval", "Периодичность опроса наличия входного файла в мс", "interval", "1000");
    parser.addOption(intervalOption);

    QCommandLineOption xorValueOption(QStringList() << "x" << "xor", "8-байтовое значение для XOR операции в шестнадцатеричном формате", "xor", "0000000000000000");
    parser.addOption(xorValueOption);

    // Опция командной строки для разового запуска
    QCommandLineOption singleRunOption(QStringList() << "s" << "single", "Разовый запуск");
    parser.addOption(singleRunOption);

    // Парсинг аргументов командной строки
    parser.process(app);

    // Получение значений из аргументов командной строки
    inputMask = parser.value(inputMaskOption);
    outputPath = parser.value(outputPathOption);
    deleteInputFiles = parser.isSet(deleteOption);
    overwriteFiles = parser.isSet(overwriteOption);
    pollInterval = parser.value(intervalOption).toInt();
    xorValue = parser.value(xorValueOption).toULongLong(nullptr, 16);
    singleRun = parser.isSet(singleRunOption); // Получение значения для разового запуска

    // Проверка обязательных аргументов
    if (inputMask.isEmpty() || outputPath.isEmpty()) {
        qDebug() << "Маска входных файлов и путь для сохранения результирующих файлов должны быть заданы.";
        return 1;
    }

    // Создание выходной директории, если не существует
    QDir dir;
    if (!dir.exists(outputPath)) {
        if (!dir.mkpath(outputPath)) {
            qDebug() << "Не удалось создать выходную директорию:" << outputPath;
            return 1;
        }
    }

    // Если разовый запуск, выполняем обработку файлов один раз
    if (singleRun) {
        processFiles();
        return 0;
    } else {
        // Настройка таймера для периодической проверки наличия файлов
        QTimer timer;
        QObject::connect(&timer, &QTimer::timeout, processFiles);
        timer.start(pollInterval);

        return app.exec();
    }
}
