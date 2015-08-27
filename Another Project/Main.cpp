#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <iostream>
#include <d3d11.h>
#include <d3dx11.h>
#include <d3dx10.h>

#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "d3dx11.lib")
#pragma comment (lib, "d3dx10.lib")
#define SCR_W 800
#define SCR_H 600
HWND hWnd = NULL;

IDXGISwapChain * swapchain; // swap chain pointer (swap chain is a series of buffers which get switched in and out)
ID3D11Device * dev; // device pointer (a device is a representation of the gpu and manages the vram)
ID3D11DeviceContext * devcon; // device context pointer (device context manages the gpu and the rendering pipeline(we use this to render))
ID3D11RenderTargetView * backbuffer; //back buffer target 
ID3D11InputLayout * pLayout; //input layout pointer
ID3D11VertexShader * pVS; //vertex shader pointer
ID3D11PixelShader * pPS; //pixel shader pointer
ID3D11Buffer * pVBuffer; //vertex buffer pointer

LRESULT CALLBACK WinProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam); //here we prototype the winproc function so we can call it in WinMain before defining it
bool MakeWindow(HINSTANCE hInstance, bool show, bool windowed, int nShowCmd);
int MessageLoop();
bool InitD3D(HINSTANCE hInstance);
void RenderFrame(void);
void CleanD3D(void);
bool InitGraphics(void);

void RenderFrame()
{
	D3DXCOLOR bgColor(1.0f, 1.0f, 1.0f, 1.0f);
	devcon->ClearRenderTargetView(backbuffer, bgColor);
	swapchain->Present(0, 0);
}

void CleanD3D()
{
	swapchain->Release();
	dev->Release();
	devcon->Release();
}

bool InitD3D(HINSTANCE hInstance)
{
	HRESULT hr;

	//Describe our Buffer
	DXGI_MODE_DESC bufferDesc;

	ZeroMemory(&bufferDesc, sizeof(DXGI_MODE_DESC));

	bufferDesc.Width = SCR_W;
	bufferDesc.Height = SCR_H;
	bufferDesc.RefreshRate.Numerator = 60;
	bufferDesc.RefreshRate.Denominator = 1;
	bufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	bufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	bufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	DXGI_SWAP_CHAIN_DESC scd; //struct for swap chain info
							  //clearing the memory for the struct just like the wndclassex in winmain
	ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));

	scd.BufferCount = 1;  // one buffer
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // 32 bit color
	scd.BufferDesc.Width = SCR_W; //self explanatory
	scd.BufferDesc.Height = SCR_H; //self explanatory
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // how to use the swap chain (this one is for drawing to the back buffer)
	scd.OutputWindow = hWnd; // window to use, this is from the argument
	scd.SampleDesc.Count = 4; // how many multisamples (antialiasing)
	scd.Windowed = TRUE; //windowed or fullscreen
	scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; //allow switch to/from fullscreen using alt+enter

														//create a device, device context and swap chain
	hr = D3D11CreateDeviceAndSwapChain(NULL, //which graphics adapter to use? NULL lets DXGI decide
		D3D_DRIVER_TYPE_HARDWARE, //how to render
		NULL, //something about software rendering
		NULL, //some flags (multi/singlethread, debug, reference rendering, direct2d)
		NULL, //if you need the end user to have certain hardware capabilities, this is where you set that
		NULL, //how many did you have in your previous list you scrub
		D3D11_SDK_VERSION, //eh
		&scd, //swap chain struct
		&swapchain, //swap chain object
		&dev, //device object
		NULL, //feature level variable
		&devcon); //device context object

	if (FAILED(hr))
	{
		return false;
	}
				  //get backbuffer address
	ID3D11Texture2D * pBackBuffer;
	hr = swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
	if (FAILED(hr))
	{
		return false;
	}

	//create a render target using the back buffer address
	hr = dev->CreateRenderTargetView(pBackBuffer, NULL, &backbuffer);
	if (FAILED(hr))
	{
		return false;
	}
	pBackBuffer->Release();
	//set the render target as the back buffer
	devcon->OMSetRenderTargets(1, &backbuffer, NULL);

	//set the viewport
	D3D11_VIEWPORT viewport;
	ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));

	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = SCR_W;
	viewport.Height = SCR_H;

	devcon->RSSetViewports(1, &viewport);
	return true;
}

int MessageLoop() 
{
	MSG msg = { 0 };

	//wait for a messasge, store it in msg
	while (TRUE)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			//translate keystroke messages
			TranslateMessage(&msg);

			//send the message to WinProc
			DispatchMessage(&msg);

			if (msg.message == WM_QUIT)
				break;
		}
		else
		{
			RenderFrame();
		}
	}

	return msg.wParam;
}

bool MakeWindow(HINSTANCE hInstance, bool show, bool windowed, int nShowCmd)
{
	WNDCLASSEX wc; //wndclassex is a struct containing lots of variables concerning the window class
	ZeroMemory(&wc, sizeof(WNDCLASSEX)); //this is a function built into windows that will clear all memory starting from the address of wc and ending at the total size it will encompass

										 //now we will set all the variables within wc - we can now do this because we cleared memory for it
	wc.cbSize = sizeof(WNDCLASSEX); //cbSize is simply the size of the structure, so naturally we set this to the size of the strucutre.
	wc.style = CS_HREDRAW | CS_VREDRAW; //these are enums for window style in windows. the ones chosen cause the window to redraw itself if it is resized on either axis
	wc.lpfnWndProc = WinProc; // lpfnWndProc wants a pointer to the function that handles messages. we already prototyped this above
	wc.hInstance = hInstance; //this just wants the instance handle for our program, which we get from the arguments of WinMain
	wc.hCursor = LoadCursor(NULL, IDC_ARROW); //this sets the mouse cursor to a default
	wc.lpszClassName = "WindowClass1"; //this should be a pointer to a null terminated string (typing a string "like this" does that for us.) this specifies the window class name.

	if (!RegisterClassEx(&wc)) //here we register our window class using our previously defined WNDCLASSEX struct - if we find an error we will tell WinMain and it will end with an error
	{
		return false;
	}
	RECT wr = { 0,0,SCR_W,SCR_H };
	AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

	hWnd = CreateWindowEx(NULL, "WindowClass1", "Program", WS_OVERLAPPEDWINDOW, 300, 300, wr.right - wr.left, wr.bottom - wr.top, NULL, NULL, hInstance, NULL);
	//extended window style, window class name, window name, window style, x position, y position, width, height, parent window, menu (popup), instance handle, used for multiple windows
	if (!hWnd) //now we see if the window exists
	{
		return false;
	}

	ShowWindow(hWnd, nShowCmd);

	return true;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	//make a window
	if (!MakeWindow(hInstance, true, false, nShowCmd))
	{
		MessageBox(0, "Oh man, you broke something. ERROR: The window failed to be created.", "Error", MB_OK);
		return 0;
	}

	if (!InitD3D(hInstance))
	{
		MessageBox(0, "Oh man, you broke something. ERROR: Direct3D failed to initialise.", "Error", MB_OK);
		return 0;
	}

	//now that we have a window we can go into the message loop

	MessageLoop();

	return 0;
}

//this is the message handler
LRESULT CALLBACK WinProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	//switch statement for finding what the message was
	switch (message)
	{
	case WM_DESTROY:
		PostQuitMessage(0); //rip program 2k15
		return 0;
		break;
	default:
		//windows i don't want this message please go away
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
}


/*#include <windows.h>
#include <windowsx.h>
#include <d3d11.h>
#include <d3dx11.h>
#include <d3dx10.h>

#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "d3dx11.lib")
#pragma comment (lib, "d3dx10.lib")
#define SCR_W 1920
#define SCR_H 1080

IDXGISwapChain * swapchain; // swap chain pointer (swap chain is a series of buffers which get switched in and out)
ID3D11Device * dev; // device pointer (a device is a representation of the gpu and manages the vram)
ID3D11DeviceContext * devcon; // device context pointer (device context manages the gpu and the rendering pipeline(we use this to render))
ID3D11RenderTargetView * backbuffer;
ID3D11InputLayout * pLayout; //input layout pointer
ID3D11VertexShader * pVS; //vertex shader pointer
ID3D11PixelShader * pPS; //pixel shader pointer
ID3D11Buffer * pVBuffer; //vertex buffer pointer
float r = 1.0;
float g = 1.0;
float b = 1.0;
float a = 1.0;

//a struct for a single vertex
struct VERTEX { FLOAT X, Y, Z; D3DXCOLOR Color; };

void InitD3D(HWND HWnd); //prototype for the function that initialises direct3d
void RenderFrame(void); //prototype for rendering a frame
void CleanD3D(void); //prototype for function that closes direct3d and releases its memory
void InitGraphics(void); //prototype for creating a shape for rendering
void InitPipeline(void); //loads shaders for the rendering pipeline
						 //filling this with comments makes me feel really awkward when I have to commit them to github
						 //hi github

void InitD3D(HWND hWnd)
{
	DXGI_SWAP_CHAIN_DESC scd; //struct for swap chain info
							  //clearing the memory for the struct just like the wndclassex in winmain
	ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));

	scd.BufferCount = 1;  // one buffer
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // 32 bit color
	scd.BufferDesc.Width = SCR_W; //self explanatory
	scd.BufferDesc.Height = SCR_H; //self explanatory
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // how to use the swap chain (this one is for drawing to the back buffer)
	scd.OutputWindow = hWnd; // window to use, this is from the argument
	scd.SampleDesc.Count = 4; // how many multisamples (antialiasing)
	scd.Windowed = TRUE; //windowed or fullscreen
	scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; //allow switch to/from fullscreen using alt+enter

														//create a device, device context and swap chain
	D3D11CreateDeviceAndSwapChain(NULL, //which graphics adapter to use? NULL lets DXGI decide
		D3D_DRIVER_TYPE_HARDWARE, //how to render
		NULL, //something about software rendering
		NULL, //some flags (multi/singlethread, debug, reference rendering, direct2d)
		NULL, //if you need the end user to have certain hardware capabilities, this is where you set that
		NULL, //how many did you have in your previous list you scrub
		D3D11_SDK_VERSION, //eh
		&scd, //swap chain struct
		&swapchain, //swap chain object
		&dev, //device object
		NULL, //feature level variable
		&devcon); //device context object

				  //get backbuffer address
	ID3D11Texture2D * pBackBuffer;
	swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);

	//create a render target using the back buffer address
	dev->CreateRenderTargetView(pBackBuffer, NULL, &backbuffer);
	pBackBuffer->Release();
	//set the render target as the back buffer
	devcon->OMSetRenderTargets(1, &backbuffer, NULL);

	//set the viewport
	D3D11_VIEWPORT viewport;
	ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));

	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = SCR_W;
	viewport.Height = SCR_H;

	devcon->RSSetViewports(1, &viewport);
	InitPipeline();
	InitGraphics();
}

void RenderFrame(float r, float b, float g, float a)
{
	//clear the back buffer by filling it with blue
	devcon->ClearRenderTargetView(backbuffer, D3DXCOLOR(r, g, b, a));

	//select which vertex buffer to display
	UINT stride = sizeof(VERTEX);
	UINT offset = 0;
	devcon->IASetVertexBuffers(0, 1, &pVBuffer, &stride, &offset);
	//what primitive type are we using
	devcon->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//draw the vertex buffer to the back buffer
	devcon->Draw(3, 0);

	//switch the front buffer and back buffer
	swapchain->Present(0, 0);
}

void InitGraphics()
{
	//create a triangle using VERTEX
	VERTEX OurVertices[] =
	{
		{ 0.0f, 0.5f, 0.0f, D3DXCOLOR(1.0f, 0.0f, 0.0f, 1.0f) },
		{ 0.45f, -0.5f, 0.0f, D3DXCOLOR(0.0f, 1.0f, 0.0f, 1.0f) },
		{ -0.45f, -0.5f, 0.0f, D3DXCOLOR(0.0f, 0.0f, 1.0f, 1.0f) }
	};

	//create the vertex buffer
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd)); //clear out memory for the vertex buffer

	bd.Usage = D3D11_USAGE_DYNAMIC; //write access by CPU and GPU
	bd.ByteWidth = sizeof(VERTEX) * 3; //size of three vertex structs
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER; // use as a vertex buffer
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; //allow cpu to write in buffer

	dev->CreateBuffer(&bd, NULL, &pVBuffer);

	//copy vertices into buffer
	D3D11_MAPPED_SUBRESOURCE ms;
	devcon->Map(pVBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms); //map buffer
	memcpy(ms.pData, OurVertices, sizeof(OurVertices)); //copy data to buffer
	devcon->Unmap(pVBuffer, NULL); //unmap buffer
}

void InitPipeline()
{
	//load the two shaders
	ID3D10Blob *VS, *PS;
	D3DX11CompileFromFile("shaders.shader", 0, 0, "VShader", "vs_4_0", 0, 0, 0, &VS, 0, 0);
	D3DX11CompileFromFile("shaders.shader", 0, 0, "PShader", "ps_4_0", 0, 0, 0, &PS, 0, 0);

	//encapsulate both shaders into objects
	dev->CreateVertexShader(VS->GetBufferPointer(), VS->GetBufferSize(), NULL, &pVS);
	dev->CreatePixelShader(PS->GetBufferPointer(), PS->GetBufferSize(), NULL, &pPS);

	//set shader objects
	devcon->VSSetShader(pVS, 0, 0);
	devcon->PSSetShader(pPS, 0, 0);

	//create input layout object
	D3D11_INPUT_ELEMENT_DESC ied[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	dev->CreateInputLayout(ied, 2, VS->GetBufferPointer(), VS->GetBufferSize(), &pLayout);
	devcon->IASetInputLayout(pLayout);
}

void CleanD3D()
{
	swapchain->SetFullscreenState(FALSE, NULL);    //get out of fullscreen before closing to avoid issues
	pLayout->Release();
	pVS->Release();
	pPS->Release();
	pVBuffer->Release();
	swapchain->Release();
	backbuffer->Release();
	dev->Release();
	devcon->Release();
}


LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam); //here we prototype the windowproc function so we can call it in WinMain before defining it

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	HWND hWnd; //create a handle to a window - we will fill this later on
	WNDCLASSEX wc; //wndclassex is a struct containing lots of variables concerning the window class
	ZeroMemory(&wc, sizeof(WNDCLASSEX)); //this is a function built into windows that will clear all memory starting from the address of wc and ending at the total size it will encompass

										 //now we will set all the variables within wc - we can now do this because we cleared memory for it
	wc.cbSize = sizeof(WNDCLASSEX); //cbSize is simply the size of the structure, so naturally we set this to the size of the strucutre.
	wc.style = CS_HREDRAW | CS_VREDRAW; //these are enums for window style in windows. the ones chosen cause the window to redraw itself if it is resized on either axis
	wc.lpfnWndProc = WindowProc; // lpfnWndProc wants a pointer to the function that creates windows. we already prototyped this above
	wc.hInstance = hInstance; //this just wants the instance handle for our program, which we get from the arguments of WinMain
	wc.hCursor = LoadCursor(NULL, IDC_ARROW); //this sets the mouse cursor to a default
											  //wc.hbrBackground = (HBRUSH)COLOR_WINDOW; //a handle to the background brush (background of our window) there are a large number of HBRUSH types available
	wc.lpszClassName = "WindowClass1"; //this should be a pointer to a null terminated string (typing a string "like this" does that for us.) this specifies the window class name.

	RegisterClassEx(&wc); //here we register our window class using our previously defined WNDCLASSEX struct

	RECT wr = { 0,0,SCR_W,SCR_H };
	AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

	hWnd = CreateWindowEx(NULL, "WindowClass1", "Our First Window", WS_OVERLAPPEDWINDOW, 300, 300, wr.right - wr.left, wr.bottom - wr.top, NULL, NULL, hInstance, NULL);
	//extended window style, window class name, window name, window style, x position, y position, width, height, parent window, menu (popup), instance handle, used for multiple windows

	ShowWindow(hWnd, nShowCmd); //put our window on the screen

	InitD3D(hWnd);

	//now that we have a window we can go into the main loop

	//msg is a struct that holds windows events
	MSG msg = { 0 };

	//wait for a messasge, store it in msg
	while (TRUE)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			//translate keystroke messages
			TranslateMessage(&msg);

			//send the message to WindowProc
			DispatchMessage(&msg);

			if (msg.message == WM_QUIT)
				break;
		}
		RenderFrame(r, g, b, a);
	}

	CleanD3D();

	return msg.wParam;
}

//this is the message handler
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	//switch statement for finding what the message was
	switch (message)
	{
	case WM_DESTROY: //this is when the window is closed
	{
		//close the application
		PostQuitMessage(0);
		return 0;
	} break;
	}

	//handle any messages the switch statement aws not able to handle
	return DefWindowProc(hWnd, message, wParam, lParam);
}*/