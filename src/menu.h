/**
 * @file    menu.h
 * @author  André Botelho (keyoted@gmail.com)
 * @brief   API simples para mostrar informação e capturar input do utilizador.
 * @version 1
 * @date 2020-01-01
 *
 * @copyright Copyright (c) 2020
 */

#ifndef MENU_H
#define MENU_H

#include <ctype.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "encomenda.h"
#include "utilizador.h"
#include "utilities.h"

#ifndef encomendacol_H
#    define encomendacol_H
#    define COL_TIPO encomenda
#    define COL_NOME encomendacol
#    define COL_DEALOC(X) freeEncomenda(X)
#    define COL_WRITE(X, F) save_encomenda(F, X)
#    define COL_READ(X, F) load_encomenda(F, X)
#    include "colecao.h"
#endif

#ifndef strcol_H
#    define strcol_H
#    define COL_TIPO char*
#    define COL_NOME strcol
#    define COL_DEALOC(X) free(*X)
#    define COL_WRITE(X, F) save_str(F, *X)
#    define COL_READ(X, F) load_str(F, X)
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

int64_t menu_readInt64_tMinMax(const int64_t min, const int64_t max);
int64_t menu_selection(const strcol* const itens);
char*   menu_readNotNulStr();
void    menu_printDiv();
void    menu_printError(const char* const err, ...);
void    menu_printInfo(const char* const info, ...);
void    menu_printHeader(const char* header);
void    menu_printUtilizador(const utilizador u);
void    menu_printArtigo(const artigo* const a);
void    menu_printArtigoStock(const artigo* const a);
void    menu_printCompra(const compra* const c, const artigocol* const av);
int64_t menu_readInt64_t();
int     menu_YN(const char Y, const char N);

void menu_printEncomendaBrief(const encomenda* const e, const utilizadorcol* const uv, const artigocol* const av);

#endif
