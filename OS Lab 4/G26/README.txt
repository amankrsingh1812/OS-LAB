Given scripts require zfsutils-linux for running. The installation instructions are:-
1. APT update
      sudo apt update
2. APT upgrade
      sudo apt upgrade
3. Install JRE
      sudo apt-get install openjdk-8-jre
4. If Ubuntu version is below 20.04 use the following command to add latest zfs repo:- 
      sudo add-apt-repository ppa:jonathonf/zfs
5. Run the command to install zfsutils-linux :- 
      sudo apt-get install zfsutils-linux

STEPS TO RUN (Provide root user password whenever prompted):
1. Copy all the .sh and .conf files in vdbench directory
2. To install ZFS(with appropriate parameters) and test Compression Feature, Run the command:- 
      bash compressionTestScript.sh
3. To install ZFS(with appropriate parameters) and test Encryption Feature, Run the command:- 
      bash encryptionTestScript.sh
4. For removing all virtual disks and image files which were created during the test, Run the command:- 
      bash cleanScript.sh
   (In case some files don't exist and a warning may be printed, you may ignore it)
