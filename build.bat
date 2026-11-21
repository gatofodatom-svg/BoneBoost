@echo off
echo Compilando o Bone Boost... (pode demorar uns 15 minutos)
cmake -B build || goto err
cmake --build build --config Release || goto err
xcopy /E /I /Y "build\BoneBoost_artefacts\Release\VST3\Bone Boost.vst3" "%CommonProgramFiles%\VST3\Bone Boost.vst3" || goto err
echo.
echo PRONTO! Agora abra o FL Studio e faca o scan dos plugins.
pause
exit /b 0
:err
echo.
echo Deu erro. Tire um print desta tela e mande para o Claude.
pause
exit /b 1
