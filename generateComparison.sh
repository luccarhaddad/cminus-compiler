./killbuild
# shellcheck disable=SC2164
cd build
../compLabAluno.bash
# shellcheck disable=SC2103
cd ..
# shellcheck disable=SC2164
mkdir check && cd check
mkdir aluno && mkdir gabarito
cp alunodetail/*_syn.txt check/aluno/
cp detail/*_syn.txt check/gabarito/
kdiff3 check/aluno/ check/gabarito
rm -rf check/