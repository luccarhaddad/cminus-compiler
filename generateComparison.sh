./killbuild
cd build
../compLabAluno.bash
cd ..
mkdir "check" && cd check
mkdir aluno && mkdir gabarito
mv alunodetail/*_syn.txt check/aluno/
mv detail/*_syn.txt check/gabarito/
kdiff3 check/aluno/ check/gabarito
rm -rf check/