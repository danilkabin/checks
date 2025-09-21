cd src

make clean; bear -- make

echo "OUTPUT:"

./build/bin/opium -h

echo "OUTPUT END!"

cd ..
