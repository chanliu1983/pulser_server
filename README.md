In order to build on Mac OS you need to install following:

brew install pkg-config
brew install libevent
brew install rapidjson
brew install lz4
brew install zlib
brew install openssl

Linux:

sudo apt-get install libleveldb-devsudo apt-get install libleveldb-dev
sudo apt-get install rapidjson-dev
sudo apt-get install liblz4-dev
sudo apt-get install libssl-dev


Generate the self signed cert

mkdir -p cert
openssl genpkey -algorithm RSA -out cert/server.key -aes256
openssl req -new -x509 -key cert/server.key -out cert/server.crt -days 365
