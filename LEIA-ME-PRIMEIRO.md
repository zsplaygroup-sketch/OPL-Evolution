# SD CEARÁ GAMES — Evolution XMB v0.4 | Kit de compilação no GitHub

Este **não é um ELF compilado**. É um pacote de código-fonte de personalização para
produzir o ELF experimental do PlayStation 2 usando o fork
`zsplaygroup-sketch/OPL-Evolution` no GitHub Actions.

## Repositório e referência

- Fork: https://github.com/zsplaygroup-sketch/OPL-Evolution
- Branch examinada: `evo-development`
- Commit base: `47119b7128f347eb046c52a48dee36a9559b2d1b`
- Proposta: interface SD Ceará Games, boot e barra com 4s adicionais,
  biblioteca PS2/PS1 e aba APPS com `title.cfg`, `conf_apps.cfg` e ELF solto
- O mecanismo original de execução de jogos ISO/ZSO não é modificado.

## Como este pacote está organizado

A estrutura deste ZIP é intencional: copie **as pastas `.github` e `sdceara`**
para a RAIZ do seu fork. Preserve o resto da árvore do OPL Evolution.

```text
OPL-Evolution/
  .github/
    workflows/
      sd-ceara-evolution.yml
  sdceara/
    tools/
      aplicar_v04.py
      aplicar_no_opl_evolution.py
      gerar_title_cfg.py
    assets/
      gfx/
        evo_boot.png
        evo_dashboard.png
  src/
  include/
  gfx/
  Makefile
```

Ao publicar essa estrutura na branch `evo-development` do seu fork, abra
GitHub > Actions > SD Ceara Games - Compilar Evolution XMB > Run workflow.
Ao concluir com sucesso, abra a execução e baixe o *artifact*
`SD-CEARA-GAMES-EVOLUTION-XMB-v0.4-TESTE`. O ELF estará dentro dele.

**Segurança:** o workflow aplica a personalização somente na cópia temporária
usada para compilar; não altera a branch principal nem o seu ELF funcional.
A instalação original no console deve permanecer disponível para recuperação.

## Teste no PS2

Copie o ELF experimental para um pendrive separado e abra via wLaunchELF.
Teste tela inicial, barra, leitura/execução ISO/ZSO, PS1 configurado, listagem
APPS e abertura de `title.cfg`/ELF avulso. Não use como substituto da
instalação funcional até validar.

## Limitações e direitos

O código v0.4 foi validado estaticamente contra trechos de fonte da revisão
indicada, mas **não foi compilado nem executado em um PS2 neste ambiente**.
Uma compilação no GitHub pode revelar erros que ainda precisarão de correção.
Mantenha avisos e créditos dos autores do OPL sob AFL-3.0.
Não há arquivos de jogos, BIOS, POPStarter ou outras obras protegidas no pacote.
