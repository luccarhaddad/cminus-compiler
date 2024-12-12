./killbuild
# shellcheck disable=SC2164
cd build
cmake ..
make
make ddiff
cd ..
python3 compare_diffs.py
