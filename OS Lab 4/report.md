**Important Note -** Imitation of the following experiments requires an environment with *JRE* support and preferably the latest updated and upgraded kernel support for *ZFS* installation.

## Feature 1 - Compression

The first feature that we have chosen is `compression`. It compresses your files on the fly and therefore lets you store more data using limited storage.

And we have chosen 2 instances of `ZFS` file system for evaluation:
- *with* Compression
- *without* Compression 

### Implementation of Compression
ZFS uses the `LZ4` compression algorithm.

LZ4 is lossless compression algorithm, providing compression speed greater than 500 MB/s per core  (>0.15 Bytes/cycle).  It features an extremely fast decoder, with speed in multiple GB/s per core (~1 Byte/cycle).      

- The LZ4 algorithm represents the data as a series of sequences. Each  sequence begins with a one-byte token that is broken into two 4-bit  fields.
- The first field represents the number of literal bytes that are  to be copied to the output. The second field represents the number of  bytes to copy from the already decoded output buffer (with 0  representing the minimum match length of 4 bytes). 
- A value of 15 in  either of the bit-fields indicates that the length is larger and there is an extra byte of data that is to be added to the length. 
- A value of 255 in these extra bytes indicates that yet another byte to be added. 
- Hence arbitrary lengths are represented by a series of extra bytes containing the value 255. The string of literals comes after the token and any  extra bytes needed to indicate string length. 
- This is followed by an  offset that indicates how far back in the output buffer to begin  copying. The extra bytes (if any) of the match-length come at the end of the sequence.

### Analysing the Benefits of Compression

**Note -** To run the testing code, use the bash script `test.sh`. This will automatically install the file systems with the appropriate parameters, run `vdbench` on them, and also provide the respective outputs for each instance

To quantify the benefits we have used the following workload - 
```
Code of the workload <test.conf ?>
```
The workload does the following -
1. Turn the compression ON. 
2. Create the directory structure with `depth=2` and `width=2` 
3. Create 2 files into each directory.
4. Write `8MB` of data in each file. 
5. Read the files from the disk  
6. Delete the directory structure

Then for same workload is run again, but with compression ON.

<u>The following benefits of compression were observed in `ZFS`</u> - 

- The space utilised for the same amount of information is less. This can be seen by this metric ...

<!-- Insert screenshot here -->

- Space occupied is inversely proportional to both the cost of storage as well as the time required to transfer such files. Hence compression leads to lower costs and faster transfers.


### Disadvantages of Compression

- *More CPU usage* - Higher CPU usage was observed during reading and writing with compression ON. This is because 
    - While writing, the data has to be compressed by the LZ4 compression algorithm. This requires more computation power than write without compression.
    - While reading, the data has to be uncompressed. This requires computation power than read without compression.
    This can be seen by this metric ... 

<!-- Insert screenshot here -->


- *More Time* - It takes more time to read and write with compression ON. This is because 
    - While writing, the data has to be compressed by the LZ4 compression algorithm. Some additional time gets utilised for running the algorithm.
    - While reading, the data has to be uncompressed. Some additional time gets utilised for running the algorithm.
    This can be seen by this metric ... 

<!-- Insert screenshot here -->

**Important Note -** Imitation of the following experiments requires an environment with *JRE* support and preferably the latest updated and upgraded kernel support for *ZFS* installation.

## Feature 2 - Encryption

The first feature that we have chosen is `compression`. It compresses your files on the fly and therefore lets you store more data using limited storage.

And we have chosen 2 instances of `ZFS` file system for evaluation:
- *with* Encryption
- *without* Encryption 

### Implementation of Encryption
ZFS uses the `AES-GCM 256 bit` authenticated encryption algorithm.

`AES-GCM` or Advanced Encryption Standard with Galois Counter Mode is a block cipher mode of operation that provides high speed authenticated encryption and data integrity. It provides high throughput rates for state-of-the-art, high-speed data transfer without any expensive hardware requirements.

The algorithm takes 4 inputs - 
- `Secret key` - Secret key is the cipher key of length 256 bit.  
- `Initialization vector (IV)` - A randomly generated number that is used along with a secret key for data encryption 
- `Unencrypted text` - This is the plain-text that has to be encrypted
- `Additional Authenticated Data (AAD)` - It is a string that can be used later on to decrypt the encrypted data. It is like a password which when later given, can decrypt the data

The algorithm gives 2 output - 
- `Message Authentication Code (MAC or Tag)` - The code is a short piece of information used to authenticate a message. It can be used later on for authentication of the user
- `Cipher Text` - This is the encrypted text that the algorithm output


The data is considered as a series of blocks of size 128 bits. Blocks are numbered sequentially, and then this block number is combined with an initialization vector and encrypted with the `secret key`. The cipher-text blocks are considered coefficients of a polynomial which is evaluated at key-dependent points, using finite field arithmetic. The result is then XORed with the unencrypted text, to produce the final cypher text. A random/arbitrary `IV` is required for each encryption or else it would result in a less secure cipher-text adn the Message Authentication Code. 

### Analysing the Benefits of Encryption

To quantify the benefits we have used the following workload - 
```shell

```
The workload does the following -
2. Create the directory structure with `depth=2` and `width=2` 
3. Create 2 files into each directory.
4. Write `8MB` of data in each file. 
5. Read the files from the disk  
6. Delete the directory structure

Then for same workload is run twice, once each on the file system with encryption enabled and encryption disabled ZFS.

<u>The following benefits of encryption were observed in `ZFS`</u> - 

- The space utilised for the same amount of information is less. This can be seen by this metric ...

<!-- Insert screenshot here -->

- Space occupied is inversely proportional to both the cost of storage as well as the time required to transfer such files. Hence compression leads to lower costs and faster transfers.


### Disadvantages of Encryption

- *More CPU usage* - Higher CPU usage was observed during reading and writing with encryption ON. This is because 
    - While writing, the data has to be encrypted by the AES-GCM encryption algorithm. This requires more computation power than write without encryption.
    - While reading, the data has to be decrypted. This requires computation power than read without decryption.
    This can be seen by this metric ... 

<!-- Insert screenshot here -->


- *More Time* - It takes more time to read and write with encryption ON. This is because 
    - While writing, the data has to be compressed by the AES-GCM encryption algorithm. Some additional time gets utilised for running the algorithm.
    - While reading, the data has to be decrypted. Some additional time gets utilised for running the algorithm.
    This can be seen by this metric ... 

<!-- Insert screenshot here -->



