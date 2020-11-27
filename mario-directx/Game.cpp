#include <iostream>
#include <fstream>

#include "Game.h"
#include "Utils.h"

#include "IntroScene.h"
#include "MapScene.h"
#include "PlayScence.h"
#include "Resources.h"
#include "PlayZone.h"
#include "Camera.h"

CGame * CGame::__instance = NULL;

/*
	Initialize DirectX, create a Direct3D device for rendering within the window, initial Sprite library for 
	rendering 2D images
	- hInst: Application instance handle
	- hWnd: Application window handle
*/
void CGame::Init(HWND hWnd)
{
	LPDIRECT3D9 d3d = Direct3DCreate9(D3D_SDK_VERSION);

	this->hWnd = hWnd;									

	D3DPRESENT_PARAMETERS d3dpp;

	ZeroMemory(&d3dpp, sizeof(d3dpp));

	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
	d3dpp.BackBufferCount = 1;

	RECT r;
	GetClientRect(hWnd, &r);	// retrieve Window width & height 

	d3dpp.BackBufferHeight = r.bottom + 1;
	d3dpp.BackBufferWidth = r.right + 1;

	screen_height = r.bottom + 1;
	screen_width = r.right + 1;

	d3d->CreateDevice(
		D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		hWnd,
		D3DCREATE_SOFTWARE_VERTEXPROCESSING,
		&d3dpp,
		&d3ddv);

	if (d3ddv == NULL)
	{
		OutputDebugString(L"[ERROR] CreateDevice failed\n");
		return;
	}

	d3ddv->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backBuffer);

	// Initialize sprite helper from Direct3DX helper library
	D3DXCreateSprite(d3ddv, &spriteHandler);

	OutputDebugString(L"[INFO] InitGame done;\n");
}

/*
	Utility function to wrap LPD3DXSPRITE::Draw 
*/
void CGame::Draw(float x, float y, LPDIRECT3DTEXTURE9 texture, int left, int top, int right, int bottom, int alpha)
{
	float cam_x, cam_y;
	CCamera::GetInstance()->GetPosition(cam_x, cam_y);

	D3DXVECTOR3 p(x - cam_x, y - cam_y, 0);
	RECT r; 
	r.left = left;
	r.top = top;
	r.right = right;
	r.bottom = bottom;
	spriteHandler->Draw(texture, &r, NULL, &p, D3DCOLOR_ARGB(alpha, 255, 255, 255));
}

int CGame::IsKeyDown(int KeyCode)
{
	return (keyStates[KeyCode] & 0x80) > 0;
}

void CGame::InitKeyboard()
{
	HRESULT
		hr = DirectInput8Create
		(
			(HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE),
			DIRECTINPUT_VERSION,
			IID_IDirectInput8, (VOID**)&di, NULL
		);

	if (hr != DI_OK)
	{
		DebugOut(L"[ERROR] DirectInput8Create failed!\n");
		return;
	}

	hr = di->CreateDevice(GUID_SysKeyboard, &didv, NULL);

	// TO-DO: put in exception handling
	if (hr != DI_OK) 
	{
		DebugOut(L"[ERROR] CreateDevice failed!\n");
		return;
	}

	// Set the data format to "keyboard format" - a predefined data format 
	//
	// A data format specifies which controls on a device we
	// are interested in, and how they should be reported.
	//
	// This tells DirectInput that we will be passing an array
	// of 256 bytes to IDirectInputDevice::GetDeviceState.

	hr = didv->SetDataFormat(&c_dfDIKeyboard);

	hr = didv->SetCooperativeLevel(hWnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);


	// IMPORTANT STEP TO USE BUFFERED DEVICE DATA!
	//
	// DirectInput uses unbuffered I/O (buffer size = 0) by default.
	// If you want to read buffered data, you need to set a nonzero
	// buffer size.
	//
	// Set the buffer size to DINPUT_BUFFERSIZE (defined above) elements.
	//
	// The buffer size is a DWORD property associated with the device.
	DIPROPDWORD dipdw;

	dipdw.diph.dwSize = sizeof(DIPROPDWORD);
	dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
	dipdw.diph.dwObj = 0;
	dipdw.diph.dwHow = DIPH_DEVICE;
	dipdw.dwData = KEYBOARD_BUFFER_SIZE; // Arbitary buffer size

	hr = didv->SetProperty(DIPROP_BUFFERSIZE, &dipdw.diph);

	hr = didv->Acquire();
	if (hr != DI_OK)
	{
		DebugOut(L"[ERROR] DINPUT8::Acquire failed!\n");
		return;
	}

	DebugOut(L"[INFO] Keyboard has been initialized successfully\n");
}

void CGame::ProcessKeyboard()
{
	HRESULT hr; 

	// Collect all key states first
	hr = didv->GetDeviceState(sizeof(keyStates), keyStates);
	if (FAILED(hr))
	{
		// If the keyboard lost focus or was not acquired then try to get control back.
		if ((hr == DIERR_INPUTLOST) || (hr == DIERR_NOTACQUIRED))
		{
			HRESULT h = didv->Acquire();
			if (h==DI_OK)
			{ 
				DebugOut(L"[INFO] Keyboard re-acquired!\n");
			}
			else return;
		}
		else
		{
			//DebugOut(L"[ERROR] DINPUT::GetDeviceState failed. Error: %d\n", hr);
			return;
		}
	}

	keyHandler->KeyState((BYTE *)&keyStates);

	// Collect all buffered events
	DWORD dwElements = KEYBOARD_BUFFER_SIZE;
	hr = didv->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), keyEvents, &dwElements, 0);
	if (FAILED(hr))
	{
		//DebugOut(L"[ERROR] DINPUT::GetDeviceData failed. Error: %d\n", hr);
		return;
	}

	// Scan through all buffered events, check if the key is pressed or released
	for (DWORD i = 0; i < dwElements; i++)
	{
		int KeyCode = keyEvents[i].dwOfs;
		int KeyState = keyEvents[i].dwData;
		if ((KeyState & 0x80) > 0)
			keyHandler->OnKeyDown(KeyCode);
		else
			keyHandler->OnKeyUp(KeyCode);
	}
}

CGame::~CGame()
{
	if (spriteHandler != NULL) spriteHandler->Release();
	if (backBuffer != NULL) backBuffer->Release();
	if (d3ddv != NULL) d3ddv->Release();
	if (d3d != NULL) d3d->Release();
}

/*
	Standard sweptAABB implementation
	Source: GameDev.net
*/
void CGame::SweptAABB(
	float ml, float mt,	float mr, float mb,			
	float dx, float dy,			
	float sl, float st, float sr, float sb,
	float &t, float &nx, float &ny)
{

	float dx_entry, dx_exit, tx_entry, tx_exit;
	float dy_entry, dy_exit, ty_entry, ty_exit;

	float t_entry; 
	float t_exit; 

	t = -1.0f;			// no collision
	nx = ny = 0;

	//
	// Broad-phase test 
	//

	float bl = dx > 0 ? ml : ml + dx;
	float bt = dy > 0 ? mt : mt + dy;
	float br = dx > 0 ? mr + dx : mr;
	float bb = dy > 0 ? mb + dy : mb;

	if (br < sl || bl > sr || bb < st || bt > sb) return;


	if (dx == 0 && dy == 0) return;		// moving object is not moving > obvious no collision

	if (dx > 0)
	{
		dx_entry = sl - mr; 
		dx_exit = sr - ml;
	}
	else if (dx < 0)
	{
		dx_entry = sr - ml;
		dx_exit = sl- mr;
	}


	if (dy > 0)
	{
		dy_entry = st - mb;
		dy_exit = sb - mt;
	}
	else if (dy < 0)
	{
		dy_entry = sb - mt;
		dy_exit = st - mb;
	}

	if (dx == 0)
	{
		tx_entry = -999999.0f;
		tx_exit = 999999.0f;
	}
	else
	{
		tx_entry = dx_entry / dx;
		tx_exit = dx_exit / dx;
	}
	
	if (dy == 0)
	{
		ty_entry = -99999.0f;
		ty_exit = 99999.0f;
	}
	else
	{
		ty_entry = dy_entry / dy;
		ty_exit = dy_exit / dy;
	}
	

	if (  (tx_entry < 0.0f && ty_entry < 0.0f) || tx_entry > 1.0f || ty_entry > 1.0f) return;

	t_entry = max(tx_entry, ty_entry);
	t_exit = min(tx_exit, ty_exit);
	
	if (t_entry > t_exit) return; 

	t = t_entry; 

	if (tx_entry > ty_entry)
	{
		ny = 0.0f;
		dx > 0 ? nx = -1.0f : nx = 1.0f;
	}
	else 
	{
		nx = 0.0f;
		dy > 0?ny = -1.0f:ny = 1.0f;
	}

}

CGame *CGame::GetInstance()
{
	if (__instance == NULL) __instance = new CGame();
	return __instance;
}

#define MAX_GAME_LINE 1024


#define GAME_FILE_SECTION_UNKNOWN -1
#define GAME_FILE_SECTION_SETTINGS 1
#define GAME_FILE_SECTION_SCENES 2

void CGame::_ParseSection_SETTINGS(string line)
{
	vector<string> tokens = split(line);

	if (tokens.size() < 2) return;
	if (tokens[0] == "start")
		current_scene = atoi(tokens[1].c_str());
	else if (tokens[0] == "textures_path")
		CResources::GetInstance()->SetTexturesPath(ToLPCWSTR(tokens[1]));
	else if (tokens[0] == "sprites_path")
		CResources::GetInstance()->SetSpritesPath(ToLPCWSTR(tokens[1]));
	else if (tokens[0] == "animations_path")
		CResources::GetInstance()->SetAnimationsPath(ToLPCWSTR(tokens[1]));
	else if (tokens[0] == "animation_sets_path")
		CResources::GetInstance()->SetAnimationSetsPath(ToLPCWSTR(tokens[1]));
	else if (tokens[0] == "object_list")
		CResources::GetInstance()->SetGameObjectList(ToLPCWSTR(tokens[1]));
	else
		DebugOut(L"[ERROR] Unknown game setting %s\n", ToWSTR(tokens[0]).c_str());
}

void CGame::_ParseSection_SCENES(string line)
{
	vector<string> tokens = split(line);

	if (tokens.size() < 2) return;

	LPCWSTR path;
	LPCWSTR tilesetFileName;
	LPCWSTR tiledBackgroundFileName;
	LPCWSTR objectsFileName;
	LPSCENE scene;
	int world;

	int id = atoi(tokens[0].c_str());
	int scene_type = atoi(tokens[1].c_str());
	switch (scene_type)
	{
	case SCENE_TYPE_INTRO:
		path = ToLPCWSTR(tokens[2]);
		objectsFileName = ToLPCWSTR(tokens[3]);

		scene = new CIntroScene(id, path, objectsFileName);
		scenes[id] = scene;
		break;
	case SCENE_TYPE_MAP:
		world = atoi(tokens[2].c_str());
		path = ToLPCWSTR(tokens[3]);
		tilesetFileName = ToLPCWSTR(tokens[4]);
		tiledBackgroundFileName = ToLPCWSTR(tokens[5]);
		LPCWSTR mapNodeList = ToLPCWSTR(tokens[6]);

		scene = new CMapScene(id, path, tilesetFileName, tiledBackgroundFileName, mapNodeList, world);
		scenes[id] = scene;
		break;
	case SCENE_TYPE_PLAY:
		int world = atoi(tokens[2].c_str());
		path = ToLPCWSTR(tokens[3]);
		tilesetFileName = ToLPCWSTR(tokens[4]);
		tiledBackgroundFileName = ToLPCWSTR(tokens[5]);
		float tile_startX = (float)atof(tokens[6].c_str());
		float tile_startY = (float)atof(tokens[7].c_str());

		objectsFileName = ToLPCWSTR(tokens[8]);

		int currentZone = atoi(tokens[9].c_str());

		vector<CPlayZone> playZones;
		playZones.clear();

		// information of each zone in the scene
		unsigned int i = 10;
		while (i < tokens.size())
		{
			CPlayZone playZone;

			float minPixelWidth = (float)atof(tokens[i].c_str());
			float maxPixelWidth = (float)atof(tokens[i + 1].c_str());
			playZone.SetHorizontalBounds(minPixelWidth, maxPixelWidth);

			float minPixelHeight = (float)atof(tokens[i + 2].c_str());
			float maxPixelHeight = (float)atof(tokens[i + 3].c_str());
			playZone.SetVerticalBounds(minPixelHeight, maxPixelHeight);

			float marioStartingX = (float)atof(tokens[i + 4].c_str());
			float marioStartingY = (float)atof(tokens[i + 5].c_str());
			playZone.SetPlayerStartPosition(marioStartingX, marioStartingY);

			playZone.SetAllowSavingPosition(atoi(tokens[i + 6].c_str()));

			playZones.push_back(playZone);
			i += 7;
		}
		

		scene = new CPlayScene(id, path, tilesetFileName, tiledBackgroundFileName, tile_startX, tile_startY, objectsFileName, currentZone, playZones, world);
		scenes[id] = scene;
		break;
	}
	
}

/*
	Load game campaign file and load/initiate first scene
*/
void CGame::Load(LPCWSTR gameFile)
{
	DebugOut(L"[INFO] Start loading game file : %s\n", gameFile);

	ifstream f;
	f.open(gameFile);
	char str[MAX_GAME_LINE];

	// current resource section flag
	int section = GAME_FILE_SECTION_UNKNOWN;

	while (f.getline(str, MAX_GAME_LINE))
	{
		string line(str);

		if (line[0] == '#') continue;	// skip comment lines	

		if (line == "[SETTINGS]") { section = GAME_FILE_SECTION_SETTINGS; continue; }
		if (line == "[SCENES]") { section = GAME_FILE_SECTION_SCENES; continue; }

		//
		// data section
		//
		switch (section)
		{
			case GAME_FILE_SECTION_SETTINGS: _ParseSection_SETTINGS(line); break;
			case GAME_FILE_SECTION_SCENES: _ParseSection_SCENES(line); break;
		}
	}
	f.close();

	DebugOut(L"[INFO] Loading game file : %s has been loaded successfully\n",gameFile);

	CResources::GetInstance()->LoadResources();

	SwitchScene(current_scene);
}

void CGame::SwitchScene(int scene_id)
{
	DebugOut(L"[INFO] Switching to scene %d\n", scene_id);

	scenes[current_scene]->Unload();

	/*CTextures::GetInstance()->Clear();
	CSprites::GetInstance()->Clear();
	CAnimations::GetInstance()->Clear();
	CAnimationSets::GetInstance()->Clear();*/

	current_scene = scene_id;
	LPSCENE s = scenes[scene_id];
	CGame* game = CGame::GetInstance();
	game->SetKeyHandler(s->GetKeyEventHandler());

	if (dynamic_cast<CIntroScene*>(s))
	{
		game->SetBackgroundColor(BACKGROUND_COLOR_INTRO_SCENE_BEFORE);
		CHUD::GetInstance()->SetState(HUD_STATE_INTRO_SCENE);
	}
	else
	{
		game->SetBackgroundColor(BACKGROUND_COLOR_INTRO_SCENE_AFTER);
		CHUD::GetInstance()->SetState(HUD_STATE_PLAY_SCENE);
	}

	s->Load();	
}

void CGame::ChangePlayZone(int zoneID)
{
	if (dynamic_cast<CPlayScene*>(scenes[current_scene]))
	{
		CPlayScene *playScene = dynamic_cast<CPlayScene*>(scenes[current_scene]);
		playScene->ChangePlayZone(zoneID);
	}
}
