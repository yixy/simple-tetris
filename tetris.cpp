#include <iostream>
#include <thread>
#include <vector>
using namespace std;

#include <stdio.h>
#include <ncurses.h>
#include <wchar.h>
#include <stdlib.h>
#include <unistd.h>

#include <string.h>

#include "kbhit.h"

int nScreenWidth = 80;			// Console Screen Size X (columns)
int nScreenHeight = 30;			// Console Screen Size Y (rows)
wstring tetromino[7];
int nPlayFieldWidth = 12;
int nPlayFieldHeight = 18;
unsigned char *playField = nullptr;

int Rotate(int px, int py, int r){
	int pi = 0;
	switch (r % 4)
	{
	case 0: // 0 degrees			// 0  1  2  3
		pi = py * 4 + px;			// 4  5  6  7
		break;						// 8  9 10 11
									//12 13 14 15

	case 1: // 90 degrees			//12  8  4  0
		pi = 12 + py - (px * 4);	//13  9  5  1
		break;						//14 10  6  2
									//15 11  7  3

	case 2: // 180 degrees			//15 14 13 12
		pi = 15 - (py * 4) - px;	//11 10  9  8
		break;						// 7  6  5  4
									// 3  2  1  0

	case 3: // 270 degrees			// 3  7 11 15
		pi = 3 - py + (px * 4);		// 2  6 10 14
		break;						// 1  5  9 13
	}								// 0  4  8 12

	return pi;
}

bool DoesPieceFit(int nTetromino, int nRotation, int nPosX, int nPosY){
	// All Field cells >0 are occupied
	for (int px = 0; px < 4; px++)
		for (int py = 0; py < 4; py++){
			// get index into piece
			int pi = Rotate(px, py, nRotation);

			// get index into field
			int fi = (nPosY + py) * nPlayFieldWidth + (nPosX + px);

			// fi(field index) is in top-left corner of nTetromino piece.
			if (nPosX + px >= 0 && nPosX + px < nPlayFieldWidth){
				if (nPosY + py >= 0 && nPosY + py < nPlayFieldHeight){
					// In Bounds so do collision check
					if (tetromino[nTetromino][pi] != L'.' && playField[fi] != 0)
						return false; // fail on first hit
				}
			}
		}

	return true;
}

int main(){
	// Create Screen Buffer
	wchar_t *screen = new wchar_t[nScreenWidth*nScreenHeight];
	for (int i = 0; i < nScreenWidth*nScreenHeight; i++) screen[i] = L' ';

	// init ncurses
	initscr();
	noecho(); // 禁止显示输入字符
	cbreak(); // 禁用行缓冲
	curs_set(FALSE); // 隐藏光标
	keypad(stdscr, TRUE); // 开启特殊按键的读取功能
	nodelay(stdscr, TRUE); // 设置非阻塞输入模式

	// 创建一个窗口，大小与终端一样
	WINDOW *win = newwin(nScreenHeight, nScreenWidth, 0, 0);

	tetromino[0].append(L"..X...X...X...X."); 	// Tetronimos 4x4
												// ..X.
												// ..X.
												// ..X.
												// ..X.
	tetromino[1].append(L"..X..XX...X.....");
												// ..X.
												// .XX.
												// ..X.
												// ....
	tetromino[2].append(L".....XX..XX.....");
												// ....
												// .XX.
												// .XX.
												// ....
	tetromino[3].append(L"..X..XX..X......");
												// ..X.
												// .XX.
												// .X..
												// ....
	tetromino[4].append(L".X...XX...X.....");
												// .X..
												// .XX.
												// ..X.
												// ....
	tetromino[5].append(L".X...X...XX.....");
												// .X..
												// .X..
												// .XX.
												// ....
	tetromino[6].append(L"..X...X..XX.....");
												// ..X.
												// ..X.
												// .XX.
												// ....

	playField = new unsigned char[nPlayFieldWidth*nPlayFieldHeight]; // Create play field buffer
	for (int x = 0; x < nPlayFieldWidth; x++) // Board Boundary
		for (int y = 0; y < nPlayFieldHeight; y++)
			playField[y*nPlayFieldWidth + x] = (x == 0 || x == nPlayFieldWidth - 1 || y == nPlayFieldHeight - 1) ? 9 : 0;

	// Game Logic
	bool bKey[4];

	//0123456
	//nCurrentPiece+1
	//L" ABCDEFG=#"
	int nCurrentPiece = 0;	

	int nCurrentRotation = 0;
	int nCurrentX = nPlayFieldWidth / 2;
	int nCurrentY = 0;
	int nSpeed = 20;
	int nSpeedCount = 0;
	bool bForceDown = false;
	int nPieceCount = 0;
	int nScore = 0;
	vector<int> vLines;
	bool bGameOver = false;

    term_setup();
	const char* button[4]={"d","a","s","w"};
	while (!bGameOver) {// Main Loop
		// Timing =======================
		this_thread::sleep_for(50ms); // Small Step = 1 Game Tick
		nSpeedCount++;
		bForceDown = (nSpeedCount == nSpeed);

		// Input ========================
		if(kbhit()){
			for (int k = 0; k < 4; k++){
				bKey[k] = keydown(button[k]);
			}
		}else{
			memset(bKey, false, sizeof(bKey));
		}

		// Game Logic ===================

		// Handle player movement
		nCurrentX += (bKey[0] && DoesPieceFit(nCurrentPiece, nCurrentRotation, nCurrentX + 1, nCurrentY)) ? 1 : 0;
		nCurrentX -= (bKey[1] && DoesPieceFit(nCurrentPiece, nCurrentRotation, nCurrentX - 1, nCurrentY)) ? 1 : 0;
		nCurrentY += (bKey[2] && DoesPieceFit(nCurrentPiece, nCurrentRotation, nCurrentX, nCurrentY + 1)) ? 1 : 0;

		// Rotate, but latch to stop wild spinning
		if (bKey[3]){
			nCurrentRotation += DoesPieceFit(nCurrentPiece, nCurrentRotation + 1, nCurrentX, nCurrentY) ? 1 : 0;
		}

		// Force the piece down the playfield if it's time
		if (bForceDown){
			// Update difficulty every 50 pieces
			nSpeedCount = 0;
			nPieceCount++;
			if (nPieceCount % 50 == 0)
				if (nSpeed >= 10) nSpeed--;

			// Test if piece can be moved down
			if (DoesPieceFit(nCurrentPiece, nCurrentRotation, nCurrentX, nCurrentY + 1)){
				nCurrentY++; // It can, so do it!
			}else{
				// It can't! Lock the piece in place
				for (int px = 0; px < 4; px++)
					for (int py = 0; py < 4; py++)
						if (tetromino[nCurrentPiece][Rotate(px, py, nCurrentRotation)] != L'.')
							playField[(nCurrentY + py) * nPlayFieldWidth + (nCurrentX + px)] = nCurrentPiece + 1;

				// Check for lines
				for (int py = 0; py < 4; py++)
					if(nCurrentY + py < nPlayFieldHeight - 1){
						bool bLine = true;
						for (int px = 1; px < nPlayFieldWidth - 1; px++)
							bLine &= (playField[(nCurrentY + py) * nPlayFieldWidth + px]) != 0;

						if (bLine){
							// Remove Line, set to =
							for (int px = 1; px < nPlayFieldWidth - 1; px++)
								playField[(nCurrentY + py) * nPlayFieldWidth + px] = 8;
							vLines.push_back(nCurrentY + py);
						}
					}

				nScore += 25;
				if(!vLines.empty())	nScore += (1 << vLines.size()) * 100;

				// Pick New Piece
				nCurrentX = nPlayFieldWidth / 2;
				nCurrentY = 0;
				nCurrentRotation = 0;
				nCurrentPiece = rand() % 7;

				// If piece does not fit straight away, game over!
				bGameOver = !DoesPieceFit(nCurrentPiece, nCurrentRotation, nCurrentX, nCurrentY);
			}
		}

		// Display ======================

		// Draw Field
		for (int x = 0; x < nPlayFieldWidth; x++)
			for (int y = 0; y < nPlayFieldHeight; y++)
				screen[(y + 2)*nScreenWidth + (x + 2)] = L" ABCDEFG=#"[playField[y*nPlayFieldWidth + x]];

		// Draw Current Piece
		for (int px = 0; px < 4; px++)
			for (int py = 0; py < 4; py++)
				if (tetromino[nCurrentPiece][Rotate(px, py, nCurrentRotation)] != L'.')
					screen[(nCurrentY + py + 2)*nScreenWidth + (nCurrentX + px + 2)] = nCurrentPiece + 65;

		// Draw Score
		//swprintf_s(&screen[2 * nScreenWidth + nPlayFieldWidth + 6], 16, L"SCORE: %8d", nScore);
		swprintf(&screen[2 * nScreenWidth + nPlayFieldWidth + 6], 16, L"SCORE: %8d", nScore);

		// Animate Line Completion
		if (!vLines.empty())
		{
			for (int i = 0; i < nScreenWidth * nScreenHeight; i++) {
				mvwaddch(win, i / nScreenWidth, i % nScreenWidth, screen[i]);
			}

			this_thread::sleep_for(400ms); // Delay a bit

			for (auto &v : vLines)
				for (int px = 1; px < nPlayFieldWidth - 1; px++)
				{
					for (int py = v; py > 0; py--)
						playField[py * nPlayFieldWidth + px] = playField[(py - 1) * nPlayFieldWidth + px];
					playField[px] = 0;
				}

			vLines.clear();
		}

		// Display Frame
		for (int i = 0; i < nScreenWidth * nScreenHeight; i++) {
			mvwaddch(win, i / nScreenWidth, i % nScreenWidth, screen[i]);
		}

		// 刷新窗口
		wrefresh(win);
	}
    term_restore();

	// 清理ncurses库
	endwin();

	cout << "Game Over!! Score:" << nScore << endl;
	return 0;
}