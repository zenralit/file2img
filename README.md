# File to Image Converter

A lightweight command-line utility written in **C++** that converts arbitrary binary files into image formats and restores the original files back from those images. The project demonstrates low-level binary processing, manual image format generation, and reversible data encoding without relying on external libraries.

---

## Features

-  Convert any file into an image.
-  Restore the original file from the generated image.
-  Supports **PPM**, **BMP**, and **PNG** output formats.
-  Multiple encoding modes:
  - **RGB** (3 bytes per pixel)
  - **Grayscale**
  - **XOR** encoding with a user-defined key
-  Stores original file metadata inside the generated image.
-  Works entirely without third-party libraries.
-  Supports both interactive mode and command-line arguments.

---

## Technologies

- **C++17**
- **STL**
- **Binary File I/O (`fstream`)**
- Manual **BMP**, **PPM**, and **PNG** file generation
- CRC32 implementation
- Adler-32 checksum
- Custom DEFLATE (Stored Block) encoder
- Bitwise operations

---

## Skills Demonstrated

- Binary file processing
- Manual implementation of image file formats
- File serialization and deserialization
- Metadata embedding
- Lossless data encoding
- Checksum algorithms (CRC32, Adler-32)
- Binary stream manipulation
- Command-line application development
- Input validation and exception handling

---

## Supported Formats

### Input

- Any binary file
- Text files
- Executables
- Archives
- Images
- Documents

### Output

- **PPM**
- **BMP**
- **PNG**

---

## Encoding Modes

### RGB

Stores every three bytes of the source file as one RGB pixel.

```text
Byte1 → Red
Byte2 → Green
Byte3 → Blue
```

### Grayscale

Each byte is replicated across the RGB channels.

```text
Value → (R, G, B)
128   → (128, 128, 128)
```

### XOR

Applies a user-defined XOR key before encoding.

```text
EncodedByte = OriginalByte XOR Key
```

The same key is used during decoding to recover the original data.

---

## Metadata Storage

The application embeds additional metadata into the generated image, including:

- Original file size
- Original binary payload

This allows complete reconstruction of the source file without requiring any external metadata files.

---

## Command Line Usage

### Encode

```bash
file2image.exe input.bin output.png rgb
```

### Encode with XOR

```bash
file2image.exe input.bin output.png xor 55
```

### Decode

```bash
file2image.exe --decode image.png restored.bin
```

If no arguments are provided, the program automatically switches to interactive mode.

---

## Project Structure

The project includes implementations of:

- Binary file reader
- Image size calculation
- RGB/Grayscale/XOR encoders
- Manual BMP writer
- Manual PPM writer
- Manual PNG writer
- PNG chunk generation
- CRC32 calculation
- Adler-32 calculation
- Basic DEFLATE encoder
- Image decoders
- Metadata extraction
- Command-line argument parser

---

## Purpose

This project was created as an exercise in low-level file processing, binary serialization, and manual implementation of common image formats. It demonstrates an understanding of binary data representation, image container structures, checksum algorithms, lossless data encoding, and reversible file transformations without external dependencies.
