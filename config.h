#pragma once

// Convert preprocessor macro to string
#define xstr(s) strify(s)
#define strify(s) #s

#define RootDir "../../"
#define ShaderDir RootDir "shaders/"
#define FontDir RootDir "fonts/"
#define TextureDir RootDir "textures/"
#define LevelDir RootDir "levels/"

#ifdef RTMP_STREAM
#define STREAM_PROTOCOL "rtmp://"
#define STREAM_ADDRESS "127.0.0.1"
#define RTMP_PATH "/live/test"
#define STREAM_PATH STREAM_PROTOCOL STREAM_ADDRESS RTMP_PATH
#else // RTMP_STREAM
#define UDP_STREAM
#define STREAM_PROTOCOL "udp://"
#define STREAM_ADDRESS "236.0.0.1:2000"
#define STREAM_PATH STREAM_PROTOCOL STREAM_ADDRESS
#endif // RTMP_STREAM
#define CODEC_CRF 5
#define RENDER_WIDTH 1600
#define RENDER_HEIGHT 900
#define STREAM_WIDTH 1280
#define STREAM_HEIGHT 720
#define CLIENT_WIDTH 1366
#define CLIENT_HEIGHT 768
#define TEX_WIDTH 256
#define TEX_HEIGHT 256
#define SHADOW_WIDTH 2048
#define SHADOW_HEIGHT 2048
#define REMOTE_GAME_PORT 61000

#define MAX_PROXIES 32766
#define ROTATION_GIZMO_SIZE 15.0f
