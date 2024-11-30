./killbuild
# shellcheck disable=SC2164
cd build
cmake ..
make
make ddiff
cd ..
python3 compare_diffs.py
# shellcheck disable=SC2103
#cd ..
#rm -rf check/
## shellcheck disable=SC2164
#mkdir check && cd check
#mkdir aluno && mkdir gabarito
#cd ..
#cp alunodetail/*_tab.txt check/aluno/
#cp detail/*_tab.txt check/gabarito/
#kdiff3 check/aluno/ check/gabarito
