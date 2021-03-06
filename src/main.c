/**
 * @file    main.c
 * @author  André Botelho (keyoted@gmail.com)
 * @brief   Ficheiro principal onde consta maior parte da lógica do programa.
 * @version 1
 * @date 2020-01-01
 *
 * @copyright Copyright (c) 2020
 */

#ifdef DEBUG_BUILD
#    include <sys/stat.h>
#endif

#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <unistd.h>
#include <strings.h>
#include <locale.h>




// Estado do programa
// *********************************************************************************************************************
#define COL_IMPLEMENTACAO
#include "artigo.h"
#include "encomenda.h"
#include "utilizador.h"
#include "menu.h"

#ifndef artigocol_H
#    define artigocol_H
#    define COL_TIPO artigo
#    define COL_NOME artigocol
#    define COL_DEALOC(X) freeArtigo(X)
#    define COL_WRITE(X, F) save_artigo(F, X)
#    define COL_READ(X, F) load_artigo(F, X)
#    include "colecao.h"
#endif

#ifndef encomendacol_H
#    define encomendacol_H
#    define COL_TIPO encomenda
#    define COL_NOME encomendacol
#    define COL_DEALOC(X) freeEncomenda(X)
#    define COL_WRITE(X, F) save_encomenda(F, X)
#    define COL_READ(X, F) load_encomenda(F, X)
#    include "colecao.h"
#endif

#ifndef utilizadorcol_H
#    define utilizadorcol_H
#    define COL_TIPO utilizador
#    define COL_NOME utilizadorcol
#    define COL_DEALOC(X) freeUtilizador(X)
#    define COL_WRITE(X, F) save_utilizador(F, X)
#    define COL_READ(X, F) load_utilizador(F, X)
#    include "colecao.h"
#endif

artigocol     artigos;    ///< Artigos da seção atual
encomendacol  encomendas; ///< Encomendas
utilizadorcol clientes;   ///< Utilizadores existentes no registo

#include "outrasListagens.h"




// De interface_imprimir_recibo
// *********************************************************************************************************************
/**
 * @brief      Pode ser utilizado como um iterador, imprime encomenda num recibo.
 * @param e    Encomenda a ser impressa
 * @param data Ano e mês do recibo como também alguns dados remetentes ao total
 *             do preço do recibo e artigos compras e encomendas vendidas.
 * @returns    0
 */
int pred_printencRec(encomenda const* const e, struct {
    int      ano;
    int      mes;
    uint64_t total;
    uint64_t art;
    uint64_t compras;
    uint64_t encomendas;
} * data) {
    struct tm const* const t = localtime(&e->tempo);
    if (t->tm_mon == data->mes && t->tm_year == data->ano) {
        printf("* Dia %d/%d/%d\n", 1900 + t->tm_year, t->tm_mon + 1, t->tm_mday);
        printf("    * NOME %s\n", protectStr(clientes.data[e->ID_cliente].nome));
        printf("    * NIF  %9.9s\n", clientes.data[e->ID_cliente].NIF);
        printf("    * CC   %12.12s\n", clientes.data[e->ID_cliente].CC);
        printf("    * ARTIGOS COMPRADOS:\n");
        int64_t   preco_art;
        colSize_t i;
        for (i = 0; i < e->compras.size; i++) {
            // inicio
            compra const* const c = &e->compras.data[i];
            artigo const* const a = &artigos.data[c->IDartigo];
            preco_art             = a->preco_cent;
            printf("        * ");
            // consumo
            printf("CONSUMO %s", (a->meta & ARTIGO_GRUPO_ANIMAL) ? "ANIMAL" : "HUMANO");
            // preço
            printf("\t- PREÇO %luc\t- IVA", a->preco_cent);
            // iva
            switch (a->meta & ARTIGO_IVA) {
                case ARTIGO_IVA_INTERMEDIO:
                    printf(" %d%%", (int) ((ARTIGO_IVA_INTERMEDIO_VAL - 1) * 100));
                    preco_art *= ARTIGO_IVA_INTERMEDIO_VAL;
                    break;
                case ARTIGO_IVA_NORMAL:
                    printf(" %d%%", (int) ((ARTIGO_IVA_NORMAL_VAL - 1) * 100));
                    preco_art *= ARTIGO_IVA_NORMAL_VAL;
                    break;
                case ARTIGO_IVA_REDUZIDO:
                    printf(" %d%%", (int) ((ARTIGO_IVA_REDUZIDO_VAL - 1) * 100));
                    preco_art *= ARTIGO_IVA_REDUZIDO_VAL;
                    break;
            }
            // total
            printf("\t- TOTAL: %ldc", preco_art * c->qtd);
            // quantidade
            printf("\t- QUANTIDADE: %ld", c->qtd);
            // receita
            if (a->meta & ARTIGO_NECESSITA_RECEITA) {
                printf("\t- RECEITA (%12.12s)", c->receita);
            } else
                printf("\t- ARTIGO DE VENDA LIVRE ");
            // nome
            printf("\t\"%s\"", protectStr(a->nome));
            // fim
            data->art += c->qtd;
            printf("\n");
            fflush(stdout);
        }
        const uint64_t tot = encomenda_CalcPreco(e, &artigos);
        printf("    * TOTAL %ld\n", tot);
        fflush(stdout);
        data->compras += i;
        data->total += tot;
        data->encomendas += 1;
    }
    return 0;
}




// De interface_cliente
// *********************************************************************************************************************
/**
 * @brief   Pode ser utilizado como um iterador, imprime um utilizador.
 * @param u Utilizador a ser impresso.
 * @param i Deve ser inicializado como 0, no final irá conter o número de
 *          clientes impressos.
 * @returns 0
 */
int pred_printUti(utilizador const* const u, int64_t* const i) {
    printf("   %8lu   |   ", (*i)++);
    menu_printUtilizador(*u);
    printf("\n");
    return 0;
}

/**
 * @brief       Função responsável por editar todos os parametros de um cliente.
 * @param c     Cliente que será editado.
 * @param isNew Deve ser 1 se cliente é novo e 0 se é um cliente válido que será
 *              editado.
 * @returns     1 Se o clienre foi editado.
 * @returns     0 Se o cliente não foi editado/ foi eleminado; o cliente terá
 *              que ser eleminado pois é inválido.
 */
int form_editar_cliente(utilizador* const u, int isNew) {
    menu_printDiv();
    if (isNew)
        menu_printHeader("Novo Utilizador");
    else
        menu_printHeader("Editar Utilizador");

    printf("Inserir nome");
    if (!isNew) printf(" (%s)", protectStr(u->nome));
    freeN(u->nome);
    u->nome = menu_readNotNulStr();

    char* tmp = NULL;
    while (1) {
        freeN(tmp);
        printf("Inserir NIF");
        if (!isNew) printf(" (%9.9s)", u->NIF);
        tmp = menu_readNotNulStr();
        if (strlen(tmp) == 9) {
            for (int i = 0; i < 9; i++) {
                if (!isdigit(tmp[i])) {
                    menu_printError("digitos do NIF são characteres");
                    break;
                }
                u->NIF[i] = tmp[i];
            }
            break;
        } else {
            menu_printError("NIF tem 9 caracteres");
        }
    }

    while (1) {
        freeN(tmp);
        printf("Inserir CC");
        if (!isNew) printf(" (%12.12s)", u->CC);
        tmp = menu_readNotNulStr();
        if (strlen(tmp) == 12 && utilizador_eCCValido(tmp)) {
            for (int i = 0; i < 12; i++) { u->CC[i] = tmp[i]; }
            break;
        } else {
            menu_printError("CC tem 12 caracteres [9] digitos seguidos de [2] letras e [1] digito final");
        }
    }
    free(tmp);

    return 1;
}




// De interface_artigo
// *********************************************************************************************************************
/**
 * @brief   Pode ser utilizado como um iterador, imprime um artigo.
 * @param a Artigo a ser impresso.
 * @param i Deve ser inicializado como 0, no final irá conter o número de
 *          artigos impressos.
 * @returns 0
 */
int pred_printArt(artigo const* const a, int64_t* const i) {
    printf("   %8lu   |   ", (*i)++);
    menu_printArtigoStock(a);
    printf("\n");
    return 0;
}

/**
 * @brief       Função responsável por editar todos os parametros de um artigo.
 * @param a     Artigo que será editado.
 * @param isNew Deve ser 1 se o artigo é novo e 0 se é um artigo válido que será
 *              editado.
 * @returns     1 Se o artigo foi editado.
 * @returns     0 Se o artigo não foi editado/ foi eleminado; o artigo terá que
 *              ser eleminado pois é inválido.
 */
int form_editar_artigo(artigo* const a, int isNew) {
    menu_printDiv();
    menu_printHeader("Editar Artigo");
    int64_t tmp;

    if (!isNew) {
        printf("Desativar artigo? (S / N)");
        if (!isNew) printf(" (o artigo está %s)", (a->meta & ARTIGO_DESATIVADO) ? "desativado" : "ativado");
        if (menu_YN('S', 'N')) {
            a->meta = a->meta | ARTIGO_DESATIVADO;
            return 1;
        } else
            a->meta = a->meta & (~ARTIGO_DESATIVADO);
    }

    printf("Inserir nome de artigo");
    if (!isNew) printf(" (%s)", protectStr(a->nome));
    freeN(a->nome);
    a->nome = menu_readNotNulStr();

    while (1) {
        printf("Inserir preço de artigo (cent)");
        if (!isNew) printf(" (%ldc)", a->preco_cent);
        tmp = menu_readInt64_t();
        if (tmp < 0) {
            menu_printError("preço de artigo tem que ser positivo");
        } else {
            a->preco_cent = tmp;
            break;
        }
    }

    while (1) {
        printf("Inserir stock de artigo");
        if (!isNew) printf(" (%ld)", a->stock);
        tmp = menu_readInt64_t();
        if (tmp < 0) {
            menu_printError("stock de artigo tem que ser positivo");
        } else {
            a->stock = tmp;
            break;
        }
    }

    printf("Qual a taxa de IVA do artigo?\n");
    if (!isNew) {
        switch (a->meta & ARTIGO_IVA) {
            case ARTIGO_IVA_NORMAL: printf(" ( Normal )\n"); break;
            case ARTIGO_IVA_INTERMEDIO: printf(" ( Intermédio )\n"); break;
            case ARTIGO_IVA_REDUZIDO: printf(" ( Reduzido )\n"); break;
        }
    }

    switch (menu_selection(&(strcol) {.size = 3,
                                      .data = (char*[]) {
                                          "Artigo tem IVA normal",     // 0
                                          "Artigo tem IVA intermédio", // 1
                                          "Artigo tem IVA reduzido"    // 2
                                      }})) {
        case -1: return 0;
        case 0: a->meta = (a->meta & (~ARTIGO_IVA)) + ARTIGO_IVA_NORMAL; break;
        case 1: a->meta = (a->meta & (~ARTIGO_IVA)) + ARTIGO_IVA_INTERMEDIO; break;
        case 2: a->meta = (a->meta & (~ARTIGO_IVA)) + ARTIGO_IVA_REDUZIDO; break;
    }

    printf("O artigo necessita de receita? (S / N)");
    if (!isNew) printf(" ( o artigo %snecessita de receita )", (a->meta & ARTIGO_NECESSITA_RECEITA) ? "" : "não ");
    if (menu_YN('S', 'N'))
        a->meta = a->meta | ARTIGO_NECESSITA_RECEITA;
    else
        a->meta = a->meta & (~ARTIGO_NECESSITA_RECEITA);

    printf("O artigo é de utilização Animal ou Humana? (A / H)");
    if (!isNew) printf(" ( o artigo é do grupo %s )", (a->meta & ARTIGO_GRUPO_ANIMAL) ? "animal" : "humano");
    if (menu_YN('A', 'H'))
        a->meta = a->meta | ARTIGO_GRUPO_ANIMAL;
    else
        a->meta = a->meta & (~ARTIGO_GRUPO_ANIMAL);

    return 1;
}




// De form_editar_encomenda
// *********************************************************************************************************************
/**
 * @brief   Pode ser utilizado como um iterador, imprime uma compra.
 * @param c Compra a ser impressa.
 * @param i Deve ser inicializado como 0, no final irá conter o número de
 *          compras impressas.
 * @returns 0
 */
int pred_printCom(compra const* const c, int64_t* const i) {
    printf("   %8lu   |   ", (*i)++);
    menu_printCompra(c, &artigos);
    printf("\n");
    return 0;
}

/**
 * @brief       Função responsável por editar todos os parametros de uma compra.
 * @param c     Compra que será editada.
 * @param isNew Deve ser 1 se a compra é nova e 0 se é uma compra válida que
 *              será editada.
 * @returns     1 Se a compra foi editada.
 * @returns     0 Se a compra não foi editada/ foi eleminada; a compra terá que
 *              ser eleminada pois é inválida.
 */
int form_editar_compra(compra* const c, int isNew) {
    menu_printDiv();
    if (isNew)
        menu_printHeader("Criar Nova Compra");
    else
        menu_printHeader("Editar Compra");

    artigo* art;
    if (!isNew) {
        art = &artigos.data[c->IDartigo];
        // Fazer reset do stock
        art->stock += c->qtd;
        // Eleminar compra
        printf("Eleminar compra (S / N)");
        int YN = 2;
        while (YN == 2) {
            YN = menu_YN('S', 'N');
            switch (YN) {
                case 0: break;
                case 1: return 0;
            }
        }
    } else {
        // Ler tipo de artigo
        menu_printInfo("escolher artigo");
        int64_t id = -2;
        int64_t max;
        while (id == -2) {
            printf("      ID      |   Item\n");
            printf("         -2   |   Reimprimir\n");
            printf("         -1   |   Sair\n");
            max = 0;
            artigocol_iterateFW(&artigos, (artigocol_pred_t) &pred_printArt, &max);
            menu_printInfo("Insira o ID do artigo que será vendido na compra");
            id = menu_readInt64_tMinMax(-2, max - 1);
        }
        c->IDartigo = id;
        art         = &artigos.data[id];

        // Ler receita
        char* tmp = NULL;
        if (art->meta & ARTIGO_NECESSITA_RECEITA) {
            menu_printInfo("artigo necessita de receita para ser vendido");
            while (1) {
                printf("Insira os 19 characteres da receita do artigo");
                freeN(tmp);
                tmp = menu_readNotNulStr();
                if (strlen(tmp) != 19) {
                    menu_printError("receitas médicas necessitam de ter 19 dígitos");
                } else {
                    size_t i;
                    for (i = 0; i < 19; i++) {
                        if (!isdigit(tmp[i])) {
                            menu_printError("receitas médicas são compostas apenas de dígitos");
                            break;
                        }
                    }
                    if (i == 19) {
                        memcpy(&c->receita, tmp, 19);
                        break;
                    }
                }
            }
        }
        freeN(tmp);
    }

    // Ler quantidade
    if (art->stock == 0) {
        menu_printError("não existe stock do artigo atual");
        return 0;
    }
    if (art->meta & ARTIGO_DESATIVADO) {
        menu_printError("o artigo selecionado não se encontra disponivél para venda");
        return 0;
    }
    printf("Insira a quantidade de artigos para vender nesta compra");
    if (!isNew) printf(" (%ld)", c->qtd);
    c->qtd = menu_readInt64_tMinMax(1, art->stock);
    art->stock -= c->qtd;

    return 1;
}




// De interface_encomenda
// *********************************************************************************************************************
/**
 * @def GENERIC_EDIT(nome, colect, col, col_pred, editfnc, nomenew)
 *          Macro que implementa uma funcionalidade reutilizada bastantes vezes
 *           para editar uma coleção.
 *          nome - String com o nome dos objetos representados na coleção;
 *          colect - prefixo da coleçao a ser editada;
 *          col - nome da variavél da coleção;
 *          col_pred - iterador para imprimir objetos da coleção;
 *          editfnc - função de edita os objetos da coleção, com a assinatura
 *           int func(col_type* a, int isNew) que retorna 0 caso queira remover
 *           o objeto da colção - onde 'a' é o objeto - e isNew é 0 caso o
 *           artigo já exista na coleção;
 *          nomenew - nome da função que cria um novo artigo, com a assinatura
 *           col_type func().
 */
#define GENERIC_EDIT(nome, colect, col, col_pred, editfnc, nomenew)                                                    \
    int64_t id = -2;                                                                                                   \
    int64_t max;                                                                                                       \
    while (1) {                                                                                                        \
        id = -2;                                                                                                       \
        menu_printDiv();                                                                                               \
        menu_printHeader("Selecione " nome);                                                                           \
        while (id == -2) {                                                                                             \
            printf("      ID      |   Item\n");                                                                        \
            printf("         -2   |   Reimprimir\n");                                                                  \
            printf("         -1   |   Sair\n");                                                                        \
            max = 0;                                                                                                   \
            COL_EVAL(colect, _iterateFW)(&col, (COL_EVAL(colect, _pred_t)) & col_pred, &max);                          \
            printf("   %8lu   |   Criar Novo " nome "\n", max++);                                                      \
            menu_printInfo("Insira o ID do " nome " para editar");                                                     \
            id = menu_readInt64_tMinMax(-2, max - 1);                                                                  \
        }                                                                                                              \
        if (id != -1) {                                                                                                \
            if (id == max - 1) {                                                                                       \
                /* Novo, adicionar ao vetor*/                                                                          \
                protectFcnCall(COL_EVAL(colect, _push)(&col, nomenew()), #colect "_push falhou");                      \
            }                                                                                                          \
                                                                                                                       \
            /* id é o ID do cliente a editar */                                                                       \
            if (!editfnc(&col.data[id], id == max - 1)) {                                                              \
                COL_EVAL(colect, _DEALOC)(&col.data[id]);                                                              \
                COL_EVAL(colect, _moveBelow)(&col, id);                                                                \
                menu_printInfo(nome " removido.");                                                                     \
            }                                                                                                          \
        } else                                                                                                         \
            break;                                                                                                     \
    }

/**
 * @brief   Pode ser utilizado como um iterador, imprime uma encomenda.
 * @param e Encomenda a ser impressa.
 * @param i Deve ser inicializado como 0, no final irá conter o número de
 *          encomendas impressas.
 * @returns 0
 */
int pred_printEnc(encomenda const* const e, int64_t* const i) {
    printf("   %8lu   |   ", (*i)++);
    menu_printEncomendaBrief(e, &clientes, &artigos);
    printf("\n");
    return 0;
}

/**
 * @brief       Função responsável por editar todos os parametros de uma
 *              encomenda.
 * @param e     Encomenda que será editada.
 * @param isNew Deve ser 1 se a encomenda é nova e 0 se é uma encomenda válida
 *              que será editada.
 * @returns     1 Se a encomenda foi editada.
 * @returns     0 Se a encomenda não foi editada/ foi eleminada; a encomenda
 *              terá que ser eleminada pois é inválida.
 */
int form_editar_encomenda(encomenda* const e, int isNew) {
    GENERIC_EDIT("Compra", compracol, e->compras, pred_printCom, form_editar_compra, new_compra);
    if (!isNew) printf("Deseja alterar o id do cliente? (S / N)");
    if (isNew || menu_YN('S', 'N')) {
        menu_printHeader("Selecione Cliente");
        id = -2;
        while (id == -2) {
            printf("      ID      |   Item\n");
            printf("         -2   |   Reimprimir\n");
            printf("         -1   |   Sair\n");
            max = 0;
            utilizadorcol_iterateFW(&clientes, (utilizadorcol_pred_t) &pred_printUti, &max);
            menu_printInfo("Insira o ID do Cliente");
            id = menu_readInt64_tMinMax(-2, max - 1);
        }
        if (id != -1) {
            e->ID_cliente = id;
        } else
            return 1;
    }
    e->tempo = time(NULL);
    return 1;
}




// De inteface_diretor
// *********************************************************************************************************************

/**
 * @brief Premite editar clientes.
 */
void interface_editar_cliente() {
    GENERIC_EDIT("Cliente", utilizadorcol, clientes, pred_printUti, form_editar_cliente, newUtilizador);
}

/**
 * @brief Premite editar artigos.
 */
void interface_editar_artigo() {
    GENERIC_EDIT("Artigo", artigocol, artigos, pred_printArt, form_editar_artigo, newArtigo);
}

/**
 * @brief Premite editar encomendas.
 */
void interface_editar_encomenda() {
    GENERIC_EDIT("Encomenda", encomendacol, encomendas, pred_printEnc, form_editar_encomenda, newEncomenda);
}

/**
 * @brief Premite imprimir um recibo para um certo mês.
 */
void interface_imprimir_recibo() {
    printf("Inserir ano");
    int64_t ano = menu_readInt64_t();
    menu_printInfo("Inserir mês");
    int64_t mes = menu_readInt64_tMinMax(1, 12);

    int bak = dup(1);
    protectFcnCall((bak != -1), "dup falhou");
    int new               = 0;
    int printBoth         = 0;
    int needsToRestoreOut = 0;
    switch (menu_selection(&(strcol) {.size = 3,
                                      .data = (char*[]) {
                                          "Imprimir no ecrã",     // 0
                                          "Imprimir em ficheiro", // 1
                                          "Imprimir em ambos",    // 2
                                      }})) {
        case -1: return;
        case 0: break;
        case 2: printBoth = 1;
        case 1:
            needsToRestoreOut = 1;
            printf("Introduza nome de ficheiro");
            char* f = menu_readNotNulStr();
            new     = open(f, O_WRONLY | O_CREAT);
            protectFcnCall((new != -1), "open falhou");
            protectFcnCall((dup2(new, 1) != -1), "dup2 falhou");
            close(new);
            freeN(f);
            break;
    }

PRINT_BEGUIN:
    menu_printDiv();
    menu_printHeader("Recibo Mensal");

    printf("\n*** Mês do recibo: %lu/%lu\n", ano, mes);
    struct {
        int      ano;
        int      mes;
        uint64_t total;
        uint64_t art;
        uint64_t compras;
        uint64_t encomendas;
    } data;
    data.ano        = ((int) ano) - 1900;
    data.mes        = ((int) mes) - 1;
    data.total      = 0;
    data.art        = 0;
    data.compras    = 0;
    data.encomendas = 0;
    encomendacol_iterateFW(&encomendas, (encomendacol_pred_t) &pred_printencRec, &data);
    printf("*** Artigos vendidos neste mês: %ld\n", data.art);
    printf("*** Compras vendidas neste mês: %ld\n", data.compras);
    printf("*** Encomendas vendidas neste mês: %ld\n", data.encomendas);
    printf("*** Total mensal: %ld c\n", data.total);
    menu_printHeader("Final do Recibo");
    menu_printDiv();

    if (needsToRestoreOut) {
        needsToRestoreOut = 0;
        fflush(stdout);
        dup2(bak, 1);
        close(bak);
        if (printBoth) { goto PRINT_BEGUIN; }
    }
}

/**
 * @brief Listagens proporstas pelo aluno.
 */
void interface_outras_listagens() {
    // TODO: Finalizar mplementação
    while (1) {
        menu_printDiv();
        menu_printHeader("Listagens Extra");
        switch (menu_selection(&(strcol) {.size = 3,
                                          .data = (char*[]) {
                                              "Recibo individual",         // 0
                                              "Pesquisa",                  // 1
                                              "Clientes que mais gastaram" // 2
                                          }})) {
            case -1: return;
            case 0: listagem_imprimir_recibo(); break;
            case 1: listagem_procura(); break;
            case 2: listagem_utiMaisGasto(); break;
        }
    }
}




// De interface_funcionario
// *********************************************************************************************************************
/**
 * @brief Permite criar uma nova encomenda.
 */
void interface_criar_encomenda() {
    menu_printDiv();
    menu_printHeader("Adicionar Compras a Nova Encomenda");
    encomendacol_push(&encomendas, newEncomenda());
    if (!form_editar_encomenda(&encomendas.data[encomendas.size - 1], 1)) {
        freeEncomenda(&encomendas.data[encomendas.size - 1]);
        encomendacol_pop(&encomendas);
    }
}




// De interface_inicio
// *********************************************************************************************************************
/**
 * @brief As opções que remetem ao diretor.
 */
void interface_diretor() {
    int64_t i;
    while (1) {
        menu_printDiv();
        menu_printHeader("Menu de Diretor Clínico");
        switch (menu_selection(&(strcol) {.size = 6,
                                          .data = (char*[]) {
                                              "Editar/ criar cliente",   // 0
                                              "Editar/ criar artigo",    // 1
                                              "Editar/ criar encomenda", // 2
                                              "Consultar stock",         // 3
                                              "Imprimir recibo mensal",  // 4
                                              "Outras Listagens",        // 5
                                          }})) {
            case -1: return;
            case 0: interface_editar_cliente(); break;
            case 1: interface_editar_artigo(); break;
            case 2: interface_editar_encomenda(); break;
            case 3:
                menu_printDiv();
                menu_printHeader("Stock");
                i = 0;
                artigocol_iterateFW(&artigos, (artigocol_pred_t) &pred_printArt, &i);
                break;
            case 4: interface_imprimir_recibo(); break;
            case 5: interface_outras_listagens(); break;
        }
    }
}

/**
 * @brief As opções que remetem a um funcionário.
 */
void interface_funcionario() {
    int64_t i;
    while (1) {
        menu_printDiv();
        menu_printHeader("Menu de Funcionário");
        switch (menu_selection(&(strcol) {.size = 3,
                                          .data = (char*[]) {
                                              "Editar/ criar cliente",  // 0
                                              "Criar encomenda",        // 1
                                              "Imprimir recibo mensal", // 2
                                              "Consultar stock",        // 3
                                          }})) {
            case -1: return;
            case 0: interface_editar_cliente(); break;
            case 1: interface_criar_encomenda(); break;
            case 2: interface_imprimir_recibo(); break;
            case 3:
                menu_printDiv();
                menu_printHeader("Stock");
                i = 0;
                artigocol_iterateFW(&artigos, (artigocol_pred_t) &pred_printArt, &i);
                break;
        }
    }
}

/**
 * @brief Responsavél por gravar os dados em ficheiro.
 */
void funcional_save() {
    menu_printDiv();
    menu_printInfo("a escrever em ficheiro");

    // Abrir ficheiro
    FILE* dataFile;
    protectVarFcnCall(dataFile, fopen("saved_data.bin", "wb"), "ficheiro não pode ser aberto");

    // Escrever artigos
    protectFcnCall(artigocol_write(&artigos, dataFile), "impossível escrever artigos no ficheiro");

    // Escrever encomendas
    protectFcnCall(encomendacol_write(&encomendas, dataFile), "impossível escrever encomendas no ficheiro");

    // Escrever clientes
    protectFcnCall(utilizadorcol_write(&clientes, dataFile), "impossível escrever clientes no ficheiro");

    fclose(dataFile);
    menu_printInfo("ficheiro gravado");
}

/**
 * @brief Responsavél por carregar o estado de ficheiro.
 */
void funcional_load() {
    menu_printDiv();
    menu_printInfo("a carregar de ficheiro");

    // Eliminar dados
    artigocol_free(&artigos);
    encomendacol_free(&encomendas);
    utilizadorcol_free(&clientes);

    // Abrir ficheiro
    FILE* dataFile;
    protectVarFcnCall(dataFile, fopen("saved_data.bin", "rb"), "ficheiro não pode ser aberto");

    // Carregar artigos
    protectFcnCall(artigocol_read(&artigos, dataFile), "impossível carregar artigos de ficheiro");

    // Carregar encomendas
    protectFcnCall(encomendacol_read(&encomendas, dataFile), "impossível carregar encomendas de ficheiro");

    // Carregar clientes
    protectFcnCall(utilizadorcol_read(&clientes, dataFile), "impossível carregar clientes de ficheiro");

    fclose(dataFile);
    menu_printInfo("dados carregados");
}




// Entry point
// *********************************************************************************************************************
/**
 * @brief Menu inicial.
 */
void interface_inicio() {
    char* login = NULL;
    while (1) {
        switch (menu_selection(&(strcol) {.size = 3,
                                          .data = (char*[]) {
                                              "Log in",         // 0
                                              "Salvar Dados",   // 1
                                              "Carregar Dados", // 2
                                          }})) {
            case -1: return;
            case 0:
                printf("(F / DC)");
                login = menu_readNotNulStr();
                if (strcasecmp(login, "F") == 0) {
                    free(login);
                    interface_funcionario();
                } else if (strcasecmp(login, "DC") == 0) {
                    free(login);
                    interface_diretor();
                } else {
                    free(login);
                    menu_printError("Log in %s é inválido", login);
                    printf("Inserir \"F\"  para permissões de funcionário\n"
                           "Inserir \"DC\" para permissões de diretor clínico\n");
                }
                break;
            case 1: funcional_save(); break;
            case 2: funcional_load(); break;
        }
    }
}

/**
 * @brief   Ponto de entrada do programa, inicia as variáveis globais, chama
 *          interface_inicio e desaloca as globais no final.
 * @returns 0
 */
int main() {
#ifdef DEBUG_BUILD
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("DEBUG BULD\n");
    printf("COMPILADO EM: " __DATE__ " - " __TIME__ "\n");
    printf("COMPILADOR: "
#    if defined(_MSC_VER)
           "VISUAL STUDIO"
           " \nVERÇÂO DO COMPILADOR: " MACRO_QUOTE(_MSC_VER)
#    elif defined(__clang__)
           "CLANG"
           " \nVERÇÂO DO COMPILADOR: " MACRO_QUOTE(__clang_major__) "." MACRO_QUOTE(__clang_minor__) "." MACRO_QUOTE(
               __clang_patchlevel__)
#    elif defined(__MINGW32__)
           "MINGW"
           " \nVERÇÂO DO COMPILADOR: " MACRO_QUOTE(__MINGW32_MAJOR_VERSION) "." MACRO_QUOTE(__MINGW32_MINOR_VERSION)
#    elif defined(__TINYC__)
           "TINY C"
#    elif defined(__llvm__)
           "DESCONHECIDO - LLVM BACKEND"
#    elif defined(__GNUC__) && !defined(__INTEL_COMPILER)
           "GNU C COMPILER"
           " \nVERÇÂO DO COMPILADOR: " MACRO_QUOTE(__GNUC__) "." MACRO_QUOTE(__GNUC_MINOR__)
#    else
           "DESCONHECIDO"
#    endif
               "\n");
#endif

    menu_printDiv();
    menu_printHeader("A Iniciar");
    setlocale(LC_ALL, "en_US.UTF-8");
    artigos    = artigocol_new();
    encomendas = encomendacol_new();
    clientes   = utilizadorcol_new();

    interface_inicio();

    menu_printHeader("A Terminar");
    artigocol_free(&artigos);
    encomendacol_free(&encomendas);
    utilizadorcol_free(&clientes);
    menu_printDiv();

    return 0;
}