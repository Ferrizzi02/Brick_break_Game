#include "address_map_arm.h"
#include <stdbool.h>


// -------- FUNÇÕES DO CÓDIGO EXEMPLO-----------------

/* function prototypes */
void video_text(int, int, char *);
void video_box(int, int, int, int, short);
int  resample_rgb(int, int);
int  get_data_bits(int);

#define STANDARD_X 320
#define STANDARD_Y 240
#define BACKGROUND_BLUE 0x2D3241
/* global variables */
int screen_x;
int screen_y;
int res_offset;
int col_offset;
int db;

// ---------------------------------------------------

/* Variaveis do jogo */

#define LARGURA_BLOCO 29
#define ALTURA_BLOCO 10

#define LARGURA_PLAYER 40
#define ALTURA_PLAYER  8
#define Y_PLAYER       195
#define VEL_PLAYER     2
#define COR_PLAYER     0xFFFFFF

short player_x;

typedef struct {
    short Type;
    short Pos_X;
    short Pos_Y;
    bool IsAlive;
    short Contador;
    int Cor;
}Bloco;

Bloco blocos[5][10];

typedef struct{
    short Type_mov;
    short Pos_X;
    short Pos_Y;
}Bola;

typedef struct {
    short Pox_X;
    short Pos_Y;
    short Type;
}Poderes;

// Instancia global da estrutura da bola
Bola bola;

// Do stackoverflow.
// Muda dinâmicamente a cor do bloco dependendo da fila
int AtualizarCor(short Pos_Y){
    short G = Pos_Y + 20;
    short B = Pos_Y + 30;
    return ((Pos_Y & 0xff) << 16) + ((G & 0xff) << 8) + (B & 0xff);
}

void EstadoInicial(){
    //Lógica dos blocos, vão ser 5*10 blocos
    short x =10; short y = 10;
    int db;

    for(int i=0; i<5; i++){
        for(int j=0; j<10; j++){
            blocos[i][j].Type = 0;
            blocos[i][j].Pos_X = x + (j * (LARGURA_BLOCO+1));
            blocos[i][j].Pos_Y = y + (i * (ALTURA_BLOCO +1));
            blocos[i][j].IsAlive = true;
            blocos[i][j].Cor = AtualizarCor(blocos[i][j].Pos_Y);
            //desenhar bloco
            int cor_Fundo = resample_rgb(db, blocos[i][j].Cor);
            video_box(blocos[i][j].Pos_X, blocos[i][j].Pos_Y, blocos[i][j].Pos_X + LARGURA_BLOCO - 1, blocos[i][j].Pos_Y + ALTURA_BLOCO - 1, cor_Fundo);
        }
    }
}

int score = 0;

//Realiza a atualização do score
void AtualizarScore(){
    char texto[5];
    texto[0] = '0' + (score / 1000) % 10;
    texto[1] = '0' + (score / 100)  % 10;
    texto[2] = '0' + (score / 10)   % 10;
    texto[3] = '0' + (score)        % 10;
    texto[4] = '\0';
    video_text(20, 55, texto);
}

void delay(int delay){
    for(volatile int i = 0; i < delay; i++);
}


// -------- FUNÇÕES DO CÓDIGO EXEMPLO-----------------

/*******************************************************************************
    Partes dessas linhas de código foram aproveitadas do exemplo fornecido pelo programa
    Não esquecer: O quadrado de fundo tem uma resolução de 600*400
    Tem um padding de 20px

 ******************************************************************************/
int main(void) {
    volatile int * video_resolution = (int *)(PIXEL_BUF_CTRL_BASE + 0x8);
    screen_x                        = *video_resolution & 0xFFFF;
    screen_y                        = (*video_resolution >> 16) & 0xFFFF;

    volatile int * rgb_status = (int *)(RGB_RESAMPLER_BASE);
    db = get_data_bits(*rgb_status & 0x3F);

    /* check if resolution is smaller than the standard 320 x 240 */
    res_offset = (screen_x == 160) ? 1 : 0;

    /* check if number of data bits is less than the standard 16-bits */
    col_offset = (db == 8) ? 1 : 0;

    /* create a message to be displayed on the video and LCD displays */
    char text_score[40]    = "SCORE\0";
    char text_pontoacao[40] = "0000\0";

    /* update color */
    short background_color = resample_rgb(db, BACKGROUND_BLUE); //faz a compressão se for necessário

    video_text(5, 55, text_score);
    video_text(20, 55, text_pontoacao);
    video_box(0, 0, STANDARD_X, STANDARD_Y, 0); // fundo preto
    video_box(10, 10, 310 - 1, 210 - 1, background_color);
    EstadoInicial();
    
    // Inicializa player centralizado
    player_x = 135;
    video_box(player_x, Y_PLAYER, player_x + LARGURA_PLAYER - 1, Y_PLAYER + ALTURA_PLAYER - 1, resample_rgb(db, COR_PLAYER));

    // Inicializa a fisica e posicao da bola acima do player caindo
    bola.Pos_X = 155;
    bola.Pos_Y = Y_PLAYER - 15;
    bola.Type_mov = 0;

    volatile int *KEY_ptr = (int *)KEY_BASE;

    // Loop principal do jogo
    while (1) {
        int botoes = *KEY_ptr;  

        // Apaga player só se vai mover
        short novo_x = player_x;    

        if (botoes & 0x1) {
            novo_x += VEL_PLAYER; // KEY0 = direita
        }
        if (botoes & 0x2) {
            novo_x -= VEL_PLAYER; // KEY1 = esquerda
        }   

        if (novo_x < 10){
            novo_x = 10;
        } 
        if (novo_x > 310 - LARGURA_PLAYER){
            novo_x = 310 - LARGURA_PLAYER; 
        }   

        // Só redesenha se moveu 
        if (novo_x != player_x) {
            // Apaga só a fatia que ficou pra trás
            if (novo_x > player_x) {
                // moveu direita: apaga a esquerda
                video_box(player_x, Y_PLAYER, novo_x - 1, Y_PLAYER + ALTURA_PLAYER - 1, resample_rgb(db, BACKGROUND_BLUE));
            } 
            else {
                // moveu esquerda: apaga a direita
                video_box(novo_x + LARGURA_PLAYER, Y_PLAYER, player_x + LARGURA_PLAYER - 1, Y_PLAYER + ALTURA_PLAYER - 1, resample_rgb(db, BACKGROUND_BLUE));
            }
            player_x = novo_x;
            video_box(player_x, Y_PLAYER, player_x + LARGURA_PLAYER - 1, Y_PLAYER + ALTURA_PLAYER - 1, resample_rgb(db, COR_PLAYER));
        }   
    

        // Apaga a bola na posicao antiga (tamanho quadrado de 4x4 pixels)
        video_box(bola.Pos_X, bola.Pos_Y, bola.Pos_X + 3, bola.Pos_Y + 3, resample_rgb(db, BACKGROUND_BLUE));

        // Movimentacao da bola dependendo do estado atual do Type_mov
        if (bola.Type_mov == 0) {
            bola.Pos_Y += 1;
        }
        if (bola.Type_mov == 1) {
            bola.Pos_Y -= 1;
        }
        if (bola.Type_mov == 2) {
            bola.Pos_X -= 1;
            bola.Pos_Y -= 1;
        }
        if (bola.Type_mov == 3) {
            bola.Pos_X += 1;
            bola.Pos_Y -= 1;
        }
        if (bola.Type_mov == 4) {
            bola.Pos_X -= 1;
            bola.Pos_Y += 1;
        }
        if (bola.Type_mov == 5) {
            bola.Pos_X += 1;
            bola.Pos_Y += 1;
        }

        // Colisao simples com as paredes do cenario
        if (bola.Pos_X < 10) {
            bola.Pos_X = 10;
            if (bola.Type_mov == 2) {
                bola.Type_mov = 3;
            }
            if (bola.Type_mov == 4) {
                bola.Type_mov = 5;
            }
        }
        if (bola.Pos_X > 306) {
            bola.Pos_X = 306;
            if (bola.Type_mov == 3) {
                bola.Type_mov = 2;
            }
            if (bola.Type_mov == 5) {
                bola.Type_mov = 4;
            }
        }
        if (bola.Pos_Y < 10) {
            bola.Pos_Y = 10;
            if (bola.Type_mov == 1) {
                bola.Type_mov = 0;
            }
            if (bola.Type_mov == 2) {
                bola.Type_mov = 4;
            }
            if (bola.Type_mov == 3) {
                bola.Type_mov = 5;
            }
        }
        // Se cair no fundo do mapa reinicia acima do player
        if (bola.Pos_Y > 208) {
            bola.Pos_X = player_x;
            bola.Pos_Y = Y_PLAYER - 15;
            bola.Type_mov = 0;
        }

        // Colisao com o player dividido em 3 partes (esquerda, meio, direita)
        if ((bola.Pos_Y + 3 >= Y_PLAYER - 1) && (bola.Pos_Y <= Y_PLAYER + ALTURA_PLAYER - 1)) {
            if ((bola.Pos_X + 3 >= player_x) && (bola.Pos_X <= player_x + LARGURA_PLAYER)) {
                if (bola.Pos_X < player_x + 14) {
                    bola.Type_mov = 2;
                }
                else if (bola.Pos_X <= player_x + 26) {
                    bola.Type_mov = 1;
                }
                else {
                    bola.Type_mov = 3;
                }
            }
        }

        // Mapeamento matematico em Hash direto na matriz de blocos sem usar loops
        short teste_x = bola.Pos_X;
        short teste_y = bola.Pos_Y;

        if (bola.Type_mov == 0) {
            teste_x = bola.Pos_X + 1;
            teste_y = bola.Pos_Y + 3;
        }
        if (bola.Type_mov == 1) {
            teste_x = bola.Pos_X + 1;
            teste_y = bola.Pos_Y;
        }
        if (bola.Type_mov == 2) {
            teste_x = bola.Pos_X;
            teste_y = bola.Pos_Y;
        }
        if (bola.Type_mov == 3) {
            teste_x = bola.Pos_X + 3;
            teste_y = bola.Pos_Y;
        }
        if (bola.Type_mov == 4) {
            teste_x = bola.Pos_X;
            teste_y = bola.Pos_Y + 3;
        }
        if (bola.Type_mov == 5) {
            teste_x = bola.Pos_X + 3;
            teste_y = bola.Pos_Y + 3;
        }

        int col_bloco = (teste_x - 10) / 30;
        int row_bloco = (teste_y - 10) / 11;

        if ((row_bloco >= 0) && (row_bloco < 5)) {
            if ((col_bloco >= 0) && (col_bloco < 10)) {
                if ((bola.Pos_X + 3 >= blocos[row_bloco][col_bloco].Pos_X) && (bola.Pos_X <= blocos[row_bloco][col_bloco].Pos_X + LARGURA_BLOCO) && (bola.Pos_Y + 3 >= blocos[row_bloco][col_bloco].Pos_Y) && (bola.Pos_Y <= blocos[row_bloco][col_bloco].Pos_Y + ALTURA_BLOCO) && (blocos[row_bloco][col_bloco].IsAlive == true)) {
                    blocos[row_bloco][col_bloco].IsAlive = false;
                    video_box(blocos[row_bloco][col_bloco].Pos_X, blocos[row_bloco][col_bloco].Pos_Y, blocos[row_bloco][col_bloco].Pos_X + LARGURA_BLOCO - 1, blocos[row_bloco][col_bloco].Pos_Y + ALTURA_BLOCO - 1, background_color);
                    score+= 10;
                    AtualizarScore();
                    if (bola.Type_mov == 0) {
                        bola.Type_mov = 1;
                    }
                    else if (bola.Type_mov == 1) {
                        bola.Type_mov = 0;
                    }
                    else if (bola.Type_mov == 2) {
                        bola.Type_mov = 4;
                    }
                    else if (bola.Type_mov == 3) {
                        bola.Type_mov = 5;
                    }
                    else if (bola.Type_mov == 4) {
                        bola.Type_mov = 2;
                    }
                    else if (bola.Type_mov == 5) {
                        bola.Type_mov = 3;
                    }
                }
            }
        }

        // Desenha a bola na nova posicao corrigida
        video_box(bola.Pos_X, bola.Pos_Y, bola.Pos_X + 3, bola.Pos_Y + 3, resample_rgb(db, COR_PLAYER));

        delay(50000); // delay para controlar a velocidade do jogo   
    }
}

/*******************************************************************************
 * Subroutine to send a string of text to the video monitor
 ******************************************************************************/
void video_text(int x, int y, char * text_ptr) {
    int             offset;
    volatile char * character_buffer =
        (char *)FPGA_CHAR_BASE; // video character buffer

    /* assume that the text string fits on one line */
    offset = (y << 7) + x;
    while (*(text_ptr)) {
        *(character_buffer + offset) =
            *(text_ptr); // write to the character buffer
        ++text_ptr;
        ++offset;
    }
}

/*******************************************************************************
 * Draw a filled rectangle on the video monitor
 * Takes in points assuming 320x240 resolution and adjusts based on differences
 * in resolution and color bits.
 ******************************************************************************/
void video_box(int x1, int y1, int x2, int y2, short pixel_color) {
    int pixel_buf_ptr = *(int *)PIXEL_BUF_CTRL_BASE;
    int pixel_ptr, row, col;
    int x_factor = 0x1 << (res_offset + col_offset);
    int y_factor = 0x1 << (res_offset);
    x1           = x1 / x_factor;
    x2           = x2 / x_factor;
    y1           = y1 / y_factor;
    y2           = y2 / y_factor;

    /* assume that the box coordinates are valid */
    for (row = y1; row <= y2; row++)
        for (col = x1; col <= x2; ++col) {
            pixel_ptr = pixel_buf_ptr +
                        (row << (10 - res_offset - col_offset)) + (col << 1);
            *(short *)pixel_ptr = pixel_color; // set pixel color
        }
}

/*******************************************************************************
 * Resamples 24-bit color to 16-bit or 8-bit color
 *******************************************************************************/
int resample_rgb(int num_bits, int color) {
    if (num_bits == 8) {
        color = (((color >> 16) & 0x000000E0) | ((color >> 11) & 0x0000001C) |
                 ((color >> 6) & 0x00000003));
        color = (color << 8) | color;
    } else if (num_bits == 16) {
        color = (((color >> 8) & 0x0000F800) | ((color >> 5) & 0x000007E0) |
                 ((color >> 3) & 0x0000001F));
    }
    return color;
}

/*******************************************************************************
 * Finds the number of data bits from the mode
 *******************************************************************************/
int get_data_bits(int mode) {
    switch (mode) {
    case 0x0:
        return 1;
    case 0x7:
        return 8;
    case 0x11:
        return 8;
    case 0x12:
        return 9;
    case 0x14:
        return 16;
    case 0x17:
        return 24;
    case 0x19:
        return 30;
    case 0x31:
        return 8;
    case 0x32:
        return 12;
    case 0x33:
        return 16;
    case 0x37:
        return 32;
    case 0x39:
        return 40;
    }
}

// ---------------------------------------------------