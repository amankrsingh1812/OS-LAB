## Feature 1
The first feature that we have chosen is `compression`

And we have chosen 2 instances of `ZFS` file system for evaluation:
- *with* Compression
- *without* Compression 

### Implementation of compression
In ZFS compression is implemented by the ... algorithm

### Benefits of compression (Yahan output ko analyse karna hai. Heading change kardena is section ka)

**Note -** To run the testing code, use the bash script `test.sh`. This will automatically install the filesystems with the appropriate parameters, run `vdbench` on them, and also provide the respective outputs for each instance

To quantify the benefits we have used the following workload - 
```
Code of the workload
```
The workload does the following -
1. Turn the compression ON. 
2. Create the directory structure with `depth=2` and `width=2` 
3. Create 2 files into each directory.
4. Write `8MB` of data in each file. 
5. Read the files from the disk  
6. Delete the directory structure

Then for same workload is run again, but with compression ON

The following benefits of compression were observed in `ZFS` - 
- The space utilised in less. This can be seen by this metric ...

<!-- Insert screenshot here -->

- Aur koi benefit hai compression ka??


### Disadvantages of compression

- *More CPU usage* - Higher CPU usage was observed during reading and writing with compression ON. This is because 
- While writing, the data has to be compressed by the ... compression algorithm. This requires more computation power than write without compression
- While Reading, the data has to be uncompressed. This requires computation power than read without compression
 This can be seen by this metric ... 

<!-- Insert screenshot here -->

- *More Time* - It takes more time to read and write with compression ON. This is because 
- While writing, the data has to be compressed by the ... compression algorithm. Some additional time gets utilized for running the algorithm
- While Reading, the data has to be uncompressed. Some additional time gets utilized for running the algorithm 
 This can be seen by this metric ... 

<!-- Insert screenshot here -->















