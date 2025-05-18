# Contribuindo para o pathofAsgard

Tabela de Conteúdos
-------------------

  * [Relatar Bugs](#relatar-bugs)
  * [Sugerir Melhorias](#sugerir-melhorias)
  * [Rótulos de Problemas](#rótulos-de-problemas)
  * [Ambiente de Desenvolvimento Local](#ambiente-de-desenvolvimento-local)
  * [Torne-se um Membro da Equipe](#torne-se-um-membro-da-equipe)

Relatar Bugs
------------

Este guia o orientará no processo de criação de um relatório de bug para o pathofAsgard! Problemas podem ser usados não apenas para rastrear bugs, mas também para acompanhar melhorias e tarefas. Quanto mais detalhado for seu relatório, mais fácil será para os desenvolvedores reproduzirem e resolverem o bug!

### Encontrou um bug? :bug:

* **Certifique-se de que o bug não vem de uma personalização** nos seus arquivos!
* **Certifique-se de que o bug não foi relatado anteriormente** procurando no GitHub em [Issues](https://github.com/AoShinRo/pathofAsgard/issues). Se o mesmo problema existir, sinta-se à vontade para deixar um comentário dizendo que você também está enfrentando o problema e, se possível, adicione algumas informações adicionais ou ausentes!
* Se não encontrar um problema aberto que trate do problema, [abra um novo](#enviar-um-relatório-de-bug)!

#### Enviar um Relatório de Bug :inbox_tray:

Há vários elementos que compõem um bom relatório de bug!

Aqui está um resumo de uma Issue genérica:
* **Título** deve fornecer uma visão geral do que o bug se trata.
* **Descrição** deve fornecer mais detalhes sobre o bug que não podem ser explicados no **Título**.
* **Rótulos** são coloridos para representar uma categoria na qual se enquadram.
* **Marcos** são usados pelos desenvolvedores para agrupar tarefas e avaliar rapidamente quão próximo o projeto está da conclusão.
* **Atribuídos** são os desenvolvedores diretamente vinculados para resolver o problema.
* **Comentários** permitem que outros membros deem feedback sobre o problema.

#### Quais são alguns bons detalhes para incluir no relatório de bug? :pencil2:

Ao descrever seu problema na área de **Descrição**, recomenda-se fornecer o máximo de informações possível para que o problema seja resolvido rapidamente. Lembre-se de que você pode marcar pessoas usando a funcionalidade `@mention`. Você também pode marcar outras Issues ou Pull Requests digitando `#`, o que exibirá uma lista de problemas. Você pode encontrar um guia de markdown em [Mastering Markdown](https://guides.github.com/features/mastering-markdown/).
Aqui estão algumas informações para lembrar ao criar uma Issue:
* **GitHub Hash**: O hash é uma string alfanumérica de 40 caracteres (que pode ser reduzida aos 7 primeiros caracteres) que indica a versão que você está usando. (**Se você estiver usando SVN em vez de Git:** Inclua a data da alteração e a primeira linha da mensagem de commit ao lado do número da revisão, ou não conseguiremos buscar o hash correspondente do Git).
* **Data do Cliente**: A data do cliente fornece detalhes específicos dependendo do problema. O principal detalhe é que ajuda a restringir problemas relacionados a pacotes.
* **Modificações que podem afetar os resultados**: Sempre é melhor tentar reproduzir o problema em uma instalação limpa do pathofAsgard se você tiver muitas modificações.
* **Descrição do Problema**: Descreva seu problema em detalhes! Capturas de tela e vídeos ajudam muito! Forneça também dumps de crash se um dos servidores estiver travando.
* **Como Reproduzir o Problema**: Descreva como reproduzir o problema em detalhes! Quanto mais informações, melhor!
* **Informações Oficiais**: Forneça fontes confiáveis para indicar por que é um bug! Por favor, não forneça links da iRO Wiki, pois há uma grande chance de que não correspondam ao comportamento do kRO.

#### Cuidado com a funcionalidade `@mention`! :warning:

Como o pathofAsgard usa `@comandos` personalizados, ao descrever um problema que trata desses comandos, lembre-se de que isso entra em conflito com o sistema de `@mention` do GitHub! Sempre coloque o texto entre aspas ao mencionar um ``` `@comando` ```(dessa forma) para não marcar usuários do GitHub não relacionados!

Sugerir Melhorias
-----------------

### Você escreveu um patch que corrige um bug? :bookmark_tabs:

* Abra um novo Pull Request no GitHub com o patch.
* Certifique-se de que a descrição do PR descreva claramente o problema e a solução. Inclua o número do problema relevante, se aplicável.

### Pretende adicionar um novo recurso ou alterar um existente? :bulb:

* Abra um novo Pull Request no GitHub com a adição ou alteração de recursos.
* Certifique-se de que a descrição do PR descreva claramente para que servem a adição ou as alterações. Inclua o número do problema relevante, se aplicável.

#### Como criar Pull Requests :pencil:

1. Certifique-se de ter uma [conta no GitHub](https://github.com/signup/free).
2. Em seguida, você precisará [fazer um fork do pathofAsgard](https://help.github.com/articles/fork-a-repo/#fork-an-example-repository) para sua conta.
3. Antes de fazer alterações, certifique-se de [criar um novo branch](https://help.github.com/articles/creating-and-deleting-branches-within-your-repository/) para sua árvore de trabalho.
4. Após concluir suas alterações, faça commit e push para seu branch.
5. Agora você está pronto para [criar um Pull Request](https://help.github.com/articles/creating-a-pull-request/) para o pathofAsgard!
  * Ao criar o Pull Request, siga nosso [template](https://github.com/AoShinRo/pathofAsgard/blob/master/.github/PULL_REQUEST_TEMPLATE.md) e forneça as informações necessárias.
  * **OPCIONAL**: Agradecemos muito aqueles que marcam a opção para [permitir edições por mantenedores](https://help.github.com/articles/allowing-changes-to-a-pull-request-branch-created-from-a-fork/), para que possamos aplicar pequenas correções ou adições às suas alterações antes de fazer o merge!

Rótulos de Problemas
--------------------

Na maioria das vezes, você, como usuário, não precisará se preocupar com as partes de **Marco** ou **Atribuído** de uma Issue. Os diferentes **Rótulos** permitem que os desenvolvedores entendam rapidamente o problema e também permitem uma busca rápida ou ordenação.

:bangbang: Os usuários devem estar cientes dos rótulos do tipo 'Modo' e 'Status', pois às vezes requerem feedback! :bangbang:

#### Componente

| Nome do Rótulo | Link de Busca | Descrição |
| --- | --- | --- |
| `component:core` | [buscar][buscar-pathofAsgard-label-componentcore] | Um erro no núcleo do pathofAsgard. |
| `component:database` | [buscar][buscar-pathofAsgard-label-componentdatabase] | Um erro no banco de dados do pathofAsgard. |
| `component:documentation` | [buscar][buscar-pathofAsgard-label-componentdocumentation] | Um erro na documentação do pathofAsgard. |
| `component:script` | [buscar][buscar-pathofAsgard-label-componentscript] | Um erro nos scripts do pathofAsgard. |
| `component:skill` | [buscar][buscar-pathofAsgard-label-componentskill] | Um erro que lida especificamente com uma habilidade. |
| `component:tool` | [buscar][buscar-pathofAsgard-label-componenttool] | Um erro em uma ferramenta do pathofAsgard. |

#### Faltando
| Nome do Rótulo | Link de Busca | Descrição |
| --- | --- | --- |
| `missing:clientdate` | [buscar][buscar-pathofAsgard-label-missingclientdate] | O **Título** ou **Descrição** do problema não indica a data do cliente usada para criar o bug. |
| `missing:mode` | [buscar][buscar-pathofAsgard-label-missingmode] | O **Título** ou **Descrição** do problema não indica o modo pré-renovação ou renovação. |
| `missing:revision` | [buscar][buscar-pathofAsgard-label-missingrevision] | A **Descrição** do problema não indica a revisão do pathofAsgard usada quando o bug ocorreu. |

#### Modo
| Nome do Rótulo | Link de Busca | Descrição |
| --- | --- | --- |
| `mode:prerenewal` | [buscar][buscar-pathofAsgard-label-modeprerenewal] | Um erro que existe no modo pré-renovação. |
| `mode:renewal` | [buscar][buscar-pathofAsgard-label-moderenewal] | Um erro que existe no modo renovação. |

#### Prioridade

| Nome do Rótulo | Link de Busca | Descrição |
| --- | --- | --- |
| `priority:high` | [buscar][buscar-pathofAsgard-label-priorityhigh] | Um erro que torna o pathofAsgard instável ou inutilizável. |
| `priority:medium` | [buscar][buscar-pathofAsgard-label-prioritymedium] | Um erro que causa repercussões significativas, mas não torna o pathofAsgard inutilizável. |
| `priority:low` | [buscar][buscar-pathofAsgard-label-prioritylow] | Um erro que é trivial. |

Ambiente de Desenvolvimento Local
---------------------------------

Veja o guia de configuração em nosso [README.md](https://github.com/AoShinRo/pathofAsgard/blob/master/README.md).

Torne-se um Membro da Equipe
----------------------------

Tem interesse em se tornar um colaborador ativo do pathofAsgard? Gostaria de se tornar um membro da equipe oficial? Existem alguns requisitos:

* **Seja ativo no desenvolvimento do projeto** - Corrija bugs, envie novos recursos e tenha bons hábitos de programação.
* **Atue de forma madura e responsável na comunidade** - Ajude outros desenvolvedores e mantenha uma atitude profissional em relação a contribuições.

#### Interações na Comunidade

Se você está buscando ajuda ou quer discutir melhorias para o projeto, não hesite em se envolver! Existem várias formas de entrar em contato e discutir com outros desenvolvedores, como fóruns e plataformas de comunicação do pathofAsgard.
