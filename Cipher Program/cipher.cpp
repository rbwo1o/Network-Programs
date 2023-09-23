/*
* Author:      Robert Blaine Wilson
*
* Date:        8/7/2023
*
* Synopsis:    This program encrypts and decrypts an input file based on a provided 4 byte key and writes the ciphertext to an
*              output file. This app accepts three command line arguments: an input file, an outputfile, and a 4 byte key. First,
*              the program verifies that the provided 4 byte key is indeed 4 bytes long and is in hexadecimal format. Then the program
*              verifies that the input and output file streams can be opened successfully. Once the command line parameters are verified,
*              the program will expand the key to 32 bytes which matches the size of each block to be read. Then it will read through the
*              input 32 bytes at a time. During each iteration, the block is encrypted with the expanded key, the block is wrote to the output
*              file, and the key is roated to accommodate for the next block.
*
* Help:        While writting this file, I followed along the material provided in Module 9. I also followed the key expansion 
*              and rotation algorithms provided in the assignment instructions.
*
* Compilation: g++ -c cipher.cpp
*              g++ -o cipher cipher.o
*
* Usage:       ./cipher <input file> <output file> <key>
*/

#include <iostream>
#include <fstream>
#include <string>
#include <cctype>


using namespace std;


/* Function Parameters */
bool stringToHex(string, uint32_t &);
void expandKey(uint8_t*, int, uint32_t);
void encrypt(uint8_t*, uint8_t*, int);
void rotateKey(uint8_t*, int);


int main(int argc, char* argv[])
{
    // validate command line arguments
    if(argc != 4)
    {
        cout << "Usage: " << argv[0] << " <input file> <output file> <key>" << endl;
        return -1;
    }

    // validate key
    uint32_t kv;
    if(!stringToHex((string)argv[3], kv))
    {
        return -1;
    }
    
    // validate input file
    ifstream inputFile(argv[1]);
    if(!inputFile)
    {
        perror(argv[1]);
        return -1;
    }

    // validate ouput file
    ofstream outputFile(argv[2]);
    if(!outputFile)
    {
        perror(argv[2]);
        return -1;
    }

    // expand the key to fit the block size
    int blockSize = 32;
    uint8_t key[blockSize];
    expandKey(key, blockSize, kv);

    while(!inputFile.eof())
    {
        uint8_t block[blockSize];

        inputFile.read((char*)&block, blockSize);
        size_t bytes = inputFile.gcount();
        
        encrypt(block, key, bytes);
        
        outputFile.write((char*)&block, bytes);
        
        // block size is constant 32 bytes, if we send num bytes read then the key will only rotate valid bytes
        rotateKey(key, bytes);
    }

    inputFile.close();
    outputFile.close();
    return 0;
}



/*
 * Function: stringToHex
 * Parameters: This function accepts a string text value and a unsigned 4 byte variable as a refence to store the converted hex.
 * Return: This function will return false if it is unable to convert the text to hex.
 * This function attempts to convert a string to a 4 byte hex variable. If it is unable to convert the text, it will return false.
*/
bool stringToHex(string text, uint32_t &hex)
{
    // first check for "0x" substring
    int pos = text.find("0x");
    if(pos != string::npos)
    {
        // set substring
        text = text.substr(pos + 2);
    }

    // check length -> must be 8 bytes .... (2 char = 1 hex byte so 8 char = 4 hex bytes)
    if(text.length() != 8)
    {
        cout << text << ": must be a 4 byte key." << endl;
        return false;
    }

    // iterate text to ensure valid hex digits
    for(int i = 0; i < text.length(); i++)
    {
        if(!isxdigit(text[i]))
        {
            cout << text << ": is not a valid hex key." << endl;
            return false;
        }
    }

    // valid therefore convert
    hex = strtoul(text.c_str(), NULL, 16);
    
    return true;
}



void expandKey(uint8_t* key, int size, uint32_t kv)
{
    key[0] = (kv >> 24) - (kv >> 16) + (kv >> 8) + kv;

    for(int i = 0; i < size; i++)
    {
        key[i] = key[i-1] + (kv >> 24) + (kv >> 16) - (kv >> 8) + kv;
    }
}



void encrypt(uint8_t* block, uint8_t* key, int size)
{
    for(int i = 0; i < size; i++)
    {
        block[i] = block[i] ^ key[i];
    }
}



void rotateKey(uint8_t* key, int size)
{
    uint8_t t = key[size-1] + 1;

    for(int b = 0; b < size - 2; b++)
    {
        key[b+1] = key[b];
    }

    key[0] = t;
}