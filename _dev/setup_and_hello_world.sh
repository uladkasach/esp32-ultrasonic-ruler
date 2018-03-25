## with ubuntu terminal


##########################################
## setup env
##########################################
#http://iot-bits.com/esp32/getting-started-with-esp32-esp-idf-part-2/
#http://esp-idf.readthedocs.io/en/latest/get-started/linux-setup.html

## 1 download package dependencies
sudo apt-get install git wget make libncurses-dev flex bison gperf python python-serial -y

## 2 download toolchain
mkdir ~/esp32
cd ~/esp32
wget https://dl.espressif.com/dl/xtensa-esp32-elf-linux64-1.22.0-80-g6c4433a-5.2.0.tar.gz
tar -xzf xtensa-esp32-elf-linux64-1.22.0-80-g6c4433a-5.2.0.tar.gz

##3 add toolchain to env
export PATH=$PATH:$HOME/esp32/xtensa-esp32-elf/bin
## check it installed correctly
xtensa-esp32-elf-gcc -v

##4 add esp32 esp-idf
git clone --recursive https://github.com/espressif/esp-idf.git
export IDF_PATH=~/esp32/esp-idf


##########################################
## configure environment (that terminal session)
##########################################
export PATH=$PATH:$HOME/esp32/xtensa-esp32-elf/bin
export IDF_PATH=~/esp32/esp-idf


##########################################
## get started with example: http://esp-idf.readthedocs.io/en/latest/get-started/index.html#start-a-project
##########################################
## 1. copy the hello world example to your ~/esp32 directory
cd ~/esp32
cp -r $IDF_PATH/examples/get-started/hello_world .

## 2. compile your program for the freertos os
cd ~/esp32/hello_world
make menuconfig ##  bring up the configuration menu, which lets you set parameters such as flash related parameters and other important values that define your ESP32 hardware:
                ## leave everything as default values -> press save (save to default location), press exit
make ## if make has errors, make sure that you have most up to date toolchain

## 2. download your program to the esp-32 board
make flash ## the flash part takes care of moving it to the port automatically.
           ## if the error says "Errno 13] Permission denied: '/dev/ttyUSB0'", see http://esp-idf.readthedocs.io/en/latest/get-started/linux-setup.html#permission-issues-dev-ttyusb0
                ## in summary, run `sudo usermod -a -G dialout $USER` and LOG OUT (! IMPORTANT !) and then log back in

## 3. monitor the result!
make monitor # To exit the monitor use shortcut Ctrl+].
