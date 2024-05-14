#include <unistd.h>
#include <limits.h>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

#include "include/XOXO_Button.h"

/* the prefix for this module is XOXO_ */

typedef struct XOXO_Tile XOXO_Tile;
struct XOXO_Tile
{
    int top_left_x;
    int top_left_y;
    // states are 'b' - blank, 'x' and 'o'
    int map_pos_x;
    int map_pos_y;

    char state;
};

typedef struct XOXO_Map XOXO_Map;
struct XOXO_Map
{
    // real map size = map_size^2 (map_size is one dimension)
    int map_size;
    // map offset from 0,0
    int map_x_offset;
    int map_y_offset;
    int tile_size;
    // gap between indivudual tiles
    int tile_gap;
    // c99 flexible array member
    XOXO_Tile tiles[];
};

typedef enum XOXO_MacroState XOXO_MacroState;
enum XOXO_MacroState
{
    XOXO_ms_Start,
    XOXO_ms_Playing,
    XOXO_ms_Paused,
    XOXO_ms_End
};

typedef struct XOXO_GameState XOXO_GameState;
struct XOXO_GameState
{
    int check_size;
    int last_chosen_x;
    int last_chosen_y;

    char player_char;
    char enemy_char;

    // either player or enemy 
    char turn_of;


    char winner;
    // logic
    bool player_acted;
    bool XOXO_game_loop_running;
    bool tied;
    // enum macro_state = main_menu 'm'/playing 'p'/finished 'f'
    char macro_state;


    // images
    SDL_Texture *x_texture;
    SDL_Texture *o_texture;
    SDL_Texture *you_won;
    SDL_Texture *you_lost;
    SDL_Texture *tie;

    // renderer
    SDL_Renderer *renderer;
    SDL_Window *window;

    // font
    TTF_Font *game_font;

    XOXO_ButtonTile XOXO_gs_buttons[2];
    // map, because the Map has FAB, it has to be last in the struct.
    XOXO_Map map;
};


// function prototypes
void mouse_click_tile_change_state(XOXO_GameState *gameState);
void XOXO_Map_set_blank(XOXO_GameState *gameState);
bool XOXO_check_for_win(XOXO_Map *map,
                   char current_player,
                   int win_conditon,
                   int start_x,
                   int start_y);
void XOXO_check_winner_and_set_gameloop(XOXO_GameState *gameState);

// functions

void XOXO_CleanMemory(XOXO_GameState *gameState)
{
    // shutdown game and unload all memmory
    SDL_DestroyTexture(gameState->o_texture);
    SDL_DestroyTexture(gameState->x_texture);
    SDL_DestroyTexture(gameState->you_lost);
    SDL_DestroyTexture(gameState->you_won);
    SDL_DestroyTexture(gameState->tie);
    SDL_DestroyTexture(gameState->XOXO_gs_buttons->XOXO_btn_texture);

    // close and destroy
    SDL_DestroyWindow(gameState->window);
    SDL_DestroyRenderer(gameState->renderer);

    // clean up
    free(gameState);

    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}

void XOXO_PresentWinner(XOXO_GameState *gameState)
{
    // message rect
    SDL_Rect messageRect = {
        212,
        180,
        200, // width
        100  // height
        };

    if (gameState->winner == 'o')
    {
        printf("O WON!");
        fflush(stdout);
        SDL_RenderCopy(gameState->renderer, gameState->you_won, NULL, &messageRect);
    }
    else if (gameState->winner == 'x')
    {
        printf("X WON!");
        fflush(stdout);
        SDL_RenderCopy(gameState->renderer, gameState->you_lost, NULL, &messageRect);
    }
    else if (gameState->winner == 'b')
    {
        printf("TIE!");
        fflush(stdout);
        SDL_RenderCopy(gameState->renderer, gameState->tie, NULL, &messageRect);
    }
    SDL_RenderPresent(gameState->renderer);    
}

bool XOXO_did_tied(XOXO_GameState *gameState)
{
    int count = 0;
    for (int i = 0; i < gameState->map.map_size * gameState->map.map_size; i++)
    {
        if (gameState->map.tiles[i].state == 'x' || 
            gameState->map.tiles[i].state == 'o') count++;
    }       

    if (count == gameState->map.map_size * gameState->map.map_size)
    {
        return true;
    }
    return false;
}


void XOXO_loadButtons(XOXO_GameState *gameState)
{
    XOXO_ButtonTile start_button = {.XOXO_btn_color = {255,255,255,255},
                                    .XOXO_btn_rect = {50,50,100,100},
                                    .XOXO_btn_font = TTF_OpenFont("font/DePixelHalbfett.ttf", 24)};

    start_button.XOXO_btn_surface = TTF_RenderText_Solid(start_button.XOXO_btn_font, "Start!", start_button.XOXO_btn_color);
    start_button.XOXO_btn_texture = SDL_CreateTextureFromSurface(gameState->renderer ,start_button.XOXO_btn_surface);
    SDL_FreeSurface(start_button.XOXO_btn_surface);
    start_button.XOXO_btn_surface = NULL;

    gameState->XOXO_gs_buttons[0] = start_button;


}

void XOXO_loadGameTextures(XOXO_GameState* gameState)
{
    SDL_Surface *surface = NULL;


    
    // Load images and create rendering textures from them
    surface = IMG_Load("images/o.png");
    if (surface == NULL)
    {
        printf("Cannot find o.png!\n\n");
        SDL_Quit();
        exit(1);
    }
    gameState->o_texture = SDL_CreateTextureFromSurface(gameState->renderer, surface);
    SDL_FreeSurface(surface);
    if (gameState->o_texture == NULL)
    {
        printf("Couldnt create the texture for gameState->o_texture.png!\n\n");
        SDL_Quit();
        exit(1);
    }

    surface = IMG_Load("images/x.png");
    if (surface == NULL)
    {
        printf("Cannot find x.png!\n\n");
        SDL_Quit();
        exit(1);
    }
    gameState->x_texture = SDL_CreateTextureFromSurface(gameState->renderer, surface);
    SDL_FreeSurface(surface);
    if (gameState->x_texture == NULL)
    {
        printf("Couldnt create the texture for gameState->x_texture.png!\n\n");
        SDL_Quit();
        exit(1);
    }

    surface = IMG_Load("images/you_lost.png");
    if (surface == NULL)
    {
        printf("Cannot find you_lost.png!\n\n");
        SDL_Quit();
        exit(1);
    }
    gameState->you_lost = SDL_CreateTextureFromSurface(gameState->renderer, surface);
    SDL_FreeSurface(surface);
    if (gameState->x_texture == NULL)
    {
        printf("Couldnt create the texture for gameState->you_lost.png!\n\n");
        SDL_Quit();
        exit(1);
    }
   
    surface = IMG_Load("images/you_won.png");
    if (surface == NULL)
    {
        printf("Cannot find you_won.png!\n\n");
        SDL_Quit();
        exit(1);
    }
    gameState->you_won = SDL_CreateTextureFromSurface(gameState->renderer, surface);
    SDL_FreeSurface(surface);
    if (gameState->x_texture == NULL)
    {
        printf("Couldnt create the texture for gameState->you_won.png!\n\n");
        SDL_Quit();
        exit(1);
    }

    surface = IMG_Load("images/tie.png");
    if (surface == NULL)
    {
        printf("Cannot find tie.png!\n\n");
        SDL_Quit();
        exit(1);
    }
    gameState->tie = SDL_CreateTextureFromSurface(gameState->renderer, surface);
    SDL_FreeSurface(surface);
    if (gameState->x_texture == NULL)
    {
        printf("Couldnt create the texture for gameState->tie.png!\n\n");
        SDL_Quit();
        exit(1);
    }

}

void XOXO_Map_set_blank(XOXO_GameState *gameState)
{
    gameState->winner = 'b';
    // init map
    for (int i = 0; i < gameState->map.map_size; i++)
    {
        for (int j = 0; j < gameState->map.map_size; j++)
        {
            gameState->map.tiles[i * gameState->map.map_size + j].state = 'b';
        }
    }

}


void XOXO_loadGameMapSetup(XOXO_GameState* gameState)
{
    // map_size was set up in main
    gameState->map.tile_gap = 12;
    gameState->map.tile_size = 64;

    // map offset from (0,0)
    gameState->map.map_x_offset = 204;
    gameState->map.map_y_offset = 124;

    // init map
    for (int i = 0; i < gameState->map.map_size; i++)
    {
        for (int j = 0; j < gameState->map.map_size; j++)
        {
            //printf("\nI+J = %d\n", i + j);
            //fflush(stdout);
            // works as it should
            // printf("%d\n",i * gameState->map.map_size + j);
            gameState->map.tiles[i * gameState->map.map_size + j].top_left_x = 
            j * 64 + gameState->map.map_x_offset + j * gameState->map.tile_gap;
            gameState->map.tiles[i * gameState->map.map_size + j].top_left_y = 
            i * 64 + gameState->map.map_y_offset + i * gameState->map.tile_gap;
            gameState->map.tiles[i * gameState->map.map_size + j].state = 'b';
            gameState->map.tiles[i * gameState->map.map_size + j].map_pos_x = j;
            gameState->map.tiles[i * gameState->map.map_size + j].map_pos_y = i;

        }
    }
}

void XOXO_setPlayers(XOXO_GameState* gameState)
{
    gameState->player_char = 'o';
    gameState->enemy_char = 'x';
}

void XOXO_loadGame(XOXO_GameState* gameState)
{
    XOXO_setPlayers(gameState);
    XOXO_loadGameTextures(gameState);
    XOXO_loadGameMapSetup(gameState);
    XOXO_loadButtons(gameState);
}

void XOXO_updateRects(XOXO_GameState* gameState)
{
    
    for (int i = 0; i < gameState->map.map_size; i++)
    {
        for (int j = 0; j < gameState->map.map_size; j++)
        {
            //printf("\nI+J = %d\n", i + j);
            //fflush(stdout);
            // works as it should
            // printf("%d\n",i * gameState->map.map_size + j);
            gameState->map.tiles[i * gameState->map.map_size + j].top_left_x = 
            i * 64 + gameState->map.map_x_offset + i * gameState->map.tile_gap;
            gameState->map.tiles[i * gameState->map.map_size + j].top_left_y = 
            j * 64 + gameState->map.map_y_offset + j * gameState->map.tile_gap;
            
        }
    }
}


void XOXO_renderMap(XOXO_GameState *gameState)
{
    SDL_SetRenderDrawColor(gameState->renderer, 216,164,127,255);
    // draw map, we are squaring it because its a square map
    // and map_size holds only one axis len
    int map_size = gameState->map.map_size;
    for (int i = 0; i < map_size*map_size; i++)
    {
        SDL_Rect tileRect = {
            gameState->map.tiles[i].top_left_x,
            gameState->map.tiles[i].top_left_y,
            gameState->map.tile_size,
            gameState->map.tile_size};
        
        if (gameState->map.tiles[i].state == 'b')
        {
            SDL_RenderFillRect(gameState->renderer, &tileRect);
            SDL_RenderDrawRect(gameState->renderer, &tileRect);
        }
        else if (gameState->map.tiles[i].state == 'o')
        {
            SDL_RenderCopy(gameState->renderer, gameState->o_texture, NULL, &tileRect);
            
        }
        else if (gameState->map.tiles[i].state == 'x')
        {
            SDL_RenderCopy(gameState->renderer, gameState->x_texture, NULL, &tileRect);
        }        
    }
}

void XOXO_renderButtons(XOXO_GameState *gameState)
{
    SDL_SetRenderDrawColor(gameState->renderer, 160,206,217,255);
    SDL_RenderFillRect(gameState->renderer, &gameState->XOXO_gs_buttons->XOXO_btn_rect);
    SDL_RenderDrawRect(gameState->renderer, &gameState->XOXO_gs_buttons->XOXO_btn_rect);

    // WIP removing this print creates a segfault
    SDL_RenderCopy(
        gameState->renderer,
        gameState->XOXO_gs_buttons[0].XOXO_btn_texture,
        NULL,
        &gameState->XOXO_gs_buttons[0].XOXO_btn_rect
        );
        
}

void XOXO_doRender(SDL_Renderer *renderer, XOXO_GameState *gameState)
{
    // render display
    SDL_SetRenderDrawColor(renderer,15,15,15,255);
    SDL_RenderClear(renderer);


    if (gameState->macro_state == XOXO_ms_Start)
    {
        XOXO_renderButtons(gameState);
    }
    else if (gameState->macro_state == XOXO_ms_Playing)
    {
        XOXO_renderMap(gameState);
    }
    else if (gameState->macro_state == XOXO_ms_Paused)
    {
        XOXO_renderMap(gameState);
        XOXO_renderButtons(gameState);
    }
    else if (gameState->macro_state == XOXO_ms_End)
    {
        XOXO_renderMap(gameState);
        XOXO_renderButtons(gameState);
    }

    SDL_RenderPresent(renderer);    

}

bool processEvents(SDL_Window *window, XOXO_GameState *gameState)
{
    SDL_Event event;
    int running = true;
    
    // game loop
    while(SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_WINDOWEVENT:
            switch (event.window.event)
            {
                case SDL_WINDOWEVENT_CLOSE:
                    if (window)
                    {
                        // gameState->macro_state = XOXO_ms_End;
                        running = false;
                    }
                    break;
                default:
                    break;
            }
        break;
        case SDL_KEYDOWN:
        {
            switch(event.key.keysym.sym)
            {
                case SDLK_ESCAPE:
                    if (gameState->macro_state == XOXO_ms_Playing)
                    {
                        gameState->macro_state = XOXO_ms_Paused;
                    }
                    else if (gameState->macro_state == XOXO_ms_Paused)
                    {
                        gameState->macro_state = XOXO_ms_Playing;
                    }
                break;
                // case SDLK_w:
                //     if (gameState->macro_state == XOXO_ms_Playing)
                //         gameState->map.map_y_offset-=20;
                // break;
                // case SDLK_s:
                //     if (gameState->macro_state == XOXO_ms_Playing)
                //         gameState->map.map_y_offset+=20; 
                // break;
                // case SDLK_a:
                //     if (gameState->macro_state == XOXO_ms_Playing)
                //         gameState->map.map_x_offset-=20; 
                // break;
                // case SDLK_d:
                //     if (gameState->macro_state == XOXO_ms_Playing)
                //         gameState->map.map_x_offset+=20; 
                // break;
            }
        }
        break;
        case SDL_MOUSEBUTTONDOWN:
        {
            if (gameState->macro_state != XOXO_ms_Paused)
            {
                mouse_click_tile_change_state(gameState);
                if (XOXO_ButtonTileClicked(&gameState->XOXO_gs_buttons->XOXO_btn_rect))
                {
                    XOXO_Map_set_blank(gameState);
                    gameState->macro_state = XOXO_ms_Playing;
                }
            }
            else
            {
                if (XOXO_ButtonTileClicked(&gameState->XOXO_gs_buttons->XOXO_btn_rect))
                {
                    XOXO_Map_set_blank(gameState);
                    gameState->macro_state = XOXO_ms_Playing;
                }
            }
        }
        break;
        case SDL_QUIT:
            running = false;
        break;
        }
    }
    return running;
}

char check_for_win_specific(XOXO_GameState *gameState, int x_pos, int y_pos, char player)
{
    if (XOXO_check_for_win(&gameState->map, player, gameState->check_size, x_pos, y_pos) == true)
    {
        return player;
    }
    else
    {
        return 'b';
    }
}

char check_for_win(XOXO_GameState *gameState, int x_pos, int y_pos)
{
    if (XOXO_check_for_win(&gameState->map, 'o', gameState->check_size, x_pos, y_pos) == true)
    {
        return 'o';
    }
    else if (XOXO_check_for_win(&gameState->map, 'x', gameState->check_size, x_pos, y_pos) == true)
    {
        return 'x';
    }
    else
    {
        return 'b';
    }
}

/**
 * @brief Check if current_player has won.
 * We go back and forth between two directions starting in the place
 * where we have placed our mark. We check all relevant direction pairs.
 * We check it for one player only.
 * @param map The current XOXO_Map object
 * @param current_player For which letter are we counting score
 * @param win_condition How much tiles have to match for player to win
 * @param pos_x x coordinate of where has the player placed its mark
 * @param pos_x y coordinate of where has the player placed its mark
 * @return true or false
 * 
 */
bool XOXO_check_for_win(XOXO_Map *map,
                   char current_player,
                   int win_conditon,
                   int start_x,
                   int start_y)
{
    // we start with one score, because we start at current players mark
    int score_count = 1;
    int offset_idx = 1;

    bool go_first_dir = 1;
    bool go_second_dir = 1;

    // visually vertical
    // left and right
    while (go_first_dir || go_second_dir)
    {
        // horizontal right guard
        if (start_x + (start_y * map->map_size) + offset_idx >
           (start_y * map->map_size) + map->map_size - 1)
        {
            go_first_dir = false;
        }
        
        // horizontal left guard
        if (start_x + (start_y * map->map_size) - offset_idx <
           (start_y * map->map_size))
        {
            go_second_dir = false;
        }

        if (go_first_dir)
        {
            if (map->tiles[start_x + (start_y * map->map_size) + offset_idx].state
            == current_player)
            {
                score_count++;
            }
            else
            {
                go_first_dir = false;
            }
        }

        if (go_second_dir)
        {
            // second dir has negative offset
            if (map->tiles[start_x + (start_y * map->map_size) - offset_idx].state
            == current_player)
            {
                score_count++;
            }
            else
            {
                go_second_dir = false;
            }
        }

        offset_idx++;
        if (score_count == win_conditon) return true;
    }

    score_count = 1;
    offset_idx = 1;

    go_first_dir = 1;
    go_second_dir = 1;

    // visually horizontal
    // up and down
    while (go_first_dir || go_second_dir)
    {
        
        // vertical up guard
        if (start_x + (start_y * map->map_size) + offset_idx > 
            map->map_size * map->map_size)
        {
            go_first_dir = false;
        }
        
        // vertical down guard
        if (start_x + (start_y * map->map_size) - offset_idx < 0)
        {
            go_second_dir = false;
        }

        if (go_first_dir)
        {
            if (map->tiles[start_x + (start_y * map->map_size) + offset_idx * map->map_size].state
            == current_player)
            {
                score_count++;
            }
            else
            {
                go_first_dir = false;
            }
        }

        if (go_second_dir)
        {
            // second dir has negative offset
            if (map->tiles[start_x + (start_y * map->map_size) - offset_idx * map->map_size].state
            == current_player)
            {
                score_count++;
            }
            else
            {
                go_second_dir = false;
            }
        }

        offset_idx++;
        if (score_count == win_conditon) return true;
    }

    score_count = 1;
    offset_idx = 1;

    go_first_dir = 1;
    go_second_dir = 1;

    // visually diagonal
    // top right to bottom left
    while (go_first_dir || go_second_dir)
    {
        // vertical up guard
        if (start_x + (start_y * map->map_size) + offset_idx * map->map_size - offset_idx > 
            map->map_size * map->map_size ||
            (start_x + (start_y * map->map_size) + offset_idx * map->map_size - offset_idx) / 
            map->map_size == 
            (start_x + (start_y * map->map_size) + (offset_idx-1) * map->map_size - (offset_idx-1))/
            map->map_size)

        {
            go_first_dir = false;
        }
        
        // vertical down guard
        if (start_x + (start_y * map->map_size) - offset_idx * map->map_size + offset_idx < 0 ||
            (start_x + (start_y * map->map_size) - offset_idx * map->map_size + offset_idx) / 
            map->map_size ==
            (start_x + (start_y * map->map_size) - (offset_idx-1) * map->map_size + (offset_idx-1))/
            map->map_size)
        {
            go_second_dir = false;
        }

        if (go_first_dir)
        {
            if (map->tiles[start_x + (start_y * map->map_size) + offset_idx * map->map_size - offset_idx].state
            == current_player)
            {
                score_count++;
            }
            else
            {
                go_first_dir = false;
            }
        }

        if (go_second_dir)
        {
            // second dir has negative offset
            if (map->tiles[start_x + (start_y * map->map_size) - offset_idx * map->map_size + offset_idx].state
            == current_player)
            {
                score_count++;
            }
            else
            {
                go_second_dir = false;
            }
        }

        offset_idx++;
        if (score_count == win_conditon) return true;
    }

    score_count = 1;
    offset_idx = 1;

    go_first_dir = 1;
    go_second_dir = 1;

    // visually diagonal
    // top left to bottom right
    while (go_first_dir || go_second_dir)
    {
        
        // vertical up guard
        if (start_x + (start_y * map->map_size) + offset_idx * map->map_size + offset_idx > 
            map->map_size * map->map_size ||
            (start_x + (start_y * map->map_size) + offset_idx * map->map_size + offset_idx) / 
            map->map_size ==
            (start_x + (start_y * map->map_size) + (offset_idx-1) * map->map_size + (offset_idx-1))/
            map->map_size
            )
        {
            go_first_dir = false;
        }
        
        // vertical down guard
        if (start_x + (start_y * map->map_size) - offset_idx * map->map_size - offset_idx < 0 ||
        (start_x + (start_y * map->map_size) - offset_idx * map->map_size - offset_idx) / 
        map->map_size == 
        (start_x + (start_y * map->map_size) - (offset_idx-1) * map->map_size - (offset_idx-1)) /
        map->map_size
        )

        {
            go_second_dir = false;
        }

        if (go_first_dir)
        {
            if (map->tiles[start_x + (start_y * map->map_size) + offset_idx * map->map_size + offset_idx].state
            == current_player)
            {
                score_count++;
            }
            else
            {
                go_first_dir = false;
            }
        }

        if (go_second_dir)
        {
            // second dir has negative offset
            if (map->tiles[start_x + (start_y * map->map_size) - offset_idx * map->map_size - offset_idx].state
            == current_player)
            {
                score_count++;
            }
            else
            {
                go_second_dir = false;
            }
        }

        offset_idx++;
        if (score_count == win_conditon) return true;
    }

    return false;
}

bool board_full(XOXO_GameState *gameState)
{
    for (int i = 0; i < gameState->map.map_size * gameState->map.map_size; i++)
    {
        if (gameState->map.tiles[i].state != 'b')
        {
            return false;
        }
    }    
    return true;
}

/**
 * @brief 0 is the leaf depth
 * 
 * @param gameState 
 * @return int depth we are currently on 
 */
int measure_depth(XOXO_GameState *gameState)
{
    int depth = 1;
    for (int i = 0; i < gameState->map.map_size * gameState->map.map_size; i++)
    {
        if (gameState->map.tiles[i].state == 'b')
        {
            depth += 1;
        }
    }    
    return depth;
}

/**
 * @brief x are minimizing, o are maximizing
 * 
 * @param winner 
 * @return int 
 */
int minimaxScoring(char winner)
{
    switch (winner)
    {
    case 'b':
        return 0;
    break;
    case 'x':
        return -1;
    break;
    case 'o':
        return 1;
    break;
    default:
        return 0;
    break;
    }
}

/**
 * @brief implementation of minimax algorithm
 * o - maximizing, x - minimizing 
 * 
 */
int minimax(XOXO_GameState *gameState, int depth, bool isMaximizing, int alpha, int beta)
{
    // traverse the board
    // if spot is available, try going there
    char player_in_turn = 'b';
    if (isMaximizing)
    {
        player_in_turn = 'x';
    }
    else
    {
        player_in_turn = 'o';
    }

    char winner = check_for_win_specific(gameState, 
                                gameState->last_chosen_x,
                                gameState->last_chosen_y,
                                player_in_turn);
    if (depth == 1 || winner != 'b')
    {
        int score = minimaxScoring(winner) * depth;
        return score;
    }
    
    if (isMaximizing)   // O turn
    {
        int best_score = INT_MIN;
        int current_score = best_score;
        for (int i = 0; i < gameState->map.map_size * gameState->map.map_size; i++)
        {
            if (gameState->map.tiles[i].state == 'b')
            {
                gameState->map.tiles[i].state = 'o';
                gameState->last_chosen_x = gameState->map.tiles[i].map_pos_x;
                gameState->last_chosen_y = gameState->map.tiles[i].map_pos_y;
                current_score = minimax(gameState, depth-1, false, alpha, beta);
                gameState->map.tiles[i].state = 'b';
                // the o is maximizing, thus >
                if (current_score > best_score)
                {
                    best_score = current_score;
                    alpha = best_score;
                    if (beta <= alpha)
                    {
                        break;
                    }
                }
            }   
        }       
        return best_score;
    }
    else                // X turn
    {
        int best_score = INT_MAX;
        int current_score = best_score;
        for (int i = 0; i < gameState->map.map_size * gameState->map.map_size; i++)
        {
            if (gameState->map.tiles[i].state == 'b')
            {
                gameState->map.tiles[i].state = 'x';
                gameState->last_chosen_x = gameState->map.tiles[i].map_pos_x;
                gameState->last_chosen_y = gameState->map.tiles[i].map_pos_y;
                current_score = minimax(gameState, depth-1, true, alpha, beta);
                gameState->map.tiles[i].state = 'b';
                // the x is minimizing, thus <
                if (current_score < best_score)
                {
                    best_score = current_score;
                    beta = best_score;
                    if (beta <= alpha)
                    {
                        break;
                    }
                }
            }   
        }
        return best_score;   
    }
}

/**
 * @brief 
 * 
 * @param gameState 
 */
void ai_make_move(XOXO_GameState *gameState)
{
    gameState->turn_of = gameState->enemy_char;
    gameState->player_acted = false;
    int depth = measure_depth(gameState);
    int best_score = INT_MAX;
    int best_score_idx = -1;
    int current_score = best_score;

    for (int i = 0; i < gameState->map.map_size * gameState->map.map_size; i++)
    {
        if (gameState->map.tiles[i].state == 'b')
        {
            gameState->map.tiles[i].state = 'x';

            gameState->last_chosen_x = gameState->map.tiles[i].map_pos_x;
            gameState->last_chosen_y = gameState->map.tiles[i].map_pos_y;
            current_score = minimax(gameState, depth-1, true, INT_MIN, INT_MAX);

            gameState->map.tiles[i].state = 'b';

            // the x is minimizing, thus <
            if (current_score < best_score)
            {
                best_score = current_score;
                best_score_idx = i;
            }
        }   
    }    
    if (best_score_idx != -1)
    {
        printf("ai_move: x: %d, y: %d\n",
                gameState->map.tiles[best_score_idx].map_pos_x,
                gameState->map.tiles[best_score_idx].map_pos_y);
        fflush(stdout);
        gameState->last_chosen_x = gameState->map.tiles[best_score_idx].map_pos_x;
        gameState->last_chosen_y = gameState->map.tiles[best_score_idx].map_pos_y;
        gameState->map.tiles[best_score_idx].state = 'x';
        XOXO_check_winner_and_set_gameloop(gameState);
    }
}

char check_if_last_chosen_a_winner(XOXO_GameState *gameState)
{
    char winner = check_for_win_specific(gameState, 
                                gameState->last_chosen_x, 
                                gameState->last_chosen_y,
                                gameState->turn_of);
    return winner;
}

void mouse_click_tile_change_state(XOXO_GameState *gameState)
{
    int x, y;
    SDL_GetMouseState(&x, &y);
    for (int i = 0; i < gameState->map.map_size * gameState->map.map_size; i++)
    {
        if (x >= gameState->map.tiles[i].top_left_x &&
            x <= gameState->map.tiles[i].top_left_x + gameState->map.tile_size &&
            y >= gameState->map.tiles[i].top_left_y &&
            y <= gameState->map.tiles[i].top_left_y + gameState->map.tile_size &&
                 gameState->map.tiles[i].state == 'b')
            {
                // mouse is inside some tile
                gameState->map.tiles[i].state = 'o';
                gameState->player_acted = true;
                gameState->last_chosen_x = gameState->map.tiles[i].map_pos_x;
                gameState->last_chosen_y = gameState->map.tiles[i].map_pos_y;
                // get idx x and idx y of this tile
                printf("last_chosen_player: x: %d, y: %d\n", gameState->last_chosen_x, gameState->last_chosen_y);
                fflush(stdout);
                XOXO_check_winner_and_set_gameloop(gameState);
                printf("last_chosen_player: x: %d, y: %d\n", gameState->last_chosen_x, gameState->last_chosen_y);
                fflush(stdout);
            }
    }
}

void XOXO_process_winner(XOXO_GameState *gameState)
{
    if (gameState->winner == 'o')
    {
        gameState->XOXO_game_loop_running = false;
        gameState->macro_state = XOXO_ms_End;
    }
    else if (gameState->winner == 'x')
    {
        gameState->XOXO_game_loop_running = false;
        gameState->macro_state = XOXO_ms_End;
    }
    else if (gameState->winner == 'b' && measure_depth(gameState) == 0)
    {
        gameState->XOXO_game_loop_running = false;
        gameState->macro_state = XOXO_ms_End;
    }
}

void XOXO_check_winner_and_set_gameloop(XOXO_GameState *gameState)
{
    gameState->winner = check_if_last_chosen_a_winner(gameState);
    XOXO_process_winner(gameState);
}

void XOXO_GameLoop(XOXO_GameState *gameState)
{
    // The window is open: enter program loop (see SDL_PollEvent)
    gameState->XOXO_game_loop_running = true;
    gameState->winner = 'b';
    while (gameState->XOXO_game_loop_running)
    {   

        // game_state_loops
        // XOXO_Start implements general config such as map_size, player char
        while (gameState->macro_state == XOXO_ms_Start)
        {   
            gameState->XOXO_game_loop_running = processEvents(gameState->window, gameState);

            XOXO_doRender(gameState->renderer, gameState);
            printf("start");
            fflush(stdout);
            if (gameState->XOXO_game_loop_running == false)
            {
                break;
            }
        }

        // XOXO_Playing implements the game itself
        while (gameState->macro_state == XOXO_ms_Playing)
        {

            printf("playing");
            fflush(stdout);
            // printf("x: %i, y: %i\n", gameState->last_chosen_x, gameState->last_chosen_y);
            // fflush(stdout);
            // process events and player input
            gameState->turn_of = gameState->player_char;
            gameState->XOXO_game_loop_running = processEvents(gameState->window, gameState);

            if (gameState->winner == 'b' && (XOXO_did_tied(gameState) == false))
            {
                if (gameState->player_acted == true) 
                {
                    ai_make_move(gameState);
                }
            }
            else
            {
                gameState->macro_state = XOXO_ms_End;
            }

            XOXO_updateRects(gameState);
            XOXO_doRender(gameState->renderer, gameState);
            if (gameState->XOXO_game_loop_running == false)
            {
                break;
            }
        }

        while (gameState->macro_state == XOXO_ms_Paused) 
        {
            printf("macro_state: paused\n");
            fflush(stdout);

            gameState->XOXO_game_loop_running = processEvents(gameState->window, gameState);
            XOXO_doRender(gameState->renderer, gameState);
            if (gameState->XOXO_game_loop_running == false)
            {
                break;
            }
        }

        // XOXO_End Handles end state and allows to exit or restart
        while (gameState->macro_state == XOXO_ms_End) 
        {
            printf("macro_state: end\n");
            fflush(stdout);

            gameState->XOXO_game_loop_running = processEvents(gameState->window, gameState);
            XOXO_PresentWinner(gameState);
            if (gameState->XOXO_game_loop_running == false)
            {
                break;
            }
        }
    }
}

int main()
{
    // setup memory for the gamestate and the map
    int tile_ammount = 3;

    // int total_memmory_reserved = sizeof(GameState); // once again tile_ammount needs to be quadratic
    XOXO_GameState *gameState = (XOXO_GameState*)calloc(1, sizeof(XOXO_GameState) + (tile_ammount * tile_ammount * sizeof(XOXO_Tile)));
    
    gameState->map.map_size = tile_ammount;
    
    SDL_Window *window = NULL;

    SDL_Renderer *renderer = NULL;

    SDL_Init(SDL_INIT_VIDEO); //SDL2 init

    if (TTF_Init() < 0)
    {
        printf("Couldn't initialize SDL TTF: %s", SDL_GetError());
        exit(1);
    }

    window = SDL_CreateWindow("Game Window",                //window title
                             SDL_WINDOWPOS_UNDEFINED,       //init x
                             SDL_WINDOWPOS_UNDEFINED,       //init y
                             640,                           //width
                             480,                           //height
                             0                              //flags
                             );


    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE | SDL_RENDERER_PRESENTVSYNC);
    gameState->renderer = renderer;
    gameState->window = window;
    gameState->check_size = 3;
    gameState->macro_state = XOXO_ms_Start;

    XOXO_loadGame(gameState);
    XOXO_GameLoop(gameState);
    XOXO_CleanMemory(gameState);
    return 0;
}