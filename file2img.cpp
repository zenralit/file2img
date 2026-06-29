#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

using namespace std;
struct ImageSize
{
    size_t width = 0;
    size_t height = 0;
    size_t pixelCount = 0;
};

struct ProgramOptions
{
    string inputPath;
    string outputPath;
    string mode = "rgb";
    string xorKeyText;
    unsigned char xorKey = 0;
    bool interactive = false;
    bool decode = false;
};

void printUsage()
{
    cout << "Usage: file2ppm.exe [--decode] [input_file output.ppm [rgb|gray|xor] [xor_key]]" << endl;
    cout << "Если аргументы не переданы, программа запросит путь к файлу и параметры в консоли." << endl;
    cout << "Поддерживаются любые файлы: исполняемые, текстовые, архивы, изображения и т.д." << endl;
    cout << "Поддерживаемые выходные форматы: .ppm, .bmp и .png" << endl;
    cout << "Для восстановления файла из изображения используйте --decode или задайте вход .ppm/.bmp/.png и выход без расширения изображения." << endl;
    cout << "Default mode: rgb" << endl;
    cout << "Example: file2ppm.exe file.exe out.png xor 55" << endl;
    cout << "Example: file2ppm.exe --decode image.png restored.bin" << endl;
}

string readConsoleLine(const string& prompt, const string& defaultValue)
{
    cout << prompt;
    if (!defaultValue.empty())
    {
        cout << " [" << defaultValue << "]";
    }
    cout << ": ";

    string value;
    getline(cin, value);
    if (value.empty() && !defaultValue.empty())
    {
        return defaultValue;
    }
    return value;
}

bool isImageExtension(const string& path);

string readModeFromConsole(string& xorKeyText)
{
    while (true)
    {
        cout << "Выберите режим кодирования:" << endl;
        cout << "1) rgb" << endl;
        cout << "2) gray" << endl;
        cout << "3) xor" << endl;

        const string choice = readConsoleLine("Введите номер", "1");
        if (choice == "1" || choice == "rgb")
        {
            return "rgb";
        }
        if (choice == "2" || choice == "gray")
        {
            return "gray";
        }
        if (choice == "3" || choice == "xor")
        {
            xorKeyText = readConsoleLine("Введите XOR-ключ", "0");
            return "xor";
        }
        cout << "Неверный выбор. Повторите." << endl;
    }
}

bool shouldDecode(const ProgramOptions& options)
{
    if (options.decode)
    {
        return true;
    }

    if (options.inputPath.empty())
    {
        return false;
    }

    if (!isImageExtension(options.inputPath))
    {
        return false;
    }

    if (options.outputPath.empty())
    {
        return true;
    }

    return !isImageExtension(options.outputPath);
}

bool readFile(const string& inputPath, vector<unsigned char>& data, uint64_t& fileSize)
{
    ifstream input(inputPath, ios::binary | ios::ate);
    if (!input)
    {
        cerr << "Ошибка: не удалось открыть входной файл '" << inputPath << "'." << endl;
        return false;
    }

    const streampos position = input.tellg();
    if (position < 0)
    {
        cerr << "Ошибка: не удалось определить размер файла '" << inputPath << "'." << endl;
        return false;
    }

    fileSize = static_cast<uint64_t>(position);
    data.clear();
    data.resize(static_cast<size_t>(fileSize));

    input.seekg(0, ios::beg);
    if (fileSize > 0)
    {
        input.read(reinterpret_cast<char*>(data.data()), static_cast<streamsize>(fileSize));
        if (!input)
        {
            cerr << "Ошибка: не удалось прочитать файл '" << inputPath << "'." << endl;
            return false;
        }
    }

    return true;
}

ImageSize calculateImageSize(uint64_t fileSize)
{
    ImageSize result;

    const size_t pixelCount = static_cast<size_t>((fileSize + 2) / 3);
    result.pixelCount = pixelCount;

    if (pixelCount == 0)
    {
        result.width = 1;
        result.height = 1;
        return result;
    }

    result.width = static_cast<size_t>(ceil(sqrt(static_cast<double>(pixelCount))));
    if (result.width < 1)
    {
        result.width = 1;
    }

    result.height = (pixelCount + result.width - 1) / result.width;
    if (result.height < 1)
    {
        result.height = 1;
    }

    return result;
}

void convertRGB(const vector<unsigned char>& inputData,
    vector<unsigned char>& imageData,
    size_t pixelCount)
{
    for (size_t pixelIndex = 0; pixelIndex < pixelCount; ++pixelIndex)
    {
        const size_t outputOffset = pixelIndex * 3;
        for (int component = 0; component < 3; ++component)
        {
            const size_t inputIndex = static_cast<size_t>(pixelIndex * 3 + component);
            if (inputIndex < inputData.size())
            {
                imageData[outputOffset + component] = inputData[inputIndex];
            }
            else
            {
                imageData[outputOffset + component] = 0;
            }
        }
    }
}

void convertGray(const vector<unsigned char>& inputData,
    vector<unsigned char>& imageData,
    size_t pixelCount)
{
    for (size_t pixelIndex = 0; pixelIndex < pixelCount; ++pixelIndex)
    {
        const size_t outputOffset = pixelIndex * 3;
        const unsigned char value = (pixelIndex < inputData.size()) ? inputData[pixelIndex] : 0;
        imageData[outputOffset] = value;
        imageData[outputOffset + 1] = value;
        imageData[outputOffset + 2] = value;
    }
}

void convertXor(const vector<unsigned char>& inputData,
    vector<unsigned char>& imageData,
    size_t pixelCount,
    unsigned char xorKey)
{
    for (size_t pixelIndex = 0; pixelIndex < pixelCount; ++pixelIndex)
    {
        const size_t outputOffset = pixelIndex * 3;
        for (int component = 0; component < 3; ++component)
        {
            const size_t inputIndex = static_cast<size_t>(pixelIndex * 3 + component);
            if (inputIndex < inputData.size())
            {
                const unsigned char value = static_cast<unsigned char>(inputData[inputIndex] ^ xorKey);
                imageData[outputOffset + component] = value;
            }
            else
            {
                imageData[outputOffset + component] = 0;
            }
        }
    }
}

bool writeLittleEndian(ofstream& output, uint32_t value)
{
    output.put(static_cast<char>(value & 0xFF));
    output.put(static_cast<char>((value >> 8) & 0xFF));
    output.put(static_cast<char>((value >> 16) & 0xFF));
    output.put(static_cast<char>((value >> 24) & 0xFF));
    return output.good();
}

bool writeLittleEndian64(ofstream& output, uint64_t value)
{
    for (int shift = 0; shift < 64; shift += 8)
    {
        output.put(static_cast<char>((value >> shift) & 0xFF));
    }
    return output.good();
}

bool writeBigEndian(ofstream& output, uint32_t value)
{
    output.put(static_cast<char>((value >> 24) & 0xFF));
    output.put(static_cast<char>((value >> 16) & 0xFF));
    output.put(static_cast<char>((value >> 8) & 0xFF));
    output.put(static_cast<char>(value & 0xFF));
    return output.good();
}

bool writeLittleEndian16(ofstream& output, uint16_t value)
{
    output.put(static_cast<char>(value & 0xFF));
    output.put(static_cast<char>((value >> 8) & 0xFF));
    return output.good();
}

void appendBigEndian(vector<unsigned char>& data, uint32_t value)
{
    data.push_back(static_cast<unsigned char>((value >> 24) & 0xFF));
    data.push_back(static_cast<unsigned char>((value >> 16) & 0xFF));
    data.push_back(static_cast<unsigned char>((value >> 8) & 0xFF));
    data.push_back(static_cast<unsigned char>(value & 0xFF));
}

void appendLittleEndian64(vector<unsigned char>& data, uint64_t value)
{
    for (int shift = 0; shift < 64; shift += 8)
    {
        data.push_back(static_cast<unsigned char>((value >> shift) & 0xFF));
    }
}

uint32_t calculateCRC32(const vector<unsigned char>& data)
{
    uint32_t crc = 0xFFFFFFFFu;
    for (unsigned char byte : data)
    {
        crc ^= static_cast<uint32_t>(byte);
        for (int bit = 0; bit < 8; ++bit)
        {
            if ((crc & 1u) != 0u)
            {
                crc = (crc >> 1) ^ 0xEDB88320u;
            }
            else
            {
                crc >>= 1;
            }
        }
    }
    return crc ^ 0xFFFFFFFFu;
}

uint32_t calculateAdler32(const vector<unsigned char>& data)
{
    uint32_t a = 1;
    uint32_t b = 0;
    for (unsigned char byte : data)
    {
        a = (a + byte) % 65521u;
        b = (b + a) % 65521u;
    }
    return (b << 16) | a;
}

vector<unsigned char> buildStoredDeflateData(const vector<unsigned char>& uncompressedData)
{
    vector<unsigned char> result;
    result.reserve(uncompressedData.size() + 64);

    auto writeBits = [&](uint32_t value, int bitCount, unsigned char& currentByte, int& bitPosition) {
        for (int bitIndex = 0; bitIndex < bitCount; ++bitIndex)
        {
            const unsigned int bit = (value >> bitIndex) & 1u;
            currentByte |= static_cast<unsigned char>(bit << bitPosition);
            ++bitPosition;
            if (bitPosition == 8)
            {
                result.push_back(currentByte);
                currentByte = 0;
                bitPosition = 0;
            }
        }
        };

    const size_t maxBlockSize = 65535u;
    for (size_t offset = 0; offset < uncompressedData.size();)
    {
        const size_t blockSize = (offset + maxBlockSize < uncompressedData.size()) ? maxBlockSize : (uncompressedData.size() - offset);
        const bool isFinalBlock = (offset + blockSize >= uncompressedData.size());

        unsigned char currentByte = 0;
        int bitPosition = 0;
        writeBits(isFinalBlock ? 1u : 0u, 1, currentByte, bitPosition);
        writeBits(0u, 2, currentByte, bitPosition);
        writeBits(static_cast<uint32_t>(blockSize & 0xFFFFu), 16, currentByte, bitPosition);
        writeBits(static_cast<uint32_t>((~blockSize) & 0xFFFFu), 16, currentByte, bitPosition);

        for (size_t index = 0; index < blockSize; ++index)
        {
            writeBits(uncompressedData[offset + index], 8, currentByte, bitPosition);
        }

        if (bitPosition != 0)
        {
            result.push_back(currentByte);
        }

        offset += blockSize;
    }

    return result;
}

vector<unsigned char> buildZlibCompressedData(const vector<unsigned char>& uncompressedData)
{
    const vector<unsigned char> deflateData = buildStoredDeflateData(uncompressedData);
    vector<unsigned char> result;
    result.reserve(2 + deflateData.size() + 4);
    result.push_back(0x78);
    result.push_back(0x01);
    result.insert(result.end(), deflateData.begin(), deflateData.end());

    const uint32_t adler = calculateAdler32(uncompressedData);
    result.push_back(static_cast<unsigned char>((adler >> 24) & 0xFF));
    result.push_back(static_cast<unsigned char>((adler >> 16) & 0xFF));
    result.push_back(static_cast<unsigned char>((adler >> 8) & 0xFF));
    result.push_back(static_cast<unsigned char>(adler & 0xFF));
    return result;
}

bool writePNGChunk(ofstream& output, const string& type, const vector<unsigned char>& data)
{
    if (!writeBigEndian(output, static_cast<uint32_t>(data.size())) || !output)
    {
        return false;
    }

    output.write(type.c_str(), 4);
    if (!output)
    {
        return false;
    }

    if (!data.empty())
    {
        output.write(reinterpret_cast<const char*>(data.data()), static_cast<streamsize>(data.size()));
    }
    if (!output)
    {
        return false;
    }

    vector<unsigned char> crcInput(type.begin(), type.end());
    crcInput.insert(crcInput.end(), data.begin(), data.end());
    const uint32_t crc = calculateCRC32(crcInput);
    return writeBigEndian(output, crc);
}

bool savePPM(const string& outputPath,
    const vector<unsigned char>& imageData,
    size_t width,
    size_t height,
    uint64_t originalSize,
    const vector<unsigned char>& originalData)
{
    ofstream output(outputPath, ios::binary);
    if (!output)
    {
        cerr << "Ошибка: не удалось создать выходной файл '" << outputPath << "'." << endl;
        return false;
    }

    output << "P6\n";
    output << "# Generated by file2ppm\n";
    output << "# OriginalSize=" << originalSize << "\n";
    output << width << " " << height << "\n";
    output << "255\n";

    if (!imageData.empty())
    {
        output.write(reinterpret_cast<const char*>(imageData.data()), static_cast<streamsize>(imageData.size()));
    }

    output.put('\n');
    output.write("F2PM", 4);
    if (!writeLittleEndian64(output, originalSize))
    {
        return false;
    }
    if (!originalData.empty())
    {
        output.write(reinterpret_cast<const char*>(originalData.data()), static_cast<streamsize>(originalData.size()));
    }

    if (!output)
    {
        cerr << "Ошибка: не удалось записать данные PPM в '" << outputPath << "'." << endl;
        return false;
    }

    return true;
}

bool saveBMP(const string& outputPath,
    const vector<unsigned char>& imageData,
    size_t width,
    size_t height,
    uint64_t originalSize,
    const vector<unsigned char>& originalData)
{
    ofstream output(outputPath, ios::binary);
    if (!output)
    {
        cerr << "Ошибка: не удалось создать выходной файл '" << outputPath << "'." << endl;
        return false;
    }

    if (width > 0x7FFFFFFF || height > 0x7FFFFFFF)
    {
        cerr << "Ошибка: слишком большой размер изображения для BMP." << endl;
        return false;
    }

    const uint32_t rowSize = static_cast<uint32_t>(((width * 3) + 3) & ~3u);
    const uint32_t pixelDataSize = rowSize * static_cast<uint32_t>(height);
    const uint32_t fileSize = 54u + pixelDataSize;

    output.put('B');
    output.put('M');
    if (!writeLittleEndian(output, fileSize))
    {
        return false;
    }
    if (!writeLittleEndian(output, 0u))
    {
        return false;
    }
    if (!writeLittleEndian(output, 54u))
    {
        return false;
    }
    if (!writeLittleEndian(output, 40u))
    {
        return false;
    }
    if (!writeLittleEndian(output, static_cast<uint32_t>(width)))
    {
        return false;
    }
    if (!writeLittleEndian(output, static_cast<uint32_t>(height)))
    {
        return false;
    }
    if (!writeLittleEndian16(output, 1u))
    {
        return false;
    }
    if (!writeLittleEndian16(output, 24u))
    {
        return false;
    }
    if (!writeLittleEndian(output, 0u))
    {
        return false;
    }
    if (!writeLittleEndian(output, pixelDataSize))
    {
        return false;
    }
    if (!writeLittleEndian(output, 2835u))
    {
        return false;
    }
    if (!writeLittleEndian(output, 2835u))
    {
        return false;
    }
    if (!writeLittleEndian(output, 0u))
    {
        return false;
    }
    if (!writeLittleEndian(output, 0u))
    {
        return false;
    }

    for (size_t y = height; y > 0; --y)
    {
        const size_t rowIndex = (y - 1) * width;
        for (size_t x = 0; x < width; ++x)
        {
            const size_t pixelIndex = (rowIndex + x) * 3;
            if (pixelIndex + 2 < imageData.size())
            {
                output.put(static_cast<char>(imageData[pixelIndex + 2]));
                output.put(static_cast<char>(imageData[pixelIndex + 1]));
                output.put(static_cast<char>(imageData[pixelIndex]));
            }
            else
            {
                output.put('\0');
                output.put('\0');
                output.put('\0');
            }
        }

        const size_t padding = rowSize - width * 3;
        for (size_t i = 0; i < padding; ++i)
        {
            output.put('\0');
        }
    }

    output.write("F2PM", 4);
    if (!writeLittleEndian64(output, originalSize))
    {
        return false;
    }
    if (!originalData.empty())
    {
        output.write(reinterpret_cast<const char*>(originalData.data()), static_cast<streamsize>(originalData.size()));
    }

    if (!output)
    {
        cerr << "Ошибка: не удалось записать данные BMP в '" << outputPath << "'." << endl;
        return false;
    }

    return true;
}

bool savePNG(const string& outputPath,
    const vector<unsigned char>& imageData,
    size_t width,
    size_t height,
    uint64_t originalSize,
    const vector<unsigned char>& originalData)
{
    ofstream output(outputPath, ios::binary);
    if (!output)
    {
        cerr << "Ошибка: не удалось создать выходной файл '" << outputPath << "'." << endl;
        return false;
    }

    const unsigned char signature[8] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };
    output.write(reinterpret_cast<const char*>(signature), 8);
    if (!output)
    {
        cerr << "Ошибка: не удалось записать сигнатуру PNG в '" << outputPath << "'." << endl;
        return false;
    }

    vector<unsigned char> ihdrData;
    appendBigEndian(ihdrData, static_cast<uint32_t>(width));
    appendBigEndian(ihdrData, static_cast<uint32_t>(height));
    ihdrData.push_back(8);
    ihdrData.push_back(2);
    ihdrData.push_back(0);
    ihdrData.push_back(0);
    ihdrData.push_back(0);
    if (!writePNGChunk(output, "IHDR", ihdrData))
    {
        cerr << "Ошибка: не удалось записать заголовок PNG в '" << outputPath << "'." << endl;
        return false;
    }

    vector<unsigned char> rawImageData;
    rawImageData.reserve(height * (width * 3 + 1));
    for (size_t row = 0; row < height; ++row)
    {
        rawImageData.push_back(0);
        const size_t rowOffset = row * width * 3;
        for (size_t column = 0; column < width; ++column)
        {
            const size_t pixelOffset = rowOffset + column * 3;
            rawImageData.push_back((pixelOffset + 2 < imageData.size()) ? imageData[pixelOffset] : 0);
            rawImageData.push_back((pixelOffset + 2 < imageData.size()) ? imageData[pixelOffset + 1] : 0);
            rawImageData.push_back((pixelOffset + 2 < imageData.size()) ? imageData[pixelOffset + 2] : 0);
        }
    }

    const vector<unsigned char> zlibData = buildZlibCompressedData(rawImageData);
    if (!writePNGChunk(output, "IDAT", zlibData))
    {
        cerr << "Ошибка: не удалось записать данные PNG в '" << outputPath << "'." << endl;
        return false;
    }

    vector<unsigned char> metadataData;
    appendLittleEndian64(metadataData, originalSize);
    metadataData.insert(metadataData.end(), originalData.begin(), originalData.end());
    if (!writePNGChunk(output, "f2pm", metadataData))
    {
        cerr << "Ошибка: не удалось записать метаданные PNG в '" << outputPath << "'." << endl;
        return false;
    }

    vector<unsigned char> emptyData;
    if (!writePNGChunk(output, "IEND", emptyData))
    {
        cerr << "Ошибка: не удалось завершить PNG в '" << outputPath << "'." << endl;
        return false;
    }

    return true;
}

string getLowerFileExtension(const string& path)
{
    string lowerPath = path;
    for (char& ch : lowerPath)
    {
        ch = static_cast<char>(tolower(static_cast<unsigned char>(ch)));
    }
    return lowerPath;
}

bool readPPM(const string& inputPath, vector<unsigned char>& data, uint64_t& originalSize)
{
    ifstream input(inputPath, ios::binary);
    if (!input)
    {
        cerr << "Ошибка: не удалось открыть PPM-файл '" << inputPath << "'." << endl;
        return false;
    }

    auto readToken = [&](string& token) -> bool {
        token.clear();
        char ch = 0;
        while (input.get(ch))
        {
            if (ch == '#')
            {
                string comment;
                getline(input, comment);
                continue;
            }
            if (isspace(static_cast<unsigned char>(ch)))
            {
                continue;
            }
            token.push_back(ch);
            while (input.get(ch))
            {
                if (isspace(static_cast<unsigned char>(ch)))
                {
                    break;
                }
                token.push_back(ch);
            }
            return true;
        }
        return false;
        };

    string magic;
    if (!readToken(magic) || magic != "P6")
    {
        cerr << "Ошибка: неподдерживаемый PPM-файл '" << inputPath << "'." << endl;
        return false;
    }

    string widthText;
    string heightText;
    string maxValueText;
    if (!readToken(widthText) || !readToken(heightText) || !readToken(maxValueText))
    {
        cerr << "Ошибка: повреждён заголовок PPM-файла '" << inputPath << "'." << endl;
        return false;
    }

    size_t width = static_cast<size_t>(strtoul(widthText.c_str(), nullptr, 10));
    size_t height = static_cast<size_t>(strtoul(heightText.c_str(), nullptr, 10));
    uint32_t maxValue = static_cast<uint32_t>(strtoul(maxValueText.c_str(), nullptr, 10));
    if (maxValue != 255 || width == 0 || height == 0)
    {
        cerr << "Ошибка: неподдерживаемый PPM-файл '" << inputPath << "'." << endl;
        return false;
    }

    char separator = 0;
    if (!input.get(separator))
    {
        cerr << "Ошибка: не удалось прочитать данные PPM '" << inputPath << "'." << endl;
        return false;
    }

    const size_t pixelDataSize = width * height * 3;
    vector<unsigned char> rawPixels(pixelDataSize, 0);
    input.read(reinterpret_cast<char*>(rawPixels.data()), static_cast<streamsize>(pixelDataSize));
    const streamsize readCount = input.gcount();
    if (readCount < 0)
    {
        cerr << "Ошибка: не удалось прочитать пиксели PPM '" << inputPath << "'." << endl;
        return false;
    }

    vector<unsigned char> tail;
    tail.assign(istreambuf_iterator<char>(input), istreambuf_iterator<char>());

    if (tail.size() >= 12 && string(tail.begin(), tail.begin() + 4) == "F2PM")
    {
        uint64_t sizeValue = 0;
        for (int shift = 0; shift < 64; shift += 8)
        {
            sizeValue |= static_cast<uint64_t>(tail[4 + shift / 8]) << shift;
        }
        originalSize = sizeValue;
        const size_t payloadOffset = 12;
        if (originalSize <= static_cast<uint64_t>(tail.size() - payloadOffset))
        {
            data.assign(tail.begin() + payloadOffset, tail.begin() + payloadOffset + originalSize);
            return true;
        }
    }

    string commentLine;
    istringstream commentStream(string(tail.begin(), tail.end()));
    string line;
    while (getline(commentStream, line))
    {
        if (line.empty())
        {
            continue;
        }
        const size_t hashPos = line.find('#');
        if (hashPos != string::npos)
        {
            string comment = line.substr(hashPos + 1);
            while (!comment.empty() && isspace(static_cast<unsigned char>(comment.front())))
            {
                comment.erase(comment.begin());
            }
            if (comment.rfind("OriginalSize=", 0) == 0)
            {
                originalSize = static_cast<uint64_t>(strtoull(comment.substr(13).c_str(), nullptr, 10));
            }
        }
    }

    if (originalSize == 0 && !rawPixels.empty())
    {
        originalSize = static_cast<uint64_t>(pixelDataSize);
    }

    data.assign(rawPixels.begin(), rawPixels.begin() + (originalSize < rawPixels.size() ? originalSize : rawPixels.size()));
    return true;
}

bool readBMP(const string& inputPath, vector<unsigned char>& data, uint64_t& originalSize)
{
    ifstream input(inputPath, ios::binary | ios::ate);
    if (!input)
    {
        cerr << "Ошибка: не удалось открыть BMP-файл '" << inputPath << "'." << endl;
        return false;
    }

    const streamsize fileSize = input.tellg();
    input.seekg(0, ios::beg);
    vector<unsigned char> bytes(static_cast<size_t>(fileSize), 0);
    input.read(reinterpret_cast<char*>(bytes.data()), fileSize);
    if (!input)
    {
        cerr << "Ошибка: не удалось прочитать BMP-файл '" << inputPath << "'." << endl;
        return false;
    }

    if (bytes.size() < 54)
    {
        cerr << "Ошибка: повреждён BMP-файл '" << inputPath << "'." << endl;
        return false;
    }

    size_t pixelOffset = 54;
    const uint32_t width = static_cast<uint32_t>(bytes[18]) | (static_cast<uint32_t>(bytes[19]) << 8) |
        (static_cast<uint32_t>(bytes[20]) << 16) | (static_cast<uint32_t>(bytes[21]) << 24);
    const uint32_t height = static_cast<uint32_t>(bytes[22]) | (static_cast<uint32_t>(bytes[23]) << 8) |
        (static_cast<uint32_t>(bytes[24]) << 16) | (static_cast<uint32_t>(bytes[25]) << 24);
    const uint16_t bitCount = static_cast<uint16_t>(bytes[28]) | (static_cast<uint16_t>(bytes[29]) << 8);
    if (bitCount != 24)
    {
        cerr << "Ошибка: BMP-файл должен быть 24-bit '" << inputPath << "'." << endl;
        return false;
    }

    if (bytes.size() > 10)
    {
        const uint32_t offset = static_cast<uint32_t>(bytes[10]) | (static_cast<uint32_t>(bytes[11]) << 8) |
            (static_cast<uint32_t>(bytes[12]) << 16) | (static_cast<uint32_t>(bytes[13]) << 24);
        if (offset > 0)
        {
            pixelOffset = offset;
        }
    }

    const size_t rowSize = ((width * 3) + 3) & ~static_cast<uint32_t>(3);
    vector<unsigned char> storedPixels;
    storedPixels.reserve(static_cast<size_t>(width) * height * 3);

    for (uint32_t rowIndex = 0; rowIndex < height; ++rowIndex)
    {
        const size_t rowOffset = pixelOffset + static_cast<size_t>(rowIndex) * rowSize;
        for (uint32_t column = 0; column < width; ++column)
        {
            const size_t pixelPosition = rowOffset + static_cast<size_t>(column) * 3;
            if (pixelPosition + 2 >= bytes.size())
            {
                break;
            }
            const unsigned char blue = bytes[pixelPosition];
            const unsigned char green = bytes[pixelPosition + 1];
            const unsigned char red = bytes[pixelPosition + 2];
            storedPixels.push_back(red);
            storedPixels.push_back(green);
            storedPixels.push_back(blue);
        }
    }

    const string marker = "F2PM";
    const size_t markerPos = bytes.size() >= marker.size() ? bytes.size() - marker.size() : 0;
    size_t foundPos = string::npos;
    for (size_t index = markerPos; index + marker.size() <= bytes.size(); --index)
    {
        if (index == 0)
        {
            break;
        }
        if (string(bytes.begin() + index, bytes.begin() + index + marker.size()) == marker)
        {
            foundPos = index;
            break;
        }
    }

    if (foundPos != string::npos && foundPos + 4 + 8 <= bytes.size())
    {
        uint64_t sizeValue = 0;
        for (int shift = 0; shift < 64; shift += 8)
        {
            sizeValue |= static_cast<uint64_t>(bytes[foundPos + 4 + shift / 8]) << shift;
        }
        originalSize = sizeValue;
        const size_t payloadOffset = foundPos + 4 + 8;
        const size_t payloadSize = bytes.size() - payloadOffset;
        data.assign(storedPixels.begin(), storedPixels.begin() + (originalSize < storedPixels.size() ? originalSize : storedPixels.size()));
        if (payloadSize > 0 && originalSize <= payloadSize)
        {
            data.assign(bytes.begin() + payloadOffset, bytes.begin() + payloadOffset + originalSize);
        }
        return true;
    }

    originalSize = static_cast<uint64_t>(storedPixels.size());
    data.assign(storedPixels.begin(), storedPixels.begin() + (originalSize < storedPixels.size() ? originalSize : storedPixels.size()));
    return true;
}

bool readPNG(const string& inputPath, vector<unsigned char>& data, uint64_t& originalSize)
{
    ifstream input(inputPath, ios::binary | ios::ate);
    if (!input)
    {
        cerr << "Ошибка: не удалось открыть PNG-файл '" << inputPath << "'." << endl;
        return false;
    }

    const streamsize fileSize = input.tellg();
    input.seekg(0, ios::beg);
    vector<unsigned char> bytes(static_cast<size_t>(fileSize), 0);
    input.read(reinterpret_cast<char*>(bytes.data()), fileSize);
    if (!input)
    {
        cerr << "Ошибка: не удалось прочитать PNG-файл '" << inputPath << "'." << endl;
        return false;
    }

    if (bytes.size() < 8)
    {
        cerr << "Ошибка: повреждён PNG-файл '" << inputPath << "'." << endl;
        return false;
    }

    size_t position = 8;
    while (position + 12 <= bytes.size())
    {
        const uint32_t length = static_cast<uint32_t>(bytes[position]) << 24 |
            static_cast<uint32_t>(bytes[position + 1]) << 16 |
            static_cast<uint32_t>(bytes[position + 2]) << 8 |
            static_cast<uint32_t>(bytes[position + 3]);
        const string chunkType(bytes.begin() + position + 4, bytes.begin() + position + 8);
        const size_t chunkDataOffset = position + 8;
        const size_t chunkDataSize = static_cast<size_t>(length);
        if (chunkType == "f2pm" && chunkDataOffset + chunkDataSize + 4 <= bytes.size())
        {
            const vector<unsigned char> payload(bytes.begin() + chunkDataOffset, bytes.begin() + chunkDataOffset + chunkDataSize);
            if (payload.size() >= 8)
            {
                uint64_t sizeValue = 0;
                for (int shift = 0; shift < 64; shift += 8)
                {
                    sizeValue |= static_cast<uint64_t>(payload[shift / 8]) << shift;
                }
                originalSize = sizeValue;
                if (payload.size() > 8)
                {
                    data.assign(payload.begin() + 8, payload.begin() + 8 + originalSize);
                }
                return true;
            }
        }

        position += 12 + chunkDataSize;
    }

    cerr << "Ошибка: в PNG-файле нет данных для восстановления." << endl;
    return false;
}

bool saveImageByExtension(const string& outputPath,
    const vector<unsigned char>& imageData,
    size_t width,
    size_t height,
    uint64_t originalSize,
    const vector<unsigned char>& originalData)
{
    const string lowerPath = getLowerFileExtension(outputPath);

    if (lowerPath.size() >= 4 && lowerPath.substr(lowerPath.size() - 4) == ".bmp")
    {
        return saveBMP(outputPath, imageData, width, height, originalSize, originalData);
    }

    if (lowerPath.size() >= 4 && lowerPath.substr(lowerPath.size() - 4) == ".png")
    {
        return savePNG(outputPath, imageData, width, height, originalSize, originalData);
    }

    return savePPM(outputPath, imageData, width, height, originalSize, originalData);
}

bool decodeImageFile(const string& inputPath, const string& outputPath)
{
    const string lowerPath = getLowerFileExtension(inputPath);
    vector<unsigned char> recoveredData;
    uint64_t originalSize = 0;

    bool ok = false;
    if (lowerPath.size() >= 4 && lowerPath.substr(lowerPath.size() - 4) == ".ppm")
    {
        ok = readPPM(inputPath, recoveredData, originalSize);
    }
    else if (lowerPath.size() >= 4 && lowerPath.substr(lowerPath.size() - 4) == ".bmp")
    {
        ok = readBMP(inputPath, recoveredData, originalSize);
    }
    else if (lowerPath.size() >= 4 && lowerPath.substr(lowerPath.size() - 4) == ".png")
    {
        ok = readPNG(inputPath, recoveredData, originalSize);
    }
    else
    {
        cerr << "Ошибка: неподдерживаемый формат изображения '" << inputPath << "'." << endl;
        return false;
    }

    if (!ok)
    {
        return false;
    }

    ofstream output(outputPath, ios::binary);
    if (!output)
    {
        cerr << "Ошибка: не удалось создать выходной файл '" << outputPath << "'." << endl;
        return false;
    }

    if (!recoveredData.empty())
    {
        output.write(reinterpret_cast<const char*>(recoveredData.data()), static_cast<streamsize>(recoveredData.size()));
    }

    if (!output)
    {
        cerr << "Ошибка: не удалось записать восстановленный файл '" << outputPath << "'." << endl;
        return false;
    }

    cout << "Восстановлен файл из изображения." << endl;
    cout << "Размер восстановленного файла: " << originalSize << " байт" << endl;
    return true;
}

bool prepareOptions(int argc, char* argv[], ProgramOptions& options)
{
    if (argc == 1)
    {
        options.interactive = true;
        cout << "Режим выбора файла из консоли." << endl;
        options.inputPath = readConsoleLine("Введите путь к входному файлу", "");
        if (options.inputPath.empty())
        {
            cerr << "Ошибка: путь к входному файлу не может быть пустым." << endl;
            return false;
        }

        options.outputPath = readConsoleLine("Введите путь к выходному файлу (.ppm/.bmp/.png)", "");
        if (options.outputPath.empty())
        {
            if (isImageExtension(options.inputPath))
            {
                options.outputPath = "restored.bin";
            }
            else
            {
                options.outputPath = "out.png";
            }
        }

        if (isImageExtension(options.inputPath) && (options.outputPath.empty() || !isImageExtension(options.outputPath)))
        {
            options.decode = true;
            if (options.outputPath.empty())
            {
                options.outputPath = "restored.bin";
            }
        }
        else
        {
            options.mode = readModeFromConsole(options.xorKeyText);
            for (char& ch : options.mode)
            {
                ch = static_cast<char>(tolower(static_cast<unsigned char>(ch)));
            }
        }
        return true;
    }

    int index = 1;
    if (string(argv[index]) == "--decode" || string(argv[index]) == "decode")
    {
        options.decode = true;
        ++index;
    }

    if (argc > index)
    {
        options.inputPath = argv[index++];
    }
    if (argc > index)
    {
        options.outputPath = argv[index++];
    }
    if (!options.decode && argc > index)
    {
        options.mode = argv[index++];
    }
    if (!options.decode && argc > index)
    {
        options.xorKeyText = argv[index++];
    }

    for (char& ch : options.mode)
    {
        ch = static_cast<char>(tolower(static_cast<unsigned char>(ch)));
    }

    return true;
}

bool isImageExtension(const string& path)
{
    const string lowerPath = getLowerFileExtension(path);
    return lowerPath.size() >= 4 && (lowerPath.substr(lowerPath.size() - 4) == ".ppm" || lowerPath.substr(lowerPath.size() - 4) == ".bmp" ||  lowerPath.substr(lowerPath.size() - 4) == ".png");
}

bool parseModeAndKey(const ProgramOptions& options, unsigned char& xorKey)
{
    if (options.mode != "rgb" && options.mode != "gray" && options.mode != "xor")
    {
        cerr << "Ошибка: неизвестный режим '" << options.mode << "'." << endl;
        printUsage();
        return false;
    }

    if (options.mode == "xor")
    {
        if (options.xorKeyText.empty())
        {
            cerr << "Ошибка: для режима xor требуется ключ." << endl;
            printUsage();
            return false;
        }

        char* end = nullptr;
        const unsigned long parsedValue = strtoul(options.xorKeyText.c_str(), &end, 0);
        if (end == options.xorKeyText.c_str() || *end != '\0' || parsedValue > 255)
        {
            cerr << "Ошибка: недопустимый XOR-ключ '" << options.xorKeyText << "'." << endl;
            return false;
        }
        xorKey = static_cast<unsigned char>(parsedValue);
    }
    else if (!options.xorKeyText.empty())
    {
        cout << "Примечание: xor_key ignored for mode '" << options.mode << "'." << endl;
    }

    return true;
}

int main(int argc, char* argv[])
{
    setlocale(LC_ALL, "RUS");
    ProgramOptions options;
    if (!prepareOptions(argc, argv, options))
    {
        return 1;
    }

    if (shouldDecode(options))
    {
        return decodeImageFile(options.inputPath, options.outputPath) ? 0 : 1;
    }

    if (options.outputPath.empty())
    {
        options.outputPath = "out.ppm";
    }

    unsigned char xorKey = 0;
    if (!parseModeAndKey(options, xorKey))
    {
        return 1;
    }

    vector<unsigned char> inputData;
    uint64_t fileSize = 0;
    if (!readFile(options.inputPath, inputData, fileSize))
    {
        return 1;
    }

    const ImageSize imageSize = calculateImageSize(fileSize);
    const size_t imageBufferSize = imageSize.width * imageSize.height * 3;
    vector<unsigned char> imageData(imageBufferSize, 0);

    const size_t pixelCount = imageSize.pixelCount;
    if (options.mode == "rgb")
    {
        convertRGB(inputData, imageData, pixelCount);
    }
    else if (options.mode == "gray")
    {
        convertGray(inputData, imageData, pixelCount);
    }
    else
    {
        convertXor(inputData, imageData, pixelCount, xorKey);
    }

    if (!saveImageByExtension(options.outputPath, imageData, imageSize.width, imageSize.height, fileSize, inputData))
    {
        return 1;
    }

    cout << "Размер файла: " << fileSize << " байт" << endl;
    cout << "Размер изображения: " << imageSize.width << "x" << imageSize.height << endl;
    cout << "Количество пикселей: " << pixelCount << endl;
    cout << "Использованный режим: " << options.mode << endl;

    return 0;
}

