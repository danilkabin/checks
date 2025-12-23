cd project 

make clean; bear -- make -j$(proc)

echo "OUTPUT:"

cd ..

./build/bin/project -h

echo "OUTPUT END!"
