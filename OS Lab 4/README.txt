Given scrips requries zfsutils-linux for running, the installation instructions are:-
1. If ubuntu version is below 20.04 use the following command to add latest zfs repo:- sudo add-apt-repository ppa:jonathonf/zfs
2. Run the command to install zfsutils-linux :- sudo apt install zfsutils-linux

STEPS TO RUN(Provide root user password whenever prompted):
1. Copy all the .sh files in vdbench directory
2. To test Compression Feature, Run the command:- bash compressionTestScript.sh
3. To test Encryption Feature, Run the command:- bash encryptionTestScript.sh
4. For removing all virtual disks and image files, Run the command:- bash cleanScript.sh
   (In case some files don't exist a warning may be printed, you may ignore it)
