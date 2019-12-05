/*
Copyright <2019> <katsurakanade>

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), 
to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, 
and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "input.h"

// JoyCon�p���~�b�g
#define DEADZONE		50	
#define RANGE_MAX		1000			
#define RANGE_MIN		-1000		


static bool Initialize(HINSTANCE hInstance);
static void Finalize(void);

LPDIRECTINPUT8			g_pInput = NULL;
Keyboard keyboard;
JoyCon joycon[JOYCON_MAX];

//�L�[�{�[�h
static LPDIRECTINPUTDEVICE8	g_pDevKeyboard = NULL;
static BYTE					g_aKeyState[NUM_KEY_MAX];
static BYTE					g_aKeyStateTrigger[NUM_KEY_MAX];
static BYTE					g_aKeyStateRelease[NUM_KEY_MAX];

// Joycon��
static int					g_padCount = 0;										

bool initialize(HINSTANCE hInstance)
{

	if(g_pInput == NULL) {
		if(FAILED(DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&g_pInput, NULL))) {
			return false;
		}
	}

	return true;
}

void finalize(void)
{
	if(g_pInput != NULL) {
		g_pInput->Release();
		g_pInput = NULL;
	}
}

BOOL CALLBACK SearchGamePadCallback(LPDIDEVICEINSTANCE lpddi, LPVOID)
{
	HRESULT result;

	result = g_pInput->CreateDevice(lpddi->guidInstance, &joycon[g_padCount++].Device, NULL);

	return DIENUM_CONTINUE;
}

bool Keyboard::Initialize(HINSTANCE hInstance, HWND hWnd)
{

	try
	{
		if (!initialize(hInstance)) {
			throw "DirectInput Object Not Found ";
			return false;
		}
		if (FAILED(g_pInput->CreateDevice(GUID_SysKeyboard, &this->DevKeyboard, NULL)))
		{
			throw "Keyboard NotFound";
			return false;
		}
		if (FAILED(this->DevKeyboard->SetDataFormat(&c_dfDIKeyboard)))
		{
			throw "Keyboard DataFormat Setting Failed";
			return false;
		}
		if (FAILED(this->DevKeyboard->SetCooperativeLevel(hWnd, (DISCL_FOREGROUND | DISCL_NONEXCLUSIVE))))
		{
			throw "Keyboard CooperativeLevel Setting Failed";
			return false;
		}
	}
	catch (char *error)
	{
		MessageBox(hWnd, error, "WARNING�I", MB_ICONWARNING);
	}

	this->DevKeyboard->Acquire();

	return true;
}

void Keyboard::Finalize(void)
{
	if(DevKeyboard != NULL)
	{
		DevKeyboard->Unacquire();
		DevKeyboard->Release();
		DevKeyboard = NULL;
	}

	finalize();
}

void Keyboard::Update(void)
{
	BYTE aKeyState[NUM_KEY_MAX];

	if(SUCCEEDED(DevKeyboard->GetDeviceState(sizeof(aKeyState), aKeyState)))
	{
		for(int nCnKey = 0; nCnKey < NUM_KEY_MAX; nCnKey++)
		{
			KeyStateTrigger[nCnKey] = (KeyState[nCnKey] ^ aKeyState[nCnKey]) & aKeyState[nCnKey];
			KeyStateRelease[nCnKey] = (KeyState[nCnKey] ^ aKeyState[nCnKey]) & KeyState[nCnKey];

			KeyState[nCnKey] = aKeyState[nCnKey];
		}
	}
	else
	{
		DevKeyboard->Acquire();
	}
}

bool Keyboard::IsPress(int nKey)
{
	return (KeyState[nKey] & 0x80) ? true : false;
}

bool Keyboard::IsTrigger(int nKey)
{
	return (KeyStateTrigger[nKey] & 0x80) ? true: false;
}

bool Keyboard::IsRelease(int nKey)
{
	return (KeyStateRelease[nKey] & 0x80) ? true: false;
}

bool JoyCon::Initialize(HINSTANCE hInstance, HWND hWnd)
{

	if (!initialize(hInstance)) {

		MessageBox(hWnd, "DirectInput Object Not Found ", "WARNING�I", MB_ICONWARNING);
		return false;
	}

	HRESULT		result;

	g_pInput->EnumDevices(DI8DEVCLASS_GAMECTRL, (LPDIENUMDEVICESCALLBACK)SearchGamePadCallback, NULL, DIEDFL_ATTACHEDONLY);

	result = Device->SetDataFormat(&c_dfDIJoystick);

	for (int i = 0; i < JOYCON_MAX; i++) {
	
		if (FAILED(result))
			return false; 

		DIPROPRANGE				diprg;
		ZeroMemory(&diprg, sizeof(diprg));
		diprg.diph.dwSize = sizeof(diprg);
		diprg.diph.dwHeaderSize = sizeof(diprg.diph);
		diprg.diph.dwHow = DIPH_BYOFFSET;
		diprg.lMin = RANGE_MIN;
		diprg.lMax = RANGE_MAX;

		// X���͈̔͂�ݒ�
		diprg.diph.dwObj = DIJOFS_X;
		joycon[i].Device->SetProperty(DIPROP_RANGE, &diprg.diph);
		// Y���͈̔͂�ݒ�
		diprg.diph.dwObj = DIJOFS_Y;
		joycon[i].Device->SetProperty(DIPROP_RANGE, &diprg.diph);

		// Rz���͈̔͂�ݒ�
		diprg.diph.dwObj = DIJOFS_RZ;
		joycon[i].Device->SetProperty(DIPROP_RANGE, &diprg.diph);

		// Slider1�͈̔͂�ݒ�
		diprg.diph.dwObj = DIJOFS_SLIDER(0);
		joycon[i].Device->SetProperty(DIPROP_RANGE, &diprg.diph);

		// Slider2�͈̔͂�ݒ�
		diprg.diph.dwObj = DIJOFS_SLIDER(1);
		joycon[i].Device->SetProperty(DIPROP_RANGE, &diprg.diph);

		DIPROPDWORD				dipdw;
		dipdw.diph.dwSize = sizeof(DIPROPDWORD);
		dipdw.diph.dwHeaderSize = sizeof(dipdw.diph);
		dipdw.diph.dwHow = DIPH_BYOFFSET;
		dipdw.dwData = DEADZONE;
		//X���̖����]�[����ݒ�
		dipdw.diph.dwObj = DIJOFS_X;
		joycon[i].Device->SetProperty(DIPROP_DEADZONE, &dipdw.diph);
		//Y���̖����]�[����ݒ�
		dipdw.diph.dwObj = DIJOFS_Y;
		joycon[i].Device->SetProperty(DIPROP_DEADZONE, &dipdw.diph);

		// Rz���̖����]�[����ݒ�
		dipdw.diph.dwObj = DIJOFS_RZ;
		joycon[i].Device->SetProperty(DIPROP_DEADZONE, &dipdw.diph);

		// Slider1�̖����]�[����ݒ�
		dipdw.diph.dwObj = DIJOFS_SLIDER(0);
		joycon[i].Device->SetProperty(DIPROP_DEADZONE, &dipdw.diph);

		// Slider2�̖����]�[����ݒ�
		dipdw.diph.dwObj = DIJOFS_SLIDER(1);
		joycon[i].Device->SetProperty(DIPROP_DEADZONE, &dipdw.diph);

		joycon[i].Device->Acquire();
	}

	return true;
}

void JoyCon::Finalize()
{

	if (Device)
	{
		Device->Unacquire();
		Device->Release();
	}

	finalize();
}

void JoyCon::Update()
{
	HRESULT			result;
	DIJOYSTATE2		dijs;

		DWORD lastPadState;
		lastPadState = State;
		State = 0x00000000l;

		result = Device->Poll();

		if (FAILED(result)) {
			result = Device->Acquire();
			while (result == DIERR_INPUTLOST)
				result = Device->Acquire();
		}

		result = Device->GetDeviceState(sizeof(DIJOYSTATE), &dijs);	
		if (result == DIERR_INPUTLOST || result == DIERR_NOTACQUIRED) {
			result = Device->Acquire();
			while (result == DIERR_INPUTLOST)
				result = Device->Acquire();
		}

		// JoyCon
		if (dijs.lY > 0) State |= JOYCON_STICK_UP;
		if (dijs.lY < 0) State |= JOYCON_STICK_DOWN;
		if (dijs.lX < 0) State |= JOYCON_STICK_LEFT;
		if (dijs.lX > 0) State |= JOYCON_STICK_RIGHT;
		
		if (dijs.rgbButtons[0] & 0x80)	State |= JOYCON_DOWN;
		if (dijs.rgbButtons[1] & 0x80)	State |= JOYCON_UP;
		if (dijs.rgbButtons[2] & 0x80)	State |= JOYCON_RIGHT;
		if (dijs.rgbButtons[3] & 0x80)	State |= JOYCON_LEFT;
		if (dijs.rgbButtons[4] & 0x80)State |= JOYCON_SR_LEFT;
		if (dijs.rgbButtons[5] & 0x80)	State |= JOYCON_SL_LEFT;
		if (dijs.rgbButtons[6] & 0x80) State |= JOYCON_L;
		if (dijs.rgbButtons[7] & 0x80)	State |= JOYCON_ZL;
		if (dijs.rgbButtons[8] & 0x80)	State |= JOYCON_MIN;
		if (dijs.rgbButtons[11] & 0x80) State |= JOYCON_L3;
		if (dijs.rgbButtons[13] & 0x80)	State |= JOYCON_SCREENSHOT;
		if (dijs.rgbButtons[16] & 0x80)	State |= JOYCON_Y;
		if (dijs.rgbButtons[17] & 0x80)	State |= JOYCON_X;
		if (dijs.rgbButtons[18] & 0x80)	State |= JOYCON_B;
		if (dijs.rgbButtons[19] & 0x80)	State |= JOYCON_A;
		if (dijs.rgbButtons[20] & 0x80)	State |= JOYCON_SR_RIGHT;
		if (dijs.rgbButtons[21] & 0x80)	State |= JOYCON_SL_RIGHT;
		if (dijs.rgbButtons[22] & 0x80)	State |= JOYCON_R;
		if (dijs.rgbButtons[23] & 0x80)	State |= JOYCON_ZR;
		if (dijs.rgbButtons[25] & 0x80)	State |= JOYCON_PLUS;
		if (dijs.rgbButtons[26] & 0x80)	State |= JOYCON_R3;
		if (dijs.rgbButtons[28] & 0x80)	State |= JOYCON_HOME;

		// �����x�ݒ�
		SetAccelerometer(dijs.lRz);
		// �W���C���ݒ�
		SetGyro(dijs.lRz, dijs.rglSlider[0], dijs.rglSlider[1]);

		 // Trigger�ݒ�
		Trigger = ((lastPadState ^ State)	& State);				
	
}

void JoyCon::SetAccelerometer(DWORD data) {
	Accelerometer = data;
}

void JoyCon::SetGyro(DWORD x, DWORD y, DWORD z) {
	Gyro[0] = x;
	Gyro[1] = y;
	Gyro[2] = z;
}

int JoyCon::GetGyro_X() {
	return (int)Gyro[0];
}

int JoyCon::GetGyro_Y() {
	return (int)Gyro[1];
}

int JoyCon::GetGyro_Z() {
	return (int)Gyro[2];
}

int JoyCon::GetAccelerometer() {
	return Accelerometer;
}

int JoyCon::GetTrigger() {
	return Trigger;
}

BOOL JoyCon::IsPress(DWORD button)
{
	return (button & State);
}

BOOL JoyCon::IsTrigger(DWORD button)
{
	return (button & Trigger);
}

