./killbuild
cd build
../compLabAluno.bash
cd ..
cp alunodetail/*_syn.txt check/aluno/
cp detail/*_syn.txt check/gabarito/
kdiff3 check/aluno/ check/gabarito/
