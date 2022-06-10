///////////////////////////////////////////////////////////////////////////////
// Headers.

#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include "system.h"
#include "chesspieces.h"
#include "chess_logic.h"
#include "joystick.h"
#include "emulator.h"
#include <pthread.h>

int main(void) {
////////////////////////////////////////////////////////////////////////////////
// Setup.
	//pthread_t input;
	//int iret1 = pthread_create(&input, NULL, shield_input, (void*)&joystick);
	//pthread_detach(input);

	int device = open("/dev/ttyUSB0", O_RDWR | O_NONBLOCK | O_NOCTTY);
 	
	set_interface_attribs (device, B38400, 0);  

	printf("joystick connection open at %i device\n", device);

	gpu_p32[0] = 3; // RGB333 mode.
	gpu_p32[0x800] = 0x00ff00ff; // Magenta for HUD.

	// Game state.
	game_state_t* gs = setup_game();
	joystick_t joystick;

	int pickup_piece = -1;
	int key_pressed = 0;
	point_t pos_prev;
	color_type player_to_move = WHITE;

	int frame_counter = 0;

	while(1){
		/////////////////////////////////////
		// Poll controls.
		shield_input(&joystick, device);	

		int a = joystick.buttons & 0x1;
		int b = (joystick.buttons & 0x2) >> 1;
		int c = (joystick.buttons & 0x4) >> 2;
		int d = (joystick.buttons & 0x8) >> 3;
		//printf("%x %i %i %i %i %i %i\n", joystick.magic, a, b, c, d, joystick.x, joystick.y);
		 
		int mov_x = 0;
		int mov_y = 0;
		
		// ADC logic 
		if(joystick.x < -THRESHOLD)
		{
			mov_x = -1;
		}
		if(joystick.x > THRESHOLD)
		{
			mov_x = 1;
		}
		if(joystick.y < -THRESHOLD)
		{
			mov_y = 1;
		}
		if(joystick.y > THRESHOLD)
		{
			mov_y = -1;
		}

		chess_piece_t* chesspieces;
		/////////////////////////////////////
		// Gameplay.
		
		if(frame_counter == 5)
		{
			if(gs->p1.x + mov_x*STEP < gs->chessboard_offset[0])
			{
				gs->p1.x = gs->chessboard_offset[0];
			}
			else if(gs->p1.x + mov_x*STEP < gs->chessboard_offset[0] + CHESSBOARD - SQ_A)
			{
				gs->p1.x += mov_x*STEP;
			}
			else
			{
				gs->p1.x = gs->chessboard_offset[0] + CHESSBOARD - SQ_A;
			}
			if(gs->p1.y + mov_y*STEP < gs->chessboard_offset[1])
			{
				gs->p1.y = gs->chessboard_offset[1];
			}
			else if(gs->p1.y + mov_y*STEP < gs->chessboard_offset[1] + CHESSBOARD - SQ_A)
			{
				gs->p1.y += mov_y*STEP;
			}
			else
			{
				gs->p1.y = gs->chessboard_offset[1] + CHESSBOARD - SQ_A;
			}

			//key_pressed = 0;

			if(player_to_move == WHITE)
				chesspieces = gs->white_pieces;
			else
				chesspieces = gs->black_pieces;

			if(joypad.a && pickup_piece == -1)
			{
				for(uint8_t i = 0; i < PIECE_NUM / 2; i++)
				{
					if(chesspieces[i].pos.x == gs->p1.x && chesspieces[i].pos.y == gs->p1.y && chesspieces[i].c == player_to_move)
					{
						pickup_piece = i;
						pos_prev.x = chesspieces[i].pos.x;
						pos_prev.y = chesspieces[i].pos.y;
						break;
					}
				}
			}
			if(joypad.a && pickup_piece > -1)
			{
				chesspieces[pickup_piece].pos.x = gs->p1.x;
				chesspieces[pickup_piece].pos.y = gs->p1.y;
				
			}
			if(joypad.a == 0 && pickup_piece > -1)
			{

				// tests for valid moves goes here!
				// test 1 no overlap of pieces!
				// TODO expand this test to allow for captures!
				for(uint8_t i = 0; i < PIECE_NUM / 2; i++)
				{
					if(chesspieces[i].pos.x == chesspieces[pickup_piece].pos.x && 
							chesspieces[i].pos.y == chesspieces[pickup_piece].pos.y && 
							i != pickup_piece)
					{
						printf("Invalid move, pieces can't overlap.\n");
						chesspieces[i].pos.x = pos_prev.x;
						chesspieces[i].pos.y = pos_prev.y;
						break;
					}
				}

				switch(chesspieces[pickup_piece].p)
				{
					case KING:
						break;
					case QUEEN:
						break;
					case ROOK:
						break;
					case BISHOP:
						break;
					case KNIGHT:
						break;
					case PAWN:
						if(validate_pawn(gs, chesspieces[pickup_piece], pos_prev) != 0)
						{
							chesspieces[pickup_piece].pos.x = pos_prev.x;
							chesspieces[pickup_piece].pos.y = pos_prev.y;
						}
						break;
				}

				pos_prev.x = 0;
				pos_prev.y = 0;
				pickup_piece = -1;
				player_to_move *= -1;
			}
			frame_counter = 0;
		}

		/////////////////////////////////////
		// Drawing.
		frame_counter++;
		
		// Detecting rising edge of VSync.
		WAIT_UNITL_0(gpu_p32[2]);
		WAIT_UNITL_1(gpu_p32[2]);
		// Draw in buffer while it is in VSync.
		
		draw_background();
		draw_chessboard(gs);
		draw_player_cursor(gs, 0x000d);

		for(uint16_t i = 0; i < PIECE_NUM / 2; i++)
		{
			// Draw chesspiece
			draw_sprite(chess_sprites__p, 10, 10, gs->white_pieces[i].pos.x, gs->white_pieces[i].pos.y, gs->white_pieces[i].atlas.x, gs->white_pieces[i].atlas.y);
		}
		for(uint16_t i = 0; i < PIECE_NUM / 2; i++)
		{
			// Draw chesspiece
			draw_sprite(chess_sprites__p, 10, 10, gs->black_pieces[i].pos.x, gs->black_pieces[i].pos.y, gs->black_pieces[i].atlas.x, gs->black_pieces[i].atlas.y);
		}
	}

	free(gs);

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
