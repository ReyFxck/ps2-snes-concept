# PS2 SNES Concept

Primeiro conceito funcional de um port de SNES para PS2 usando base do Snes9x 2005.

## Status atual
- Boota no NetherSX2
- Video funcionando
- Cores corrigidas
- Input funcionando
- Jogo entra e roda

## Limitacoes atuais
- Performance ainda baixa
- Backend de video ainda simples
- Ainda e uma prova de conceito

## Estrutura
- ps2boot/main.c -> bootstrap e integracao com o core
- ps2boot/ps2_video.c -> video PS2
- ps2boot/ps2_input.c -> input PS2
- ps2boot/build.sh -> build automatico

## Build
Entre em ps2boot e rode:
bash build.sh

O ELF gerado e copiado para /sdcard/ps2/

## Observacoes
A ROM de teste nao vai no repositorio.
